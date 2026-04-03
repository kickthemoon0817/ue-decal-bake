# DecalBaker — UE Decal Baking Plugin Design Spec

**Date:** 2026-04-03  
**Status:** Draft  
**Target:** Unreal Engine 5.3+

## Problem

Unreal Engine deferred decals are a screen-space rendering effect — they modify the GBuffer at runtime and are completely invisible to any export pipeline. When exporting UE scenes via the Omniverse Connector to USD for NVIDIA Isaac Sim, all decals are lost. There is no built-in UE tool or existing plugin that solves this end-to-end.

## Solution

An editor plugin (**DecalBaker**) that bakes deferred decal projections into the underlying mesh textures before export. The baked textures carry the decal contribution embedded in standard PBR channels, making them export-compatible with any format (USD, FBX, OBJ).

## Constraints

- UE 5.3+
- Primary target: Omniverse Connector → USD → Isaac Sim pipeline
- Deferred Decals (UDecalComponent) as primary decal type, extensible to others
- All PBR channels the decal material outputs (BaseColor, Normal, Roughness, Metallic, Emissive, Opacity)
- Must handle overlapping UVs
- Must scale to thousands of decals per scene
- Non-destructive — never modifies original meshes or materials

## Architecture Approach: UV-Space Material Rendering

For each mesh affected by decals, render the mesh "unwrapped" in UV space using a custom vertex shader (UVs become clip-space positions). A fragment shader evaluates each decal's projection matrix, samples the decal material, and composites onto render targets per PBR channel. Results are saved as static texture assets.

---

## 1. Plugin Module Structure

Three UE modules:

### DecalBakerRuntime (Runtime Module)

Core bake logic, no editor dependencies. Usable at runtime or in editor.

| File | Responsibility |
|------|----------------|
| `DecalBakerSubsystem.h/cpp` | UEngineSubsystem — orchestrates the full bake pipeline |
| `DecalBakerSettings.h` | UDeveloperSettings — plugin configuration (exposed in Project Settings) |
| `DecalBakerTypes.h` | Shared structs and enums |
| `DecalMeshPair.h` | FDecalMeshPair — decal-to-mesh relationship data |
| `DecalProjection.cpp` | Projection matrix computation from UDecalComponent transform + size |
| `UVOverlapDetector.cpp` | Triangle-pair intersection tests to detect UV0 overlaps |
| `UVLayoutGenerator.cpp` | Non-overlapping UV channel generation (wraps FLayoutUV) |
| `TextureBaker.cpp` | GPU render target management → static UTexture2D conversion |

### DecalBakerShaders (Shaders Module)

Isolated module for shader directory registration via FGlobalShaderMap.

| File | Responsibility |
|------|----------------|
| `DecalBakeVS.usf` | Vertex shader: UV coordinates → clip space |
| `DecalBakePS.usf` | Pixel shader: decal projection, sampling, per-channel MRT output (compositing via blend state) |
| `DecalBakerShadersModule.cpp` | Shader directory registration |

### DecalBakerEditor (Editor Module)

UI, menus, commandlet, export hooks. Only loaded in editor.

| File | Responsibility |
|------|----------------|
| `DecalBakerEditorModule.cpp` | Menu extension, toolbar button registration |
| `DecalBakerEditorCommands.h` | Slate UI commands |
| `SDecalBakerWidget.cpp` | Dockable settings panel (resolution, channels, UV strategy, etc.) |
| `DecalBakerCommandlet.cpp` | CLI batch baking for CI pipelines |
| `OmniverseExportHook.cpp` | Pre-export automation hook for Omniverse Connector |

---

## 2. Data Flow Pipeline

### Stage 1: Discovery

- Scan the level for all UDecalComponents
- For each decal, perform an OBB overlap query to find affected UStaticMeshComponents
- Filter by `bReceivesDecals` property
- Output: `TArray<FDecalMeshPair>` containing `{ DecalComponent*, MeshComponent*, ProjectionMatrix }`

### Stage 2: UV Analysis

- For each unique mesh in the pair list, read vertex/UV data from `FStaticMeshLODResources`
- Run triangle-pair intersection test on UV0 using a 2D spatial grid
- If overlapping, flag for UV regeneration
- Output: Per-mesh UV readiness status + optional generated UV channel index

**UV Overlap Resolution — 3 tiers:**

| Tier | Strategy | When Used |
|------|----------|-----------|
| 1 | Use existing lightmap UVs (UV1) | UV1 exists and is non-overlapping |
| 2 | Generate new UV layout via FLayoutUV | UV1 unavailable or also overlapping |
| 3 | Per-triangle atlas baking | UV generation fails or produces poor results |

User can override with: Auto (1→2→3) / Force UV0 / Force UV1 / Force Generate.

### Stage 3: GPU Bake

- For each mesh, create `UTextureRenderTarget2D` per PBR channel
- Initialize render targets with the mesh's existing base textures
- For each decal affecting this mesh (sorted by `SortOrder`):
  - Bind decal projection matrix + decal material textures as shader parameters
  - Draw the mesh using DecalBakeVS + DecalBakePS
  - Alpha-blend result into channel render targets (SrcAlpha / InvSrcAlpha blend state)
- Uses MRT (Multi-Render Target): single draw call per decal outputs BaseColor→RT0, Normal→RT1, Roughness/Metallic→RT2, Emissive→RT3, Opacity→RT4

**Handling complex decal materials:**

| Decal Type | Strategy |
|------------|----------|
| Simple (texture-based) | Direct texture sampling in custom shader — fast path |
| Complex (material graph) | Pre-flatten via UE's FExportMaterialProxy to intermediate textures, then use those in bake shader |

### Stage 4: Texture Export

- For each render target, call `UTextureRenderTarget2D::ConstructTexture2D()` to create static UTexture2D
- Save to user-configured output folder

### Stage 5: Material Assignment

- Create new `UMaterialInstanceConstant` (persistent asset, unlike UMaterialInstanceDynamic which is runtime-only and won't export)
- Swap texture parameters to reference baked textures
- If UV was regenerated, set UV channel index parameter
- Assign new material to mesh component

---

## 3. Shader Architecture

### Vertex Shader: DecalBakeVS.usf

```hlsl
// Input: mesh vertex position (world), normal, UV
// Output: clip position from UV, world position for pixel shader

void MainVS(
    in float3 Position : ATTRIBUTE0,
    in float3 Normal : ATTRIBUTE1,
    in float2 UV : ATTRIBUTE2,
    out float4 OutClipPos : SV_Position,
    out float3 OutWorldPos : TEXCOORD0,
    out float3 OutWorldNormal : TEXCOORD1
)
{
    // UV coordinates become screen position — mesh is "unwrapped" flat
    OutClipPos.x = UV.x * 2.0 - 1.0;
    OutClipPos.y = -(UV.y * 2.0 - 1.0); // Y flipped
    OutClipPos.z = 0.0;
    OutClipPos.w = 1.0;

    OutWorldPos = TransformLocalToWorld(Position);
    OutWorldNormal = TransformLocalToWorldNormal(Normal);
}
```

### Pixel Shader: DecalBakePS.usf

```hlsl
// Input: world position, world normal, decal parameters
// Output: MRT — one render target per PBR channel

void MainPS(
    in float3 WorldPos : TEXCOORD0,
    in float3 WorldNormal : TEXCOORD1,
    out float4 OutBaseColor : SV_Target0,
    out float4 OutNormal : SV_Target1,
    out float4 OutRoughnessMetallic : SV_Target2,
    out float4 OutEmissive : SV_Target3,
    out float4 OutOpacity : SV_Target4
)
{
    // Transform world position into decal local space
    float3 DecalLocalPos = mul(DecalProjectionMatrix, float4(WorldPos, 1.0)).xyz;
    float2 DecalUV = DecalLocalPos.yz * 0.5 + 0.5;

    // Clip: discard if outside decal box
    if (any(abs(DecalLocalPos) > 1.0)) discard;

    // Back-face rejection
    if (dot(WorldNormal, DecalForwardDir) > 0.0) discard;

    // Sample decal textures
    float4 Color = DecalBaseColorTex.Sample(LinearSampler, DecalUV);
    float4 Normal = DecalNormalTex.Sample(LinearSampler, DecalUV);
    float Roughness = DecalRoughnessTex.Sample(LinearSampler, DecalUV).r;
    float Metallic = DecalMetallicTex.Sample(LinearSampler, DecalUV).r;
    float4 Emissive = DecalEmissiveTex.Sample(LinearSampler, DecalUV);
    float Opacity = Color.a * DecalOpacity;

    // Output with alpha for blend-state compositing
    OutBaseColor = float4(Color.rgb, Opacity);
    OutNormal = float4(Normal.rgb, Opacity);
    OutRoughnessMetallic = float4(Roughness, Metallic, 0.0, Opacity);
    OutEmissive = float4(Emissive.rgb, Opacity);
    OutOpacity = float4(Opacity, 0.0, 0.0, 1.0);
}
```

### Blend State

Render target blend: `SrcAlpha / InvSrcAlpha` — each decal draw accumulates onto render targets initialized with base textures. Decals are drawn in `SortOrder` so overlapping decals composite correctly.

---

## 4. Editor UI & Automation

### Access Points

1. **Toolbar button** — Level Editor top bar, "Bake Decals" icon
2. **Right-click context menu** — on selected actors, "Bake Decals to Textures"
3. **Dockable panel** — `SDecalBakerWidget`, full settings and controls, dockable like any Slate tab

### Settings Panel

| Setting | Default | Description |
|---------|---------|-------------|
| Output Resolution | 2048 | Baked texture size per mesh |
| PBR Channels | All enabled | Toggle BaseColor, Normal, Roughness, Metallic, Emissive, Opacity |
| UV Strategy | Auto | Auto / Force UV0 / Force UV1 / Force Generate |
| UV Padding | 4px | Gutter between UV islands |
| Output Path | /Game/BakedDecals/ | Where baked texture assets are saved |
| Scope | Selected | Selected actors / All in level / By folder |
| Preview Mode | Off | Show bake result on mesh before committing |

### Workflow

1. Select meshes (or choose "All in level")
2. Click "Bake Decals"
3. Progress bar shows per-mesh progress
4. Preview result in viewport
5. "Apply" commits the baked materials, "Revert" restores originals

### Automated Pre-Export Hook

Integrates with Omniverse Connector:

- Binds to `UOmniverseExporter::OnPreExport` delegate if available
- Fallback: hooks into `UEditorEngine::OnPreExportScene`
- If neither is available: custom "Export with Decals" button that runs Bake → Omniverse Export → Revert

### Commandlet (Batch/CI)

```bash
UnrealEditor-Cmd.exe MyProject.uproject -run=DecalBaker \
    -map=/Game/Maps/Warehouse \
    -output=/Game/BakedDecals/ \
    -resolution=2048 \
    -uvstrategy=auto
```

---

## 5. Output & Asset Management

### Folder Structure

```
/Game/BakedDecals/                     # Root output folder (configurable)
├── SM_Wall_01/                        # Per-mesh subfolder
│   ├── SM_Wall_01_BaseColor.uasset
│   ├── SM_Wall_01_Normal.uasset
│   ├── SM_Wall_01_Roughness.uasset
│   ├── SM_Wall_01_Metallic.uasset
│   ├── SM_Wall_01_Emissive.uasset
│   ├── SM_Wall_01_Opacity.uasset
│   └── MI_SM_Wall_01_Baked.uasset     # Material instance with baked textures
├── SM_Floor_03/
│   └── ...
└── DecalBakeManifest.json
```

### Manifest File

Tracks bake history for incremental re-bakes and revert:

```json
{
  "version": 1,
  "bakeTime": "2026-04-03T14:30:00Z",
  "entries": [
    {
      "meshPath": "/Game/Meshes/SM_Wall_01",
      "originalMaterialPath": "/Game/Materials/M_Wall_01",
      "bakedMaterialPath": "/Game/BakedDecals/SM_Wall_01/MI_SM_Wall_01_Baked",
      "uvChannel": 0,
      "resolution": 2048,
      "decals": [
        { "actorName": "DecalActor_12", "sortOrder": 0 },
        { "actorName": "DecalActor_15", "sortOrder": 1 }
      ]
    }
  ]
}
```

### Revert Capability

- Manifest stores original material paths
- "Revert" button restores original materials on all baked meshes
- Baked texture assets can be cleaned up or kept
- Non-destructive: original meshes and materials are never modified

### Incremental Re-bake

- Compare current decal state against manifest
- Only re-bake meshes whose affecting decals changed
- Significant time savings on large scenes

---

## 6. Key Dependencies

| Dependency | Purpose |
|------------|---------|
| `RenderCore` | Shader compilation, render target management |
| `RHI` | GPU resource management, draw commands |
| `Engine` | UDecalComponent, UStaticMeshComponent, UTextureRenderTarget2D |
| `MeshDescription` | Mesh vertex/UV data access |
| `MaterialBaking` | FExportMaterialProxy for complex material flattening |
| `Slate` / `EditorStyle` | Editor UI |
| `UnrealEd` | Editor utilities, commandlet base class |

## 7. Risk & Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Complex decal materials can't be flattened | Some decals bake incorrectly | Pre-flatten via FExportMaterialProxy; log warnings for unsupported nodes |
| UV generation produces poor layouts | Texture waste, visible seams | Allow manual UV channel selection; UV padding to prevent seam bleeding |
| Performance on thousands of decals | Long bake times | MRT reduces draw calls; incremental re-bake skips unchanged meshes; progress bar with cancel |
| Omniverse Connector has no pre-export hook | Can't automate | Fallback: custom "Export with Decals" button |
| Decal sort order ambiguity | Incorrect compositing | Use UDecalComponent::SortOrder; warn if duplicates found |
| Normal map compositing artifacts | Incorrect lighting in Isaac Sim | Re-orient normals from decal space to mesh tangent space during bake |

# DecalBaker Demo Project

> **Note:** This sample project has Python remote execution disabled by default. Enable it manually in Project Settings if needed for automation.

A lightweight UE 5.7 sample project demonstrating the DecalBaker plugin.

## Setup

1. **Copy the plugin** into this project using the installer (recommended):
   ```bash
   python3 ../Tools/install_plugin.py   # Mac/Linux
   py ..\Tools\install_plugin.py        # Windows
   ```
   Or copy manually:
   ```bash
   cp -r ../../../DecalBaker DecalBakerDemo/Plugins/DecalBaker
   ```
   Or create a symlink:
   ```bash
   mkdir -p DecalBakerDemo/Plugins
   ln -s $(pwd)/../../../DecalBaker DecalBakerDemo/Plugins/DecalBaker
   ```

2. **Open the project** in UE 5.7:
   - Double-click `DecalBakerDemo.uproject`, or
   - Launch from Epic Games Launcher

3. **Enable Python plugin** if prompted (should be auto-enabled via .uproject)

## Running the Demo

### Step 1: Create the demo scene

Open the **Output Log** (Window > Output Log), then run:
```
python3 setup_demo_scene.py   # Mac/Linux
py setup_demo_scene.py        # Windows
```

This creates:
- A floor plane and wall cube
- 3 deferred decals projected onto the surfaces
- A directional light

> **Note:** You'll want to assign decal materials to the 3 decals. Create simple decal materials with a texture (e.g., a warning sign, logo, or grunge pattern) and assign them to Demo_Decal_Floor, Demo_Decal_Floor2, and Demo_Decal_Wall.

### Step 2: Bake and export

After assigning decal materials, run:
```
python3 run_bake_and_export.py   # Mac/Linux
py run_bake_and_export.py        # Windows
```

This will:
1. Discover all decal-mesh pairs in the scene
2. Bake decal projections into the mesh textures
3. Export the scene to USD at `<ProjectDir>/Export/DecalBakerDemo.usda`

### Step 3: Verify in Isaac Sim

Open the exported `.usda` file in:
- **Isaac Sim** (drag into the stage)
- **usdview** (`usdview DecalBakerDemo.usda`)
- **NVIDIA Omniverse**

The decals should now be visible as part of the mesh textures.

## Alternative: Manual Workflow

1. Open the **Decal Baker** panel (toolbar button or Window > Decal Baker)
2. Configure settings (resolution, UV strategy, output path)
3. Click **Bake All**
4. Export via File > Export All > USD

## Project Structure

```
DecalBakerDemo/
├── DecalBakerDemo.uproject
├── Config/
│   ├── DefaultEngine.ini      # DBuffer=True, Python paths
│   ├── DefaultEditor.ini
│   └── DefaultGame.ini
├── Content/
│   └── Python/
│       ├── setup_demo_scene.py       # Creates demo scene
│       └── run_bake_and_export.py    # Bakes decals + exports USD
├── Plugins/
│   └── DecalBaker/            # (copy or symlink from repo root)
└── Source/
    └── DecalBakerDemo/        # Minimal game module
```

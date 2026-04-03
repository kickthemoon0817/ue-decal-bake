"""
DecalBaker Demo — Bake and Export to USD
Run in UE Python console: py run_bake_and_export.py

Steps:
1. Runs the DecalBaker subsystem to bake all decals in the level
2. Exports the scene to USD format
"""

import unreal
import os

def bake_decals():
    """Run the DecalBaker bake pipeline on all meshes in the level."""
    subsystem = unreal.get_engine_subsystem(unreal.DecalBakerSubsystem)
    if not subsystem:
        unreal.log_error("DecalBaker: Subsystem not found. Is the DecalBaker plugin enabled?")
        return False

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        unreal.log_error("DecalBaker: No editor world found")
        return False

    # Discover decal-mesh pairs
    empty_scope = unreal.Array(unreal.StaticMeshComponent)
    pairs = subsystem.discover_decal_mesh_pairs(world, empty_scope)
    unreal.log("DecalBaker: Found {} decal-mesh pairs".format(len(pairs)))

    # Run full bake pipeline
    manifest = subsystem.bake_decals(world, empty_scope)
    unreal.log("DecalBaker: Baked {} meshes".format(len(manifest.entries)))

    return True


def export_to_usd(output_path=None):
    """Export the current level to USD format."""
    if output_path is None:
        project_dir = unreal.Paths.project_dir()
        output_path = os.path.join(project_dir, "Export", "DecalBakerDemo.usda")

    # Ensure export directory exists
    export_dir = os.path.dirname(output_path)
    if not os.path.exists(export_dir):
        os.makedirs(export_dir)

    # Use UE's built-in USD export
    task = unreal.AssetExportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("filename", output_path)

    world = unreal.EditorLevelLibrary.get_editor_world()

    # Export via LevelExporterUSD if available
    try:
        export_options = unreal.LevelExporterUSDOptions()
        task.set_editor_property("object", world)
        task.set_editor_property("exporter", unreal.LevelExporterUSD())
        task.set_editor_property("options", export_options)

        success = unreal.Exporter.run_asset_export_task(task)
        if success:
            unreal.log("DecalBaker: USD exported to {}".format(output_path))
        else:
            unreal.log_warning("DecalBaker: USD export returned false")
    except Exception as e:
        unreal.log_warning("DecalBaker: LevelExporterUSD not available ({}). Trying alternate method...".format(str(e)))

        # Fallback: use USD Stage Actor export
        try:
            unreal.EditorLevelUtils.export_level_to_usd(
                world,
                output_path,
                True,  # bSelected
                True   # bExportLevels
            )
            unreal.log("DecalBaker: USD exported via EditorLevelUtils to {}".format(output_path))
        except Exception as e2:
            unreal.log_error("DecalBaker: USD export failed: {}".format(str(e2)))
            unreal.log("DecalBaker: You can manually export via File > Export All > USD")
            return False

    return True


def run():
    """Main entry point: bake decals then export to USD."""
    unreal.log("=" * 60)
    unreal.log("DecalBaker Demo: Bake and Export")
    unreal.log("=" * 60)

    # Step 1: Bake
    unreal.log("Step 1: Baking decals into mesh textures...")
    bake_ok = bake_decals()
    if not bake_ok:
        unreal.log_error("DecalBaker: Bake failed, aborting export")
        return

    # Step 2: Export
    unreal.log("Step 2: Exporting to USD...")
    export_ok = export_to_usd()

    if export_ok:
        unreal.log("=" * 60)
        unreal.log("DecalBaker Demo: Complete!")
        unreal.log("USD file is in: <ProjectDir>/Export/DecalBakerDemo.usda")
        unreal.log("Open this file in Isaac Sim or usdview to verify decals are baked in")
        unreal.log("=" * 60)
    else:
        unreal.log_warning("DecalBaker: Export step had issues. Check logs above.")


if __name__ == "__main__" or True:
    run()

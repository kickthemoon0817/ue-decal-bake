"""
DecalBaker Demo — Create and Save a minimal demo level.
Run headlessly or in-editor:
  py save_demo_level.py
"""

import unreal

editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_editor_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_lib = unreal.EditorAssetLibrary

# ---- Create Materials ----

def create_base_material():
    """Simple gray base material with texture parameter slots for baking."""
    if editor_asset_lib.does_asset_exist("/Game/Demo/Materials/M_Base"):
        return unreal.load_asset("/Game/Demo/Materials/M_Base")

    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Base", "/Game/Demo/Materials", unreal.Material, factory)
    editor_asset_lib.save_asset("/Game/Demo/Materials/M_Base")
    unreal.log("Created M_Base")
    return mat


def create_decal_material(name):
    """Simple translucent decal material."""
    path = f"/Game/Demo/Materials/{name}"
    if editor_asset_lib.does_asset_exist(path):
        return unreal.load_asset(path)

    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(name, "/Game/Demo/Materials", unreal.Material, factory)

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)

    editor_asset_lib.save_asset(path)
    unreal.log(f"Created {name}")
    return mat


# ---- Build Scene ----

def build_scene():
    # Create a new empty level (delete existing first if present)
    if editor_asset_lib.does_asset_exist("/Game/Demo/Maps/DecalBakeDemo"):
        editor_asset_lib.delete_asset("/Game/Demo/Maps/DecalBakeDemo")
    level_editor_subsystem.new_level("/Game/Demo/Maps/DecalBakeDemo")

    # Materials
    base_mat = create_base_material()
    decal_mat_red = create_decal_material("M_Decal_Red")
    decal_mat_blue = create_decal_material("M_Decal_Blue")
    decal_mat_yellow = create_decal_material("M_Decal_Yellow")

    # Floor
    floor = editor_actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0, 0, 0),
        unreal.Rotator(0, 0, 0)
    )
    floor.set_actor_label("Target_Floor")
    floor_mesh = unreal.load_asset("/Engine/BasicShapes/Plane")
    floor.static_mesh_component.set_static_mesh(floor_mesh)
    floor.set_actor_scale3d(unreal.Vector(5, 5, 1))
    if base_mat:
        floor.static_mesh_component.set_material(0, base_mat)

    # Wall
    wall = editor_actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(250, 0, 125),
        unreal.Rotator(0, 0, 0)
    )
    wall.set_actor_label("Target_Wall")
    wall_mesh = unreal.load_asset("/Engine/BasicShapes/Cube")
    wall.static_mesh_component.set_static_mesh(wall_mesh)
    wall.set_actor_scale3d(unreal.Vector(0.1, 5, 2.5))
    if base_mat:
        wall.static_mesh_component.set_material(0, base_mat)

    # Decal 1: on floor center (red)
    d1 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor,
        unreal.Vector(0, 0, 50),
        unreal.Rotator(-90, 0, 0)
    )
    d1.set_actor_label("Decal_Floor_Red")
    d1.decal.set_editor_property("decal_size", unreal.Vector(100, 80, 80))
    if decal_mat_red:
        d1.decal.set_decal_material(decal_mat_red)

    # Decal 2: on floor offset (blue)
    d2 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor,
        unreal.Vector(-100, 100, 50),
        unreal.Rotator(-90, 45, 0)
    )
    d2.set_actor_label("Decal_Floor_Blue")
    d2.decal.set_editor_property("decal_size", unreal.Vector(80, 60, 60))
    if decal_mat_blue:
        d2.decal.set_decal_material(decal_mat_blue)

    # Decal 3: on wall (yellow)
    d3 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor,
        unreal.Vector(200, 0, 125),
        unreal.Rotator(0, 180, 0)
    )
    d3.set_actor_label("Decal_Wall_Yellow")
    d3.decal.set_editor_property("decal_size", unreal.Vector(100, 60, 60))
    if decal_mat_yellow:
        d3.decal.set_decal_material(decal_mat_yellow)

    # Directional light
    light = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0, 0, 400),
        unreal.Rotator(-45, 0, 0)
    )
    light.set_actor_label("Demo_Light")

    # Sky light for ambient
    sky = editor_actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0, 0, 300),
        unreal.Rotator(0, 0, 0)
    )
    sky.set_actor_label("Demo_SkyLight")

    # Count spawned actors to verify
    actors = editor_actor_subsystem.get_all_level_actors()
    unreal.log_warning(f"ACTORS_SPAWNED: {len(actors)} actors in level")

    # Save — try multiple methods
    world = editor_actor_subsystem.get_world()

    # Method 1: save_map with path
    try:
        unreal.EditorLoadingAndSavingUtils.save_map(world, "/Game/Demo/Maps/DecalBakeDemo")
        unreal.log_warning("SAVE_METHOD_1: EditorLoadingAndSavingUtils.save_map OK")
    except Exception as e:
        unreal.log_warning(f"SAVE_METHOD_1 failed: {e}")

    # Method 2: save_dirty_packages
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(False, True)
        unreal.log_warning("SAVE_METHOD_2: save_dirty_packages OK")
    except Exception as e:
        unreal.log_warning(f"SAVE_METHOD_2 failed: {e}")

    # Method 3: level editor subsystem
    try:
        level_editor_subsystem.save_all_dirty_levels()
        unreal.log_warning("SAVE_METHOD_3: save_all_dirty_levels OK")
    except Exception as e:
        unreal.log_warning(f"SAVE_METHOD_3 failed: {e}")

    unreal.log_warning("DEMO_COMPLETE")


if __name__ == "__main__":
    build_scene()

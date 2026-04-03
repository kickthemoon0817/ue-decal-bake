"""
DecalBaker Demo — Scene Setup
Run in UE Python console: py setup_demo_scene.py

Creates a minimal scene with:
- A floor plane with a base material
- A wall cube with a base material
- 3 deferred decals projected onto the surfaces
"""

import unreal

editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material_edit_lib = unreal.MaterialEditingLibrary

# ---- Create Base Materials ----

def create_solid_material(name, color):
    """Create a simple solid-color material with texture parameters for baking."""
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(name, "/Game/Demo/Materials", unreal.Material, mat_factory)

    # The material needs texture parameters so the baked MIC can override them
    # For now, set base color via constant
    material_edit_lib.set_material_instance_vector_parameter_value(mat, "BaseColor", color)

    return mat


def create_base_material(name, base_color):
    """Create a parameterized material suitable for decal baking."""
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(name, "/Game/Demo/Materials", unreal.Material, mat_factory)
    unreal.EditorAssetLibrary.save_asset("/Game/Demo/Materials/" + name)
    return mat


def create_decal_material(name, color):
    """Create a simple decal material."""
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(name, "/Game/Demo/Materials", unreal.Material, mat_factory)

    # Set blend mode to Translucent and material domain to Deferred Decal
    mat.set_editor_property("material_domain", unreal.MaterialDomain.DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("decal_blend_mode", unreal.DecalBlendMode.DBM_TRANSLUCENT)

    unreal.EditorAssetLibrary.save_asset("/Game/Demo/Materials/" + name)
    return mat


# ---- Create Scene ----

def setup_scene():
    world = unreal.EditorLevelLibrary.get_editor_world()

    # Floor plane
    floor_loc = unreal.Vector(0, 0, 0)
    floor_rot = unreal.Rotator(0, 0, 0)
    floor_scale = unreal.Vector(5, 5, 1)

    floor = editor_actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, floor_loc, floor_rot
    )
    floor.set_actor_label("Demo_Floor")
    floor_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane")
    floor.static_mesh_component.set_static_mesh(floor_mesh)
    floor.set_actor_scale3d(floor_scale)

    # Wall cube
    wall_loc = unreal.Vector(250, 0, 125)
    wall_rot = unreal.Rotator(0, 0, 0)
    wall_scale = unreal.Vector(0.1, 5, 2.5)

    wall = editor_actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, wall_loc, wall_rot
    )
    wall.set_actor_label("Demo_Wall")
    wall_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    wall.static_mesh_component.set_static_mesh(wall_mesh)
    wall.set_actor_scale3d(wall_scale)

    # Decal 1 — on the floor, centered
    decal1_loc = unreal.Vector(0, 0, 50)
    decal1_rot = unreal.Rotator(-90, 0, 0)  # pointing down

    decal1 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor, decal1_loc, decal1_rot
    )
    decal1.set_actor_label("Demo_Decal_Floor")
    decal1.decal.set_editor_property("decal_size", unreal.Vector(100, 80, 80))

    # Decal 2 — on the floor, offset
    decal2_loc = unreal.Vector(-100, 100, 50)
    decal2_rot = unreal.Rotator(-90, 45, 0)

    decal2 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor, decal2_loc, decal2_rot
    )
    decal2.set_actor_label("Demo_Decal_Floor2")
    decal2.decal.set_editor_property("decal_size", unreal.Vector(80, 60, 60))

    # Decal 3 — on the wall
    decal3_loc = unreal.Vector(200, 0, 125)
    decal3_rot = unreal.Rotator(0, 180, 0)  # pointing at wall (-X)

    decal3 = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DecalActor, decal3_loc, decal3_rot
    )
    decal3.set_actor_label("Demo_Decal_Wall")
    decal3.decal.set_editor_property("decal_size", unreal.Vector(100, 60, 60))

    # Add a light
    light_loc = unreal.Vector(0, 0, 400)
    light_rot = unreal.Rotator(-45, 0, 0)

    light = editor_actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight, light_loc, light_rot
    )
    light.set_actor_label("Demo_Light")

    unreal.log("DecalBaker Demo: Scene created with 2 meshes, 3 decals, 1 light")
    unreal.log("Next step: assign materials to decals, then run run_bake_and_export.py")


if __name__ == "__main__" or True:
    setup_scene()

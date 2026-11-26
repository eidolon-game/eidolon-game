"""Generate the native Valdris prologue graybox, or validate its layout without Unreal."""

import math
import sys

MAP_FOLDER = "/Game/Eidolon/Maps"
MAP_PACKAGE = MAP_FOLDER + "/ValdrisPrologue"
MODE_CLASS = "/Script/Eidolon.EidolonValdrisPrologueGameMode"
FLOOR_SIZE = (3600, 2600)

START = (-1230, -570, 110)
ANCHORS = {
    "veyr_farewell": (-1330, -420, 110),
    "veyr_reveal_door": (-1120, -350, 110),
    "guild_registration": (-150, 390, 110),
    "guild_dormitory": (450, 390, 110),
    "abyss_entrance": (1330, -500, 110),
}
VOLUMES = {
    "house": ((-1370, -880, 150), (570, 500, 300)),
    "guild": ((105, 770, 190), (1130, 590, 380)),
    "abyss_facade": ((1550, -620, 170), (200, 470, 340)),
}


def validate():
    locations = [START, *ANCHORS.values()]
    if len(set(locations)) != len(locations):
        raise ValueError("Each prologue interaction requires a distinct location.")
    for x, y, z in locations:
        if abs(x) >= FLOOR_SIZE[0] / 2 - 100 or abs(y) >= FLOOR_SIZE[1] / 2 - 100 or z != 110:
            raise ValueError(f"Invalid walking position: {(x, y, z)}")
        for name, ((cx, cy, _), (width, depth, _)) in VOLUMES.items():
            if abs(x - cx) <= width / 2 + 42 and abs(y - cy) <= depth / 2 + 42:
                raise ValueError(f"Walking position {(x, y, z)} intersects {name}")
    if math.dist(START, ANCHORS["veyr_farewell"]) > 220:
        raise ValueError("The player must start within reach of the farewell anchor.")


def build(unreal):
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(MAP_PACKAGE):
        unreal.log("Reusing the existing Valdris prologue map.")
        return

    mode = unreal.load_class(None, MODE_CLASS)
    if not mode:
        raise RuntimeError("Build EidolonEditor to load the Valdris prologue GameMode.")
    if not assets.does_directory_exist(MAP_FOLDER) and not assets.make_directory(MAP_FOLDER):
        raise RuntimeError("Could not create the Maps folder.")

    editor = unreal.EditorLoadingAndSavingUtils
    world = editor.new_blank_map(False)
    if not world:
        raise RuntimeError("Could not create the Valdris prologue map.")
    settings = unreal.GameplayStatics.get_actor_of_class(world, unreal.WorldSettings)
    if not settings:
        raise RuntimeError("The Valdris prologue map has no WorldSettings.")
    settings.set_editor_property("default_game_mode", mode)

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Unreal basic cube.")

    def spawn(actor_type, label, position, rotation=(0, 0, 0)):
        actor = actors.spawn_actor_from_class(
            actor_type, unreal.Vector(*position), unreal.Rotator(*rotation)
        )
        if not actor:
            raise RuntimeError(f"Could not place {label}.")
        actor.set_actor_label(label)
        return actor

    def box(label, position, size):
        actor = spawn(unreal.StaticMeshActor, label, position)
        component = actor.static_mesh_component
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the cube mesh to {label}.")
        component.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*(axis / 100 for axis in size)))

    width, depth = FLOOR_SIZE
    box("street_ground", (0, 0, -25), (width, depth, 50))
    for edge in (-1, 1):
        box(f"wall_east_west_{edge}", (edge * width / 2, 0, 155), (40, depth + 40, 310))
        box(f"wall_north_south_{edge}", (0, edge * depth / 2, 155), (width, 40, 310))
    for label, (position, size) in VOLUMES.items():
        box(label, position, size)

    for label, position in ANCHORS.items():
        actor = spawn(unreal.TargetPoint, label, position)
        actor.set_editor_property("tags", [unreal.Name(label)])
        spawn(unreal.PointLight, f"light_{label}", (position[0], position[1], 290))
    for label, text, position in (
        ("sign_house", "Veyr", (-1370, -601, 235)),
        ("sign_guild", "Gilda / Guild", (105, 451, 275)),
        ("sign_abyss", "Abisso / Abyss", (1425, -500, 235)),
    ):
        sign = spawn(unreal.TextRenderActor, label, position)
        component = sign.get_component_by_class(unreal.TextRenderComponent)
        component.set_text(text)
        component.set_world_size(42.0)
    spawn(unreal.PlayerStart, "valdris_player_start", START)
    light = spawn(unreal.DirectionalLight, "sun", (0, 0, 700), (0, -35, -25))
    light.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", 6.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError("Could not save the Valdris prologue map.")


if __name__ == "__main__":
    validate()
    if "--validate" in sys.argv[1:]:
        print("Validated the Valdris prologue layout and five distinct interaction points.")
    else:
        import unreal

        build(unreal)

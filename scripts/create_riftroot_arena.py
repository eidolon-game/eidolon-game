"""Build the floor 20 Riftroot arena or validate its graybox layout offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, create_graybox

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor020"
MODE_PATH = "/Script/Eidolon.EidolonRiftrootGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_020_authored.json"


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    approach, arena, outlet = (layout[key] for key in ("approach", "arena", "outlet"))
    approach_start, arena_start = approach["x_cm"]
    gate_x, outlet_end = outlet["x_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    gate = layout["gate"]
    anchors = layout["anchors_cm"]
    start, checkpoint, encounter, spawn, advance, exit_anchor = (anchors[key] for key in (
        "player_start", "checkpoint_start", "encounter", "boss_spawn", "boss_advance", "exit",
    ))

    if arena["x_cm"] != [arena_start, gate_x] or not approach_start < arena_start < gate_x < outlet_end:
        raise ValueError("Approach, arena and outlet must meet at their thresholds.")
    if not 0 < gate["width_cm"] < 2 * outlet["half_width_cm"] or not 180 < gate["height_cm"] < roof:
        raise ValueError("The gate must close the full opening inside the outlet jambs.")
    if min(approach["half_width_cm"], outlet["half_width_cm"]) <= 0 or wall <= 0:
        raise ValueError("The graybox needs positive corridor and wall dimensions.")
    if any(y != 0 for _, y, _ in anchors.values()):
        raise ValueError("The Riftroot anchors must use the arena centerline.")
    if any(point[2] != 110 for point in (start, checkpoint, encounter, exit_anchor)) or spawn[2] != 0 or advance[2] != 0:
        raise ValueError("Player anchors must match standing height and both boss anchors must be floor-level.")
    if not approach_start < start[0] < checkpoint[0] < approach["baffle_x_cm"] - wall / 2:
        raise ValueError("The player and checkpoint must be behind the approach baffle.")
    if not approach["baffle_x_cm"] + wall / 2 < arena_start < encounter[0] < spawn[0] < advance[0] < gate_x < exit_anchor[0] < outlet_end:
        raise ValueError("The encounter, boss movement and exit must occur in order.")
    if not 300 <= advance[0] - spawn[0] <= 850 or layout["boss_spawn_yaw_deg"] != 180:
        raise ValueError("The boss must face the approach and have a valid forward movement anchor.")

    body_length, body_width = arena["boss_footprint_cm"]
    flank = arena["flank_inner_abs_y_cm"]
    body_radius = math.hypot(body_length, body_width) / 2
    shoulder = arena["half_width_cm"] - approach["half_width_cm"]
    passage = approach["half_width_cm"] - approach["baffle_half_width_cm"]
    if not 300 <= passage < min(body_length, body_width) or shoulder <= 0:
        raise ValueError("The sheltered approach must allow a walking bypass on either side and exclude the boss footprint.")
    if flank < body_radius or arena["half_width_cm"] - flank < 400:
        raise ValueError("The two flank bypass lanes must be clear of the boss body and at least 400 cm wide.")
    if spawn[0] - body_radius - arena_start - wall / 2 < 300 or gate_x - wall / 2 - (advance[0] + body_length / 2) < 300:
        raise ValueError("Both cross-connections need room to reach the flanks around the boss.")
    return layout


def build(unreal, layout):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]

    for name in ("approach", "arena", "outlet"):
        start_x, end_x = layout[name]["x_cm"]
        x, length = (start_x + end_x) / 2, end_x - start_x
        half_width = layout[name]["half_width_cm"]
        box(f"riftroot_{name}_floor", (x, 0, -10), (length, 2 * half_width, 20))
        box(f"riftroot_{name}_ceiling", (x, 0, roof + 10), (length, 2 * half_width, 20))
        for side in (-1, 1):
            box(f"riftroot_{name}_wall_{side}", (x, side * (half_width + wall / 2), roof / 2),
                (length + wall, wall, roof))

    approach = layout["approach"]
    west_x, arena_x = approach["x_cm"]
    box("riftroot_west_cap", (west_x - wall / 2, 0, roof / 2),
        (wall, 2 * (approach["half_width_cm"] + wall), roof))
    box("riftroot_checkpoint_baffle", (approach["baffle_x_cm"], 0, roof / 2),
        (wall, 2 * approach["baffle_half_width_cm"], roof))

    arena_half = layout["arena"]["half_width_cm"]
    shoulder = arena_half - approach["half_width_cm"]
    for side in (-1, 1):
        box(f"riftroot_entry_shoulder_{side}", (arena_x, side * (arena_half - shoulder / 2), roof / 2),
            (wall, shoulder + wall, roof))

    gate_x, east_x = layout["outlet"]["x_cm"]
    gate = layout["gate"]
    width, height = gate["width_cm"], gate["height_cm"]
    jamb = arena_half - width / 2
    for side in (-1, 1):
        box(f"riftroot_gate_jamb_{side}", (gate_x, side * (width + jamb) / 2, roof / 2),
            (wall, jamb, roof))
    box("riftroot_gate_header", (gate_x, 0, (roof + height) / 2), (wall, width, roof - height))
    box(gate["tag"], (gate_x, 0, height / 2), (wall, width, height), movable=True)
    box("riftroot_east_cap", (east_x + wall / 2, 0, roof / 2),
        (wall, 2 * (layout["outlet"]["half_width_cm"] + wall), roof))

    for name, position in layout["anchors_cm"].items():
        actor_class = unreal.PlayerStart if name.endswith("_start") else unreal.TargetPoint
        actor = spawn(actor_class, f"abyss_020_{name}", position)
        if name == "boss_spawn":
            actor.set_actor_rotation(unreal.Rotator(yaw=layout["boss_spawn_yaw_deg"]), False)

    lights = ((300, 0), (720, -360), (720, 360), (1150, 0),
              (1580, -850), (1580, 850), (2180, 0), (2760, 0))
    for index, (x, y) in enumerate(lights, 1):
        actor = spawn(unreal.PointLight, f"riftroot_light_{index:02d}", (x, y, roof - 90))
        light = actor.get_component_by_class(unreal.PointLightComponent)
        light.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity", 2550.0)
        light.set_editor_property("attenuation_radius", 1200.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save Riftroot Tangle at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']} graybox: sheltered approach, two 420 cm flank lanes and one closed outlet.")
    else:
        import unreal

        build(unreal, layout)

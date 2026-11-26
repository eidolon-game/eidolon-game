"""Build the Broken Portico survey or validate its authored grid offline.

Set EIDOLON_MARROW_INTERACTIVE=1 before the Unreal commandlet to build the
separate gated encounter map.
"""

import json
import os
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor005Survey"
MODE_PATH = "/Script/Eidolon.EidolonAbyssSurveyGameMode"
INTERACTIVE_PACKAGE = MAP_FOLDER + "/AbyssFloor005"
INTERACTIVE_MODE_PATH = "/Script/Eidolon.EidolonMarrowRamGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_005_authored.json"
ANCHORS = ("entry", "checkpoint", "encounter", "latch", "exit")
INTERACTION_RADIUS_CM = 180


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Broken Portico needs unique cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("Expected the 200 cm catacomb kit with 300 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 4:
        raise ValueError("The descent requires the floor-four receipt.")

    start, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The Broken Portico gate must face north between floor cells.")
    gate_edge = frozenset((near, far))
    beam = tuple(layout["collision_beam"]["cell"])
    if beam not in cells:
        raise ValueError("The beam must occupy the charging lane.")
    walkable = cells - {beam}
    before_gate = visit(walkable, start, gate_edge)
    if visit(walkable, start) != walkable or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The gate must be the only cut between the arena and the exit.")
    if not {tuple(layout[name]) for name in ANCHORS[:-1]} <= before_gate:
        raise ValueError("The checkpoint, encounter and latch must be accessible before the gate.")

    encounter = layout["encounter_definition"]
    rx, ry = encounter["spawn_cell"]
    bx, by = beam
    charge_lane = {(x, ry) for x in range(rx, bx + 1)}
    if (rx >= bx or ry != by or tuple(encounter["charge_lane_end"]) != beam
            or not charge_lane <= cells):
        raise ValueError("The ram needs a straight eastbound charge to the beam.")
    if tuple(layout["encounter"]) not in charge_lane - {beam}:
        raise ValueError("The player's activation marker must be between the ram and the beam.")

    lanes = layout["escape_lanes"]
    size = layout["cell_cm"]
    _, beam_width, _ = layout["collision_beam"]["dimensions_cm"]
    if size - beam_width / 2 <= 42:
        raise ValueError("The beam must clear the player's capsule in both side lanes.")
    for direction, shift in (("south", -1), ("north", 1)):
        lane = {tuple(cell) for cell in lanes[direction]["cells"]}
        expected = {(x, ry + shift) for x in range(rx + 1, bx + 2)}
        refuge = tuple(lanes[direction]["refuge"])
        if lane != expected or not (lane | {refuge}) <= before_gate:
            raise ValueError("Both escape lanes and their refuge cells must be accessible.")

    bridge = layout["side_bridge"]
    if not {tuple(cell) for cell in bridge["cells"]} <= before_gate or not 0 < bridge["height_cm"] <= 30:
        raise ValueError("The side bridge must be reachable without a jump or gate release.")

    activation, latch = layout["encounter"], layout["latch"]
    for a, b in ((activation, latch), (latch, exit_cell)):
        dx, dy = (a[0] - b[0]) * size, (a[1] - b[1]) * size
        if dx * dx + dy * dy <= (2 * INTERACTION_RADIUS_CM) ** 2:
            raise ValueError("The encounter, latch and exit need separate interaction zones.")
    return layout, cells


def build(unreal, layout, cells, interactive=False):
    package = INTERACTIVE_PACKAGE if interactive else MAP_PACKAGE
    mode_path = INTERACTIVE_MODE_PATH if interactive else MODE_PATH
    graybox = create_graybox(unreal, package, mode_path)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    build_shell(layout, cells, box)

    size = layout["cell_cm"]
    height = layout["height_cm"]
    thickness = layout["wall_thickness_cm"]
    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gy += size / 2
    width, clearance = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - width) / 2
    for sign in (-1, 1):
        box(f"ram_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("ram_gate_header", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    if interactive:
        box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)
    else:
        spawn(unreal.TargetPoint, gate["id"], (gx, gy, clearance / 2))

    beam = layout["collision_beam"]
    bx, by = center(beam["cell"], size)
    box(beam["id"], (bx, by, beam["dimensions_cm"][2] / 2), beam["dimensions_cm"])

    bridge = layout["side_bridge"]
    for index, cell in enumerate(bridge["cells"]):
        x, y = center(cell, size)
        box(f"ram_side_bridge_{index:02d}", (x, y, bridge["height_cm"] / 2),
            (size - 6, bridge["width_cm"], bridge["height_cm"]))

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        spawn(unreal.TargetPoint, f"abyss_005_{anchor}", (x, y, 110))
        light = spawn(unreal.PointLight, f"light_{anchor}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1800.0)
        component.set_editor_property("attenuation_radius", 750.0)

    encounter = layout["encounter_definition"]
    x, y = center(encounter["spawn_cell"], size)
    ram = spawn(unreal.TargetPoint, encounter["spawn_marker"], (x, y, 0))
    ram.set_actor_rotation(unreal.Rotator(yaw=encounter["spawn_yaw_deg"]), False)
    x, y = center(layout["entry"], size)
    start = spawn(unreal.PlayerStart, "abyss_005_player_start", (x, y, 110))
    start.set_actor_rotation(unreal.Rotator(yaw=layout["entry_yaw_deg"]), False)
    if interactive:
        x, y = center(layout["checkpoint"], size)
        checkpoint = spawn(unreal.PlayerStart, "abyss_005_checkpoint_start", (x, y, 110))
        checkpoint.set_actor_rotation(unreal.Rotator(yaw=layout["checkpoint_yaw_deg"]), False)

    if not editor.save_map(world, package):
        raise RuntimeError(f"Could not save the Broken Portico at {package}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, beam, two escape lanes and a gated exit.")
    else:
        import unreal

        build(unreal, layout, cells, interactive=os.environ.get("EIDOLON_MARROW_INTERACTIVE") == "1")

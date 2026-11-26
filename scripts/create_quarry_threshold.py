"""Author the Quarry Threshold in Unreal or validate its paths offline."""

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, WALL_THICKNESS, build_shell, create_graybox, visit

LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_021_authored.json"
MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor021"
MODE_PATH = "/Script/Eidolon.EidolonQuarryThresholdGameMode"


def rectangle(region):
    x0, y0 = region["minimum"]
    x1, y1 = region["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    if layout["id"] != "abyss_021" or layout["requires_completed_floor"] != 20 or layout["next_floor"] != "abyss_022":
        raise ValueError("The Quarry Threshold must follow Riftroot and lead to the Rail Fork.")
    size = layout["cell_cm"]
    arrival, approach, gallery, outlet = (rectangle(layout[name]) for name in
                                           ("arrival", "approach", "gallery", "outlet"))
    recesses = [rectangle(region) for region in layout["recesses"]]
    areas = (arrival, approach, gallery, outlet, *recesses)
    cells = set().union(*areas)
    if size != 200 or len(recesses) != 2 or any(not area for area in areas) or sum(map(len, areas)) != len(cells):
        raise ValueError("The layout needs distinct dry approaches, gallery, recesses and outlet.")
    anchors = layout["anchors_cm"]
    anchor_cells = {name: (x // size, y // size) for name, (x, y, _) in anchors.items()}
    arrivals = ("player_start", "arrival_recovery", "checkpoint_start")
    if any(anchor_cells[name] not in arrival for name in arrivals) or len({tuple(anchors[name]) for name in arrivals}) != 3:
        raise ValueError("Three distinct player placements are required in the protected arrival.")
    if any(anchors[name][2] != 110 for name in (*arrivals, "quarry_latch", "exit")):
        raise ValueError("Player and interaction markers need standing height.")
    gate = layout["gate"]
    inner, outer = map(tuple, (gate["from_cell"], gate["to_cell"]))
    gate_edge = frozenset((inner, outer))
    if inner not in gallery or outer not in outlet or outer != (inner[0] + 1, inner[1]):
        raise ValueError("The gate must separate the gallery from the outlet.")
    gate_x, gate_y, gate_z = gate["center_cm"]
    _, gate_width, gate_height = gate["dimensions_cm"]
    if (gate_x, gate_y) != (outer[0] * size, (outer[1] + .5) * size) or gate_width != size or gate_height != 2 * gate_z:
        raise ValueError("The gate must fill the portal below the lintel.")
    start, latch, exit_cell = (anchor_cells[name] for name in ("player_start", "quarry_latch", "exit"))
    if visit(cells, start) != cells or latch not in gallery or exit_cell not in outlet:
        raise ValueError("The complete layout needs a connected inner control and exit.")
    before = visit(cells, start, gate_edge)
    if latch not in before or before & outlet:
        raise ValueError("The latch must be accessible while the outlet stays shut.")

    spawn_x, lane_y, spawn_z = anchors["borer_spawn"]
    limit_x, limit_y, limit_z = anchors["charge_limit"]
    body_length, body_width, body_height = layout["borer"]["footprint_cm"]
    hazard = layout["borer"]["charge_hazard_half_width_cm"]
    if (layout["borer"]["required"] or limit_y != lane_y or spawn_z != 0 or limit_z != 0
            or not 700 <= spawn_x - limit_x <= 800 or hazard < body_width / 2
            or layout["height_cm"] - body_height < 100):
        raise ValueError("The optional Borer needs a level, bounded charge lane.")
    if anchor_cells["borer_spawn"] not in gallery or anchor_cells["charge_limit"] not in gallery:
        raise ValueError("Both charge anchors need continuous gallery floor.")
    if limit_x - body_length / 2 - max(anchors[name][0] for name in arrivals) < 600:
        raise ValueError("The charge sweep must stay clear of all arrival positions.")
    if len(approach) != 1:
        raise ValueError("The entrance buffer needs one sheltered opening.")
    opening_x, opening_y = next(iter(approach))
    cover_x = (opening_x + .5) * size
    for name in arrivals:
        x, y, _ = anchors[name]
        sightline_y = y + (lane_y - y) * (cover_x - x) / (spawn_x - x)
        if opening_y * size <= sightline_y <= (opening_y + 1) * size:
            raise ValueError(f"The gallery can see the protected start {name}.")

    # Mark every tile overlapped by the charge envelope and prove a walking route around it.
    threshold = 42  # The player capsule radius in AEidolonCharacter.
    first_x = limit_x - body_length / 2 - threshold
    last_x = spawn_x + body_length / 2 + threshold
    lane = {(x, y) for x, y in gallery
            if x * size < last_x and (x + 1) * size > first_x
            and y * size < lane_y + hazard + threshold
            and (y + 1) * size > lane_y - hazard - threshold}
    bypass = visit(cells - lane, start, gate_edge)
    if latch not in bypass or any(not recess <= bypass for recess in recesses):
        raise ValueError("Both recesses and the inner latch must be reachable outside the charge envelope.")
    if size - WALL_THICKNESS < 2 * threshold + 50 or size >= body_width:
        raise ValueError("The sheltered approach must leave walking clearance and exclude the Borer.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    build_shell(layout, cells, box)
    gate = layout["gate"]
    box("abyss_021_gate", gate["center_cm"], gate["dimensions_cm"], movable=True)
    gx, gy, _ = gate["center_cm"]
    depth, width, clearance = gate["dimensions_cm"]
    roof = layout["height_cm"]
    box("quarry_gate_lintel", (gx, gy, (roof + clearance) / 2), (depth, width, roof - clearance))

    for name, location in layout["anchors_cm"].items():
        marker = unreal.PlayerStart if name in ("player_start", "arrival_recovery", "checkpoint_start") else unreal.TargetPoint
        actor = spawn(marker, f"abyss_021_{name}", location)
        if name == "borer_spawn":
            actor.set_actor_rotation(unreal.Rotator(yaw=180), False)
    box("quarry_latch_post", (2690, 535, 63), (55, 95, 126))
    for index, (x, y) in enumerate(((250, 100), (550, -294), (1050, 90),
                                   (1350, -505), (1825, 110), (2190, 650), (2630, 410), (3190, 100)), 1):
        actor = spawn(unreal.PointLight, f"quarry_light_{index:02d}", (x, y, roof - 70))
        light = actor.get_component_by_class(unreal.PointLightComponent)
        light.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity", 1950.0)
        light.set_editor_property("attenuation_radius", 900.0)
    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Quarry Threshold at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, sheltered arrivals, two recesses and inner release.")
    else:
        import unreal

        build(unreal, layout, cells)

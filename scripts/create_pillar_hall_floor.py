"""Build the Hall of Pillars or validate the Abyss floor-seven geometry offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor007"
MODE_PATH = "/Script/Eidolon.EidolonPillarHallGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_007_authored.json"
ANCHORS = ("entry", "checkpoint", "exit")
SOLVED_DIRECTIONS = {90: (0, 1), 180: (-1, 0)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Hall of Pillars needs unique cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("Expected the 200 cm catacomb kit with 300 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 6:
        raise ValueError("The descent requires the floor-six receipt.")

    entry, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The gate must face north between walkable cells.")
    gate_edge = frozenset((near, far))
    before_gate = visit(cells, entry, gate_edge)
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The gate must be the only route to the lower stair.")

    hall = layout["hall"]
    bays, cross_rows = hall["bay_columns"], hall["cross_passage_rows"]
    if len(bays) != 3 or len(cross_rows) != 3 or set(map(tuple, hall["blocked_piers"])) & cells:
        raise ValueError("The hall needs three bays, three cross passages and solid pillars.")
    if any((x, y) not in before_gate for y in cross_rows for x in range(bays[0][0], bays[-1][1] + 1)):
        raise ValueError("Every cross passage must span all three bays.")

    mirrors = layout["mirrors"]
    if [mirror["solved_yaw_deg"] for mirror in mirrors] != [90, 180]:
        raise ValueError("The receiver bearings must agree with the native mirror rules.")
    size = layout["cell_cm"]
    for mirror in mirrors:
        px, py, pz = mirror["pivot_cm"]
        rx, ry, rz = mirror["receiver_position_cm"]
        dx, dy = SOLVED_DIRECTIONS[mirror["solved_yaw_deg"]]
        distance = (rx - px) * dx + (ry - py) * dy
        if mirror["initial_yaw_deg"] != 0 or distance <= 0 or (px + distance * dx, py + distance * dy, pz) != (rx, ry, rz):
            raise ValueError("Each plaque must lie along its mirror's solved bearing.")
        if any((int(px + step * dx) // size, int(py + step * dy) // size) not in before_gate
               for step in range(0, int(distance) + 1)):
            raise ValueError("A wall or pillar obstructs the mirror's sightline.")

    leeches = layout["leech_anchors"]
    avoidance = {tuple(cell) for cell in leeches["avoidance_cut_cells"]}
    if not set(map(tuple, leeches["cells"])) <= avoidance <= before_gate or leeches["defeat_required"]:
        raise ValueError("Both leech positions must stay optional.")
    safe_route = visit(before_gate - avoidance, entry)
    controls = {tuple(mirror["control_cell"]) for mirror in mirrors}
    safe_positions = {tuple(cell) for cell in hall["safe_approach_cells"]}
    if not (controls | safe_positions | {near}) <= safe_route:
        raise ValueError("The optional leeches must have a covered bypass to both controls and the gate.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    build_shell(layout, cells, box)

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the graybox display mesh.")

    def display_mesh(tag, position, dimensions, movable=False):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        component.set_mobility(
            unreal.ComponentMobility.MOVABLE if movable else unreal.ComponentMobility.STATIC
        )
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the display mesh to {tag}.")
        actor.set_actor_scale3d(unreal.Vector(*(distance / 100 for distance in dimensions)))
        actor.set_actor_enable_collision(False)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    size = layout["cell_cm"]
    height = layout["height_cm"]
    thickness = layout["wall_thickness_cm"]
    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gy += size / 2
    width, clearance = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - width) / 2
    for sign in (-1, 1):
        box(f"pillar_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("pillar_gate_header", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    for mirror in layout["mirrors"]:
        x, y = center(mirror["control_cell"], size)
        spawn(unreal.TargetPoint, mirror["id"], (x, y, 110))
        px, py, pz = mirror["pivot_cm"]
        display_mesh(mirror["mesh_id"], (px, py, pz), (26, 88, 112), movable=True)
        display_mesh(mirror["pointer_id"], (px + layout["pointer_radius_cm"], py, pz),
                     (66, 19, 19), movable=True)

        rx, ry, rz = mirror["receiver_position_cm"]
        spawn(unreal.TargetPoint, mirror["receiver_id"], (rx, ry, rz))
        dx, dy = SOLVED_DIRECTIONS[mirror["solved_yaw_deg"]]
        is_north = bool(dy)
        plaque_size = (90, 20, 95) if is_north else (20, 90, 95)
        display_mesh(mirror["receiver_id"] + "_plaque", (rx, ry, rz), plaque_size)
        marks = mirror["plaque_marks"]
        for index in range(marks):
            spread = (index - (marks - 1) / 2) * 22
            mark_position = (rx - dx * 14 + abs(dy) * spread, ry - dy * 14 + abs(dx) * spread, rz)
            mark_size = (12, 8, 52) if is_north else (8, 12, 52)
            display_mesh(f"{mirror['receiver_id']}_mark_{index + 1}", mark_position, mark_size)

    for index, cell in enumerate(layout["leech_anchors"]["cells"], 1):
        x, y = center(cell, size)
        spawn(unreal.TargetPoint, f"abyss_007_mortar_leech_{index:02d}", (x, y, 0))

    for name in ANCHORS:
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_007_{name}", (x, y, 110))
    for name, tag in (("entry", "abyss_007_player_start"), ("checkpoint", "abyss_007_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    hall = layout["hall"]
    light_cells = {tuple(layout[name]) for name in ANCHORS}
    light_cells.update((x, y) for x in (2, 5, 9) for y in hall["cross_passage_rows"])
    light_cells.update(tuple(mirror["control_cell"]) for mirror in layout["mirrors"])
    for cell in sorted(light_cells):
        x, y = center(cell, size)
        light = spawn(unreal.PointLight, f"pillar_light_{cell[0]:02d}_{cell[1]:02d}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1700.0)
        component.set_editor_property("attenuation_radius", 750.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Hall of Pillars at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, three bays, two mirror sightlines and optional leech bypasses.")
    else:
        import unreal

        build(unreal, layout, cells)

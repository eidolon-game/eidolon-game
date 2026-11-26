"""Build the double-ring Ambulatory or validate its authored grid offline."""

from collections import deque
import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, neighbors, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor006"
MODE_PATH = "/Script/Eidolon.EidolonAmbulatoryGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_006_authored.json"
ANCHORS = ("entry", "checkpoint", "sarcophagus", "winch", "exit")


def ring_cells(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    band = bounds["band_cells"]
    return {
        (x, y)
        for x in range(x0, x1 + 1)
        for y in range(y0, y1 + 1)
        if min(x - x0, x1 - x, y - y0, y1 - y) < band
    }


def shortest_distance(cells, start, end):
    queue = deque([(start, 0)])
    seen = {start}
    while queue:
        cell, distance = queue.popleft()
        if cell == end:
            return distance
        for adjacent in neighbors(cell):
            if adjacent in cells and adjacent not in seen:
                seen.add(adjacent)
                queue.append((adjacent, distance + 1))
    raise ValueError("The sarcophagus and winch must be connected.")


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Ambulatory needs unique cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("Expected the 200 cm catacomb kit with 300 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 5:
        raise ValueError("The descent requires the floor-five receipt.")

    entry, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The gate must face north between walkable cells.")
    before_gate = visit(cells, entry, frozenset((near, far)))
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The gate must separate the rings from the lower stair.")
    if not {tuple(layout[name]) for name in ANCHORS[:-1]} <= before_gate:
        raise ValueError("All puzzle interactions must be available before the gate.")

    rings = layout["rings"]
    outer = ring_cells(rings["outer_bounds"])
    inner = ring_cells(rings["inner_bounds"])
    connectors = {tuple(cell) for cell in rings["connectors"]}
    if len(connectors) != 2 or not (outer | inner | connectors) <= before_gate:
        raise ValueError("The inner route needs two connectors to the outer ring.")
    sarcophagus, winch = tuple(layout["sarcophagus"]), tuple(layout["winch"])
    if visit(outer, sarcophagus) != outer or visit(inner, next(iter(inner))) != inner:
        raise ValueError("Each ring must be independently walkable.")
    outer_only = visit(before_gate - connectors, sarcophagus)
    if outer_only & inner or not outer <= outer_only:
        raise ValueError("The inner and outer rings must only meet at their connectors.")
    for connector in connectors:
        if not (set(neighbors(connector)) & inner and set(neighbors(connector)) & outer):
            raise ValueError("Each connector must connect both rings directly.")
    if shortest_distance(outer | inner | connectors, sarcophagus, winch) >= shortest_distance(outer, sarcophagus, winch):
        raise ValueError("The inner route must shorten the counterweight trip.")
    patrol = [tuple(cell) for cell in layout["patrol"]["cells"]]
    if len(patrol) != 4 or len(set(patrol)) != 4:
        raise ValueError("The outer patrol needs four distinct corners.")
    for (ax, ay), (bx, by) in zip(patrol, patrol[1:] + patrol[:1]):
        if ax != bx and ay != by:
            raise ValueError("Each outer patrol segment must be straight and axis-aligned.")
        segment = {(x, y) for x in range(min(ax, bx), max(ax, bx) + 1)
                   for y in range(min(ay, by), max(ay, by) + 1)}
        if not segment <= outer:
            raise ValueError("Every tile of the patrol must stay on the outer ring.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
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
        box(f"ambulatory_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("ambulatory_gate_header", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    for key in ("sarcophagus_prop", "winch_prop"):
        prop = layout[key]
        box(prop["id"], prop["position_cm"], prop["dimensions_cm"])

    weight = layout["counterweight"]
    actor = spawn(unreal.StaticMeshActor, weight["id"], weight["position_cm"])
    component = actor.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube or not component.set_static_mesh(cube):
        raise RuntimeError("Could not assign the counterweight proxy mesh.")
    actor.set_actor_scale3d(unreal.Vector(*(dimension / 100 for dimension in weight["dimensions_cm"])))
    actor.set_actor_enable_collision(False)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        spawn(unreal.TargetPoint, f"abyss_006_{anchor}", (x, y, 110))
    seat = layout["counterweight_seat"]
    spawn(unreal.TargetPoint, seat["id"], seat["position_cm"])
    for index, cell in enumerate(layout["patrol"]["cells"], 1):
        x, y = center(cell, size)
        spawn(unreal.TargetPoint, f"abyss_006_patrol_{index:02d}", (x, y, 0))

    rings = layout["rings"]
    light_cells = {tuple(layout[name]) for name in ANCHORS}
    light_cells.update(tuple(cell) for cell in rings["connectors"])
    light_cells.update(tuple(cell) for cell in layout["patrol"]["cells"])
    for cell in sorted(light_cells):
        x, y = center(cell, size)
        light = spawn(unreal.PointLight, f"ambulatory_light_{cell[0]:02d}_{cell[1]:02d}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1800.0)
        component.set_editor_property("attenuation_radius", 800.0)

    for name, tag in (("entry", "abyss_006_player_start"), ("checkpoint", "abyss_006_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Ambulatory at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, two rings and a gated exit.")
    else:
        import unreal

        build(unreal, layout, cells)

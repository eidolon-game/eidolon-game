"""Build the Abyss floor-four Sluice Conduits or validate its data offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor004"
MODE_PATH = "/Script/Eidolon.EidolonSluiceGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_004_authored.json"
ANCHORS = ("entry", "checkpoint", "sluice_west", "sluice_east", "exit")
INTERACTION_RADIUS_CM = 180


def basin_cells(basin):
    x0, y0 = basin["min_cell"]
    x1, y1 = basin["max_cell"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Sluice Conduits need unique cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("Expected the 200 cm catacomb kit with 300 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 3:
        raise ValueError("The descent requires the floor-three receipt.")

    start, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The Sluice Conduits gate must face north between walkable cells.")
    gate_edge = frozenset((near, far))
    before_gate = visit(cells, start, gate_edge)
    if visit(cells, start) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The gate must divide the controls from the exit.")

    west, east = tuple(layout["sluice_west"]), tuple(layout["sluice_east"])
    required = {tuple(layout["checkpoint"]), west, east}
    if not required <= before_gate:
        raise ValueError("Both sluice controls and the checkpoint must be reachable before the gate.")
    if layout["mechanism"]["timed"] or layout["accessibility"]["mandatory_timed_traversal"]:
        raise ValueError("The route cannot require timed traversal.")
    size = layout["cell_cm"]
    for a, b in ((west, east), (west, exit_cell), (east, exit_cell)):
        dx, dy = (a[0] - b[0]) * size, (a[1] - b[1]) * size
        if dx * dx + dy * dy <= (2 * INTERACTION_RADIUS_CM) ** 2:
            raise ValueError("Sluice and exit interactions must have separate physical zones.")

    basin = layout["water"]
    wet = basin_cells(basin)
    if not wet or not wet <= before_gate or basin["collision"]:
        raise ValueError("The water is a nonblocking visual on the accessible floor grid.")
    path = layout["raised_footpath"]
    raised = {tuple(cell) for cell in path["cells"]}
    if not raised <= wet or path["required"] or not 0 < path["height_cm"] <= 20:
        raise ValueError("The raised footpath must be a low optional route in the basin.")
    if not required | {near} <= visit(cells - raised, start, gate_edge):
        raise ValueError("The controls and gate must have a route without the raised footpath.")
    if tuple(layout["moored_hagfish"]["cell"]) not in wet - raised:
        raise ValueError("The Hagfish anchor must be in the basin and clear of the raised footpath.")
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
        box(f"sluice_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("sluice_gate_header", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    basin = layout["water"]
    minimum, maximum = center(basin["min_cell"], size), center(basin["max_cell"], size)
    wx, wy = ((a + b) / 2 for a, b in zip(minimum, maximum))
    depth = basin["height_cm"]
    water = spawn(unreal.StaticMeshActor, basin["id"], (wx, wy, depth / 2))
    component = water.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube or not component.set_static_mesh(cube):
        raise RuntimeError("Could not assign the water plane mesh.")
    water.set_actor_enable_collision(False)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    water.set_actor_scale3d(unreal.Vector(
        (maximum[0] - minimum[0] + size) / 100, (maximum[1] - minimum[1] + size) / 100, depth / 100,
    ))

    path = layout["raised_footpath"]
    for index, cell in enumerate(path["cells"]):
        x, y = center(cell, size)
        box(f"sluice_footpath_{index:02d}", (x, y, path["height_cm"] / 2),
            (size - 12, path["width_cm"], path["height_cm"]))

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        spawn(unreal.TargetPoint, f"abyss_004_{anchor}", (x, y, 110))
        light = spawn(unreal.PointLight, f"light_{anchor}", (x, y, height - 40))
        light_component = light.get_component_by_class(unreal.PointLightComponent)
        light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light_component.set_editor_property("intensity", 1750.0)
        light_component.set_editor_property("attenuation_radius", 650.0)

    hagfish = layout["moored_hagfish"]
    x, y = center(hagfish["cell"], size)
    spawn(unreal.TargetPoint, hagfish["spawn_marker"], (x, y, 0))
    for name, tag in (("entry", "abyss_004_player_start"), ("checkpoint", "abyss_004_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Sluice Conduits at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} walkable cells, independent sluices and a gated exit.")
    else:
        import unreal

        build(unreal, layout, cells)

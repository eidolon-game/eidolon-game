"""Build the Fern Basin or validate its two drain approaches offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor014"
MODE_PATH = "/Script/Eidolon.EidolonFernBasinGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_014_authored.json"
ANCHORS = ("entry", "checkpoint", "drain_upper", "drain_lower", "exit")


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def heights(layout):
    return {(x, y): z for x, y, z in layout["surface_heights_cm"]}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    entry, checkpoint, upper, lower, exit_cell = (tuple(layout[name]) for name in ANCHORS)
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, upper, lower, exit_cell} <= cells:
        raise ValueError("The basin needs unique floor cells and valid drain and player anchors.")
    if layout["cell_cm"] != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 13:
        raise ValueError("Expected the 200 cm kit after the Branch Terraces.")

    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0] + 1, near[1]) or gate["requires_durable_drains"] != 2:
        raise ValueError("The exit gate must cross the service route after both drain receipts.")
    before_gate = visit(cells, entry, frozenset((near, far)))
    if visit(cells, entry) != cells or far in before_gate or exit_cell in before_gate:
        raise ValueError("The closed outlet gate must separate the basin from the lower stair.")
    if not {checkpoint, near, upper, lower} <= before_gate:
        raise ValueError("Both drains and the checkpoint must be available before the gate.")

    profiles = heights(layout)
    if len(profiles) != len(layout["surface_heights_cm"]) or not profiles.keys() <= cells:
        raise ValueError("Elevated surfaces need unique authored height entries.")
    for x, y in cells:
        for dx, dy in ((1, 0), (0, 1)):
            adjacent = x + dx, y + dy
            if adjacent in cells and abs(profiles.get((x, y), 0) - profiles.get(adjacent, 0)) > layout["maximum_step_cm"]:
                raise ValueError("Service route risers and bank joins must be walkable.")

    route = layout["service_route"]
    service = set(map(tuple, route["cells"]))
    upper_pad, lower_pad = (set(map(tuple, route[key])) for key in ("upper_pad", "lower_pad"))
    if not service <= before_gate or not {entry, upper, lower, near} <= visit(service, entry):
        raise ValueError("An elevated or dry service route must join both drains without the central basin.")
    if len(upper_pad) != 4 or len(lower_pad) != 4 or not (upper_pad | lower_pad) <= service:
        raise ValueError("Both command pads need 400 cm of landing space.")
    if upper not in upper_pad or lower not in lower_pad or route["minimum_width_cm"] < 200:
        raise ValueError("Drain controls must stand on pads along a clear 200 cm service route.")

    stalker = layout["stalker"]
    sx, sy, sz = stalker["foot_cm"]
    size = layout["cell_cm"]
    if stalker["required"] or sz or (sx // size, sy // size) not in before_gate - service:
        raise ValueError("The optional stalker must start at ground level inside the basin.")
    for x, y in service:
        distance = math.hypot(max(x * size - sx, 0, sx - (x + 1) * size),
                              max(y * size - sy, 0, sy - (y + 1) * size))
        if distance <= stalker["home_radius_cm"]:
            raise ValueError("The entire service bypass must remain beyond the stalker's ground leash.")

    water = layout["water"]
    surface = rectangle(water)
    if not surface <= before_gate - service or (sx // size, sy // size) not in surface:
        raise ValueError("The water visual must stay in the main basin and clear the bypass.")
    top = water["initial_center_z_cm"] + water["thickness_cm"] / 2
    if water["collision"] or not 0 < top - water["partial_drop_cm"] < top or top - water["complete_drop_cm"] >= 0:
        raise ValueError("Visual water levels must drop twice and clear the final step.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    profiles = heights(layout)
    for cell in sorted(cells):
        x, y = center(cell, size)
        z = profiles.get(cell, 0)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        fill_depth = 18 + z
        box(f"fern_basin_floor_{label}", (x, y, z - fill_depth / 2), (size, size, fill_depth))
        box(f"fern_basin_canopy_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            # Put the inner face at the grid boundary to keep a full 200 cm of clear walkway.
            wx = x + dx * (size + wall) / 2
            wy = y + dy * (size + wall) / 2
            span = size + 2 * wall
            dims = (wall, span, roof) if dx else (span, wall, roof)
            box(f"fern_basin_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    rock = layout["screening_rock"]
    box("fern_basin_screening_rock", rock["center_cm"], rock["dimensions_cm"])

    water = layout["water"]
    min_xy, max_xy = center(water["minimum"], size), center(water["maximum"], size)
    wx, wy = ((a + b) / 2 for a, b in zip(min_xy, max_xy))
    actor = spawn(unreal.StaticMeshActor, water["id"], (wx, wy, water["initial_center_z_cm"]))
    component = actor.static_mesh_component
    component.set_mobility(unreal.ComponentMobility.MOVABLE)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube or not component.set_static_mesh(cube):
        raise RuntimeError("Could not assign the Fern Basin water surface mesh.")
    actor.set_actor_scale3d(unreal.Vector((max_xy[0] - min_xy[0] + size) / 100,
                                         (max_xy[1] - min_xy[1] + size) / 100,
                                         water["thickness_cm"] / 100))
    actor.set_actor_enable_collision(False)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gx += size / 2
    gz = profiles.get(tuple(gate["from_cell"]), 0)
    for sign in (-1, 1):
        box(f"fern_gate_jamb_{sign}", (gx, gy + sign * 87.5, (gz + roof) / 2), (25, 25, roof - gz))
    lintel = roof - gz - 240
    box("fern_gate_lintel", (gx, gy, gz + 240 + lintel / 2), (25, 150, lintel))
    box(gate["id"], (gx, gy, gz + 120), (25, 150, 240), movable=True)

    for name in ("drain_upper", "drain_lower", "exit"):
        cell = tuple(layout[name])
        x, y = center(cell, size)
        z = profiles.get(cell, 0) + 110
        spawn(unreal.TargetPoint, f"abyss_014_{name}", (x, y, z))
        if name != "exit":
            # Leave the target point itself clear for interaction sight checks.
            box(f"fern_basin_{name}_wheel_proxy", (x, y + 58, z - 45), (39, 27, 130))
    for name in ("entry", "checkpoint"):
        cell = tuple(layout[name])
        x, y = center(cell, size)
        tag = "abyss_014_player_start" if name == "entry" else "abyss_014_checkpoint_start"
        start = spawn(unreal.PlayerStart, tag, (x, y, profiles.get(cell, 0) + 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    stalker = layout["stalker"]
    spawn(unreal.TargetPoint, stalker["tag"], stalker["foot_cm"])

    lights = {tuple(layout[name]) for name in ANCHORS} | {(0, 9), (3, 12), (10, 12), (14, 9), (7, 8), (12, 5)}
    for index, cell in enumerate(sorted(lights), 1):
        x, y = center(cell, size)
        light = spawn(unreal.PointLight, f"fern_basin_light_{index:02d}",
                      (x, y, profiles.get(cell, 0) + 345))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1850.0)
        component.set_editor_property("attenuation_radius", 1000.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Fern Basin at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, two drain orders, safe perimeter and three water stages.")
    else:
        import unreal

        build(unreal, layout, cells)

"""Build the Suspended Trunks or validate its two bridge poses offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor017"
MODE_PATH = "/Script/Eidolon.EidolonSuspendedTrunksGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_017_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def cell_edges(a, b):
    return {((x, y), other) for x, y in a for dx, dy in DIRECTIONS
            if (other := (x + dx, y + dy)) in b}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    shore, island, side, far, outlet = (rectangle(layout[name]) for name in
                                       ("shore", "island", "side_dock", "far_dock", "outlet"))
    entry, checkpoint, exit_cell = (tuple(layout[key]) for key in ("entry", "checkpoint", "exit"))
    regions = (shore, island, side, far, outlet)
    if cells != set().union(*regions) or len(cells) != sum(map(len, regions)) or len(cells) != len(layout["cells"]):
        raise ValueError("The fixed and rotating spans need separate, non-overlapping shores.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 16:
        raise ValueError("Expected the 200 cm kit after the Rootlet Canal.")
    if not {entry, checkpoint} <= shore or exit_cell not in outlet:
        raise ValueError("The first checkpoint and far exit must remain on their own shores.")

    gated = {tuple(map(tuple, pair)) for pair in layout["gate"]["edges"]}
    if any(cell_edges(a, b) for index, a in enumerate(regions[:4]) for b in regions[index + 1:4]):
        raise ValueError("The fixed crossing and rotating bridge must span physical gaps.")
    if cell_edges(far, outlet) != gated or any(cell_edges(region, outlet) for region in regions[:3]):
        raise ValueError("The only approach to the outlet must be the gated far shore.")
    if visit(cells, entry) != shore or visit(cells, next(iter(island))) != island:
        raise ValueError("The entry and island cannot reach the exit without the crossings.")
    if visit(far | outlet, exit_cell) != far | outlet:
        raise ValueError("The far dock needs a continuous route through the gate to the outlet.")

    fixed = layout["fixed_crossing"]
    fx, fy, fz = fixed["center_cm"]
    fl, fw, ft = fixed["dimensions_cm"]
    if fw < 220 or fz + ft / 2 != 0:
        raise ValueError("The fixed crossing must be flush with 220 cm of clear width.")
    if not (fx - fl / 2 <= (layout["shore"]["maximum"][0] + 1) * size
            < layout["island"]["minimum"][0] * size <= fx + fl / 2):
        raise ValueError("The fixed crossing must overlap its shore and island at both ends.")

    bridge = layout["rotating_bridge"]
    bx, by, bz = bridge["center_cm"]
    bl, bw, bt = bridge["dimensions_cm"]
    cx, cy, _ = bridge["control_cm"]
    island_min, island_max = layout["island"]["minimum"], layout["island"]["maximum"]
    min_x, min_y = (coordinate * size for coordinate in island_min)
    max_x, max_y = ((coordinate + 1) * size for coordinate in island_max)
    if min(max_x - min_x, max_y - min_y) - layout["wall_thickness_cm"] < layout["island"]["minimum_side_cm"]:
        raise ValueError("The island must have at least 650 cm of clear maneuvering space.")
    if not (min_x < cx < max_x and min_y < cy < max_y) or (bx, by) != ((min_x + max_x) / 2, (min_y + max_y) / 2):
        raise ValueError("The bridge must pivot in the middle of the stable island with controls on the island.")
    if bw < 250 or bz + bt / 2 != 0 or (bridge["side_yaw_deg"], bridge["open_yaw_deg"]) != (90, 0):
        raise ValueError("The rotating span must dock flush to the north and east.")
    if cx <= bx or cy >= by or min(cx - bx, by - cy) <= bw / 2 + bridge["pawn_radius_cm"]:
        raise ValueError("The southeast control must stay outside the swept bridge footprint.")

    side_y = layout["side_dock"]["minimum"][1] * size
    far_x = layout["far_dock"]["minimum"][0] * size
    if not 0 <= by + bl / 2 - side_y <= 20 or not 0 <= bx + bl / 2 - far_x <= 20:
        raise ValueError("Each bridge pose must overlap its active dock by no more than 20 cm.")
    for axis, mid in (("side_contact", bx), ("far_contact", by)):
        low, high = bridge[axis]["boundary_cm"]
        if not low <= mid - bw / 2 < mid + bw / 2 <= high:
            raise ValueError("Each dock must accept the full bridge width.")
    if fy + fw / 2 >= by - bw / 2:
        raise ValueError("The fixed and moving walks must not overlap in the open pose.")

    for stalker in layout["stalkers"]:
        x, y, _ = stalker["feet_cm"]
        if (x // size, y // size) not in rectangle(layout[stalker["sector"]]):
            raise ValueError("Each stalker must have solid ground in its own sector.")
        d = math.hypot(max(min_x - x, 0, x - max_x), max(min_y - y, 0, y - max_y))
        if d < layout["minimum_island_sector_distance_cm"] or d <= layout["stalker_home_radius_cm"]:
            raise ValueError("The island must stay outside both stalker sectors.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Suspended Trunks proxy mesh.")

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    fixed = layout["fixed_crossing"]
    bridge = layout["rotating_bridge"]
    cuts = {(tuple(item["cell"]), tuple(item["normal"]))
            for item in fixed["opens_at"] + bridge["opens_at"]}
    for cell in sorted(cells):
        x, y = center(cell, size)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        box(f"trunks_floor_{label}", (x, y, -9), (size, size, 18))
        box(f"trunks_canopy_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells or (cell, (dx, dy)) in cuts:
                continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            dims = (wall, size + wall, roof) if dx else (size + wall, wall, roof)
            box(f"trunks_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    def movable(tag, position, dimensions, yaw):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        mesh = actor.static_mesh_component
        mesh.set_mobility(unreal.ComponentMobility.MOVABLE)
        if not mesh.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the rotating bridge mesh to {tag}.")
        mesh.set_collision_profile_name("BlockAllDynamic")
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        actor.set_actor_rotation(unreal.Rotator(yaw=yaw), False)
        return actor

    fx, fy, _ = fixed["center_cm"]
    fl, fw, _ = fixed["dimensions_cm"]
    box("trunks_fixed_crossing", fixed["center_cm"], fixed["dimensions_cm"])
    for sign in (-1, 1):
        box(f"trunks_fixed_guard_{sign}", (fx, fy + sign * (fw / 2 + 12.5), 95), (fl, 25, 190))
    for x in (2600, 4000):
        for y0, y1 in ((2000, fy - fw / 2), (fy + fw / 2, 2400)):
            box(f"trunks_fixed_frame_{x}_{y0}", (x, (y0 + y1) / 2, roof / 2),
                (25, y1 - y0, roof))

    bx, by, bz = bridge["center_cm"]
    bl, bw, _ = bridge["dimensions_cm"]
    rotating = movable(bridge["id"], bridge["center_cm"], bridge["dimensions_cm"], bridge["side_yaw_deg"])
    arm_length = bl / 2 - 405

    def attach_span(actor):
        keep = unreal.AttachmentRule.KEEP_WORLD
        if not actor.attach_to_actor(rotating, "", keep, keep, keep, False):
            raise RuntimeError("Could not attach a safety rail to the rotating bridge.")

    # These rails run only outside the stable island, with an end cap on the unused arm.
    for direction in (-1, 1):
        for side in (-1, 1):
            rail = movable(f"trunks_rotating_guard_{direction}_{side}",
                           (bx - side * (bw / 2 + 12.5), by + direction * (405 + arm_length / 2), 95),
                           (arm_length, 25, 190), bridge["side_yaw_deg"])
            attach_span(rail)
    attach_span(movable("trunks_unused_arm_end", (bx, by - bl / 2 + 12.5, 100),
                        (25, bw + 50, 200), bridge["side_yaw_deg"]))
    open_pose = spawn(unreal.TargetPoint, "abyss_017_bridge_open_pose", (bx, by, bz))
    open_pose.set_actor_rotation(unreal.Rotator(yaw=bridge["open_yaw_deg"]), False)
    spawn(unreal.TargetPoint, "abyss_017_bridge_control", bridge["control_cm"])

    gate = layout["gate"]
    gx, gy, gz = gate["center_cm"]
    depth, gap, clearance = gate["dimensions_cm"]
    jamb = (2 * size - gap) / 2
    for sign in (-1, 1):
        box(f"trunks_gate_jamb_{sign}", (gx, gy + sign * (gap + jamb) / 2, roof / 2),
            (depth, jamb, roof))
    header = roof - clearance
    box("trunks_gate_lintel", (gx, gy, clearance + header / 2), (depth, gap, header))
    box(gate["id"], (gx, gy, gz), gate["dimensions_cm"], movable=True)

    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_017_player_start" if name == "entry" else "abyss_017_checkpoint_start"
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    x, y = center(layout["exit"], size)
    spawn(unreal.TargetPoint, "abyss_017_exit", (x, y, 110))
    for stalker in layout["stalkers"]:
        spawn(unreal.TargetPoint, stalker["tag"], stalker["feet_cm"])
    for index, (x, y) in enumerate(((500, 2300), (2100, 2400), (2500, 2100), (4400, 2400),
                                    (4440, 3850), (5940, 2390), (7100, 2400), (8100, 2300)), 1):
        light = spawn(unreal.PointLight, f"suspended_trunks_light_{index:02d}", (x, y, 420))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1900.0)
        component.set_editor_property("attenuation_radius", 1300.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Suspended Trunks at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} shore cells, flush bridge poses and separate stalker sectors.")
    else:
        import unreal

        build(unreal, layout, cells)

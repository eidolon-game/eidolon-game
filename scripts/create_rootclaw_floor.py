"""Build the Root Antechamber graybox or validate the boss lanes offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor010"
MODE_PATH = "/Script/Eidolon.EidolonRootclawGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_010_authored.json"
ANCHORS = ("entry", "checkpoint", "encounter", "exit")


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    entry, checkpoint, encounter, exit_cell = (tuple(layout[name]) for name in ANCHORS)
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, encounter, exit_cell} <= cells:
        raise ValueError("The Root Antechamber needs unique floor cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 420):
        raise ValueError("Expected the 200 cm catacomb kit with boss headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 9:
        raise ValueError("The descent requires the floor-nine receipt.")

    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The boss gate must face north between walkable cells.")
    gate_edge = frozenset((near, far))
    before_gate = visit(cells, entry, gate_edge)
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The path to the lower stair must pass through the boss gate.")

    arena = layout["arena"]
    arena_cells = rectangle(arena)
    if not arena_cells <= before_gate or encounter not in arena_cells or checkpoint in arena_cells:
        raise ValueError("The player checkpoint must stay on the approach before the unobstructed arena.")
    size = layout["cell_cm"]
    ax, ay = arena["minimum"]
    bx, by = arena["maximum"]
    widths = ((bx - ax + 1) * size, (by - ay + 1) * size)
    if widths != (2000, 2400) or list(widths) != arena["dimensions_cm"]:
        raise ValueError("The original boss arena must measure 2000 by 2400 cm.")
    lanes = arena["lanes"]
    if len(lanes) != 3 or len(arena["crossing_rows"]) != 3:
        raise ValueError("The boss arena needs three longitudinal lanes and three crossovers.")
    previous_bound = ax * size + layout["wall_thickness_cm"] / 2
    for lane in lanes:
        low, high = lane["x_range_cm"]
        mx, my, mz = lane["marker_cm"]
        if low < previous_bound or high - low < 600 or mx != (low + high) / 2 or mz != 0:
            raise ValueError("Each non-overlapping lane requires at least 600 cm clear width.")
        if (mx // size, my // size) not in arena_cells:
            raise ValueError("The boss lane marker must be on the open arena floor.")
        previous_bound = high
    if previous_bound > (bx + 1) * size - layout["wall_thickness_cm"] / 2:
        raise ValueError("Side lanes must clear the outer wall collision.")
    for first, last in arena["crossing_rows"]:
        route = {(x, y) for x in range(ax, bx + 1) for y in range(first, last + 1)}
        if not route <= arena_cells or (last - first + 1) * size < arena["minimum_crossing_depth_cm"]:
            raise ValueError("Every lane change needs at least 300 cm of clear depth across the arena.")
    if layout["boss"]["spawn_position_cm"] != lanes[1]["marker_cm"]:
        raise ValueError("Rootclaw must start in the middle lane.")

    courtyard = layout["courtyard"]
    court = rectangle(courtyard["main"])
    chambers = courtyard["side_chambers"]
    rooms = [rectangle(chamber) | set(map(tuple, chamber["entrance_cells"])) for chamber in chambers]
    if not court <= before_gate or len(rooms) != 2 or rooms[0] & rooms[1]:
        raise ValueError("The two lateral chambers must be distinct parts of the courtyard.")
    for chamber, room in zip(chambers, rooms):
        if not room <= before_gate or room & arena_cells or tuple(chamber["spawn_cell"]) not in room:
            raise ValueError("Courtyard enemies must spawn in their own chambers outside the arena.")
    approach = visit(before_gate - rooms[0] - rooms[1], entry)
    if not {checkpoint, encounter, near} <= approach or courtyard["common_enemies_in_boss_arena"]:
        raise ValueError("The checkpoint and boss arena must have a clear route past the optional chambers.")
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
        box(f"rootclaw_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("rootclaw_gate_lintel", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    for chamber in layout["courtyard"]["side_chambers"]:
        x, y = center(chamber["trigger_cell"], size)
        spawn(unreal.TargetPoint, chamber["id"], (x, y, 110))
        x, y = center(chamber["spawn_cell"], size)
        enemy = spawn(unreal.TargetPoint, chamber["spawn_marker"], (x, y, 0))
        enemy.set_actor_rotation(unreal.Rotator(yaw=chamber["spawn_yaw_deg"]), False)
    boss = layout["boss"]
    rootclaw = spawn(unreal.TargetPoint, boss["spawn_marker"], boss["spawn_position_cm"])
    rootclaw.set_actor_rotation(unreal.Rotator(yaw=boss["spawn_yaw_deg"]), False)
    for lane in layout["arena"]["lanes"]:
        spawn(unreal.TargetPoint, lane["id"], lane["marker_cm"])

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the graybox lane strip mesh.")

    def paint(tag, position, dimensions):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the lane strip mesh to {tag}.")
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        actor.set_actor_enable_collision(False)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    arena = layout["arena"]
    first_y = arena["minimum"][1] * size
    arena_width, arena_depth = arena["dimensions_cm"]
    mid_y = first_y + arena_depth / 2
    mid_x = arena["minimum"][0] * size + arena_width / 2
    for index, lane in enumerate(arena["lanes"][:-1], 1):
        paint(f"rootclaw_lane_strip_{index}", (lane["x_range_cm"][1], mid_y, 0.6), (12, arena_depth - 80, 1))
    for index, (first, last) in enumerate(arena["crossing_rows"], 1):
        y = (first + last + 1) * size / 2
        paint(f"rootclaw_crossover_strip_{index}", (mid_x, y, 1.2), (arena_width - 130, 15, 1.2))

    for name in ANCHORS:
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_010_{name}", (x, y, 110))
    for name, tag in (("entry", "abyss_010_player_start"), ("checkpoint", "abyss_010_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    lights = {center(layout[name], size) for name in ANCHORS}
    lights.update(center(chamber["spawn_cell"], size) for chamber in layout["courtyard"]["side_chambers"])
    lights.update((lane["marker_cm"][0], (first + last + 1) * size / 2)
                  for lane in arena["lanes"] for first, last in arena["crossing_rows"])
    for index, (x, y) in enumerate(sorted(lights), 1):
        light = spawn(unreal.PointLight, f"rootclaw_light_{index:02d}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1950.0)
        component.set_editor_property("attenuation_radius", 900.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Root Antechamber at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, three clear boss lanes and isolated courtyard chambers.")
    else:
        import unreal

        build(unreal, layout, cells)

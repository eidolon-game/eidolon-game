"""Build the quiet Oblique Clearing or validate its traversal offline."""

import json
import math
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor012"
MODE_PATH = "/Script/Eidolon.EidolonObliqueClearingGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_012_authored.json"
FLOOR_THICKNESS_CM = 18
PLAYER_OFFSET_CM = 110


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def incline_run(layout):
    incline = layout["incline"]
    size = layout["cell_cm"]
    return incline["minimum"][0] * size, (incline["maximum"][0] + 1) * size


def surface_height(layout, overlook_cells, cell, x):
    if cell in overlook_cells:
        return layout["overlook"]["height_cm"]
    x0, x1 = incline_run(layout)
    incline = layout["incline"]
    fraction = min(1, max(0, (x - x0) / (x1 - x0)))
    start, end = incline["start_height_cm"], incline["end_height_cm"]
    return start + fraction * (end - start)


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    entry, checkpoint, exit_cell = (tuple(layout[name]) for name in ("entry", "checkpoint", "exit"))
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, exit_cell} <= cells:
        raise ValueError("The clearing needs unique floor cells and valid start, checkpoint and exit anchors.")
    if layout["cell_cm"] != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 11:
        raise ValueError("Expected the 200 cm floor kit after the floor-eleven receipt.")
    if layout["encounters"] or layout["accessibility"]["mandatory_timed_traversal"]:
        raise ValueError("This clearing must remain free of combat and timing pressure.")

    overlook = layout["overlook"]
    approach = tuple(overlook["approach_cell"])
    terrace = rectangle(overlook["terrace"])
    overlook_cells = terrace | {approach}
    ramp = rectangle(layout["incline"])
    landing, clearing = (rectangle(layout[name]) for name in ("lower_landing", "upper_clearing"))
    exit_walk = set(map(tuple, layout["exit_walk"]))
    regions = (landing, ramp, clearing, exit_walk, overlook_cells)
    if cells != set().union(*regions) or len(cells) != sum(map(len, regions)):
        raise ValueError("The floor must consist of distinct landings, incline, terrace and exit walk.")
    if visit(cells, entry) != cells or exit_cell not in visit(cells - overlook_cells, entry):
        raise ValueError("The overlook must be reachable and optional for the descent.")
    if not {entry, checkpoint} <= landing or exit_cell not in exit_walk or overlook["required"]:
        raise ValueError("The checkpoint belongs on the stable landing and the descent on the upper walk.")

    size = layout["cell_cm"]
    x0, x1 = incline_run(layout)
    start, end = (layout["incline"][name] for name in ("start_height_cm", "end_height_cm"))
    if not 0 < (end - start) / (x1 - x0) <= layout["incline"]["maximum_grade"] <= 0.25:
        raise ValueError("The incline must stay within a 25 percent walking grade.")
    if layout["roof_height_cm"] - end < 300:
        raise ValueError("The upper clearing needs adequate camera and character headroom.")

    ramp_edge = (approach[0], approach[1] - 1)
    if ramp_edge not in ramp:
        raise ValueError("The overlook must branch directly off the middle of the incline.")
    join_heights = [surface_height(layout, overlook_cells, ramp_edge, edge * size)
                    for edge in (approach[0], approach[0] + 1)]
    if max(abs(height - overlook["height_cm"]) for height in join_heights) > overlook["maximum_join_step_cm"]:
        raise ValueError("The overlook side step exceeds the walking tolerance.")

    guards = set(map(tuple, overlook["guarded_edge_cells"]))
    north_edge = {(x, y) for x, y in terrace if (x, y + 1) not in cells}
    if not guards or guards != north_edge or not layout["accessibility"]["overlook_has_guard"]:
        raise ValueError("Every exposed overlook edge needs an uninterrupted safety guard.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the clearing graybox mesh.")

    def mesh_box(tag, position, dimensions, rotation=None, hidden=False):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.STATIC)
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the mesh to {tag}.")
        component.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*(height / 100 for height in dimensions)))
        if rotation is not None:
            actor.set_actor_rotation(rotation, False)
        if hidden:
            actor.set_actor_hidden_in_game(True)

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    ramp = rectangle(layout["incline"])
    overlook = layout["overlook"]
    overlook_cells = rectangle(overlook["terrace"]) | {tuple(overlook["approach_cell"])}
    guard_cells = set(map(tuple, overlook["guarded_edge_cells"]))
    for cell in sorted(cells):
        x, y = center(cell, size)
        z = surface_height(layout, overlook_cells, cell, x)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell not in ramp:
            box(f"clearing_floor_{label}", (x, y, z - FLOOR_THICKNESS_CM / 2),
                (size, size, FLOOR_THICKNESS_CM))
        box(f"clearing_canopy_{label}", (x, y, roof + FLOOR_THICKNESS_CM / 2),
            (size, size, FLOOR_THICKNESS_CM))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            span = size + wall
            if cell in guard_cells and dy == 1:
                box(f"overlook_parapet_{label}", (wx, wy, z + 55), (span, wall, 110))
                mesh_box(f"overlook_safety_{label}", (wx, wy, (z + roof) / 2),
                         (span, wall, roof - z), hidden=True)
                continue
            bottom = z - 35
            wall_height = roof - bottom
            dimensions = (wall, span, wall_height) if dx else (span, wall, wall_height)
            box(f"clearing_wall_{label}_{dx}_{dy}", (wx, wy, bottom + wall_height / 2), dimensions)

    incline = layout["incline"]
    x0, x1 = incline_run(layout)
    y0, y1 = incline["minimum"][1] * size, (incline["maximum"][1] + 1) * size
    start, end = incline["start_height_cm"], incline["end_height_cm"]
    angle = math.atan2(end - start, x1 - x0)
    half_thickness = FLOOR_THICKNESS_CM / 2
    # Keep the visible top of the tilted box on the exact landing elevations.
    midpoint = ((x0 + x1) / 2 + math.sin(angle) * half_thickness, (y0 + y1) / 2,
                (start + end) / 2 - math.cos(angle) * half_thickness)
    length = math.hypot(x1 - x0, end - start)
    mesh_box("clearing_continuous_incline", midpoint, (length + 4, y1 - y0, FLOOR_THICKNESS_CM),
             unreal.Rotator(pitch=math.degrees(angle)))

    # These outcrops sit beyond the guarded ledge, below the walking surface.
    box("overlook_void_floor", (1900, 3150, -350), (1800, 1500, 34))
    for index, (position, dimensions) in enumerate((
        ((1260, 2810, 30), (205, 250, 700)),
        ((2430, 2870, 85), (260, 240, 830)),
        ((1820, 3340, 120), (460, 340, 900)),
    ), 1):
        box(f"overlook_root_outcrop_{index}", position, dimensions)

    for name, tag in (("entry", "abyss_012_player_start"), ("checkpoint", "abyss_012_checkpoint_start")):
        cell = tuple(layout[name])
        x, y = center(cell, size)
        z = surface_height(layout, overlook_cells, cell, x) + PLAYER_OFFSET_CM
        actor = spawn(unreal.PlayerStart, tag, (x, y, z))
        actor.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    exit_cell = tuple(layout["exit"])
    x, y = center(exit_cell, size)
    z = surface_height(layout, overlook_cells, exit_cell, x) + PLAYER_OFFSET_CM
    spawn(unreal.TargetPoint, "abyss_012_exit", (x, y, z))

    light_cells = {(2, 5), (5, 4), (7, 7), (9, 4), (9, 10), (11, 7), (14, 5), (18, 5)}
    for index, cell in enumerate(sorted(light_cells), 1):
        x, y = center(cell, size)
        z = min(roof - 50, surface_height(layout, overlook_cells, cell, x) + 425)
        light = spawn(unreal.PointLight, f"oblique_clearing_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1850.0)
        component.set_editor_property("attenuation_radius", 1100.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Oblique Clearing at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, quiet 20% incline and optional guarded overlook.")
    else:
        import unreal

        build(unreal, layout, cells)

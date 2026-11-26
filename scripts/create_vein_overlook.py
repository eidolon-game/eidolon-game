"""Build the quiet Vein Overlook or verify its ramp and protected vista."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor019"
MODE_PATH = "/Script/Eidolon.EidolonVeinOverlookGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_019_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def elevation(layout, y):
    size = layout["cell_cm"]
    ramp = layout["descent"]
    start, end = ramp["minimum"][1] * size, (ramp["maximum"][1] + 1) * size
    t = min(1, max(0, (y - start) / (end - start)))
    return ramp["start_height_cm"] + t * (ramp["end_height_cm"] - ramp["start_height_cm"])


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    entry, checkpoint, exit_cell = (tuple(layout[key]) for key in ("entry", "checkpoint", "exit"))
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, exit_cell} <= cells:
        raise ValueError("The overlook needs unique walkable cells and valid starts and exit.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 18:
        raise ValueError("Expected the 200 cm kit after the Double Clearing receipt.")
    if layout["encounters"] or layout["accessibility"]["mandatory_timed_traversal"]:
        raise ValueError("This quiet floor cannot require enemies or timed traversal.")

    upper, ramp_cells, landing, outlet, bay = (rectangle(layout[name]) for name in
        ("root_arrival", "descent", "stone_landing", "stone_outlet", "overlook"))
    regions = (upper, ramp_cells, landing, outlet, bay)
    if cells != set().union(*regions) or len(cells) != sum(map(len, regions)):
        raise ValueError("The root approach, descent, overlook and cut outlet must occupy distinct regions.")
    if visit(cells, entry) != cells or exit_cell not in visit(cells - bay, entry) or layout["overlook"]["required"]:
        raise ValueError("The balcony must remain a reachable optional excursion.")
    if exit_cell in visit(cells - ramp_cells, entry):
        raise ValueError("The principal route must descend through the stone-lined ramp.")
    rest = rectangle(layout["dry_rest"])
    if not rest <= upper or checkpoint not in rest or entry not in upper or exit_cell not in outlet:
        raise ValueError("Place the checkpoint on the dry rest platform before the descent.")
    ramp = layout["descent"]
    run = (ramp["maximum"][1] - ramp["minimum"][1] + 1) * size
    drop = ramp["start_height_cm"] - ramp["end_height_cm"]
    if not 0 < drop / run <= ramp["maximum_grade"] <= 0.20:
        raise ValueError("The continuous ramp must have a grade at or below 20 percent.")
    if ramp["start_height_cm"] != layout["dry_rest"]["height_cm"] or ramp["end_height_cm"] != layout["stone_landing"]["height_cm"]:
        raise ValueError("Both ramp ends must join flush to the platforms.")
    if (ramp["maximum"][0] - ramp["minimum"][0] + 1) * size - layout["wall_thickness_cm"] < 550:
        raise ValueError("The descending walkway needs a broad, collision-clear lane.")

    overlook = layout["overlook"]
    guards = set(map(tuple, overlook["guarded_east_edge"]))
    exposed = {(x, y) for x, y in bay if (x + 1, y) not in cells}
    if not guards or guards != exposed or not layout["accessibility"]["guarded_scenic_drop"]:
        raise ValueError("Every scenic edge needs a continuous fall guard.")
    view, target = overlook["eye_cm"], overlook["sight_target_cm"]
    edge_x = (overlook["maximum"][0] + 1) * size
    if not view[0] < edge_x < target[0]:
        raise ValueError("The scenic target must be beyond the protected balcony edge.")
    t = (edge_x - view[0]) / (target[0] - view[0])
    line_y = view[1] + (target[1] - view[1]) * t
    line_height = view[2] + (target[2] - view[2]) * t
    if (overlook["maximum"][0], int(line_y // size)) not in guards:
        raise ValueError("The vista must face across the protected bay, not a wall.")
    if line_height <= overlook["height_cm"] + overlook["parapet_height_cm"] + 10:
        raise ValueError("The guard rail must leave a sightline to the fissure.")
    preview = overlook["fissure_preview"]
    if not edge_x < preview["minimum_x_cm"] < target[0] < preview["maximum_x_cm"]:
        raise ValueError("The scenic fissure must remain outside the playable route.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Vein Overlook floor mesh.")

    def create_mesh(tag, position, dimensions, mesh=cube, collides=True):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.STATIC)
        if not component.set_static_mesh(mesh):
            raise RuntimeError(f"Could not assign the mesh to {tag}.")
        component.set_collision_profile_name("BlockAll" if collides else "NoCollision")
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        if not collides:
            actor.set_actor_enable_collision(False)
        return actor

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    ramp = layout["descent"]
    ramp_cells = rectangle(ramp)
    overlook = layout["overlook"]
    guards = set(map(tuple, overlook["guarded_east_edge"]))
    for cell in sorted(cells):
        x, y = center(cell, size)
        z = elevation(layout, y)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell not in ramp_cells:
            box(f"vein_overlook_floor_{label}", (x, y, z - 9), (size, size, 18))
        box(f"vein_overlook_canopy_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            if cell in guards and dx == 1:
                hp = overlook["parapet_height_cm"]
                box(f"vein_overlook_parapet_{label}", (wx, wy, z + hp / 2), (wall, size + wall, hp))
                guard = create_mesh(f"vein_overlook_guard_{label}", (wx, wy, (z + roof) / 2),
                                    (wall, size + wall, roof - z))
                guard.set_actor_hidden_in_game(True)
                continue
            bottom = z - 35
            height = roof - bottom
            dims = (wall, size + wall, height) if dx else (size + wall, wall, height)
            box(f"vein_overlook_wall_{label}_{dx}_{dy}", (wx, wy, (bottom + roof) / 2), dims)

    x0, y0 = (value * size for value in ramp["minimum"])
    x1, y1 = ((value + 1) * size for value in ramp["maximum"])
    start, end = ramp["start_height_cm"], ramp["end_height_cm"]
    angle = math.atan2(end - start, y1 - y0)
    deck = create_mesh("vein_overlook_continuous_descent",
                       ((x0 + x1) / 2, (y0 + y1) / 2 + 9 * math.sin(angle), (start + end) / 2 - 9 * math.cos(angle)),
                       (math.hypot(y1 - y0, end - start) + 4, x1 - x0, 18))
    deck.set_actor_rotation(unreal.Rotator(pitch=math.degrees(angle), yaw=90), False)

    # A shallow stone ribbon at each side reads as a cut edge, without changing the walking surface.
    for side, x in (("west", x0 + 34), ("east", x1 - 34)):
        edge = create_mesh(f"vein_overlook_cut_stone_{side}",
                           (x, (y0 + y1) / 2 - math.sin(angle), (start + end) / 2 + math.cos(angle)),
                           (math.hypot(y1 - y0, end - start) - 26, 27, 2), collides=False)
        edge.set_actor_rotation(unreal.Rotator(pitch=math.degrees(angle), yaw=90), False)

    roots = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    if not roots:
        raise RuntimeError("Could not load the arrival root proxy.")
    for index, (position, dimensions, pitch, yaw) in enumerate((
        ((645, 420, 455), (95, 105, 650), 51, 28),
        ((1710, 560, 425), (130, 100, 650), 53, -60),
        ((670, 775, 340), (88, 112, 320), 74, -9),
    ), 1):
        detail = create_mesh(f"vein_overlook_arrival_root_{index}", position, dimensions, roots, collides=False)
        detail.set_actor_rotation(unreal.Rotator(pitch=pitch, yaw=yaw), False)
    box("vein_overlook_dry_rest_bench", (900, 740, 320), (100, 48, 40))

    # The distant preview is only scenery, with readable recesses below the guarded balcony.
    fissure = overlook["fissure_preview"]
    near = fissure["minimum_x_cm"]
    far = fissure["maximum_x_cm"]
    floor = fissure["floor_top_z_cm"]
    for side, low, high in (("west", 2620, near), ("east", far, 3810)):
        box(f"vein_overlook_distant_arena_rim_{side}", ((low + high) / 2, 700, floor - 35),
            (high - low, 1040, 70))
    box("vein_overlook_fissure_shadow", ((near + far) / 2, 700, floor - 240), (far - near, 1030, 65))
    for index, (x, y, height) in enumerate(((2700, 280, 260), (3710, 330, 420), (3740, 1100, 590),
                                           (2700, 1140, 350)), 1):
        box(f"vein_overlook_distant_cut_stone_{index}", (x, y, floor + height / 2), (130, 105, height))

    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_019_player_start" if name == "entry" else "abyss_019_checkpoint_start"
        actor = spawn(unreal.PlayerStart, tag, (x, y, elevation(layout, y) + 110))
        actor.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    x, y = center(layout["exit"], size)
    spawn(unreal.TargetPoint, "abyss_019_exit", (x, y, elevation(layout, y) + 110))

    for index, (x, y, z) in enumerate(((1100, 300, 675), (1100, 700, 675), (2050, 700, 650),
                                      (1300, 1350, 620), (1300, 2350, 545), (1300, 3150, 450),
                                      (2300, 3300, 420), (3210, 700, 90)), 1):
        light = spawn(unreal.PointLight, f"vein_overlook_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 2050.0)
        component.set_editor_property("attenuation_radius", 1150.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Vein Overlook at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, dry rest, continuous descent and guarded fissure view.")
    else:
        import unreal

        build(unreal, layout, cells)

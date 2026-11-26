"""Build the Hollow Trunk Plates or verify its carrying routes offline."""

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor015"
MODE_PATH = "/Script/Eidolon.EidolonHollowTrunkGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_015_authored.json"
ANCHORS = ("entry", "checkpoint", "pin_pickup", "pin_seat", "exit")


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    entry, checkpoint, pickup, seat, exit_cell = (tuple(layout[name]) for name in ANCHORS)
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, pickup, seat, exit_cell} <= cells:
        raise ValueError("The hollow trunk needs unique floor cells and valid pin and player anchors.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 14:
        raise ValueError("Expected the 200 cm kit after the Fern Basin.")

    arrival = rectangle(layout["arrival"])
    short = rectangle(layout["short_route"])
    long = set(map(tuple, layout["long_route"]["cells"]))
    pad = rectangle(layout["assembly_pad"])
    bridge = rectangle(layout["walkway"])
    if not (arrival | short | long | pad | bridge) <= cells or not {entry, checkpoint, pickup} <= arrival or seat not in pad:
        raise ValueError("The pin must travel from the arrival alcove to the assembly pad.")
    if layout["walkway"]["starts_open"] or layout["stalker"]["required"]:
        raise ValueError("Only a seated pin may open the bridge; defeating the stalker is optional.")
    before_bridge = visit(cells - bridge, entry)
    if visit(cells, entry) != cells or exit_cell in before_bridge or not {checkpoint, pickup, seat} <= before_bridge:
        raise ValueError("The missing bridge must exclude the only exit without isolating the controls.")
    gates = list(map(tuple, layout["gate"]["adjacency"]))
    gate_crossings = set(zip(gates[::2], gates[1::2]))
    bridge_entries = {
        ((x, y), other) for x, y in before_bridge for dx, dy in DIRECTIONS
        if (other := (x + dx, y + dy)) in bridge
    }
    if len(gate_crossings) != 2 or gate_crossings != bridge_entries:
        raise ValueError("The gate frame must cover every approach to the bridge.")
    if short & long or not {pickup, seat} <= visit(arrival | short | pad, entry) or seat not in visit(arrival | long | pad, pickup):
        raise ValueError("Both carrying paths must remain usable independently.")

    y0, y1 = layout["short_route"]["minimum"][1], layout["short_route"]["maximum"][1]
    if (y1 - y0 + 1) * size - layout["wall_thickness_cm"] < layout["short_route"]["minimum_clear_width_cm"]:
        raise ValueError("The short route must clear at least 300 cm across the trunk.")
    if not rectangle(layout["short_route"]["evade_pad"]) <= short:
        raise ValueError("The short route needs a clear space to drop the pin during a pounce.")

    for refuge in layout["long_route"]["refuges"]:
        pocket = rectangle(refuge)
        if len(pocket) < 4 or not pocket <= long:
            raise ValueError("Both long-route refuges need a four-cell floor.")
        exits = {other for x, y in pocket for dx, dy in DIRECTIONS
                 if (other := (x + dx, y + dy)) in long - pocket}
        if len(exits) < 2:
            raise ValueError("A refuge must have two walkable ways back into the long route.")
    pad_bounds = layout["assembly_pad"]
    if min((high - low + 1) * size for low, high in zip(pad_bounds["minimum"], pad_bounds["maximum"])) < pad_bounds["minimum_side_cm"]:
        raise ValueError("The assembly pad must have room to carry the pin and sidestep.")

    recovery = layout["pin"]["recovery"]
    minimum, maximum = recovery["minimum_xy_cm"], recovery["maximum_xy_cm"]
    if recovery["fall_below_z_cm"] >= 0 or not recovery["requires_surface_trace"]:
        raise ValueError("Recover a lost pin after a failed floor trace or a drop beneath the walking surface.")
    if any(not (minimum[0] <= x * size < maximum[0] and minimum[1] <= y * size < maximum[1])
           for x, y in before_bridge):
        raise ValueError("The recovery envelope must include every valid pre-bridge floor cell.")
    bridge_actor = layout["walkway"]
    x0, y0 = bridge_actor["minimum"]
    x1, y1 = bridge_actor["maximum"]
    width, depth, height = bridge_actor["dimensions_cm"]
    bridge_x, bridge_y, bridge_z = bridge_actor["lowered_center_cm"]
    if width < (x1 - x0 + 1) * size or depth != (y1 - y0 + 1) * size or bridge_z + height / 2 != 0:
        raise ValueError("The lowered bridge must span the full gap with a flush top.")
    if (bridge_x, bridge_y) != ((x0 + x1 + 1) * size / 2, (y0 + y1 + 1) * size / 2):
        raise ValueError("The lowered bridge must meet both corridor floors without an offset.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    bridge = rectangle(layout["walkway"])
    for cell in sorted(cells):
        x, y = center(cell, size)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell not in bridge:
            box(f"trunk_floor_{label}", (x, y, -9), (size, size, 18))
        box(f"trunk_canopy_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            dims = (wall, size + wall, roof) if dx else (size + wall, wall, roof)
            box(f"trunk_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Hollow Trunk proxy mesh.")

    def movable(tag, position, dimensions, profile, hidden):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the mesh to {tag}.")
        component.set_collision_profile_name(profile)
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        actor.set_actor_enable_collision(False)
        actor.set_actor_hidden_in_game(hidden)

    pin = layout["pin"]
    movable(pin["id"], pin["origin_cm"], pin["dimensions_cm"], "NoCollision", False)
    walkway = layout["walkway"]
    movable(walkway["id"], walkway["lowered_center_cm"], walkway["dimensions_cm"], "BlockAllDynamic", True)

    gate = layout["gate"]
    gx, gy, gz = gate["closed_center_cm"]
    depth, gap, clearance = gate["dimensions_cm"]
    corridor = (walkway["maximum"][1] - walkway["minimum"][1] + 1) * size
    jamb = (corridor - gap) / 2
    for sign in (-1, 1):
        box(f"trunk_bridge_jamb_{sign}", (gx, gy + sign * (gap + jamb) / 2, roof / 2),
            (depth, jamb, roof))
    header = roof - clearance
    box("trunk_bridge_header", (gx, gy, clearance + header / 2),
        (depth, gap, header))
    box(gate["id"], (gx, gy, gz), gate["dimensions_cm"], movable=True)

    roots = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    if not roots:
        raise RuntimeError("Could not load the hollow trunk root proxies.")
    for index, (position, scale, pitch, yaw) in enumerate((
        ((1750, 1480, 190), (1.8, 2.0, 6.0), 78, 4),
        ((2290, 1490, 160), (1.4, 1.8, 5.8), 80, -5),
        ((1880, 600, 100), (1.1, 1.45, 3.0), 67, 8),
    ), 1):
        actor = spawn(unreal.StaticMeshActor, f"hollow_trunk_root_{index:02d}", position)
        component = actor.static_mesh_component
        if not component.set_static_mesh(roots):
            raise RuntimeError("Could not assign the hollow trunk root proxy.")
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_enable_collision(False)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        actor.set_actor_rotation(unreal.Rotator(pitch=pitch, yaw=yaw), False)

    for name in ("pin_pickup", "pin_seat", "exit"):
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_015_{name}", (x, y, 110))
    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_015_player_start" if name == "entry" else "abyss_015_checkpoint_start"
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    stalker = layout["stalker"]
    spawn(unreal.TargetPoint, stalker["id"], stalker["feet_cm"])

    lights = {tuple(layout[name]) for name in ANCHORS} | {(4, 5), (5, 8), (8, 1), (13, 4), (9, 8), (17, 8), (20, 9)}
    for index, cell in enumerate(sorted(lights), 1):
        x, y = center(cell, size)
        actor = spawn(unreal.PointLight, f"hollow_trunk_light_{index:02d}", (x, y, 380))
        component = actor.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1850.0)
        component.set_editor_property("attenuation_radius", 1050.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Hollow Trunk Plates at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, two independent carrying paths and a gated bridge.")
    else:
        import unreal

        build(unreal, layout, cells)

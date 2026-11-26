"""Build the Buried Forest Threshold or validate its routes offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor011"
MODE_PATH = "/Script/Eidolon.EidolonBuriedForestEntryGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_011_authored.json"
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
        raise ValueError("The Buried Forest Threshold needs unique floor cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 500):
        raise ValueError("Expected the 200 cm kit with a high forest canopy.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 10:
        raise ValueError("The descent requires the Rootclaw floor receipt.")

    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The forest gate must face north between walkable cells.")
    before_gate = visit(cells, entry, frozenset((near, far)))
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The lower stair must be beyond the forest gate.")

    routes = layout["routes"]
    fork, rejoin = tuple(routes["fork"]), tuple(routes["rejoin"])
    wet = set(map(tuple, routes["wet_cells"]))
    dry = set(map(tuple, routes["dry_cells"]))
    if wet & dry or not (wet | dry | {fork, rejoin}) <= before_gate:
        raise ValueError("The two paths must be distinct and reachable from the fork.")
    for corridor in (wet, dry):
        if visit(corridor | {fork, rejoin}, fork) != corridor | {fork, rejoin}:
            raise ValueError("Both the marsh and the dry route must lead to the same arch approach.")
    visual = rectangle(routes["marsh_visual"])
    if not visual <= wet or visual & dry:
        raise ValueError("The optional dry path must remain outside the marsh.")

    stone = layout["stone_vestibule"]
    forest = layout["forest"]
    arch = tuple(forest["root_arch"]["cell"])
    arena = rectangle(forest["arena"])
    spawn_cell = tuple(layout["encounter_definition"]["spawn_cell"])
    if not rectangle(stone) <= before_gate or stone["low_ceiling_cm"] >= layout["height_cm"]:
        raise ValueError("The stone vestibule must transition from a lower ceiling to the forest canopy.")
    if not (arena | {arch, checkpoint}) <= before_gate or not {encounter, spawn_cell} <= arena:
        raise ValueError("The stalker encounter needs a clear chamber beyond the root arch.")
    if encounter in visit(before_gate - {arch}, entry):
        raise ValueError("The encounter approach must pass through the root arch.")
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
    stone = layout["stone_vestibule"]
    for cell in sorted(rectangle(stone)):
        x, y = center(cell, size)
        box(f"forest_stone_soffit_{cell[0]:02d}_{cell[1]:02d}", (x, y, stone["low_ceiling_cm"] + 12),
            (size, size, 24))

    roots = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not roots or not cube:
        raise RuntimeError("Could not load the forest graybox meshes.")

    def geometry(tag, position, dimensions, mesh, rotation=None, has_collision=True):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        if not component.set_static_mesh(mesh):
            raise RuntimeError(f"Could not assign a forest proxy mesh to {tag}.")
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        if rotation is not None:
            actor.set_actor_rotation(rotation, False)
        actor.set_actor_enable_collision(has_collision)
        if has_collision:
            component.set_collision_profile_name("BlockAll")
        else:
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    forest = layout["forest"]
    arch = forest["root_arch"]
    ax, ay = center(arch["cell"], size)
    for sign in (-1, 1):
        geometry(f"forest_arch_root_{sign}", (ax, ay + sign * (arch["clear_width_cm"] / 2 + 48), 170),
                 (118, 96, 340), roots)
    # Cylinder length follows local Z. Rotate the lintel so that it spans the path.
    geometry("forest_arch_lintel", (ax, ay, arch["clear_height_cm"] + 55), (110, 100, 340),
             roots, unreal.Rotator(pitch=90, yaw=90))
    spawn(unreal.TargetPoint, "abyss_011_root_arch", (ax, ay, 110))
    for index, mass in enumerate(forest["root_masses"], 1):
        geometry(f"forest_root_mass_{index:02d}", mass["position_cm"], mass["dimensions_cm"], roots)

    routes = layout["routes"]
    marsh = routes["marsh_visual"]
    low, high = center(marsh["minimum"], size), center(marsh["maximum"], size)
    depth = marsh["height_cm"]
    geometry(marsh["id"], ((low[0] + high[0]) / 2, (low[1] + high[1]) / 2, depth / 2),
             (high[0] - low[0] + size, high[1] - low[1] + size, depth), cube, has_collision=False)
    path = routes["dry_rootwalk"]
    dry_treads = [cell for cell in routes["dry_cells"] if cell[1] == routes["fork"][1]]
    for index, cell in enumerate(dry_treads, 1):
        x, y = center(cell, size)
        box(f"forest_dry_root_tread_{index:02d}", (x, y, path["height_cm"] / 2),
            (size - 6, path["width_cm"], path["height_cm"]))

    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gy += size / 2
    width, clearance = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - width) / 2
    for sign in (-1, 1):
        box(f"forest_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("forest_gate_lintel", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    stalker = layout["encounter_definition"]
    x, y = center(stalker["spawn_cell"], size)
    enemy = spawn(unreal.TargetPoint, stalker["spawn_marker"], (x, y, 0))
    enemy.set_actor_rotation(unreal.Rotator(yaw=stalker["spawn_yaw_deg"]), False)
    for name in ANCHORS:
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_011_{name}", (x, y, 110))
    for name, tag in (("entry", "abyss_011_player_start"), ("checkpoint", "abyss_011_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    lights = {tuple(layout[name]) for name in ANCHORS} | {(3, 4), (6, 6), (9, 5), (9, 8), (14, 8), (17, 9)}
    for index, cell in enumerate(sorted(lights), 1):
        x, y = center(cell, size)
        z = stone["low_ceiling_cm"] - 40 if cell in rectangle(stone) else height - 90
        light = spawn(unreal.PointLight, f"forest_threshold_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 2200.0)
        component.set_editor_property("attenuation_radius", 950.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Buried Forest Threshold at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, dry bypass, root arch and gated exit.")
    else:
        import unreal

        build(unreal, layout, cells)

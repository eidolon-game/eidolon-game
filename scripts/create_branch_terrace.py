"""Build the Branch Terraces or validate their separated routes offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor013"
MODE_PATH = "/Script/Eidolon.EidolonBranchTerraceGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_013_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def edge(a, b):
    return frozenset((a, b))


def shortcut_edges(layout):
    shortcut = layout["shortcut"]
    sequence = [tuple(shortcut["from_cell"]), *map(tuple, shortcut["cells"]), tuple(shortcut["upper_join"])]
    return {edge(a, b) for a, b in zip(sequence, sequence[1:])}


def reachable(cells, origin, stairs, links, blocked=()):
    seen, pending = {origin}, [origin]
    while pending:
        cell = pending.pop()
        for dx, dy in DIRECTIONS:
            other = (cell[0] + dx, cell[1] + dy)
            link = edge(cell, other)
            if other not in cells or other in seen or link in blocked:
                continue
            if (cell in stairs or other in stairs) and link not in links:
                continue
            seen.add(other)
            pending.append(other)
    return seen


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    if len(cells) != len(layout["cells"]):
        raise ValueError("The terrace floor cells must be unique.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 12:
        raise ValueError("Expected the 200 cm kit after the clearing receipt.")

    entry, checkpoint, release, exit_cell = (tuple(layout[name]) for name in ("entry", "checkpoint", "stair_release", "exit"))
    upper = rectangle(layout["upper_terrace"])
    court = rectangle(layout["combat_court"])
    short, long = (rectangle(layout[name]) for name in ("short_ramp", "long_ramp"))
    stairs = set(map(tuple, layout["shortcut"]["cells"]))
    links = shortcut_edges(layout)
    if {entry, checkpoint, release, exit_cell} - cells or not {release} <= upper:
        raise ValueError("Expected a stable entry and checkpoint, high release, and lower-stair exit.")
    if not (court | short | long | stairs | upper) <= cells or len(court) != 20:
        raise ValueError("The short terrace must remain an open 1000 by 800 cm space.")
    if any(len(link) != 2 or sum(abs(a - b) for a, b in zip(*link)) != 1 for link in links):
        raise ValueError("The switchback stairs must have a connected ordered route.")
    rise = layout["shortcut"]["rise_per_step_cm"]
    if not 0 < rise <= 25 or (len(stairs) - 1) * rise != layout["upper_height_cm"]:
        raise ValueError("The shortcut must have walkable rises and reach the upper terrace exactly.")

    gate = layout["gate"]
    exit_block = edge(tuple(gate["from_cell"]), tuple(gate["to_cell"]))
    shortcut = layout["shortcut"]
    shortcut_block = edge(tuple(shortcut["from_cell"]), tuple(shortcut["to_cell"]))
    blocked = {exit_block, shortcut_block}
    if exit_cell in reachable(cells, entry, stairs, links, blocked):
        raise ValueError("The physical exit gate must isolate the descent.")
    if release not in reachable(cells - court - short, entry, stairs, links, blocked):
        raise ValueError("The long route must reach the release without using the combat court.")
    if release not in reachable(cells - long, entry, stairs, links, blocked):
        raise ValueError("The short route must reach the release without the long ramp.")
    if reachable(cells, entry, stairs, links, {exit_block}) & stairs != stairs:
        raise ValueError("Releasing the lower blocker must make the whole shortcut walkable.")

    for name in ("short_ramp", "long_ramp"):
        ramp = layout[name]
        span = (ramp["maximum"][0] - ramp["minimum"][0] + 1) * size
        if layout["upper_height_cm"] / span > ramp["maximum_grade"]:
            raise ValueError(f"The {name} exceeds the walking grade.")

    stalker = layout["stalker"]
    sx, sy, _ = stalker["spawn_cm"]
    px, py, pz = stalker["perch_cm"]
    lx, ly, lz = stalker["landing_cm"]
    if stalker["required"] or not 160 <= math.hypot(px - lx, py - ly) <= 480 or not 75 <= pz - lz <= 205:
        raise ValueError("Expected an optional stalker with a valid elevated pounce.")
    if (lx // size, ly // size) not in court or (px // size, py // size) not in set(map(tuple, stalker["perch_cells"])):
        raise ValueError("The pounce endpoints need solid walkable support.")
    if layout["roof_height_cm"] - pz < 180 or 3 * size <= 2 * stalker["body_radius_cm"]:
        raise ValueError("The perch and mouth must clear the full enemy body.")
    opposite_margin = ly - layout["combat_court"]["minimum"][1] * size - stalker["body_radius_cm"]
    if opposite_margin < layout["combat_court"]["clear_evade_margin_minimum_cm"]:
        raise ValueError("The lower terrace must leave room to evade beyond the landing.")
    for x, y in long | (upper - {release}):
        distance = math.hypot(max(x * size - sx, 0, sx - (x + 1) * size),
                              max(y * size - sy, 0, sy - (y + 1) * size))
        if distance <= stalker["home_radius_cm"]:
            raise ValueError("The long bypass must sit outside the ground stalker's home radius.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the terrace ramp mesh.")

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    thickness = layout["wall_thickness_cm"]
    high = layout["upper_height_cm"]
    short, long = (rectangle(layout[name]) for name in ("short_ramp", "long_ramp"))
    upper = rectangle(layout["upper_terrace"]) | set(map(tuple, layout["upper_exit_cells"]))
    stairs = {tuple(cell): index * layout["shortcut"]["rise_per_step_cm"]
              for index, cell in enumerate(layout["shortcut"]["cells"])}
    links = shortcut_edges(layout)
    stalker = layout["stalker"]
    perch = set(map(tuple, stalker["perch_cells"]))
    mouths = {tuple(cell) for cell in stalker["mouth_cells"]}
    guard = set(map(tuple, layout["combat_court"]["guard_cells"]))

    def rotated_ramp(tag, bounds):
        x0, y0 = (value * size for value in bounds["minimum"])
        x1, y1 = ((value + 1) * size for value in bounds["maximum"])
        angle = math.atan2(high, x1 - x0)
        actor = spawn(unreal.StaticMeshActor, tag, ((x0 + x1) / 2 + 9 * math.sin(angle), (y0 + y1) / 2,
                                                   high / 2 - 9 * math.cos(angle)))
        mesh = actor.static_mesh_component
        mesh.set_mobility(unreal.ComponentMobility.STATIC)
        if not mesh.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the walkway mesh to {tag}.")
        mesh.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector((math.hypot(x1 - x0, high) + 4) / 100, (y1 - y0) / 100, 0.18))
        actor.set_actor_rotation(unreal.Rotator(pitch=math.degrees(angle)), False)

    rotated_ramp("branch_terrace_short_ramp", layout["short_ramp"])
    rotated_ramp("branch_terrace_long_ramp", layout["long_ramp"])

    for cell in sorted(cells | perch):
        x, y = center(cell, size)
        z = 180 if cell in perch else stairs.get(cell, high if cell in upper else 0)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell not in short | long:
            floor_depth = z if cell in perch else 18
            box(f"terrace_floor_{label}", (x, y, z - floor_depth / 2), (size, size, floor_depth))
        box(f"terrace_roof_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            other = (cell[0] + dx, cell[1] + dy)
            if other in cells | perch:
                if cell in perch or other in perch:
                    if (cell in perch and other in perch) or cell in mouths or other in mouths:
                        continue
                elif (cell not in stairs and other not in stairs) or edge(cell, other) in links:
                    continue
                if cell > other:
                    continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            span = size + thickness
            dims = (thickness, span, roof) if dx else (span, thickness, roof)
            if cell in guard and dy == 1:
                box(f"terrace_parapet_{label}", (wx, wy, 55), (span, thickness, 110))
                guard_actor = spawn(unreal.StaticMeshActor, f"terrace_guard_{label}", (wx, wy, roof / 2))
                component = guard_actor.static_mesh_component
                if not component.set_static_mesh(cube):
                    raise RuntimeError("Could not assign the terrace guard mesh.")
                component.set_collision_profile_name("BlockAll")
                guard_actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dims)))
                guard_actor.set_actor_hidden_in_game(True)
            else:
                box(f"terrace_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    trunk = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    if not trunk:
        raise RuntimeError("Could not load the branch terrace trunk proxy.")
    root = spawn(unreal.StaticMeshActor, "branch_terrace_inclined_trunk", (1535, 2100, 155))
    component = root.static_mesh_component
    if not component.set_static_mesh(trunk):
        raise RuntimeError("Could not assign the branch terrace trunk proxy.")
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    root.set_actor_enable_collision(False)
    root.set_actor_scale3d(unreal.Vector(0.63, 0.75, 3.1))
    root.set_actor_rotation(unreal.Rotator(pitch=54, yaw=-90), False)

    for door, x, y, z, crosses_x in (
        (layout["shortcut"]["closed_id"], 800, 900, 0, True),
        (layout["gate"]["id"], 3300, 2000, high, False),
    ):
        for side in (-1, 1):
            pos = (x, y + side * 87.5, (z + roof) / 2) if crosses_x else (x + side * 87.5, y, (z + roof) / 2)
            box(f"{door}_jamb_{side}", pos, (25, 25, roof - z))
        header = roof - z - 240
        box(f"{door}_head", (x, y, z + 240 + header / 2),
            (25, 150, header) if crosses_x else (150, 25, header))
        box(door, (x, y, z + 120), (25, 150, 240) if crosses_x else (150, 25, 240), movable=True)

    for name in ("spawn", "perch", "landing"):
        spawn(unreal.TargetPoint, f"abyss_013_stalker_{name}", stalker[f"{name}_cm"])
    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_013_player_start" if name == "entry" else "abyss_013_checkpoint_start"
        actor = spawn(unreal.PlayerStart, tag, (x, y, 110))
        actor.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)
    for name in ("stair_release", "exit"):
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_013_{name}", (x, y, high + 110))

    for index, (x, y, z) in enumerate(((300, 900, 360), (1300, 1700, 430), (1300, 2100, 480),
                                       (1700, 300, 530), (2700, 300, 590), (3300, 1100, 630),
                                       (3300, 1700, 630), (3500, 2300, 630)), 1):
        light = spawn(unreal.PointLight, f"branch_terrace_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1900.0)
        component.set_editor_property("attenuation_radius", 1000.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Branch Terraces at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, two ramps, guarded pounce and gated switchback.")
    else:
        import unreal

        build(unreal, layout, cells)

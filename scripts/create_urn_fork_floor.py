"""Build the Urn Fork graybox or validate its two alternate routes offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor009"
MODE_PATH = "/Script/Eidolon.EidolonUrnForkGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_009_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    entry, checkpoint, exit_cell = (tuple(layout[name]) for name in ("entry", "checkpoint", "exit"))
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, exit_cell} <= cells:
        raise ValueError("The Urn Fork needs unique cells and valid start, checkpoint and exit anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 420):
        raise ValueError("Expected the 200 cm catacomb kit with enough clearance for the raised perch.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 8:
        raise ValueError("The descent requires the floor-eight receipt.")

    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0] + 1, near[1]):
        raise ValueError("The common gate must face east between walkable cells.")
    gate_edge = frozenset((near, far))
    before_gate = visit(cells, entry, gate_edge)
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("Both branches must use the same gate to reach the exit.")

    fork, convergence = rectangle(layout["fork"]), rectangle(layout["convergence"])
    if not (fork | convergence) <= before_gate or not {entry, checkpoint} <= fork or near not in convergence:
        raise ValueError("The two routes must split after the entrance vestibule and converge at the common gate.")

    upper, lower = layout["upper_route"], layout["lower_route"]
    upper_start, upper_latch = (tuple(upper[name]) for name in ("entry", "latch"))
    lower_start, lower_latch = (tuple(lower[name]) for name in ("entry", "latch"))
    branch_cells = before_gate - fork - convergence
    if not {upper_start, upper_latch, lower_start, lower_latch} <= branch_cells:
        raise ValueError("Route entries and latches must belong to the protected branches.")
    upper_cells, lower_cells = visit(branch_cells, upper_start), visit(branch_cells, lower_start)
    if upper_cells & lower_cells or upper_cells | lower_cells != branch_cells:
        raise ValueError("The two routes must remain disjoint until convergence.")
    for corridor, latch in ((upper_cells, upper_latch), (lower_cells, lower_latch)):
        if not {latch, near} <= visit(fork | corridor | convergence, entry, gate_edge):
            raise ValueError("Either route must independently reach its latch and the shared exit gate.")
    if abs(upper_latch[1] - lower_latch[1]) < 4:
        raise ValueError("The two latch interaction zones must stay on opposite approaches.")

    mantis, tick = upper["mantis"], upper["tick"]
    for spawn in (mantis, tick):
        sx, sy = spawn["cell"]
        mouth = tuple(spawn["mouth"])
        if (sx, sy) in cells or mouth != (sx, sy - 1) or mouth not in upper_cells:
            raise ValueError("The upper enemies need separate side openings outside the walking grid.")
    if layout["height_cm"] - mantis["platform_height_cm"] < mantis["body_headroom_cm"]:
        raise ValueError("The mantis needs room to leave the raised perch.")
    if mantis["cell"][0] >= 10 or tick["cell"][0] <= 11:
        raise ValueError("The mantis and tick must remain in distinct upper rooms.")

    water = rectangle(lower["canal"])
    dry = lower["dry_recess"]
    refuge, wall = tuple(dry["cell"]), tuple(dry["screening_wall"])
    if not water <= lower_cells or tuple(lower["hagfish"]["cell"]) not in water:
        raise ValueError("The hagfish must spawn in the lower canal.")
    if refuge not in lower_cells or refuge in water or wall in cells:
        raise ValueError("The lower route needs a reachable, dry refuge behind a screening wall.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    upper, lower = layout["upper_route"], layout["lower_route"]
    mantis, tick = upper["mantis"], upper["tick"]
    build_shell(layout, cells, box, wall_openings=tuple(
        (tuple(enemy["mouth"]), tuple(enemy["cell"])) for enemy in (mantis, tick)
    ))
    size = layout["cell_cm"]
    height = layout["height_cm"]
    thickness = layout["wall_thickness_cm"]

    for name, enemy in (("mantis", mantis), ("tick", tick)):
        x, y = center(enemy["cell"], size)
        ledge = enemy.get("platform_height_cm", 0)
        depth = ledge or 18
        box(f"{name}_niche_floor", (x, y, ledge - depth / 2 if ledge else -9), (size, size, depth))
        box(f"{name}_niche_back", (x, y + size / 2, height / 2), (size + thickness, thickness, height))
        box(f"{name}_niche_roof", (x, y, height + 9), (size, size, 18))
        for sign in (-1, 1):
            box(f"{name}_niche_wall_{sign}", (x + sign * size / 2, y, height / 2),
                (thickness, size + thickness, height))
        if "opening_cm" in enemy:
            width, clearance = enemy["opening_cm"]
            jamb = (size - width) / 2
            for sign in (-1, 1):
                box(f"{name}_niche_jamb_{sign}", (x + sign * (width + jamb) / 2, y - size / 2, height / 2),
                    (jamb, thickness, height))
            box(f"{name}_niche_lintel", (x, y - size / 2, (height + clearance) / 2),
                (width, thickness, height - clearance))
        anchor = spawn(unreal.TargetPoint, enemy["spawn_marker"], (x, y, ledge))
        anchor.set_actor_rotation(unreal.Rotator(yaw=enemy["yaw_deg"]), False)

    hagfish = lower["hagfish"]
    x, y = center(hagfish["cell"], size)
    anchor = spawn(unreal.TargetPoint, hagfish["spawn_marker"], (x, y, 0))
    anchor.set_actor_rotation(unreal.Rotator(yaw=hagfish["yaw_deg"]), False)
    canal = lower["canal"]
    low, high = center(canal["minimum"], size), center(canal["maximum"], size)
    depth = canal["height_cm"]
    water = spawn(unreal.StaticMeshActor, canal["id"], ((low[0] + high[0]) / 2, (low[1] + high[1]) / 2, depth / 2))
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    component = water.static_mesh_component
    if not cube or not component.set_static_mesh(cube):
        raise RuntimeError("Could not assign the shallow canal mesh.")
    water.set_actor_scale3d(unreal.Vector((high[0] - low[0] + size) / 100, (high[1] - low[1] + size) / 100, depth / 100))
    water.set_actor_enable_collision(False)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gx += size / 2
    width, clearance = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - width) / 2
    for sign in (-1, 1):
        box(f"urn_gate_jamb_{sign}", (gx, gy + sign * (width + jamb) / 2, height / 2),
            (thickness, jamb, height))
    box("urn_gate_lintel", (gx, gy, (height + clearance) / 2), (thickness, width, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (thickness, width, clearance), movable=True)

    anchors = {
        "abyss_009_upper_entry": upper["entry"],
        "abyss_009_latch_upper": upper["latch"],
        "abyss_009_lower_entry": lower["entry"],
        "abyss_009_latch_lower": lower["latch"],
        "abyss_009_dry_recess": lower["dry_recess"]["cell"],
        "abyss_009_exit": layout["exit"],
    }
    for tag, cell in anchors.items():
        x, y = center(cell, size)
        spawn(unreal.TargetPoint, tag, (x, y, 110))
    for name, tag in (("entry", "abyss_009_player_start"), ("checkpoint", "abyss_009_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    light_cells = {tuple(cell) for cell in anchors.values()} | {(2, 6), (7, 10), (14, 10), (12, 3), (20, 6)}
    for cell in sorted(light_cells):
        x, y = center(cell, size)
        light = spawn(unreal.PointLight, f"urn_fork_light_{cell[0]:02d}_{cell[1]:02d}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1850.0)
        component.set_editor_property("attenuation_radius", 850.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Urn Fork at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, independent branches and a common gated exit.")
    else:
        import unreal

        build(unreal, layout, cells)

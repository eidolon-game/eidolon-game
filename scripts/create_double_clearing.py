"""Build the Double Clearing or validate the shared-attack bypass offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor018"
MODE_PATH = "/Script/Eidolon.EidolonDoubleClearingGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_018_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def distance_to_cell(position, cell, size):
    x, y, _ = position
    a, b = cell
    return math.hypot(max(a * size - x, 0, x - (a + 1) * size),
                      max(b * size - y, 0, y - (b + 1) * size))


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    entry, checkpoint, latch, exit_cell = (tuple(layout[key]) for key in ("entry", "checkpoint", "latch", "exit"))
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, latch, exit_cell} <= cells:
        raise ValueError("The clearings need unique floor cells and valid traversal anchors.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 17:
        raise ValueError("Expected the 200 cm kit after the Suspended Trunks.")
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    blocked = frozenset((near, far))
    before_gate = visit(cells, entry, blocked)
    if far != (near[0] + 1, near[1]) or visit(cells, entry) != cells or exit_cell in before_gate or near not in before_gate:
        raise ValueError("The closed gate must be the sole threshold to the outlet.")
    if not gate["requires_two_durable_clicks"] or latch not in before_gate:
        raise ValueError("The latch must be reachable before both clicks open the gate.")

    first, second = (rectangle(layout[name]) for name in ("ground_clearing", "high_clearing"))
    arch = layout["arch"]
    arch_width = (arch["maximum"][1] - arch["minimum"][1] + 1) * size - layout["wall_thickness_cm"]
    if arch_width < arch["minimum_width_cm"] or not rectangle(arch) <= before_gate or first & second:
        raise ValueError("The separate clearing chambers need a broad continuous connecting arch.")

    stalkers = layout["stalkers"]
    ground, high = (stalkers[name]["feet_cm"] for name in ("ground", "high"))
    if (ground[0] // size, ground[1] // size) not in first or (high[0] // size, high[1] // size) not in second:
        raise ValueError("The two stalkers must begin in their own clearings.")
    hall = stalkers["high"]
    perch, landing = hall["perch_cm"], hall["landing_cm"]
    perch_cells, mouths = (set(map(tuple, hall[name])) for name in ("perch_cells", "mouth_cells"))
    if (perch[0] // size, perch[1] // size) not in perch_cells or (landing[0] // size, landing[1] // size) not in second:
        raise ValueError("The elevated stalker needs separate valid support at both ends of the pounce.")
    if perch_cells & cells or not mouths <= second or len(mouths) * size <= 2 * stalkers["body_radius_cm"]:
        raise ValueError("The raised perch must have an unobstructed mouth wide enough for the stalker.")
    if not 160 <= math.dist(perch[:2], landing[:2]) <= 480 or not 75 <= perch[2] - landing[2] <= 205:
        raise ValueError("The approved perch height and landing reach must stay inside the configured pounce bounds.")

    side = layout["side_bypass"]
    bypass = set(map(tuple, side["cells"]))
    refuge = tuple(side["protected_refuge"])
    if not bypass <= before_gate or not {latch, refuge} <= visit(bypass | rectangle(layout["arrival"]), entry):
        raise ValueError("The side path must independently reach the protected latch with the gate closed.")
    if stalkers["required"] or stalkers["max_shared_active_windows"] != 1:
        raise ValueError("The staged attacks may not block the optional combat bypass.")
    if any(distance_to_cell(spawn, cell, size) <= stalkers["home_radius_cm"]
           for spawn in (ground, high, perch) for cell in bypass | {checkpoint, latch, refuge}):
        raise ValueError("The protected route must remain outside both stalkers' reach and pursuit boundaries.")
    strip = {tuple(cell) for cell in layout["side_bypass"]["cells"] if cell[1] in (1, 2)}
    if len(strip) != 44 or 2 * size - layout["wall_thickness_cm"] < side["minimum_width_cm"]:
        raise ValueError("The main side strip needs a clear width of at least 350 cm.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    stalkers = layout["stalkers"]
    elevated = stalkers["high"]
    perch = set(map(tuple, elevated["perch_cells"]))
    mouths = set(map(tuple, elevated["mouth_cells"]))
    for cell in sorted(cells | perch):
        x, y = center(cell, size)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell in perch:
            z = elevated["perch_cm"][2]
            box(f"double_clearing_perch_{label}", (x, y, z / 2), (size, size, z))
        else:
            box(f"double_clearing_floor_{label}", (x, y, -9), (size, size, 18))
        box(f"double_clearing_canopy_{label}", (x, y, roof + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            other = cell[0] + dx, cell[1] + dy
            if other in cells | perch:
                if (cell in perch) == (other in perch) or (cell in mouths or other in mouths):
                    continue
                if cell > other:
                    continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            dims = (wall, size + wall, roof) if dx else (size + wall, wall, roof)
            box(f"double_clearing_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    arch = layout["arch"]
    arch_start = arch["minimum"][0] * size
    arch_y = (arch["minimum"][1] + arch["maximum"][1] + 1) * size / 2
    arch_span = (arch["maximum"][1] - arch["minimum"][1] + 1) * size
    box("double_clearing_arch_lintel", (arch_start + size / 2, arch_y, 545),
        (150, arch_span, 70))
    box("double_clearing_arch_far_lintel", (arch_start + 5 * size / 2, arch_y, 545),
        (150, arch_span, 70))

    gate = layout["gate"]
    gx, gy, gz = gate["center_cm"]
    depth, width, opening = gate["dimensions_cm"]
    jamb = (size - width) / 2
    for sign in (-1, 1):
        box(f"double_clearing_gate_jamb_{sign}", (gx, gy + sign * (width + jamb) / 2, roof / 2),
            (depth, jamb, roof))
    head = roof - opening
    box("double_clearing_gate_header", (gx, gy, opening + head / 2), (depth, width, head))
    box(gate["id"], (gx, gy, gz), gate["dimensions_cm"], movable=True)

    fixture = layout["latch_fixture"]
    box("double_clearing_one_sided_stone_cover", fixture["cover_center_cm"], fixture["cover_dimensions_cm"])
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the latch-handle proxy mesh.")
    handle = spawn(unreal.StaticMeshActor, fixture["tag"], fixture["position_cm"])
    mesh = handle.static_mesh_component
    mesh.set_mobility(unreal.ComponentMobility.MOVABLE)
    if not mesh.set_static_mesh(cube):
        raise RuntimeError("Could not assign the latch-handle proxy mesh.")
    mesh.set_collision_profile_name("NoCollision")
    handle.set_actor_enable_collision(False)
    handle.set_actor_scale3d(unreal.Vector(*(value / 100 for value in fixture["dimensions_cm"])))

    spawn(unreal.TargetPoint, stalkers["ground"]["tag"], stalkers["ground"]["feet_cm"])
    spawn(unreal.TargetPoint, elevated["tag"], elevated["feet_cm"])
    spawn(unreal.TargetPoint, elevated["perch_tag"], elevated["perch_cm"])
    spawn(unreal.TargetPoint, elevated["landing_tag"], elevated["landing_cm"])
    for name in ("latch", "exit"):
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_018_{name}", (x, y, 110))
    x, y = center(layout["side_bypass"]["protected_refuge"], size)
    spawn(unreal.TargetPoint, "abyss_018_side_refuge", (x, y, 110))
    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_018_player_start" if name == "entry" else "abyss_018_checkpoint_start"
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    for index, (x, y, z) in enumerate(((500, 1500, 390), (1700, 1900, 400), (2350, 1650, 430),
                                      (2600, 1750, 430), (3450, 1910, 430), (3500, 2300, 450),
                                      (4700, 1500, 410), (5500, 1500, 390), (800, 400, 400),
                                      (2200, 400, 405), (3800, 400, 405)), 1):
        light = spawn(unreal.PointLight, f"double_clearing_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1850.0)
        component.set_editor_property("attenuation_radius", 1090.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Double Clearing at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, two encounter spaces and an independent safe bypass.")
    else:
        import unreal

        build(unreal, layout, cells)

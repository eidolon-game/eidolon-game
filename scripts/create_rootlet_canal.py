"""Build the Rootlet Canal or verify its two bypasses and inner release offline."""

import json
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import DIRECTIONS, MAP_FOLDER, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor016"
MODE_PATH = "/Script/Eidolon.EidolonRootletCanalGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_016_authored.json"


def rectangle(bounds):
    x0, y0 = bounds["minimum"]
    x1, y1 = bounds["maximum"]
    return {(x, y) for x in range(x0, x1 + 1) for y in range(y0, y1 + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = set(map(tuple, layout["cells"]))
    entry, checkpoint, release, exit_cell = (tuple(layout[name]) for name in
                                              ("entry", "checkpoint", "counterweight_release", "exit"))
    if len(cells) != len(layout["cells"]) or not {entry, checkpoint, release, exit_cell} <= cells:
        raise ValueError("The Rootlet Canal needs unique floor cells and valid anchors.")
    size = layout["cell_cm"]
    if size != 200 or layout["connections"]["entry"]["requires_completed_floor"] != 15:
        raise ValueError("Expected the 200 cm kit after the Hollow Trunk Plates.")
    gallery, canal, inner, outlet = (rectangle(layout[name]) for name in ("gallery", "canal", "inner_side", "outlet"))
    ramps = [rectangle(aperture) for aperture in layout["apertures"]]
    regions = (gallery, canal, inner, outlet, *ramps)
    if len(ramps) != 2 or cells != set().union(*regions) or len(cells) != sum(map(len, regions)):
        raise ValueError("The gallery and canal must connect through two separate ramp openings.")

    gate = layout["gate"]
    inner_edge, outlet_edge = (tuple(gate[name]) for name in ("from_inner_cell", "to_outlet_cell"))
    gate_edge = frozenset((inner_edge, outlet_edge))
    if inner_edge not in inner or outlet_edge not in outlet or outlet_edge != (inner_edge[0] - 1, inner_edge[1]):
        raise ValueError("The east side of the gate must be the counterweight side.")
    before_gate = visit(cells, entry, gate_edge)
    if visit(cells, entry) != cells or exit_cell in before_gate or not {release, inner_edge} <= before_gate:
        raise ValueError("The lower canal may reach the inner release, but the gate must block the outlet.")
    if release not in inner or exit_cell not in outlet or not gate["requires_durable_release"]:
        raise ValueError("The inner release must stay behind the gate and persist before the outlet opens.")
    if release in visit(gallery | inner | outlet, entry, gate_edge):
        raise ValueError("The counterweight cannot be reached directly from the upper gallery.")
    gx = gate["closed_center_cm"][0]
    if not center(release, size)[0] > gx > center(exit_cell, size)[0]:
        raise ValueError("The inner release and exit must occupy opposite sides of the gate plane.")

    stalker = layout["stalker"]
    sx, sy, sz = stalker["feet_cm"]
    spawn_cell = sx // size, sy // size
    if stalker["required"] or spawn_cell not in gallery or sz != layout["gallery_height_cm"]:
        raise ValueError("The optional stalker starts on the flat upper gallery.")
    for aperture, span in zip(layout["apertures"], ramps):
        high, low = (set(map(tuple, aperture[name])) for name in ("gallery_pad", "canal_pad"))
        lure = tuple(aperture["lure_cell"])
        if len(high) != 4 or len(low) != 4 or not high <= gallery or not low <= canal or lure not in high:
            raise ValueError("Both openings need two dry 400 cm turning areas and an upper lure point.")
        x, y = center(lure, size)
        if lure not in visit(gallery, spawn_cell) or math.hypot(x - sx, y - sy) > stalker["home_radius_cm"]:
            raise ValueError("Both lure points must be reachable on flat ground by the stalker.")
        if not {release, inner_edge} <= visit((gallery | canal | inner | span) - {spawn_cell}, entry):
            raise ValueError("Either ramp must lead to the inner release without defeating the stalker.")
        run = (aperture["maximum"][1] - aperture["minimum"][1] + 1) * size
        if layout["gallery_height_cm"] / run > 0.25:
            raise ValueError("The ramps must remain walkable without jumping.")
    width = (layout["canal"]["maximum"][1] - layout["canal"]["minimum"][1] + 1) * size - layout["wall_thickness_cm"]
    if width < 250 or layout["canal"]["clear_height_cm"] < 320:
        raise ValueError("The lower canal needs at least 250 cm width and 320 cm headroom.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Rootlet Canal floor mesh.")

    size = layout["cell_cm"]
    roof = layout["roof_height_cm"]
    wall = layout["wall_thickness_cm"]
    height = layout["gallery_height_cm"]
    gallery, canal = (rectangle(layout[name]) for name in ("gallery", "canal"))
    ramps = set().union(*(rectangle(aperture) for aperture in layout["apertures"]))

    def mesh_actor(tag, position, dimensions, profile="BlockAll", movable=False, hidden=False):
        actor = spawn(unreal.StaticMeshActor, tag, position)
        component = actor.static_mesh_component
        component.set_mobility(unreal.ComponentMobility.MOVABLE if movable else unreal.ComponentMobility.STATIC)
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the Rootlet Canal mesh to {tag}.")
        component.set_collision_profile_name(profile)
        actor.set_actor_scale3d(unreal.Vector(*(value / 100 for value in dimensions)))
        if profile == "NoCollision":
            actor.set_actor_enable_collision(False)
        if hidden:
            actor.set_actor_hidden_in_game(True)
        return actor

    for cell in sorted(cells):
        x, y = center(cell, size)
        z = height if cell in gallery else 0
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        if cell not in ramps:
            box(f"rootlet_floor_{label}", (x, y, z - 9), (size, size, 18))
        roof_z = layout["canal"]["clear_height_cm"] if cell in canal else roof
        box(f"rootlet_canopy_{label}", (x, y, roof_z + 9), (size, size, 18))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            wx, wy = x + dx * size / 2, y + dy * size / 2
            dims = (wall, size + wall, roof) if dx else (size + wall, wall, roof)
            box(f"rootlet_wall_{label}_{dx}_{dy}", (wx, wy, roof / 2), dims)

    for aperture in layout["apertures"]:
        x0, y0 = (coordinate * size for coordinate in aperture["minimum"])
        x1, y1 = ((coordinate + 1) * size for coordinate in aperture["maximum"])
        angle = math.atan2(height, y1 - y0)
        position = ((x0 + x1) / 2, (y0 + y1) / 2 + math.sin(angle) * 9, height / 2 - math.cos(angle) * 9)
        ramp = mesh_actor(f"rootlet_ramp_{aperture['id']}", position,
                          (math.hypot(y1 - y0, height) + 4, x1 - x0, 18))
        ramp.set_actor_rotation(unreal.Rotator(pitch=math.degrees(angle), yaw=90), False)
        x, y = center(aperture["lure_cell"], size)
        spawn(unreal.TargetPoint, f"abyss_016_lure_{aperture['id']}", (x, y, height + 110))

    low, high = center(layout["canal"]["minimum"], size), center(layout["canal"]["maximum"], size)
    # Keep both ramp footings visibly dry; only the inter-aperture strip in
    # the lower canal receives this shallow, nonblocking water surface.
    water_start = (layout["apertures"][0]["maximum"][0] + 1) * size
    water_end = layout["apertures"][1]["minimum"][0] * size
    mesh_actor("rootlet_shallow_water", ((water_start + water_end) / 2, (low[1] + high[1]) / 2, 4),
               (water_end - water_start, layout["canal"]["water_visual_width_cm"], 8), "NoCollision")
    inner = layout["inner_side"]
    box("rootlet_inner_rock_screen", inner["screen_cm"], inner["screen_dimensions_cm"])

    gate = layout["gate"]
    gx, gy, gz = gate["closed_center_cm"]
    depth, width, clearance = gate["dimensions_cm"]
    for sign in (-1, 1):
        box(f"rootlet_gate_jamb_{sign}", (gx, gy + sign * 87.5, roof / 2), (depth, 25, roof))
    raised_top = gate["raised_center_cm"][2] + clearance / 2
    box("rootlet_gate_lintel", (gx, gy, (roof + raised_top) / 2), (depth, width, roof - raised_top))
    mesh_actor("rootlet_gate_overhead_guard", (gx, gy, (raised_top + clearance) / 2),
               (depth, width, raised_top - clearance), hidden=True)
    box(gate["id"], (gx, gy, gz), gate["dimensions_cm"], movable=True)
    spawn(unreal.TargetPoint, "abyss_016_gate_raised", gate["raised_center_cm"])

    weight = layout["counterweight"]
    mesh_actor(weight["id"], weight["closed_cm"], weight["dimensions_cm"], "NoCollision", movable=True)
    spawn(unreal.TargetPoint, "abyss_016_weight_released", weight["released_cm"])
    stalker = layout["stalker"]
    spawn(unreal.TargetPoint, stalker["id"], stalker["feet_cm"])

    for name in ("counterweight_release", "exit"):
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_016_{name}", (x, y, 110))
    for name in ("entry", "checkpoint"):
        x, y = center(layout[name], size)
        tag = "abyss_016_player_start" if name == "entry" else "abyss_016_checkpoint_start"
        start = spawn(unreal.PlayerStart, tag, (x, y, height + 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    for index, (x, y, z) in enumerate(((300, 1900, 550), (800, 1500, 490), (1700, 1900, 575),
                                       (2200, 1500, 490), (800, 250, 290), (1520, 250, 290),
                                       (2200, 250, 290), (3260, 250, 290), (3700, 900, 450),
                                       (2700, 900, 430)), 1):
        light = spawn(unreal.PointLight, f"rootlet_canal_light_{index:02d}", (x, y, z))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1700.0)
        component.set_editor_property("attenuation_radius", 925.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Rootlet Canal at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} cells, two walkable ramps and an inner-only release.")
    else:
        import unreal

        build(unreal, layout, cells)

"""Build the Counterweight Vault or validate its authored layout offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor008"
MODE_PATH = "/Script/Eidolon.EidolonCounterweightVaultGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_008_authored.json"
ANCHORS = ("entry", "checkpoint", "encounter", "winch", "exit")


def rectangle(lower, upper):
    return {(x, y) for x in range(lower[0], upper[0] + 1) for y in range(lower[1], upper[1] + 1)}


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Counterweight Vault needs unique cells and valid anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 340):
        raise ValueError("Expected the 200 cm catacomb kit with 340 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 7:
        raise ValueError("The descent requires the floor-seven receipt.")

    entry, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0], near[1] + 1):
        raise ValueError("The gate must face north between walkable cells.")
    before_gate = visit(cells, entry, frozenset((near, far)))
    if visit(cells, entry) != cells or near not in before_gate or far in before_gate or exit_cell in before_gate:
        raise ValueError("The gate must be the only route to the lower stair.")
    if not {tuple(layout[name]) for name in ANCHORS[:-1]} <= before_gate:
        raise ValueError("The encounter, winch and checkpoint must stay accessible before the gate.")

    rotunda = layout["rotunda"]
    clear_center = rectangle(rotunda["clear_center_min_cell"], rotunda["clear_center_max_cell"])
    if not clear_center <= before_gate:
        raise ValueError("The center of the rotunda must provide clear dodging space.")
    pincer, crawler = layout["encounters"]
    if tuple(pincer["spawn_cell"]) not in clear_center:
        raise ValueError("The pincer must spawn at the front of the central rotunda.")
    entrance_x, entrance_y = crawler["announced_entrance"]
    spawn_x, spawn_y = crawler["spawn_cell"]
    half_width = crawler["entrance_width_cells"] // 2
    lateral_bay = rectangle((entrance_x, entrance_y - half_width), (spawn_x, entrance_y + half_width))
    if spawn_y != entrance_y or spawn_x <= entrance_x or not lateral_bay <= before_gate:
        raise ValueError("The crawler needs a straight wide route from its announced side entrance.")
    size = layout["cell_cm"]
    if crawler["entrance_width_cells"] * size - layout["wall_thickness_cm"] <= max(crawler["expected_body_cm"][:2]):
        raise ValueError("The side entrance must clear the crawler's collision body.")

    recess = [tuple(cell) for cell in layout["winch_recess"]["approach"]]
    shelter = tuple(rotunda["shelter_wall_cell"])
    if not recess or recess[-1] != tuple(layout["winch"]) or not set(recess) <= before_gate or shelter in cells:
        raise ValueError("The winch must sit in a recessed route behind the rotunda wall.")
    if any(abs(a[0] - b[0]) + abs(a[1] - b[1]) != 1 for a, b in zip(recess, recess[1:])):
        raise ValueError("The approach to the winch must be walkable around the shelter wall.")
    sx, sy = center((spawn_x, spawn_y), size)
    wx, wy = center(recess[-1], size)
    span = int(max(abs(sx - wx), abs(sy - wy)))
    visible_cells = {
        (int(wx + (sx - wx) * step / span) // size, int(wy + (sy - wy) * step / span) // size)
        for step in range(span + 1)
    }
    if shelter not in visible_cells:
        raise ValueError("The offset wall must shelter the winch from the crawler's entry line.")
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
        box(f"vault_gate_jamb_{sign}", (gx + sign * (width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("vault_gate_header", (gx, gy, (height + clearance) / 2),
        (width, thickness, height - clearance))
    box(gate["id"], (gx, gy, clearance / 2), (width, thickness, clearance), movable=True)

    for encounter in layout["encounters"]:
        x, y = center(encounter["spawn_cell"], size)
        actor = spawn(unreal.TargetPoint, encounter["spawn_marker"], (x, y, 0))
        actor.set_actor_rotation(unreal.Rotator(yaw=encounter["spawn_yaw_deg"]), False)
    announced = layout["encounters"][1]["announced_entrance"]
    x, y = center(announced, size)
    spawn(unreal.TargetPoint, "abyss_008_lateral_entrance", (x, y, 110))

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the latch indicator mesh.")
    for indicator in layout["winch_recess"]["latch_indicators"]:
        actor = spawn(unreal.StaticMeshActor, indicator["id"], indicator["position_cm"])
        component = actor.static_mesh_component
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Could not assign the mesh to {indicator['id']}.")
        actor.set_actor_scale3d(unreal.Vector(0.38, 0.18, 0.46))
        actor.set_actor_enable_collision(False)
        component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_hidden_in_game(True)

    for name in ANCHORS:
        x, y = center(layout[name], size)
        spawn(unreal.TargetPoint, f"abyss_008_{name}", (x, y, 110))
    for name, tag in (("entry", "abyss_008_player_start"), ("checkpoint", "abyss_008_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    light_cells = {tuple(layout[name]) for name in ANCHORS}
    light_cells.update((tuple(announced), (5, 7), (7, 8), (9, 7)))
    for cell in sorted(light_cells):
        x, y = center(cell, size)
        light = spawn(unreal.PointLight, f"vault_light_{cell[0]:02d}_{cell[1]:02d}", (x, y, height - 40))
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_editor_property("intensity", 1950.0)
        component.set_editor_property("attenuation_radius", 850.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Counterweight Vault at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} floor cells, clear rotunda, announced entrance and sheltered winch.")
    else:
        import unreal

        build(unreal, layout, cells)

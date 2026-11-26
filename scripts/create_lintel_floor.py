"""Build the Abyss floor-three interactive graybox or validate its data offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor003"
MODE_PATH = "/Script/Eidolon.EidolonLintelGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_003_authored.json"
ANCHORS = ("entry", "checkpoint", "encounter", "lever", "exit", "secret")
INTERACTION_RADIUS_CM = 180


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The Lintel Gallery needs unique cells and reachable anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 420):
        raise ValueError("Expected the 200 cm catacomb kit with 420 cm headroom.")
    if layout["connections"]["entry"]["requires_completed_floor"] != 2:
        raise ValueError("The descent requires the floor-two receipt.")

    start, exit_cell = tuple(layout["entry"]), tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or far != (near[0] + 1, near[1]):
        raise ValueError("The Lintel Gallery gate must face east between walkable cells.")
    remaining = visit(cells, start, frozenset((near, far)))
    if visit(cells, start) != cells or near not in remaining or exit_cell in remaining:
        raise ValueError("The gate must be the cut between the encounter and the exit.")
    if any(tuple(layout[name]) not in remaining for name in ("checkpoint", "encounter", "lever", "secret")):
        raise ValueError("The checkpoint, encounter, lever and optional alcove must be reachable before the gate.")
    lever = center(layout["lever"], layout["cell_cm"])
    encounter = center(layout["encounter"], layout["cell_cm"])
    if sum((a - b) ** 2 for a, b in zip(lever, encounter)) <= (2 * INTERACTION_RADIUS_CM) ** 2:
        raise ValueError("The lever and encounter must have separate interaction areas.")
    if any(tuple(cell) in cells for cell in layout["blocked_piers"]):
        raise ValueError("The support piers must remain outside the walking grid.")

    perch = layout["lintel_perch"]
    mouth, ledge = tuple(perch["mouth_cell"]), tuple(perch["cell"])
    if mouth not in remaining or ledge in cells or ledge != (mouth[0] + 1, mouth[1]):
        raise ValueError("The perch must overlook the arena from outside the east wall.")
    if layout["height_cm"] - perch["ledge_height_cm"] < perch["minimum_body_headroom_cm"]:
        raise ValueError("The mantis needs enough vertical space to rise from the perch.")
    if perch["spawn_position_cm"] != [*center(ledge, layout["cell_cm"]), perch["ledge_height_cm"]]:
        raise ValueError("The mantis spawn must start at the ledge's floor surface.")
    landing = tuple(perch["landing_cell"])
    sidesteps = {tuple(cell) for cell in perch["sidestep_cells"]}
    expected_sidesteps = {(landing[0], landing[1] - 1), (landing[0], landing[1] + 1)}
    if landing not in remaining or sidesteps != expected_sidesteps or not sidesteps <= remaining:
        raise ValueError("The mantis needs a clear strike lane and two walkable sidesteps.")
    if perch["landing_position_cm"] != [*center(landing, layout["cell_cm"]), 0]:
        raise ValueError("The landing marker must sit on the strike lane's floor.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    perch = layout["lintel_perch"]
    mouth, ledge = tuple(perch["mouth_cell"]), tuple(perch["cell"])
    build_shell(layout, cells, box, wall_openings=((mouth, ledge),))
    size = layout["cell_cm"]
    height = layout["height_cm"]
    thickness = layout["wall_thickness_cm"]
    lx, ly = center(ledge, size)
    ledge_height = perch["ledge_height_cm"]
    box("lintel_ledge", (lx, ly, ledge_height / 2), (size, size, ledge_height))
    box("lintel_ledge_back", (lx + size / 2, ly, height / 2), (thickness, size + thickness, height))
    box("lintel_ledge_ceiling", (lx, ly, height + 9), (size, size, 18))
    for sign in (-1, 1):
        box(f"lintel_ledge_wall_{sign}", (lx, ly + sign * size / 2, height / 2),
            (size + thickness, thickness, height))

    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gx += size / 2
    gap_width, gap_height = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - gap_width) / 2
    for sign in (-1, 1):
        box(f"lintel_gate_jamb_{sign}", (gx, gy + sign * (gap_width + jamb) / 2, height / 2),
            (thickness, jamb, height))
    box("lintel_gate_header", (gx, gy, (height + gap_height) / 2),
        (thickness, gap_width, height - gap_height))
    box(gate["id"], (gx, gy, gap_height / 2), (thickness, gap_width, gap_height), movable=True)

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        spawn(unreal.TargetPoint, f"abyss_003_{anchor}", (x, y, 110))
        light = spawn(unreal.PointLight, f"light_{anchor}", (x, y, height - 45))
        light_component = light.get_component_by_class(unreal.PointLightComponent)
        light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light_component.set_editor_property("intensity", 1900.0)
        light_component.set_editor_property("attenuation_radius", 850.0)

    mantis = spawn(unreal.TargetPoint, perch["spawn_marker"], perch["spawn_position_cm"])
    mantis.set_actor_rotation(unreal.Rotator(yaw=perch["spawn_yaw_deg"]), False)
    spawn(unreal.TargetPoint, "abyss_003_lintel_landing", perch["landing_position_cm"])
    for name, tag in (("entry", "abyss_003_player_start"), ("checkpoint", "abyss_003_checkpoint_start")):
        x, y = center(layout[name], size)
        start = spawn(unreal.PlayerStart, tag, (x, y, 110))
        start.set_actor_rotation(unreal.Rotator(yaw=layout[f"{name}_yaw_deg"]), False)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError(f"Could not save the Lintel Gallery at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} walkable cells, raised perch and gated exit.")
    else:
        import unreal

        build(unreal, layout, cells)

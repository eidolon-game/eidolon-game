"""Generate the floor-two traversal graybox or validate its data offline."""

import json
from pathlib import Path
import sys

# The Unreal commandlet does not always add the executing script's folder.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from create_first_descent_floor import MAP_FOLDER, build_shell, center, create_graybox, visit

MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor002Survey"
MODE_PATH = "/Script/Eidolon.EidolonAbyssSurveyGameMode"
LAYOUT = Path(__file__).resolve().parents[1] / "Content/Eidolon/Data/Dungeons/abyss_002_authored.json"
ANCHORS = ("entry", "checkpoint", "encounter", "exit", "secret")


def read_layout():
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if len(cells) != len(layout["cells"]) or any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("The ossuary needs unique cells and reachable anchors.")
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("Expected the 200 cm catacomb kit with a 300 cm ceiling.")

    start = tuple(layout["entry"])
    exit_cell = tuple(layout["exit"])
    gate = layout["gate"]
    near, far = tuple(gate["from_cell"]), tuple(gate["to_cell"])
    if near not in cells or far not in cells or near[0] != far[0] or far[1] != near[1] + 1:
        raise ValueError("The ossuary gate must face north between connected walkable cells.")
    remaining = visit(cells, start, frozenset((near, far)))
    if visit(cells, start) != cells or near not in remaining or exit_cell in remaining:
        raise ValueError("The gate must be the cut between the encounter and the exit.")
    if any(tuple(layout[name]) not in remaining for name in ("checkpoint", "encounter", "secret")):
        raise ValueError("The checkpoint, encounter and optional alcove must be reachable before the gate.")
    if any(tuple(cell) in cells for cell in layout["blocked_island"]):
        raise ValueError("The ring arena needs a non-walkable central island.")

    niche = layout["ossuary_tick_niche"]
    mouth, cavity = tuple(niche["entrance_cell"]), tuple(niche["cell"])
    if mouth not in remaining or cavity in cells or cavity != (mouth[0] + 1, mouth[1]):
        raise ValueError("The east-facing creature niche must sit off the walking grid.")
    if len(niche["sidestep_cells"]) != 2 or any(tuple(cell) not in remaining for cell in niche["sidestep_cells"]):
        raise ValueError("The player needs two usable sidestep lanes.")
    return layout, cells


def build(unreal, layout, cells):
    graybox = create_graybox(unreal, MAP_PACKAGE, MODE_PATH)
    if graybox is None:
        return
    editor, world, spawn, box = graybox

    niche = layout["ossuary_tick_niche"]
    mouth, cavity = tuple(niche["entrance_cell"]), tuple(niche["cell"])
    build_shell(layout, cells, box, wall_openings=((mouth, cavity),))
    size = layout["cell_cm"]
    height = layout["height_cm"]
    thickness = layout["wall_thickness_cm"]
    mx, my = center(mouth, size)
    gap_width, gap_height = niche["clear_width_cm"], niche["clear_height_cm"]
    jamb = (size - gap_width) / 2
    for sign in (-1, 1):
        box(f"niche_jamb_{sign}", (mx + size / 2, my + sign * (gap_width + jamb) / 2, height / 2),
            (thickness, jamb, height))
    box("niche_lintel", (mx + size / 2, my, (height + gap_height) / 2),
        (thickness, gap_width, height - gap_height))
    nx, ny = center(cavity, size)
    box("niche_floor", (nx, ny, -9), (size, size, 18))
    box("niche_backstop", (nx + size / 2, ny, height / 2), (thickness, size, height))
    box("niche_ceiling", (nx, ny, height + 9), (size, size, 18))
    for sign in (-1, 1):
        box(f"niche_wall_{sign}", (nx, ny + sign * size / 2, height / 2),
            (size, thickness, height))

    gate = layout["gate"]
    gx, gy = center(gate["from_cell"], size)
    gy += size / 2
    gap_width, gap_height = gate["clear_width_cm"], gate["clear_height_cm"]
    jamb = (size - gap_width) / 2
    for sign in (-1, 1):
        box(f"ossuary_gate_jamb_{sign}", (gx + sign * (gap_width + jamb) / 2, gy, height / 2),
            (jamb, thickness, height))
    box("ossuary_gate_header", (gx, gy, (height + gap_height) / 2),
        (gap_width, thickness, height - gap_height))
    # The gate anchor is visible to map authors. The survey keeps the route open.
    spawn(unreal.TargetPoint, gate["id"], (gx, gy, gap_height / 2))

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        spawn(unreal.TargetPoint, f"abyss_002_{anchor}", (x, y, 110))
        light = spawn(unreal.PointLight, f"light_{anchor}", (x, y, height - 40))
        light_component = light.get_component_by_class(unreal.PointLightComponent)
        light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light_component.set_editor_property("intensity", 1500.0)
        light_component.set_editor_property("attenuation_radius", 700.0)
    spawn(unreal.TargetPoint, "abyss_002_ossuary_tick_01", niche["spawn_position_cm"])
    x, y = center(layout["entry"], size)
    start = spawn(unreal.PlayerStart, "abyss_002_player_start", (x, y, 110))
    start.set_actor_rotation(unreal.Rotator(yaw=layout["entry_yaw_deg"]), False)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError("Could not save the Ossuary traversal graybox.")


if __name__ == "__main__":
    layout, cells = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated {layout['id']}: {len(cells)} walkable cells, ring arena, gated exit and two sidestep lanes.")
    else:
        import unreal

        build(unreal, layout, cells)

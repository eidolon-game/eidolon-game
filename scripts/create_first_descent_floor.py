"""Build a traversal graybox in Unreal, or run --validate with ordinary Python.

In UnrealEditor-Cmd use -run=pythonscript -script=<absolute path to this file>.
Generate in a clean source checkout. Existing map assets are reused.
"""

import json
from pathlib import Path
import sys

MAP_FOLDER = "/Game/Eidolon/Maps"
MAP_PACKAGE = MAP_FOLDER + "/AbyssFloor001"
MODE_CLASS = "/Script/Eidolon.EidolonAbyssFloorGameMode"
LAYOUT_PATH = (
    Path(__file__).resolve().parents[1]
    / "Content" / "Eidolon" / "Data" / "Dungeons" / "abyss_001_authored.json"
)
ANCHORS = ("entry", "checkpoint", "encounter", "anomaly", "exit", "secret")
DIRECTIONS = ((1, 0), (-1, 0), (0, 1), (0, -1))
WALL_THICKNESS = 25
FLOOR_THICKNESS = 18
GATE_WIDTH = 150
GATE_HEIGHT = 240
SECOND_CRAWLER_CELL = (8, 3)
RIVEN_CELL = (3, 2)


def neighbors(cell):
    x, y = cell
    return [(x + dx, y + dy) for dx, dy in DIRECTIONS]


def visit(cells, start, blocked_edge=None):
    """Return the cells reachable by cardinal movement from start."""
    pending = [start]
    visited = {start}
    while pending:
        cell = pending.pop()
        for adjacent in neighbors(cell):
            if adjacent in cells and adjacent not in visited:
                if frozenset((cell, adjacent)) != blocked_edge:
                    visited.add(adjacent)
                    pending.append(adjacent)
    return visited


def read_layout():
    layout = json.loads(LAYOUT_PATH.read_text(encoding="utf-8"))
    cells = {tuple(cell) for cell in layout["cells"]}
    if (layout["cell_cm"], layout["height_cm"]) != (200, 300):
        raise ValueError("The floor must use the authored 200 cm grid and 300 cm wall height.")
    if len(cells) != 52 or len(layout["cells"]) != len(cells):
        raise ValueError("Expected 52 distinct floor cells.")
    if (min(x for x, _ in cells), max(x for x, _ in cells),
            min(y for _, y in cells), max(y for _, y in cells)) != (0, 15, 0, 6):
        raise ValueError("The cells must fit the 3200 x 1400 cm authored footprint.")
    if any(tuple(layout[name]) not in cells for name in ANCHORS):
        raise ValueError("Every anchor must lie on a floor cell.")
    if layout["entry"] == layout["exit"]:
        raise ValueError("Entry and exit must be separate.")

    entry = tuple(layout["entry"])
    gate_x = layout["gate_x"]
    crossings = [(x, y) for x, y in cells if x == gate_x and (x - 1, y) in cells]
    if len(crossings) != 1:
        raise ValueError("Expected one cell crossing at the combat gate.")
    gate = crossings[0]
    gate_edge = frozenset(((gate[0] - 1, gate[1]), gate))
    if visit(cells, entry) != cells:
        raise ValueError("The open gate must leave the full floor accessible.")
    before_gate = visit(cells, entry, gate_edge)
    if (tuple(layout["encounter"]) not in before_gate
            or SECOND_CRAWLER_CELL not in before_gate
            or RIVEN_CELL not in before_gate
            or tuple(layout["anomaly"]) in before_gate
            or tuple(layout["exit"]) in before_gate):
        raise ValueError("The closed gate must stand after the encounter and before the anomaly and exit.")
    return layout, cells, gate


def center(cell, size):
    return tuple(coordinate * size + size / 2 for coordinate in cell)


def build_map(unreal, layout, cells, gate):
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(MAP_PACKAGE):
        unreal.log("EIDOLON Abyss floor exists. Reusing the saved map.")
        return

    if not assets.does_directory_exist(MAP_FOLDER) and not assets.make_directory(MAP_FOLDER):
        raise RuntimeError("Failed to create the Eidolon Maps folder.")
    mode_class = unreal.load_class(None, MODE_CLASS)
    if not mode_class:
        raise RuntimeError("Build EidolonEditor before generating the Abyss floor.")
    editor = unreal.EditorLoadingAndSavingUtils
    world = editor.new_blank_map(False)
    if not world:
        raise RuntimeError("Failed to create the Abyss floor map.")
    settings = unreal.GameplayStatics.get_actor_of_class(world, unreal.WorldSettings)
    if not settings:
        raise RuntimeError("The Abyss floor has no WorldSettings.")
    settings.set_editor_property("default_game_mode", mode_class)

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not cube:
        raise RuntimeError("Could not load the Unreal geometry cube.")

    def spawn(actor_class, label, position):
        actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(*position))
        if not actor:
            raise RuntimeError(f"Failed to spawn {label}.")
        actor.set_actor_label(label)
        actor.set_editor_property("tags", [unreal.Name(label)])
        return actor

    def box(label, position, dimensions, movable=False):
        actor = spawn(unreal.StaticMeshActor, label, position)
        component = actor.static_mesh_component
        component.set_mobility(
            unreal.ComponentMobility.MOVABLE if movable else unreal.ComponentMobility.STATIC
        )
        if not component.set_static_mesh(cube):
            raise RuntimeError(f"Failed to assign cube mesh to {label}.")
        component.set_collision_profile_name("BlockAllDynamic" if movable else "BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*(dimension / 100 for dimension in dimensions)))

    size = layout["cell_cm"]
    height = layout["height_cm"]
    for cell in sorted(cells):
        x, y = center(cell, size)
        label = f"{cell[0]:02d}_{cell[1]:02d}"
        box(f"floor_{label}", (x, y, -FLOOR_THICKNESS / 2), (size, size, FLOOR_THICKNESS))
        box(f"ceiling_{label}", (x, y, height + FLOOR_THICKNESS / 2), (size, size, FLOOR_THICKNESS))
        for dx, dy in DIRECTIONS:
            if (cell[0] + dx, cell[1] + dy) in cells:
                continue
            span = size + WALL_THICKNESS
            dimensions = (WALL_THICKNESS, span, height) if dx else (span, WALL_THICKNESS, height)
            box(f"wall_{label}_{dx}_{dy}",
                (x + dx * size / 2, y + dy * size / 2, height / 2), dimensions)

    gate_x = gate[0] * size
    _, gate_y = center(gate, size)
    jamb_width = (size - GATE_WIDTH) / 2
    for sign in (-1, 1):
        box(f"gate_jamb_{sign}", (gate_x, gate_y + sign * (GATE_WIDTH + jamb_width) / 2, height / 2),
            (WALL_THICKNESS, jamb_width, height))
    header_height = height - GATE_HEIGHT
    box("gate_header", (gate_x, gate_y, GATE_HEIGHT + header_height / 2),
        (WALL_THICKNESS, GATE_WIDTH, header_height))
    box("abyss_001_combat_gate", (gate_x, gate_y, GATE_HEIGHT / 2),
        (WALL_THICKNESS, GATE_WIDTH, GATE_HEIGHT), movable=True)

    marker_names = {
        "entry": "abyss_001_entry",
        "checkpoint": "checkpoint_abyss_001",
        "encounter": "abyss_001_crawler_01",
        "anomaly": "first_anomaly",
        "exit": "surface_exit",
        "secret": "secret_alcove",
    }
    for anchor, marker in marker_names.items():
        x, y = center(layout[anchor], size)
        z = 100 if anchor in ("entry", "anomaly", "exit") else 0
        spawn(unreal.TargetPoint, marker, (x, y, z))
    x, y = center(SECOND_CRAWLER_CELL, size)
    spawn(unreal.TargetPoint, "abyss_001_crawler_02", (x, y, 0))
    x, y = center(RIVEN_CELL, size)
    spawn(unreal.TargetPoint, "abyss_001_riven", (x, y, 110))
    x, y = center(layout["entry"], size)
    spawn(unreal.PlayerStart, "abyss_001_player_start", (x, y, 110))

    for anchor in ANCHORS:
        x, y = center(layout[anchor], size)
        actor = spawn(unreal.PointLight, f"light_{anchor}", (x, y, height - 40))
        light = actor.get_component_by_class(unreal.PointLightComponent)
        light.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.set_editor_property("intensity", 1600.0)
        light.set_editor_property("attenuation_radius", 700.0)

    if not editor.save_map(world, MAP_PACKAGE):
        raise RuntimeError("Failed to save the Abyss floor map.")
    unreal.log(f"EIDOLON Abyss floor graybox saved at {MAP_PACKAGE}.")


if __name__ == "__main__":
    layout, cells, gate = read_layout()
    if "--validate" in sys.argv[1:]:
        print(f"Validated abyss_001 layout: {len(cells)} cells, one combat gate at {gate}.")
    else:
        import unreal

        build_map(unreal, layout, cells, gate)

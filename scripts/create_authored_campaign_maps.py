"""Run the authored campaign map generators in order.

Pass --validate with standard Python for structural checks. In the Unreal
editor Python commandlet, omit --validate to create the campaign packages.
"""
import json
import os
import runpy
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
PROJECT = SCRIPTS.parent
GENERATORS = (
    ("create_valdris_prologue.py", None),
    ("create_first_descent_floor.py", None),
    ("create_ossuary_floor.py", "EIDOLON_OSSUARY_INTERACTIVE"),
    ("create_lintel_floor.py", None),
    ("create_sluice_floor.py", None),
    ("create_marrow_ram_floor.py", "EIDOLON_MARROW_INTERACTIVE"),
    ("create_ambulatory_floor.py", None),
    ("create_pillar_hall_floor.py", None),
    ("create_counterweight_vault_floor.py", None),
    ("create_urn_fork_floor.py", None),
    ("create_rootclaw_floor.py", None),
    ("create_buried_forest_entry.py", None),
    ("create_oblique_clearing.py", None),
    ("create_branch_terrace.py", None),
    ("create_fern_basin.py", None),
    ("create_hollow_trunk_plates.py", None),
    ("create_rootlet_canal.py", None),
    ("create_suspended_trunks.py", None),
)


def run_generator(filename, interactive_variable):
    original_path = sys.path[:]
    previous = os.environ.get(interactive_variable) if interactive_variable else None
    try:
        sys.path.insert(0, str(SCRIPTS))
        if interactive_variable:
            os.environ[interactive_variable] = "1"
        runpy.run_path(str(SCRIPTS / filename), run_name="__main__")
    finally:
        sys.path[:] = original_path
        if interactive_variable:
            if previous is None:
                os.environ.pop(interactive_variable, None)
            else:
                os.environ[interactive_variable] = previous


def write_package_manifest():
    maps = ["ValdrisPrologue"] + [f"AbyssFloor{floor:03d}" for floor in range(1, len(GENERATORS))]
    for name in maps:
        path = PROJECT / "Content" / "Eidolon" / "Maps" / f"{name}.umap"
        if not path.is_file() or path.stat().st_size == 0:
            raise RuntimeError(f"Campaign generator did not produce a nonempty map at {path}.")

    packages = [f"/Game/Eidolon/Maps/{name}" for name in maps]
    manifest = PROJECT / "Saved" / "EidolonAuthoredMaps.json"
    manifest.parent.mkdir(parents=True, exist_ok=True)
    pending = manifest.with_suffix(".tmp")
    pending.write_text(json.dumps({"startup_map": packages[0], "maps": packages}, indent=2) + "\n", encoding="utf-8")
    pending.replace(manifest)


if __name__ == "__main__":
    for filename, interactive_variable in GENERATORS:
        run_generator(filename, interactive_variable)
    if "--validate" not in sys.argv[1:]:
        write_package_manifest()

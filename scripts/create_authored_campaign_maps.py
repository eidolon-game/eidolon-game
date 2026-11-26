"""Run the authored campaign map generators in order.

Pass --validate with standard Python for structural checks. In the Unreal
editor Python commandlet, omit --validate to create the campaign packages.
"""
import os
import runpy
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
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


if __name__ == "__main__":
    for filename, interactive_variable in GENERATORS:
        run_generator(filename, interactive_variable)

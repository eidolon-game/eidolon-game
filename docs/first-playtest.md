# First playtest — 0.1.0 development

Status: source-only, not compiled or playtested. UE5 minor and compiler still require validation (issue #1).

## Setup
1. Clone and check out feature/third-person-prototype (or develop once merged). Run git lfs install and git lfs pull.
2. Install a compatible UE5 editor and supported C++ toolchain (Windows: Visual Studio with Game development with C++ and required SDK).
3. Associate Eidolon.uproject with the chosen engine and generate project files. Build EidolonEditor Development Editor.
4. Open the project; create a Basic level with a floor, Player Start and collision obstacles. Ensure World Settings game mode uses EidolonGameMode. Save locally under Content/Eidolon/Maps.
5. Press Play. The native character has no mesh: optionally create a Blueprint child of EidolonCharacter, assign a licensed skeletal mesh and animation Blueprint, and set it as the default pawn in a Blueprint child of EidolonGameMode.

## Controls
| Action | PC | Controller |
| --- | --- | --- |
| Move | WASD | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | Cross / A |
| Sprint (hold) | Left Shift | L3 / left stick click |

## Acceptance checklist
- Camera-relative movement; character turns toward motion.
- Mouse and stick camera pitch/yaw behave consistently; verify inversion.
- Stick dead zone prevents drift; check diagonal speed and opposite key cancellation.
- Jump and sprint release/cancel correctly; sprint returns to walking after restart.
- Camera retracts near walls and recovers.
- Verify 30/60/120 FPS stick look consistency.
- Restart PIE and confirm mappings are not duplicated.
- Record UE version, compiler, build logs, controller model and results in issue #1/#2.
- No console build or performance claims until platform-specific testing.

No binary map, character mesh, animation or console SDK is included.

## Checkpoint recovery regression (Game #27)

- With a valid checkpoint, advance beyond the saved state and restart. Verify that the saved chapter, floor and flags return.
- After advancing, remove or corrupt the local test slot and restart. Verify that the level restarts with the current chapter, floor, objectives and flags in memory, and that the log records the failed load.
- In a fresh campaign with no slot, verify that restart still recovers from defeat and leaves the player at the first chapter.

## Encounter and melee regressions (Game #25 and #26)

- Force one spawn to fail in a multi-enemy encounter. Defeat the spawned enemies, retry, and verify only the missing slot spawns. Repeat at either dungeon's floor 100. Confirm dialogue, floor advancement and checkpoint writes stay gated while any required spawn is pending.
- Verify a zero-enemy chapter can still advance its dialogue.
- In the Arena, test player and boss at 259, 260, 261 and 285 cm horizontally, and 179 and 180 cm vertically. Check the player facing cone, obstacles, ordinary and giant profiles, and leaving melee range during a boss windup.

## First Descent across two native maps

Build `EidolonEditor` with UE 5.6. In a fresh checkout, run each generator in the editor Python commandlet, substituting absolute project and script paths.

```text
UnrealEditor-Cmd <Eidolon.uproject> -run=pythonscript -script=<scripts/create_valdris_prologue.py> -unattended -nop4
UnrealEditor-Cmd <Eidolon.uproject> -run=pythonscript -script=<scripts/create_first_descent_floor.py> -unattended -nop4
```

The generators create `/Game/Eidolon/Maps/ValdrisPrologue` and `/Game/Eidolon/Maps/AbyssFloor001`. They reuse existing generated assets. Use a clean checkout to reproduce edits to the generators, especially after generating the older traversal-only Abyss map. Open `ValdrisPrologue` in the editor with an isolated test save slot environment and begin a fresh campaign.

| Test | Expected result |
| --- | --- |
| Farewell at home | Interacting at the house plays the chapter 1 dialogue. Registration becomes available after a successful checkpoint. |
| Early or remote interaction | The descent, dormitory and reveal stay locked outside their required steps. Moving out of range or putting a wall between the pawn and an anchor prevents interaction. |
| Register and enter the Abyss | Registration persists first. Arrival at the authored entry records arrival, then enables the crawler encounter. |
| Defeat both crawlers | The gate stays closed after one defeat, a failed spawn or a failed checkpoint. A successful checkpoint after both defeats opens the way. |
| Observe the anomaly | Interact and acknowledge the cue at the anomaly anchor. Attempting to leave out of order does not record a return. |
| Exit and rest | The return is recorded on arrival in Valdris after the authenticated exit. Complete the dormitory interaction before approaching the Veyr door. |
| Reveal Noah | The last checkpoint saves the chapter transition and Noah's presence together. Reloading resumes chapter 3 and does not repeat the reveal. |
| Recovery and controls | Repeat after defeat, relaunch and failed writes. Cover keyboard and controller interactions. Verify that travel and combat depend on the authored maps, and that the existing Arena still starts independently. |

These maps use graybox geometry and interaction text. Character staging, final effects, world streaming and distribution of the full campaign have separate verification gates.

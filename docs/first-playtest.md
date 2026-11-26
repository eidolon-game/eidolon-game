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

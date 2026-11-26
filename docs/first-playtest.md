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

The campaign generator creates Valdris and all available interactive Abyss floors in a single editor session. Substitute absolute paths and use a fresh checkout when regenerating packages after layout edits.

```text
UnrealEditor-Cmd /path/to/Eidolon.uproject -run=pythonscript -script=/path/to/scripts/create_authored_campaign_maps.py -unattended -nop4
```

Build `EidolonEditor` with UE 5.6. In a fresh checkout, run each generator in the editor Python commandlet, substituting absolute project and script paths.

```text
UnrealEditor-Cmd <Eidolon.uproject> -run=pythonscript -script=<scripts/create_valdris_prologue.py> -unattended -nop4
UnrealEditor-Cmd <Eidolon.uproject> -run=pythonscript -script=<scripts/create_first_descent_floor.py> -unattended -nop4
```

The generators create `/Game/Eidolon/Maps/ValdrisPrologue` and `/Game/Eidolon/Maps/AbyssFloor001`. They reuse existing generated assets. Use a clean checkout to reproduce edits to the generators, especially after generating the older traversal-only Abyss map. Start the editor with `-EidolonAuthoredCampaign` to use the isolated campaign prototype slot and open `ValdrisPrologue`. The [Win64 campaign prototype launcher](windows-campaign-package.md) supplies the same flag.

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

## The Weight of Memories

From the chapter 3 checkpoint after Noah's reveal, return to the Abyss entrance in Valdris. On the first floor, find the Riven anchor beyond the checkpoint and finish the conversation. Test that wrong or obscured targets leave the checkpoint unchanged. Reload after the saved dialogue and confirm it is not repeated. Use the surface exit, then acknowledge the adventurer's warning near the Guild. Confirm that the chapter 4 checkpoint keeps Noah's presence and Riven's conversation. Riven and the adventurer use interaction markers in this graybox.

## Ossuary traversal survey

Run `python3 -B scripts/create_ossuary_floor.py --validate` to verify the authored floor-two connectivity. After building the editor, execute `scripts/create_ossuary_floor.py` with the same Unreal Python commandlet used above and open `/Game/Eidolon/Maps/AbyssFloor002Survey`. Walk from the stair to the ring and the optional alcove. Check both sidestep lanes, the 110 cm niche, and the passage to the next stair. The survey leaves the gate open and reserves actor tags for the future encounter. Generating the survey requires UE 5.6.

The standalone `abyss_progression` suite exercises 100 clears in order, the floor 90 memory receipt and the floor 100 Guardian event. Connecting the policy to native encounters and checkpoint writes remains a runtime task.

## Ossuary encounter

Generate the interactive package separately from the survey. In a Linux shell with UE 5.6 available:

```bash
EIDOLON_OSSUARY_INTERACTIVE=1 UnrealEditor-Cmd /path/to/Eidolon.uproject -run=pythonscript -script=/path/to/scripts/create_ossuary_floor.py -unattended -nop4
```

From an authored Chapter 4 checkpoint after Riven's scene and the Guild warning, interact with the Abyss entrance in Valdris to open `/Game/Eidolon/Maps/AbyssFloor002`. The first floor credit comes from the completed prologue.

- Approach the arena marker and interact. An obstruction in the spawn area must leave the gate closed and permit a retry.
- Wait for the crouch, then sidestep. Compare the missed-leap recovery with the recovery after a connect. Use a wall, a defeated pawn and a large frame time to check swept collision and one impact per leap.
- Defeat the spawned Tick. Confirm the gate opens after the checkpoint and remains open after reload. Simulate a failed write with an isolated test save. The gate must stay closed until retry succeeds. Entering the map from an earlier chapter or from a legacy Arena save must not grant floor credit.
- Interact at the lower stair. The target is reserved for `AbyssFloor003`; the current project reports an unavailable passage until that package exists.

The interactive prototype uses the shared collision mesh. The Tick GLB and scrape WAV are separate art deliverables awaiting Unreal import and integration.

## Lintel Gallery encounter

Generate `/Game/Eidolon/Maps/AbyssFloor003` with `scripts/create_lintel_floor.py` in the Unreal Python commandlet. Enter from the Ossuary exit after the floor 2 checkpoint, or resume from the Valdris entrance. Test early direct entry, duplicate tags, premature lever use, and replay from the saved checkpoint. The gate must stay closed until an authoritative defeat and a successful lever checkpoint.

Move to the encounter marker and start the fight. Use the blades and the floor marker to anticipate the fixed dive. Compare hits, sidesteps, and wall impacts. The Mantis must clear the ledge before lowering toward the floor, use one impact per dive, and offer a longer vulnerable recovery on a miss. Verify the approach at low and high frame rates. The lower stair is reserved for floor 4.

## Drainage Canals

Generate `/Game/Eidolon/Maps/AbyssFloor004` with `scripts/create_sluice_floor.py` in the Unreal Python commandlet. Enter from the Lintel Gallery after the floor 3 checkpoint, or resume from Valdris.

Activate the west then east sluice, then repeat on a fresh test save in the opposite order. After one activation, the water drops and the gate stays closed. Reload and verify that the first sluice is still open. A failed second save leaves the first activation intact and the gate closed. A successful second save drains the basin, unlocks the ramp and records floor 4 together. Reopen the map to verify the gate stays open. Verify that early entry and interactions through walls cannot grant credit.

The raised footpath is optional. The moored Hagfish activates near the canal. Test the visual warning, sidesteps, one hit per strip and clipping at walls. Test reaching both levers while it is alive. A failed spawn or defeat cannot affect a sluice, and draining the basin removes the hazard. The creature uses placeholder geometry pending GLB import. The lower stair awaits the interactive `AbyssFloor005` package.

## Broken Portico survey

Generate `/Game/Eidolon/Maps/AbyssFloor005Survey` using `scripts/create_marrow_ram_floor.py` in the Unreal Python commandlet. Inspect the beam, the two escape lanes and the bridge route to the latch. The survey uses the open route for geometry inspection.

For the interactive package, set `EIDOLON_MARROW_INTERACTIVE=1` before running the editor Python commandlet. Enter `/Game/Eidolon/Maps/AbyssFloor005` from the Canals after their saved completion.

- Activate the encounter at the arena marker. The Ram shows two hoof scrapes before charging. Bait a hit on the authored beam, then repeat with a fresh checkpoint using a kill instead. Hits on the player, floor geometry and other walls must leave the latch locked.
- Confirm that a beam hit releases the latch while the Ram stays alive. Approach from the side bridge and operate the latch. The gate must remain closed until the checkpoint succeeds.
- Simulate failed writes separately for release and latch operation. Retry at the appropriate anchor and ensure no duplicate progress. Reload after release and after opening. The gate should open only in the latter state.
- Inspect collision, escape lanes and the checkpoint spawn. The next stair is reserved for the interactive floor 6 map. The Ram uses placeholder geometry while the GLB and hoof cue await import.

## Double-Ring Ambulatory

Generate `/Game/Eidolon/Maps/AbyssFloor006` with `scripts/create_ambulatory_floor.py` in the Unreal Python commandlet. Enter from the Broken Portico once its latch is saved.

- Collect the counterweight at the sarcophagus. Confirm it follows the player and that an early interaction with the winch or the lower stair does nothing. Compare the short inner route with the outer patrol route.
- Test the Millipede on the outer path. Check that each corner is traversable, a temporary obstruction reverses or pauses the patrol, and the side warning matches the attack on both sides. Fight it or bypass it without changing the counterweight requirement.
- Lose the carrier to defeat or below the floor. The counterweight returns to its source once the recovery checkpoint succeeds. Reload after recovery and check that the weight can be collected again. Exercise a failed recovery write and retry.
- Install the carried weight at the winch. If saving fails, it stays carried and the gate remains closed. After success, the weight sits at the winch, the floor 6 credit and open gate survive reload, and a repeated interaction grants no additional progress.
- The lower stair targets the future interactive floor 7 package. The Millipede still uses placeholder geometry while the static GLB awaits rig and import.

## Pillar Hall

Generate `/Game/Eidolon/Maps/AbyssFloor007` with `scripts/create_pillar_hall_floor.py` in Unreal's Python commandlet. Enter from the Double-Ring Ambulatory after the saved counterweight installation.

- Find both mirror controls via the side passages. Turn either first. The pointer must rotate 90 degrees with that mirror after each successful save. Check the receivers from the approach, including the second mirror's opposite orientations.
- Save a partial alignment and reload. Test a failed write and verify the pointer, other mirror and closed gate stay at their saved state. When both point at their receivers, confirm the gate and floor credit persist on reload.
- Approach each Mortar Leech from two sides, step behind a pillar after it locks its thrust, and check one hit per cast. Both mirrors must be reachable while both enemies are alive or fail to spawn. The monster mesh is a prototype pending import and rig.
- The lower stair targets the future interactive floor 8 package.

## Counterweight Vault

Generate `/Game/Eidolon/Maps/AbyssFloor008` with `scripts/create_counterweight_vault_floor.py` in Unreal's Python commandlet. Enter from the Pillar Hall after the mirrors have been aligned and saved.

- Start the first fight at the arena marker. Compare the Pincer's frontal guard, its flank, and the recovery after the sweep. Interacting with the winch before the tracked enemy is defeated must leave both latches hidden and the gate closed.
- Defeat the Pincer and operate the winch. Check the first indicator and the eastern entrance warning. Reload from this checkpoint and verify only the Crawler spawns. Test a blocked spawn, clear the entrance, and retry from the winch or arena.
- Defeat the Crawler and operate the winch again. The second indicator, open gate and floor credit must appear only after saving succeeds. Retry a failed write and confirm no repeat encounter or credit.
- Fault-test both defeat receipts and the first latch separately. The lower stair targets the future floor 9 package. The Pincer still uses placeholder geometry pending import and rig.

## Urn Fork

Generate `/Game/Eidolon/Maps/AbyssFloor009` with `scripts/create_urn_fork_floor.py` in Unreal's Python commandlet. Enter after the Counterweight Vault checkpoint. Record one run per route on separate test saves.

- In the upper wing, verify that the Mantises' ledge facing and the Tick's niche are compatible with their warnings and recovery. In the lower wing, verify the Hagfish's lane, dry refuge and sightline breaks. Start from the branch entrance and reach its latch without crossing the other route.
- Try the latch from outside its interaction zone and behind cover. Neither attempt should write progress. With a failed latch save, the gate stays shut and either route is still available.
- Save either latch and check the same gate opens. Reload to confirm the original route persists, the other latch cannot override it, and both routes lead to the same stair. The stair awaits the `AbyssFloor010` package.

## Root Antechamber

Generate `/Game/Eidolon/Maps/AbyssFloor010` with `scripts/create_rootclaw_floor.py` in Unreal's Python commandlet. Enter after either Urn Fork latch is saved. The two courtyard encounters are optional. Verify the crawler remains outside the boss hall and that skipping both still allows the checkpoint and encounter.

- Start Rootclaw from the arena marker and identify its three lanes. Test each side of the sweep, the narrow thrust, and counterattack timing on controller and keyboard. Keep a readable escape direction and test the camera against both outer walls.
- Cross half health during windup, contact, and recovery. Rootclaw should finish the current move before announcing a single phase transition. Check a blocked adjacent lane and a lethal hit that crosses the threshold before moving. The lateral step should cause no damage.
- Suppress the boss spawn or a lane binding, then retry. Only loss of the registered Rootclaw should advance the boss receipt. Simulate a failed save after defeat and retry at the arena marker. Reload from a successful receipt and confirm the exit stays open and Rootclaw does not return.
- The stair opens the Soglia della Foresta Sepolta once `AbyssFloor011` is generated. The mesh and theme are previews awaiting rig, import, and runtime testing.

## Buried Forest Threshold

Generate `/Game/Eidolon/Maps/AbyssFloor011` with `scripts/create_buried_forest_entry.py` or the campaign generator. Enter through the stair after Rootclaw's defeat has been saved. Try both the raised dry path and the shallow marsh, then pass under the root arch to reach the encounter.

- Test the Stalker's line of sight from the approach and behind roots. Break line of sight before the attack locks, then repeat after the pounce direction locks. Check the ground warning, one impact per attack, the longer recovery on a miss, and the return to the ambush after losing sight of the player.
- Block the spawn, retry, then defeat the registered Stalker. Test a failed save and retry before advancing. After a successful save, reload and confirm the gate stays open and the Stalker stays defeated. The exit leads to `AbyssFloor012` once generated.
- Check route clearance, ledges, sightlines, and the arch with mouse and keyboard and with a controller. The map and enemy use placeholder art. The static Root Stalker and Root Arch meshes and the ambience preview require Unreal import and runtime integration.

## Oblique Clearing

Generate `/Game/Eidolon/Maps/AbyssFloor012` with `scripts/create_oblique_clearing.py` or the campaign generator. Enter from the Threshold after saving the Stalker defeat. Walk the incline to the next stair, then repeat via the overlook. Check both directions along the ramp, the join to the overlook and the camera near the ledge. The route has no enemies or time limit.

- Try direct map entry with floor 11 incomplete. Verify the stair does not grant credit. On an eligible save, try interacting out of range, behind cover and while defeated.
- Fail the checkpoint write at the stair. Verify progress stays on floor 11 and the player stays at the stair. Retry and check that the floor 12 credit survives reload. Interact again and verify no duplicate progress occurs.
- The stair leads to `AbyssFloor013` once generated. With that package absent, the stair reports an unavailable route and retains the saved completion. Confirm that the authored checkpoint remains reachable on reload.

## Branch Terraces

Generate `/Game/Eidolon/Maps/AbyssFloor013` with `scripts/create_branch_terrace.py` or the campaign generator. Enter after completing the Oblique Clearing. Play both routes from a floor 12 checkpoint with the Stalker alive. Use the external ramp to bypass the lower encounter or cross the short route after the fixed landing cue. Check clearance without jumping and use the camera in both orientations near the switchbacks.

- Observe the anchored climb and landing. Walk out of the impact zone after the telegraph. Test a blocked ascent or landing and a missing optional spawn or perch anchor. The upper stair release must remain reachable in every case.
- Try the release from the lower route, out of range and behind geometry. From the upper platform, fail the checkpoint and verify both the lower shortcut and the onward gate stay blocked. Save successfully, then test returning through the opened shortcut and leaving through the exit.
- Reload before and after the release. Verify the closed or open geometry matches the checkpoint and repeated interactions do not re-award completion. The exit leads to `AbyssFloor014` once generated. Rigging and import of the preview ladder and native runtime tests are pending.

## Fern Basin

Generate `/Game/Eidolon/Maps/AbyssFloor014` with `scripts/create_fern_basin.py` or the campaign generator. Enter after saving the Branch Terraces stair release. Reach each drain by the boardwalk and by the raised service route. Verify the service route, drain pads and exit remain usable with the optional Stalker alive and after a blocked spawn.

- Use the upper drain first, then repeat from a fresh checkpoint with the lower drain first. One saved drain lowers the water once and leaves the gate closed. Reload at that point and verify both the water level and the remaining prompt.
- Attempt early entry, remote and obstructed interactions, and toggling the same drain twice. Fail the second write in each order. Only the first drain should remain active. Retry and verify that the final drop, open channel and floor 14 completion appear together and survive reload.
- Take the exit after a successful checkpoint. It leads to `AbyssFloor015` once generated. Check the ledges, camera and controls in UE 5.6. The handwheel prop and audio cues await runtime integration.

## Hollow Trunk Plates

Generate `/Game/Eidolon/Maps/AbyssFloor015` with `scripts/create_hollow_trunk_plates.py` or the campaign generator. Enter after opening both Fern Basin drains. Pick up the pin in the alcove and carry it around the hollow trunk on each route. Verify that the Stalker can be bypassed, with coverage for both a successful and a blocked spawn.

- Interact away from the socket to set the pin down. Dodge while carrying and check that the pin drops and can be picked up again. Try dropping over a gap. On defeat, falling out of the carrying area or requesting recovery at the alcove, verify that the single pin returns.
- Try the socket unladen, out of range, behind cover and in the air. Fail the seat save while holding the pin. Check that the pin remains available, with the gate closed and walkway disabled, then retry. A successful save seats the pin and enables the walkway and exit together.
- Reload with a loose, carried or seated pin and verify that exactly one instance appears. The exit leads to `AbyssFloor016` once generated. Verify collision on the lowered walkway. The pin artwork and audio still need Unreal import and routing.

## Rootlet Canal

Generate `/Game/Eidolon/Maps/AbyssFloor016` with `scripts/create_rootlet_canal.py` or the campaign generator. Enter after saving the Hollow Trunk pin. Walk from the high gallery to either canal inlet and use the low route to reach the inside of the portcullis. Check that a player can backtrack and return to the checkpoint with the gate still closed.

- Try the release from the outside, behind cover and out of range. Bypass the Stalker and release the counterweight from the inner landing. Repeat by drawing the Stalker toward an opening before taking the other one. Verify support, swept movement and the fixed pounce on both approaches. A failed enemy spawn must leave both routes usable.
- With a failed checkpoint, the grate and counterweight must stay in place. Save successfully, then verify the raised grate, lowered counterweight, clear return opening and floor 16 completion. Reload before and after this change and confirm opening the grate does not award completion twice.
- Check the headroom under the lifted grate, collision on the ramps and travel toward the future `AbyssFloor017`. The separate gate art and sound still await Unreal import and routing.

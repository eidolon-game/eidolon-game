# Combat department handoff

Status: source implementation only; Unreal build, automation and playtest pending.
Base: campaign foundation fc9cf93c896f3338356fc584b72f773a3d281181.
Ownership: new Combat directory; existing Character, ArenaEnemy, GameMode, input and HUD untouched pending Core coordination.

## Integration
Use one combat state per combatant, game-time seconds for all timestamps. Never retain prototype health alongside the new state as competing authority. Core owns input/camera and character lifecycle; Animation owns montage windows and root motion; AI owns attack selection and group coordination; RPG owns persistent stats/equipment. Combat owns resource spending and hit resolution. GAS migration is deferred until the spike is compiled; do not add a second authority. Campaign objectives must fire once on confirmed death, retaining chapter attribution.

## Acceptance
Run Eidolon.Combat.Rules in UE 5.6 editor. Add actual component and collision integration, directional attacks, team filtering, hit deduplication, lock-on, status durations, cooldowns, weapon-specific timing, boss mechanics, animation and audio events. Playtest keyboard and controller at 30/60/120 fps. Verify no hits through walls, no duplicate hits, no invulnerability during recovery, no negative resources, no repeated kill credit, no stale target after death. No final assets or playable build claimed.

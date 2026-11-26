# Core architecture handoff

Status: source implemented, not compiled, not integrated, not approved.
Base: feature/campaign-foundation; stacked on draft PRs #4–#6. No shared file modified.

## Contracts
- Keep UEidolonCampaignSubsystem the sole persistent campaign authority. AEidolonCoreGameState.Region is transient world/UI context, not an unlock flag. SetRegion is authority-only; subscribe to OnRegionChanged and read current value on subscription.
- AEidolonCoreGameMode extends the existing prototype rather than replacing its behavior. Opt in only after director coordinates configuration and character changes.
- UEidolonLockOnComponent: attach to existing pawn; target implements IEidolonTargetable, has Pawn or WorldDynamic collision and supplies a finite world-space aim point. Acquire performs bounded spatial query and visibility checks; tick only while locked. Release on death, unpossession, unload, obstruction or out-of-range. Bind UI to OnTargetChanged. Combat must still validate damage independently. Input owner must bind ToggleLockOn to an unused mapping; right stick click is currently memory recovery.
- UEidolonRegionDefinition / FEidolonRegionRow: stable region ID, localized display name, soft map reference, required campaign flag. IDs and flags must match campaign definitions; no data assets generated yet. Asset manager scanning and cook rules are integration gates.
- UEidolonAsyncAssetSubsystem: game-thread C++ requests; invalid GUID means rejected request; completion receives null on load failure; cancellation suppresses completion. Caller binds weak UObject delegates and retains assets as needed. No synchronous fallback.
- No console SDKs or third-party assets introduced.

## Integration sequence (director approval required)
1. Coordinate ownership of DefaultEngine.ini, Eidolon.Build.cs and EidolonCharacter with combat/world/UI owners.
2. Select /Script/Eidolon.EidolonCoreGameMode, /Script/Eidolon.EidolonCoreGameInstance and /Script/Eidolon.EidolonAssetManager in project settings. Configure primary asset scan type EidolonRegion with base class /Script/Eidolon.EidolonRegionDefinition and actual content directory. Verify packaged asset discovery.
3. Attach lock-on component to existing character; implement target interface on living enemies. Test controller input conflict and movement/camera feel.
4. Register native gameplay tags after adding GameplayTags dependency with approval. Proposed namespaces: State.Player, State.Combat, Event.Combat, World.Region. Do not use tags as a second persistent campaign store.
5. World team creates World Partition maps, runtime data layers and streaming sources; gate traversal through campaign authority, not cell load status. Stable IDs restore actors after streaming; no hard references across cells. Test travel failure and checkpoint recovery.
6. Replace runtime-generated mappings with authored Enhanced Input assets and player remapping; preserve current fallback until verified. Platform settings and device profiles require representative profiling and licensed console validation.

## Acceptance tests
Build Editor and Win64 against UE 5.6; run Eidolon.Core.Targeting.Score and Eidolon.Campaign suites. In PIE verify acquisition, occlusion, target death/destruction, range, unpossess, no idle tick, controller input, duplicate overlaps. Test async success, missing asset, cancel, shutdown, concurrent requests and packaged cook. Test region transitions and locked-region rejection. Profile Unreal Insights, stat unit, stat gpu, memory reports, streaming hitches on representative maps; provisional 60 fps target (16.67 ms frame) not a verified budget. No engine available in current session; all runtime checks pending.

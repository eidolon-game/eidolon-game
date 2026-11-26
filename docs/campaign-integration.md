# Campaign integration

Status: uncompiled runtime foundation. The 26 chapters are indexed, not completed levels.

## Integration contract
- Narrative triggers call SetFlag(chapter_N_resolved) only after required dialogue or exploration is finished.
- Enemy death emits typed objective events. Arena kills must not advance the campaign automatically.
- Floor completion emits ReachFloor in order, and final boss death emits the chapter objective.
- The campaign subsystem owns persistent flags, memory IDs and progression. Map actors should use stable IDs and restore state when streamed.
- Noah appears after chapter 2; chapter 3 is the investigation. Vareth disappears after chapter 10. Chapter 22 requires separate disappearance and recovery scene triggers before advancement.
- The secret ending requires independent side quest rewards lost_memories and eidolon_aid plus origin_truth.
- Save at chapter boundaries and checkpoints; report failures instead of pretending success.

## Production gates
Every chapter requires a graybox, objective triggers, dialogue, combat where appropriate, navigation, checkpoint recovery, controller support and a playtest. Final art requires licensed or original geometry, physically based materials, rigs, animations, LOD or Nanite strategy and performance profiling. No placeholder primitive qualifies as photorealistic final art.

## Validation
Run scripts/validate_source.py for structural checks. Once UE5.6 is available, compile and run the Eidolon.Campaign automation suite. Test all chapter transitions, save/load and endings before merging. The current arena is a separate combat test.

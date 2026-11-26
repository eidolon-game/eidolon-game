# Save integration proposal — cycle 02 (not applied to shared files)

## Activation sequence
1. Director/01: replace implicit Initialize fallback (LoadCampaign failure -> NewCampaign) with explicit slot-selection status, distinguishing missing, corrupt, unsupported and inaccessible. Never invoke NewCampaign automatically for an occupied slot. Explicit new game must use an empty slot. Preserve EidolonCampaign01; migration is read-only until a new destination has been accepted.
2. Director/06: CampaignSubsystem owns a UPROPERTY reference to the committed UEidolonRealitySave and exposes read-only snapshots. Decode CampaignBytes only after commit; do not expose mutable legacy State. Existing AdvanceChapter must stop calling ApplyDiscrepancy for chapters 2 and 10; it must require their already committed scene events. For 22 require separately committed disappearance and recovery. AdvanceChapter, RecordObjective, ReachFloor, ChooseEnding, SetFlag, reward and relationship writes must stage against the same envelope or be disabled. Do not run two authorities.
3. 01/08: map FEidolonGameplayEvent GUID, source/target, type, chapter, quest directly into FEidolonCampaignTransaction. Authority must validate source and ordered quest step before setting bQuestValidated. Quest state (cursor/count/completion and dedup) still needs a persisted typed schema and must be committed with the transaction; do not enable this adapter as a complete quest system until that is done. Use deterministic reward IDs (quest + reward definition) and immutable registry amounts; retry same event GUID; never derive a new GUID on retry. Reject mismatched payloads at ingress (adapter currently dedups by GUID only).
4. M2: checkpoint before dungeon encounter, after defeat/return, after dormitory, and after reveal; exact checkpoint IDs and world transforms to be agreed with 16/17/08. Death reloads last accepted snapshot, not NewCampaign. No event publication, AI update, animation or reward presentation before successful commit. On write/readback failure stop and re-read slot before retry; storage may have committed despite failure to verify. Single game-thread writer only; no cross-process concurrency guarantee.
5. 05/04: snapshot contains canonical presence, memory view, checkpoint ID, revision and generation; UI/AI must consume after commit only. Distinguish NPC memory from Kael archive and reality instance from entity identity. Projection of all NPC/region instances still incomplete.

## Canonical mapping (reviewed against Quest PR #23)
| Chapter | Quest event | Reality event |
|---|---|---|
|2|noah_reveal|noah_appear|
|10|vareth_erasure_witnessed|vareth_erase|
|20|oblivion_entered|oblivion_enter|
|21|alternate_family_met|alternate_family|
|22|noah_erasure_witnessed|noah_disappear|
|22|noah_found_in_oblivion|noah_recover|

## Persistence limits
Outer frame: LE magic 0x45494453, format 1, payload length, CRC32, UE SaveGame payload (schema 2). CRC is accidental corruption detection, not authentication. Future recognizable format/schema blocks load and writes to either copy. Unframed prior draft data is blocked, not silently overwritten. A readable truncated frame with recognized header can recover from its sibling; unreadable or unknown magic is deliberately blocked. Existing platform DoesSaveGameExist may conflate inaccessible/missing; verify on platform before shipping. No fsync guarantee. A complete exact readback is logical acceptance, not physical durability. A verification failure can leave a fully committed candidate on disk; caller must reconcile before retry. No automatic migration of old draft schema-2 files. Legacy schema-1 supported only with unambiguous chapters 1–19; contradictory evidence rejected. No changes to shared files, no automatic merge.

## Build/QA
Run portable policy test: g++ -std=c++17 -Wall -Wextra -Werror -pedantic -ISource/Eidolon/Save tests/save/checkpoint_policy_test.cpp -o /tmp/eidolon_checkpoint && /tmp/eidolon_checkpoint
Run UE suites Eidolon.Save.CheckpointStorage (disposable profile; slot 9 must be empty), Eidolon.Save.TransactionRollback, Eidolon.Save.LegacyMigration and Eidolon.Discrepancy.CanonicalTransitions in UE 5.6. Validate corruption/permissions and interrupted writes on actual Win64 storage, not only simulated copies. M2 gate requires First Descent and defeat recovery playtest.

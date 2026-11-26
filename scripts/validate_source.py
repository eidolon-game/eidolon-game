import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
project = json.loads((root / "Eidolon.uproject").read_text(encoding="utf-8"))
assert any(m["Name"] == "Eidolon" for m in project["Modules"])
assert any(p["Name"] == "EnhancedInput" and p["Enabled"] for p in project["Plugins"])
for name in ("EidolonCharacter", "EidolonGameMode", "EidolonArenaEnemy", "EidolonArenaHUD", "EidolonCampaignSubsystem"):
    header = root / "Source/Eidolon" / (name + ".h")
    source = header.with_suffix(".cpp")
    assert header.is_file() and source.is_file(), name
    assert name + ".generated.h" in header.read_text(encoding="utf-8"), name
assert "EnhancedInput" in (root / "Source/Eidolon/Eidolon.Build.cs").read_text()
assert "GlobalDefaultGameMode=/Script/Eidolon.EidolonGameMode" in (root / "Config/DefaultEngine.ini").read_text()
print("Manifest and source layout validated. This is not a C++ build or playtest.")

campaign = json.loads((root / "Content/Eidolon/Data/campaign.json").read_text(encoding="utf-8"))
chapters = campaign["chapters"]
assert [c["id"] for c in chapters] == list(range(1, 27))
assert sorted(set(c["act"] for c in chapters)) == list(range(1, 11))
assert all(c["target"] >= 0 and c["objective"] for c in chapters)
assert {e["id"] for e in campaign["endings"]} == {"custodian", "origin", "guild", "freedom"}
assert chapters[6]["target"] == 100
print("Campaign manifest validated; runtime not tested.")

assert all(len(c["dialogue"]) >= 2 and all(d["speaker"] and d["text"] for d in c["dialogue"]) for c in chapters)
assert all(e["epilogue"] for e in campaign["endings"])
assert len(campaign["endings"]) == 4
print("All 26 chapter dialogue records and four epilogues present.")

for chapter in chapters:
    for key in ("title_en", "region_en", "objective_it"):
        assert chapter[key].strip(), (chapter["id"], key)
    for dialogue in chapter["dialogue"]:
        assert dialogue["speaker_en"].strip() and dialogue["text_en"].strip(), chapter["id"]
for ending in campaign["endings"]:
    assert ending["title_en"].strip() and ending["epilogue_en"].strip(), ending["id"]
print("Italian and English campaign text validated for all chapters and endings.")

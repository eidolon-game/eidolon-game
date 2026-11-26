import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
project = json.loads((root / "Eidolon.uproject").read_text(encoding="utf-8"))
assert any(m["Name"] == "Eidolon" for m in project["Modules"])
assert any(p["Name"] == "EnhancedInput" and p["Enabled"] for p in project["Plugins"])
for name in ("EidolonCharacter", "EidolonGameMode", "EidolonArenaEnemy", "EidolonArenaHUD"):
    header = root / "Source/Eidolon" / (name + ".h")
    source = header.with_suffix(".cpp")
    assert header.is_file() and source.is_file(), name
    assert name + ".generated.h" in header.read_text(encoding="utf-8"), name
assert "EnhancedInput" in (root / "Source/Eidolon/Eidolon.Build.cs").read_text()
assert "GlobalDefaultGameMode=/Script/Eidolon.EidolonGameMode" in (root / "Config/DefaultEngine.ini").read_text()
print("Manifest and source layout validated. This is not a C++ build or playtest.")

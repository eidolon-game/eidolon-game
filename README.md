# EIDOLON – Beyond the Last Wall

Unreal Engine 5 C++ dark fantasy RPG. Arena prototype: source only until a successful UE build is recorded.

## Windows first playtest
1. Install Unreal Engine 5.6 (latest available 5.6 hotfix) through Epic Games Launcher and Visual Studio 2022 with Game development with C++, MSVC and Windows SDK.
2. Download this branch as ZIP and extract to a short writable directory.
3. Open PowerShell in the extracted project and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Package-Windows.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.6" -Launch
```

The script builds the editor, generates the map and packages a Development Win64 build under Saved/Distribution/Windows. Use Eidolon.exe there after successful packaging. The whole Windows folder is required, not only the exe.

Controls: WASD / left stick; mouse / right stick; Space / A jump; Shift / L3 sprint; left mouse / RB light attack; right mouse / RT heavy attack; Q / B dodge; F / LB block; R / Y restart.

GitHub Actions validates source and checks for UE. Hosted Windows runner has no Unreal installation; no executable has been produced yet. Use a private build repository with an isolated UE-equipped runner for unattended packaging; never attach a persistent runner to the public project. No console SDKs or credentials in public Git.

This is a combat sandbox, not the finished game. Build and runtime verification remain required.

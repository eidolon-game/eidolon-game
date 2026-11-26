# Windows campaign prototype packaging

On a Windows x64 host with an authorized Unreal Engine 5.6 installation and
the required MSVC and Windows SDK, use PowerShell 7 from a fresh game checkout:

```powershell
pwsh -NoProfile -File scripts/Package-Windows.ps1 -EngineRoot "$env:UE_ROOT" -Campaign
```

The script builds the editor target, runs the authored map generators, verifies
the new `Saved/EidolonAuthoredMaps.json` manifest and every listed map package,
then passes those packages explicitly to Unreal Automation Tool for cooking.
Use a fresh checkout for retries after map generation. The script refuses
existing authored map packages, manifests and campaign prototype output so
earlier builds cannot supply stale files.

Keep the complete `Saved/DistributionCampaignPrototype/Windows` directory
together. Launch it with `Launch-Campaign-Prototype.bat` or add `-Launch` to
the packaging command. Both pass Valdris Prologue as the initial map and
`-EidolonAuthoredCampaign` to use the campaign prototype save slot.

Running the script without `-Campaign` uses the Arena build and its existing
`Saved/Distribution/Windows` output. The campaign prototype contains the
authored grayboxes listed in the manifest. Verify boot, traversal, save/load
and keyboard/controller play on the packaged build before recording a passed
runtime gate.

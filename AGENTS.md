# EIDOLON game

- Work from the official `integration/studio` baseline on a dedicated branch. Keep `main`, existing commits and other departments' work intact. Assign exclusive ownership of shared files before editing them in parallel.
- Read `docs/STUDIO-INTEGRATION.md`, the coordination documents in `eidolon-docs/production/`, and `eidolon-docs/narrative/story.md` before changing gameplay that affects the campaign. Follow the recorded contract and surface unresolved canon decisions to the director.
- Check ancestry and equivalent patches before importing work. Historical PRs #4–#6 are already represented in the experimental baseline. Integrate only reviewed contributions.
- Run `python3 scripts/validate_source.py` from this repository. It checks structure and campaign records. Record actual Unreal automation, packaging, launch and playtest outcomes separately.
- Target Unreal Engine 5.6. Follow the release criteria in the documentation repository for Win64 and authorized console builds. Record engine version, commit and logs or artifacts for every runtime claim.
- Use the assigned paths in `production/department-ownership.md` and record department handoffs in `docs/handoff/NN.md`.

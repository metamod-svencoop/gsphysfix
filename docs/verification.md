# Verification

Recorded on 2026-10-25 against GoldSrc_VibeSignatures release v20261025a (PR #406 symbols).

## Automated checks

| Target | Debug | Release |
| --- | --- | --- |
| Windows Win32 standalone build | Passed | Passed |
| Windows behavior test | Passed | Passed |
| Linux i386 standalone build (WSL Ubuntu 24.04) | Passed | Passed |
| Linux behavior test | Passed | Passed |
| Windows Win32 aggregate build and installation | — | Passed |

- Both manifests synchronized online and passed coverage validation: 11 Windows and 4 Linux platform inputs. Pruned snapshots keep `SV_CopyEdictToPhysent` for every Windows build and Half-Life 8684 Linux, and `SV_AddLinksToPM_`, `pmove` and `build_number` for Half-Life 10210, Sven Co-op 8948 (mangled walker name) and 10257 Linux. Their RVAs match the upstream validation record.
- `playermove_t`, `playermove_10152_t` and `physent_t` compile-time layout checks passed with MSVC and GCC i386.
- The Release DLL and shared object export `GiveFnptrsToDll`, `GetEntityAPI2`, `Meta_Query`, `Meta_Attach` and `Meta_Detach`.
- The format check passed with clang-format 23.1.3.

## Live-game evidence

Windows Sven Co-op 10257 listen server with this Metamod, FallGuys, ASExt and GSPhysFix:

- Metamod loaded the gamedata catalog. `meta list` reported GSPhysFix RUN, and `metamod.log` recorded `Hooking SV_CopyEdictToPhysent`.
- Test map: a copy of `test123` whose map-script copy changes `env_physicmodel` and `env_physic_vehicle` from `SOLID_BBOX` to `SOLID_SLIDEBOX`. The buggy model `models/test123/buggy.mdl` has `$flags 512`. Two test commands in that script copy placed the player above the buggy and reported the server player state.
- `sv_gsphysfix 1`, two runs: the server player landed on the buggy body and stayed there, at z 93.29 and 92.67 with `FL_ONGROUND` and ground entity `env_physic_vehicle`. The feet were below the entity's bounding-box top, so the server traced the studio hitboxes, as client prediction does.
- `sv_gsphysfix 0`, three runs: the server player never stayed on the buggy body. It was airborne at z 94.31, the height where the fixed server reports ground, and was then pushed onto a wheel or the floor. This is the server-airborne/client-grounded mismatch from MetaHookSv#931.

## Limits

- The original `test123` uses `SOLID_BBOX` for its vehicles, which the engine already traces by hitbox. The behavior difference only appears on the `SOLID_SLIDEBOX` copy.
- The `SV_AddLinksToPM_` path (Half-Life 10210, Sven Co-op 8948 and 10257 Linux) is covered by behavior tests and static gamedata checks only. The Linux game could not be run in WSL because its 32-bit runtime dependencies are missing.
- No other engine build, BLOB engine, dedicated server, multiplayer network correction or AngelScript-free configuration was run in a game. None of these is recorded as a live-game pass.

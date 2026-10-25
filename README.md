# GSPhysFix

A standalone [metamod-svencoop](https://github.com/metamod-svencoop/metamod-svencoop) plugin that fixes server-side player movement collision against `SOLID_SLIDEBOX` studio models flagged for hitbox traces. AngelScript and ASExt are not required.

## Problem

GoldSrc `SV_CopyEdictToPhysent` clears `physent_t::studiomodel` and only restores it for `SOLID_BBOX` entities whose model has `FMODEL_TRACE_HITBOX` (studio `$flags 512`). `SOLID_SLIDEBOX` entities, which include most monsters, fall through to the bounding box branch. Client prediction keeps the hitboxes for the same entities, so the server and client disagree about where a player stands on or touches such a model: prediction errors, view jitter and a client that is grounded while the server is airborne. See [MetaHookSv#931](https://github.com/MetaHookSv/MetaHookSv/issues/931) and the source fix in [HLND2T-DiligentGraphics#108](https://github.com/HLND2T/HLND2T-DiligentGraphics/pull/108).

The plugin applies the same rule as the source fix: a server physent converted from a `SOLID_SLIDEBOX` entity gets `studiomodel` when the entity's `modelindex` names a non-null studio model with `FMODEL_TRACE_HITBOX`. Entities without the flag, without a model or with a non-studio model keep their bounding box. No other physent field, the entity name and bounds, lag compensation, list capacity handling and client prediction are left unchanged.

## Hook points

| Engine | Hook |
| --- | --- |
| Builds with a called `SV_CopyEdictToPhysent` (all Windows builds, Half-Life 8684 Linux) | Calls the original conversion, then sets `studiomodel` |
| Builds that inline the conversion into `SV_AddLinksToPM_` (Half-Life 10210, Sven Co-op 8948 and 10257 Linux) | Calls the original area walker, then sets `studiomodel` on the visents and physents the outermost call appended |

The walker hook is selected when the catalog provides both `SV_AddLinksToPM_` and `pmove`, which GoldSrc_VibeSignatures publishes only for the inlined builds. The walker never reads `studiomodel` while copying visents into physents, so fixing both lists after the walk equals converting them with the rule applied. `playermove_10152_t` is used on Sven Co-op build 10152 and later; Half-Life and earlier Sven Co-op builds use the original `playermove_t`. Both layouts are checked at compile time against offsets verified on the engine binaries.

## Configuration

| Cvar | Default | Effect when nonzero |
| --- | --- | --- |
| `sv_gsphysfix` | `1` | Apply the hitbox rule to server physents |

Changes take effect on the next player movement command.

## Installation

Install the complete `addons/` payload under the game's mod directory, including **both** gamedata directories `gsphysfix_windows/` and `gsphysfix_linux/`. Add the appropriate line to `addons/metamod/plugins.ini`:

```ini
win32 addons/metamod/dlls/gsphysfix.dll
linux addons/metamod/dlls/gsphysfix.so
```

Use this project's Metamod interface 5:17 or newer compatible 5.x. Old BLOB engines additionally require the Metamod revision pinned in `cmake/Revisions.cmake` or a later revision containing BLOB source registration. Metamod loads gamedata at startup, so restart after installing new data. Runtime loading is supported when the required catalog was loaded at startup. Runtime unloading is disabled.

## Supported inputs

| Engine build | Windows Win32 | Linux i386 |
| --- | --- | --- |
| Half-Life 3248, 3266, 3329, 3647 (BLOB) | Yes | — |
| Half-Life 4554, 6153 | Yes | — |
| Half-Life 8684, 10210 | Yes | Yes |
| Sven Co-op 8948, 10257 | Yes | Yes |
| Cry of Fear 5936 | Yes | — |

Support is keyed by the exact engine file CRC from the published GoldSrc_VibeSignatures catalog, starting with release v20261025a. Unrecognized, modified or unresolved engines fail to load with a diagnostic. Counter-Strike and other mods use the Half-Life engine builds above.

## Build and test

CMake 3.21+, Git and MSVC Win32 or Linux multilib are required. Native builds download and validate gamedata with Python 3.8+.

```sh
cmake -S . -B build/x86/Release -A Win32
cmake --build build/x86/Release --config Release --parallel 4
ctest --test-dir build/x86/Release -C Release --output-on-failure
cmake --install build/x86/Release --config Release
```

On Linux omit `-A Win32` and add `-DCMAKE_BUILD_TYPE=Release`. Configure Debug in a separate `build/x86/Debug` tree. `METAMOD_SOURCE_PATH` and `FORMAT_VALIDATION_SOURCE_PATH` accept local dependency trees; otherwise pinned revisions are fetched. Default installation is `install/x86/<Config>/addons/`.

Tests cover the solid/model/flag rule, that only `studiomodel` changes, range and capacity clamping, and that only the outermost walker call fixes the lists on both playermove layouts. They do not substitute for movement tests on each engine build.

See [verification results and live-game limits](docs/verification.md).

Formatting uses clang-format 23.1.3:

```sh
cmake -S . -B build/format -DFORMAT_VALIDATION_ONLY=ON
cmake --build build/format --target gsphysfix-format-check
```

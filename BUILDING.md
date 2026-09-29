# Building

## Normal development: let GitHub Actions build it

The normal development path does not require maintaining MO2 SDK paths locally.

Push a branch or open a pull request. The `Build XnGine` workflow builds two targets in parallel:

- **retail** — the newest stable Mod Organizer 2 release, using that release's published uibase SDK and matching `mob` toolchain metadata.
- **dev** — the current Mod Organizer 2 development branch (`master`) and current uibase build.

The workflow discovers the upstream versions automatically, installs the matching Qt version, builds all four XnGine game plugins, verifies that all expected DLLs were produced, records the exact upstream commits used, and uploads the resulting plugins as workflow artifacts.

The workflow also runs weekly from the default branch. This is intentional: an upstream MO2/uibase change can be detected even when XnGine itself has not changed.

You should not need to update XnGine merely because the current MO2 retail or development version changes. If MO2 changes its release layout or build contract, the workflow is designed to fail explicitly instead of silently building against a guessed dependency version.

### CI build shards

Retail and dev are separate jobs and run concurrently. This is the first level of sharding because they use different dependency sets.

The retail job downloads the released uibase SDK instead of rebuilding it. The dev job builds uibase only when the cache for the current uibase/cmake-common/Qt combination is missing. XnGine itself is built with Ninja parallelism inside each job.

Automated parser and integration tests should be added as separate shards as the test suite grows. They should consume a prepared build/dependency state rather than independently rebuilding MO2 dependencies in every shard.

## Local build (only when you need local runtime testing)

Local building is still supported for testing inside an installed Mod Organizer 2 instance. Machine-specific paths live only in ignored local environment files.

### Prerequisites

- Windows 10 or Windows 11 (x64)
- Visual Studio 2022 Build Tools with the C++ workload
- CMake 3.16 or newer
- Ninja
- A Qt version matching the MO2 target you are testing
- vcpkg
- Mod Organizer 2 uibase and Mod Organizer 2 source tree

### One-time local setup

1. Copy `config/local.env.example.bat` to `config/local.env.bat`.
2. Fill in the local paths for the MO2 installation(s) you actually use for runtime testing.
3. Do not edit shared build scripts for personal paths.

`config/local.env.bat` is ignored by git.

### Build

From repository root:

```bat
build_ms.bat
```

`build_ms.bat` reads `config/local.env.bat` automatically if it exists.

### Dual-target local setup

You can keep one workspace configured for both a retail MO2 SDK and a newer dev SDK.

- Put shared/default values in the unsuffixed variables.
- Put retail-specific values in `*_RETAIL`.
- Put dev-specific values in `*_DEV`.
- Set `MO2_TARGET=retail` or `MO2_TARGET=dev`, or use the helper scripts below.

Helper scripts:

```bat
build_ms_retail.bat
build_ms_dev.bat
build_and_deploy_retail.bat
build_and_deploy_dev.bat
```

The active target overrides:

- `MO2_UIBASE_PATH`
- `MO2_UIBASE_LIB`
- `MO2_SRC_PATH`
- `MO2_PLUGINS_DIR`
- `MO2_INSTALL_DIR`
- `QT_ROOT`

## Optional local commands

Install/refresh vcpkg dependencies:

```bat
run_vcpkg_install.bat
```

Build with CMake presets:

```bat
cmake --preset default
cmake --build --preset default
```

## Advanced: Daggerfall build profiles

The Daggerfall plugin supports keep/prune profiles via CMake options.

- `daggerfall-runtime`: core runtime only (no toolkit, no EXE patching)
- `daggerfall-toolkit`: runtime + toolkit (default safe advanced profile)
- `daggerfall-full`: toolkit + EXE patching (explicit legacy/unsafe profile)

Use these presets:

```bat
cmake --preset daggerfall-runtime
cmake --build --preset daggerfall-runtime
```

```bat
cmake --preset daggerfall-toolkit
cmake --build --preset daggerfall-toolkit
```

```bat
cmake --preset daggerfall-full
cmake --build --preset daggerfall-full
```

## Notes

- CI is the authoritative compatibility build for current retail and dev MO2 targets.
- Local builds are for interactive/runtime testing and debugging.
- Keep personal paths in `config/local.env.bat` only.
- Do not commit machine-specific values.

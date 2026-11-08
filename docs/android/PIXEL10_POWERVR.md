# Pixel 10 Pro XL / PowerVR experimental GTA IV compatibility branch

Branch: `fix/pixel10-powervr-vulkan`

**Status: source changes applied; NOT yet compiled or tested on a Pixel 10 Pro XL.**
This is an experiment, not a supported or verified Android port for PowerVR.

## Changes carried in this branch

- Upstream proposal [#123](https://github.com/vaduur/LibertyRecompAndroid/pull/123): per-shader SPIR-V vertex input location compaction for Vulkan devices with only 16 vertex input attribute slots. Adds unit tests and the `gta4_native_force_vertex_location_compaction` diagnostic override.
- Upstream proposal [#124](https://github.com/vaduur/LibertyRecompAndroid/pull/124): replace overlapping XEX delta-patch copies with `memmove` to avoid undefined behaviour and intermittent ARM64 crashes.
- Upstream proposal [#125](https://github.com/vaduur/LibertyRecompAndroid/pull/125): descriptive Vulkan feature failures and a `lenient` feature policy, which permits renderer-defined fallbacks for `depthClamp` and `independentBlend`.

The code still needs working Vulkan 1.2, `shaderInt64` and `sampleRateShading`. These requirements are **not** bypassed. If your system driver does not expose them, a new shader/renderer strategy is needed. The relevant upstream PRs have not been hardware-verified on this Pixel.

A minimal checker was added at `glue/rexglue-sdk-main/gta4-recomp/tools/check_hack_registry.py` because the existing Android build script invokes that path but the published source did not include the file. It validates registry-entry uniqueness and the two PowerVR-related settings only; it intentionally does not claim to reproduce the missing comprehensive checker.

## Build on Windows 11

Prerequisites from upstream [BUILDING.md](BUILDING.md): Git Bash, Python 3.12, CMake 3.29+, Ninja, Android Studio (SDK platform 35 and JDK 17+), **Android NDK 29.0.14206865**.

Run in Git Bash:

```bash
git clone --branch fix/pixel10-powervr-vulkan --single-branch https://github.com/donlums/LibertyRecompAndroid.git
cd LibertyRecompAndroid
python tools/setup_repo.py
python glue/rexglue-sdk-main/gta4-recomp/tools/check_hack_registry.py
bash os/android/scripts/build_driver_proxy.sh
STRIP=1 bash os/android/scripts/build_native.sh 2
bash os/android/scripts/build_apk.sh
```

Expected APK *if build succeeds*: `os/android/app/build/outputs/apk/debug/app-debug.apk`.
This branch has not yet produced or verified that APK.

## First Pixel test (if an APK is successfully built)

1. Back up any game saves before installing an APK with a different debug signing key. Android may require uninstalling the old build.
2. In the launcher choose **System Vulkan driver**. **Do not choose bundled Turnip** for a PowerVR GPU: Turnip is an Adreno driver.
3. Add the following lines to `args.txt` under `Android/data/com.libertyrecomp/files/` (or push a file using `os/android/scripts/push_args.sh`):

```text
--diagnostics=true
--diagnostics_categories=logging
--log_level=info
--gta4_native_device_features=lenient
```

4. Start the game; record the exact launcher error if Vulkan setup fails.
5. Capture logs with `bash os/android/scripts/logs.sh --dump` while the device is attached over USB with Android debugging enabled. Capture one run only; avoid uploading dumps containing personal data or copyrighted game files.
6. If the game runs but renders black, look for `vertex-input-location-limit`, shader compilation, pipeline creation and device-feature rejection messages. The location compaction patch should remove only that specific 16-slot limitation; it cannot guarantee all frames render correctly.

## Required game data

Builds do not include GTA IV. Game installation requires your own legally obtained Xbox 360 GTA IV image and the required title update, following upstream [INSTALL.md](INSTALL.md).

## Test gates

- [x] Feature branch exists and source changes are committed.
- [x] PR #123, #124, #125 changes applied to the fork.
- [ ] Native Android build and APK packaging completed.
- [ ] GPU feature enumeration on Pixel 10 Pro XL checked.
- [ ] Initial launch / first rendered frame confirmed.
- [ ] Gameplay, stability, and FPS tested on-device.

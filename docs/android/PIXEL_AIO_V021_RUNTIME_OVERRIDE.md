# Pixel AIO v0.21 runtime override — confirmed APK bug

This note documents a **binary-only regression in the custom Pixel AIO APK**; it is not present verbatim in the older source in this repository, and this branch is still **not** a drop-in native APK build.

## Evidence

The 23 November 2026 benchmark for the working `com.libertyrecomp.aio` v0.20 app contains `--gta4_dynamic_draw_distance_min=0.45` in `args.txt`. Startup logs show this original value being superseded by other launcher-injected flags, including `--gta4_dynamic_draw_distance_min=0.55`.

In the shipping `classes2.dex`, `LibertyActivity.prepareEnvironment` contains a hard-coded `const-string` referencing `--gta4_dynamic_draw_distance_min=0.55`. Thus fixing the saved file alone is insufficient. Fix the **final effective arguments**, not just the user's saved file.

## v0.21 APK hotfix

The working v0.20 APK was repackaged with two narrowly targeted DEX instruction-reference updates:

- The `prepareEnvironment` min-distance `const-string` now references the already-existing `0.45` string ID.
- The ordinary `PixelReport.configure` log-level `const-string` now references the already-existing `--log_level=warn`; the separate `Play (Debug Log)` path retains `--log_level=info`.

No string IDs or DEX string-data records were added/reordered, so strict string-table ordering and class resolution remain intact. APK signing and ZIP alignment were revalidated; native `.so` files, resources, assets, manifest, and all other DEX files remain identical. This is a **configuration/diagnostic hotfix**, not a native Vulkan performance rewrite. Actual on-device launch/FPS testing is required.

## Source-level acceptance criteria

1. Keep user-configured `--gta4_dynamic_draw_distance_min` unless a selected profile deliberately overrides it.
2. Apply profile defaults at one well-defined point; canonicalise duplicate `--name=value` keys using explicit last-wins semantics.
3. Write `pixel-effective-args.txt` to show **exactly** what the engine receives, excluding private paths/keys.
4. Default to warning-level logs for normal play, but support opt-in information-level benchmark capture.
5. Guard `diagnostics_categories` and `diagnostics` as a valid pair, to prevent startup failures.
6. Preserve the Pixel AIO renderer binaries or reconcile exact matching source before replacing them.
7. Validate DEX class loadability in addition to sorted/unique string entries; previous v0.17 failed to load `LauncherActivity`.
8. Keep signing keys in private secrets, never in source control.

The opt-in `gta4_profile_title_handoff` native source probe in this PR measures **producer-side submission cadence only**. It does **not** establish GPU execution time or display-present timestamps.

## Follow-on

After exact native source reconciliation: measure CPU command submission, Vulkan queue submit/fence/acquire/present timestamps, and independent depth/shadow vs main-pass draw-call counts under matched gameplay. Treat persistent ~15 FPS as unproven until these independent timings establish the bottleneck.

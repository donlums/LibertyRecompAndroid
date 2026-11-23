# GTA IV Pixel — foreground-aware benchmark sampling

## Confirmed regression (23 November 2026)
The Pixel AIO benchmark sampler (`PixelThermal.run()`, present in the shipping
`classes2.dex` but absent from this older GitHub source) continues writing a row
every 2 seconds after Android sends `surfaceDestroyed()` and `onPause()` to
the actual `:game` process. In the recorded 181-second benchmark, this occurred
around t=100 s and produced artificial ~2.4 FPS background samples.

## Implementation in this branch
- `LibertyActivity` in the **actual game process** records a 2-second activity
  heartbeat while resumed and window-focused. It writes background state
  immediately when paused/destroyed/focus lost.
- `PixelForegroundState` uses `SystemClock.elapsedRealtime()` across Android
  processes, atomically replaces its status file, and treats unknown or >5-second
  stale states as **unknown**, never as confirmed gameplay.
- `PixelBenchmarkSamplePolicy` explicitly classifies GAMEPLAY,
  PAUSED_OR_BACKGROUND, UNKNOWN. Real low-FPS values remain valid if game visible.
- `TestForegroundBenchmark` covers foreground, background, malformed and stale
  markers, and valid 15 FPS gameplay (host Java test passes).

## Required integration with Pixel AIO sampler (BLOCKER)
The installed Pixel AIO `PixelThermal` class is not in this GitHub source
snapshot. When its matching Java source is restored, modify the sampler:
```java
var state = PixelForegroundState.read(appExternalFiles,
        android.os.SystemClock.elapsedRealtime());
var classification = PixelBenchmarkSamplePolicy.classify(state, measuredFps);
```
For each sample, **retain raw thermal readings**, add a `sample_class` column,
and compute gameplay FPS statistics from `GAMEPLAY` samples only. Never use
display refresh rate as proof of game activity; it can vary independently.
Keep paused and unknown rows for debugging and avoid treating a zero or low FPS
alone as proof of background state.

Test split-process visibility and on-screen launch on device.

## Current APK / native renderer safety
These source changes **have not** been shipped in an installable APK. The older
GitHub project does not contain the exact matching native Vulkan libraries and
Pixel launcher code from the working v0.21 APK. Don't rebuild and replace them
until those sources are reconciled. Do not uninstall the currently working app.

Native frame submission instrumentation remains separate in PR #2.

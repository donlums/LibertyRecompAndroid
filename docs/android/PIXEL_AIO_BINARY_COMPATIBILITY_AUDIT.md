# AIO compatibility audit — 27 November 2026

The original signed Pixel AIO v0.21 APK was inspected locally without uploading its contents to GitHub. SHA-256 matches the recorded release: 638335f20fcafee8f672d4aee00a2dca6056e0a794a04bd69efdf89f82eddc32.

Original AIO: com.libertyrecomp.aio. Source-built experiment: com.libertyrecomp. Original has 13 ARM64 native libraries, including libmain_usa.so and libpixeladaptive.so, absent in the experimental APK. All 11 shared native filenames have different hashes; classes2.dex and manifest differ.

The experimental librexruntime.so removes 199 exported names relative to AIO. Both original librexgpu-gta4-native.so and librexgpu-xenos.so import the removed rex::ui::vulkan::util::LowerUnsupportedInt64 symbol. Original libmain.so differs by tens of thousands of exports. Partial library replacement is unsafe; matching dynamic symbols alone would not prove C++ ABI compatibility.

Do not overwrite the original AIO launcher or native libraries or re-sign it with a debug key. Preserve original package, data and release signing identity. Exact AIO source reconciliation remains a prerequisite for a native AIO update.

This branch keeps existing PowerVR vertex-location compaction, Vulkan feature handling and XEX overlap fixes, and imports the CI-proven system-driver-only proxy and compiler changes. Its APK is a separate experimental compatibility candidate, not an AIO upgrade. First rendered frame, image correctness, hardware compatibility and FPS remain unverified.

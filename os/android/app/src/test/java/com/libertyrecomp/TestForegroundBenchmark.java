package com.libertyrecomp;

import java.nio.file.Files;
import java.nio.file.Path;

// javac -d /tmp/pixeltest os/android/app/src/main/java/com/libertyrecomp/{PixelForegroundState,PixelBenchmarkSamplePolicy}.java os/android/app/src/test/java/com/libertyrecomp/TestForegroundBenchmark.java
// java -cp /tmp/pixeltest com.libertyrecomp.TestForegroundBenchmark
public final class TestForegroundBenchmark {
    public static void main(String[] args) throws Exception {
        Path dir = Files.createTempDirectory("pixel-foreground-test");
        long t = 1_000_000L;
        check(dir, t, PixelForegroundState.State.UNKNOWN);
        PixelForegroundState.record(dir.toFile(), true, t);
        check(dir, t+2_000, PixelForegroundState.State.FOREGROUND);
        check(dir, t+5_001, PixelForegroundState.State.UNKNOWN);
        PixelForegroundState.record(dir.toFile(), false, t+5_100);
        check(dir, t+5_400, PixelForegroundState.State.BACKGROUND);
        if (PixelBenchmarkSamplePolicy.classify(PixelForegroundState.State.BACKGROUND, 2.4f)
                != PixelBenchmarkSamplePolicy.Classification.PAUSED_OR_BACKGROUND)
            throw new AssertionError("background 2FPS counted as gameplay");
        if (PixelBenchmarkSamplePolicy.classify(PixelForegroundState.State.FOREGROUND, 15.0f)
                != PixelBenchmarkSamplePolicy.Classification.GAMEPLAY)
            throw new AssertionError("real 15FPS lost");
        if (PixelBenchmarkSamplePolicy.classify(PixelForegroundState.State.UNKNOWN, 30.0f)
                != PixelBenchmarkSamplePolicy.Classification.UNKNOWN)
            throw new AssertionError("unknown activity state was treated as gameplay");
        Files.writeString(dir.resolve(PixelForegroundState.FILE), "malformed");
        check(dir, t+5_500, PixelForegroundState.State.UNKNOWN);
        System.out.println("PASS: foreground, background, stale, malformed and real low FPS");
    }
    private static void check(Path dir, long t, PixelForegroundState.State want) {
        var got = PixelForegroundState.read(dir.toFile(), t);
        if (got != want) throw new AssertionError(got + " != " + want);
    }
}

package com.libertyrecomp;

/** Never treat paused/background FPS readings as gameplay; retain raw thermal samples. */
public final class PixelBenchmarkSamplePolicy {
    private PixelBenchmarkSamplePolicy() {}

    public enum Classification { GAMEPLAY, PAUSED_OR_BACKGROUND, UNKNOWN }

    public static Classification classify(PixelForegroundState.State gameState, float titleFps) {
        if (gameState == PixelForegroundState.State.BACKGROUND) {
            return Classification.PAUSED_OR_BACKGROUND;
        }
        if (gameState != PixelForegroundState.State.FOREGROUND
                || Float.isNaN(titleFps) || Float.isInfinite(titleFps) || titleFps < 0) {
            return Classification.UNKNOWN;
        }
        // Real 15 FPS or even 2 FPS is still gameplay if the game is foreground.
        return Classification.GAMEPLAY;
    }
}

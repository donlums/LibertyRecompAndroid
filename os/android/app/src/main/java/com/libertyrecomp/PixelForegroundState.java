package com.libertyrecomp;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/** :game process foreground-state heartbeat, shared with launcher report sampler. */
public final class PixelForegroundState {
    public static final String FILE = "pixel-game-foreground.txt";
    public static final long FRESHNESS_MS = 5_000L;

    private PixelForegroundState() {}
    public enum State { FOREGROUND, BACKGROUND, UNKNOWN }

    public static void record(File appFiles, boolean foreground, long elapsedRealtimeMs)
            throws IOException {
        if (appFiles == null || !appFiles.isDirectory()) {
            throw new IOException("External app files directory unavailable");
        }
        Path target = new File(appFiles, FILE).toPath();
        Path temp = new File(appFiles, FILE + ".tmp").toPath();
        Files.writeString(temp, "v1," + (foreground ? "foreground" : "background")
                + "," + elapsedRealtimeMs + "\n", StandardCharsets.US_ASCII);
        try {
            Files.move(temp, target, StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING);
        } catch (java.nio.file.AtomicMoveNotSupportedException e) {
            Files.move(temp, target, StandardCopyOption.REPLACE_EXISTING);
        }
    }

    /** Uses SystemClock.elapsedRealtime() in both processes; stale markers fail closed. */
    public static State read(File appFiles, long elapsedRealtimeMs) {
        if (appFiles == null) return State.UNKNOWN;
        try {
            String content = Files.readString(new File(appFiles, FILE).toPath(),
                    StandardCharsets.US_ASCII).trim();
            String[] columns = content.split(",", -1);
            if (columns.length != 3 || !"v1".equals(columns[0])) return State.UNKNOWN;
            long updated = Long.parseLong(columns[2]);
            if (updated < 0 || elapsedRealtimeMs < updated
                    || elapsedRealtimeMs - updated > FRESHNESS_MS) return State.UNKNOWN;
            if ("foreground".equals(columns[1])) return State.FOREGROUND;
            if ("background".equals(columns[1])) return State.BACKGROUND;
            return State.UNKNOWN;
        } catch (IOException | NumberFormatException e) {
            return State.UNKNOWN;
        }
    }
}

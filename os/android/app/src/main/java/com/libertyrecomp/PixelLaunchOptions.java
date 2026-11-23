package com.libertyrecomp;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Canonical runtime flags and a narrowly-scoped, reversible Pixel profile migration. */
public final class PixelLaunchOptions {
    private PixelLaunchOptions() {}

    private static String key(String line) {
        if (!line.startsWith("--")) return null;
        int equal = line.indexOf('=');
        return equal > 2 ? line.substring(0, equal) : null;
    }

    /** Last value wins, matching the native command-line cvar parser. */
    public static List<String> canonicalize(List<String> input) {
        Map<String, Integer> lastIndex = new HashMap<>();
        for (int i = 0; i < input.size(); i++) {
            String k = key(input.get(i).trim());
            if (k != null) lastIndex.put(k, i);
        }
        List<String> output = new ArrayList<>(input.size());
        for (int i = 0; i < input.size(); i++) {
            String argument = input.get(i).trim();
            String k = key(argument);
            if (k == null || lastIndex.get(k) == i) output.add(argument);
        }
        return output;
    }

    /**
     * Upgrade the exact known v0.18/v0.19 Pixel profile once. Never change
     * hand-edited or unrelated profiles, and keep the original args.txt backup.
     * @return true when a matching old profile was migrated.
     */
    public static boolean migrateKnownPixelProfile(File argsFile) throws IOException {
        if (!argsFile.isFile()) return false;
        Path source = argsFile.toPath();
        Path marker = source.resolveSibling(".pixel_v020_profile_migrated");
        if (Files.exists(marker)) return false;
        List<String> lines = Files.readAllLines(source, StandardCharsets.UTF_8);
        if (!lines.contains("--gta4_dynamic_draw_distance_min=0.55") ||
            !lines.contains("--gta4_dynamic_draw_distance=true") ||
            !lines.contains("--gta4_shadow_cascade_count=2") ||
            !lines.contains("--gta4_output_height_cap=540")) return false;
        Path backup = source.resolveSibling("args.pixel-v019-backup.txt");
        if (!Files.exists(backup)) Files.copy(source, backup);
        List<String> changed = new ArrayList<>(lines.size());
        for (String line : lines) {
            changed.add(line.trim().equals("--gta4_dynamic_draw_distance_min=0.55")
                    ? "--gta4_dynamic_draw_distance_min=0.45" : line);
        }
        Path temp = source.resolveSibling("args.pixel-v020.tmp");
        Files.write(temp, changed, StandardCharsets.UTF_8);
        try {
            Files.move(temp, source, StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING);
        } catch (java.nio.file.AtomicMoveNotSupportedException e) {
            Files.move(temp, source, StandardCopyOption.REPLACE_EXISTING);
        }
        Files.writeString(marker, "Migrated 0.55 to 0.45; original saved in " + backup.getFileName() + "\n",
                StandardCharsets.UTF_8);
        return true;
    }

    /** Export effective flags, excluding install paths and custom key arguments. */
    public static void writeEffectiveFlags(File target, List<String> arguments) throws IOException {
        StringBuilder text = new StringBuilder("# Effective canonical GTA IV flags; sensitive paths omitted.\n");
        for (String line : arguments) {
            if (!line.startsWith("--install_") && !line.startsWith("--key=")) {
                text.append(line).append('\n');
            }
        }
        String content = text.toString();
        Path path = target.toPath();
        if (Files.isRegularFile(path) && Files.readString(path, StandardCharsets.UTF_8).equals(content)) return;
        Files.writeString(path, content, StandardCharsets.UTF_8);
    }
}

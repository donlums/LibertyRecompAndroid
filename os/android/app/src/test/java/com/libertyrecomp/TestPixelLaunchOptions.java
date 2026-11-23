package com.libertyrecomp;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

// Host test, no Android SDK required:
// javac -d /tmp/pixeltest os/android/app/src/main/java/com/libertyrecomp/PixelLaunchOptions.java \
//    os/android/app/src/test/java/com/libertyrecomp/TestPixelLaunchOptions.java
// java -cp /tmp/pixeltest com.libertyrecomp.TestPixelLaunchOptions
public class TestPixelLaunchOptions {
    public static void main(String[] args) throws Exception {
        var unique = PixelLaunchOptions.canonicalize(List.of("--a=1", "--b=2", "--a=3", "--c=4", "--b=5"));
        if (!unique.equals(List.of("--a=3", "--c=4", "--b=5"))) throw new AssertionError(unique);
        Path dir = Files.createTempDirectory("pixel-v020-test");
        Path conf = dir.resolve("args.txt");
        var before = List.of("--gta4_dynamic_draw_distance=true", "--gta4_shadow_cascade_count=2",
                "--gta4_output_height_cap=540", "--gta4_dynamic_draw_distance_min=0.55",
                "--gta4_native_shadow_interval=1");
        Files.write(conf, before, StandardCharsets.UTF_8);
        if (!PixelLaunchOptions.migrateKnownPixelProfile(conf.toFile())) throw new AssertionError("migration not applied");
        if (PixelLaunchOptions.migrateKnownPixelProfile(conf.toFile())) throw new AssertionError("migration repeated");
        if (!Files.readAllLines(conf).contains("--gta4_dynamic_draw_distance_min=0.45"))
            throw new AssertionError("wrong new flag");
        if (!Files.readAllLines(dir.resolve("args.pixel-v019-backup.txt")).equals(before))
            throw new AssertionError("backup was not preserved");
        PixelLaunchOptions.writeEffectiveFlags(dir.resolve("effective.txt").toFile(),
                List.of("--fps=30", "--install_iso=/private/game.iso", "--key=private-key"));
        String effective = Files.readString(dir.resolve("effective.txt"));
        if (!effective.contains("--fps=30") || effective.contains("private"))
            throw new AssertionError("unsafe export");
        System.out.println("PASS: last-wins flags, reversible migration, no repeat, private-path filtering");
    }
}

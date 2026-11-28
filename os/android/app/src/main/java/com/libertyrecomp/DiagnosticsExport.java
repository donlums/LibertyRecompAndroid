package com.libertyrecomp;

import android.app.Activity;
import android.content.ContentValues;
import android.net.Uri;
import android.os.Environment;
import android.provider.MediaStore;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** Bounded log-only export to shared Downloads, with no game data or saves. */
final class DiagnosticsExport {
    private static final AtomicBoolean RUNNING = new AtomicBoolean();
    private static final long MAX_FILE = 8L * 1024 * 1024;
    private DiagnosticsExport() {}

    static void save(Activity activity, File root) {
        if (root == null || !RUNNING.compareAndSet(false, true)) {
            Toast.makeText(activity, "Diagnostics export unavailable or already running", Toast.LENGTH_SHORT).show();
            return;
        }
        Toast.makeText(activity, "Saving diagnostics…", Toast.LENGTH_SHORT).show();
        new Thread(() -> {
            Uri uri = null;
            String name = "GTA-IV-Diagnostics-" +
                    new SimpleDateFormat("yyyyMMdd-HHmmss-SSS", Locale.ROOT).format(new Date()) + ".zip";
            String message;
            try {
                ContentValues values = new ContentValues();
                values.put(MediaStore.MediaColumns.DISPLAY_NAME, name);
                values.put(MediaStore.MediaColumns.MIME_TYPE, "application/zip");
                values.put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/");
                values.put(MediaStore.MediaColumns.IS_PENDING, 1);
                uri = activity.getContentResolver().insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);
                if (uri == null) throw new IOException("Could not create Downloads file");
                OutputStream stream = activity.getContentResolver().openOutputStream(uri, "w");
                if (stream == null) throw new IOException("Could not open Downloads file");
                try (ZipOutputStream zip = new ZipOutputStream(stream)) {
                    add(zip, new File(root, "last_launch.txt"), "last_launch.txt");
                    add(zip, new File(root, "pixel-effective-args.txt"), "pixel-effective-args.txt");
                    recent(zip, new File(root, "debug_logs"), "debug_logs/", 3);
                    recent(zip, new File(root, "crash_reports"), "crash_reports/", 2);
                    recent(zip, new File(root, "LibertyRecomp/logs"), "runtime/", 2);
                    recent(zip, new File(root, "Liberty Recompiled/logs"), "runtime-alt/", 2);
                }
                ContentValues finished = new ContentValues();
                finished.put(MediaStore.MediaColumns.IS_PENDING, 0);
                if (activity.getContentResolver().update(uri, finished, null, null) < 1)
                    throw new IOException("Could not publish ZIP");
                message = "Saved to Downloads/" + name;
            } catch (Exception e) {
                if (uri != null) try { activity.getContentResolver().delete(uri, null, null); }
                    catch (Exception ignored) {}
                android.util.Log.e("LibertyDiagnostics", "Export failed", e);
                message = "Could not save diagnostics: " + e.getMessage();
            } finally {
                RUNNING.set(false);
            }
            final String result = message;
            activity.runOnUiThread(() -> new android.app.AlertDialog.Builder(activity)
                    .setTitle("Diagnostics export")
                    .setMessage(result + "\n\nLogs may include device details and file paths. Review before sharing.")
                    .setPositiveButton("OK", null).show());
        }, "DiagnosticsExport").start();
    }

    private static void recent(ZipOutputStream zip, File folder, String prefix, int count) throws IOException {
        File[] files = folder.listFiles(f -> f.isFile() && !java.nio.file.Files.isSymbolicLink(f.toPath())
                && (f.getName().endsWith(".log") || f.getName().endsWith(".log.1") || f.getName().endsWith(".txt")));
        if (files == null) return;
        Arrays.sort(files, Comparator.comparingLong(File::lastModified).reversed());
        for (int i = 0; i < Math.min(count, files.length); i++)
            add(zip, files[i], prefix + files[i].getName());
    }

    private static void add(ZipOutputStream zip, File file, String entry) throws IOException {
        if (!file.isFile() || java.nio.file.Files.isSymbolicLink(file.toPath())) return;
        try (RandomAccessFile in = new RandomAccessFile(file, "r")) {
            long size = in.length();
            long remaining = Math.min(size, MAX_FILE);
            in.seek(size - remaining);
            zip.putNextEntry(new ZipEntry(entry));
            byte[] buffer = new byte[16384];
            while (remaining > 0) {
                int n = in.read(buffer, 0, (int) Math.min(buffer.length, remaining));
                if (n <= 0) break;
                zip.write(buffer, 0, n);
                remaining -= n;
            }
            zip.closeEntry();
        }
    }
}

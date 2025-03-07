package com.jordan.textrender;

import android.content.ContentResolver;
import android.content.Context;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URLConnection;

public class MiggyUtils {
    public static int toIntColor(double color, double convert) {
        return (int) (color*convert);
    }

    public static int toIntColor(double[] colors) {
        int red = toIntColor(colors[0], 255.0);
        int green = toIntColor(colors[1], 255.0);
        int blue = toIntColor(colors[2], 255.0);
        return 0xFF000000 + (red << 16) + (green << 8) + blue;
    }

    public static int toIntColor(double[] colors, double convert) {
        int red = toIntColor(colors[0], convert);
        int green = toIntColor(colors[1], convert);
        int blue = toIntColor(colors[2], convert);
        return 0xFF000000 + (red << 16) + (green << 8) + blue;
    }

    public static void fromIntColor(double[] colors, int color) {
        fromIntColor(colors, color, 255.0);
    }

    public static void fromIntColor(double[] colors, int color, double convert) {
        int colorWithoutAlpha = color & 0x00FFFFFF;
        colors[0] = (colorWithoutAlpha >> 16) / convert;
        colors[1] = ((colorWithoutAlpha & 0x0000FF00) >> 8) / convert;
        colors[2] = (colorWithoutAlpha & 0x000000FF) / convert;
        colors[3] = 1.0;
    }

    public static ByteArrayOutputStream readInputStream(InputStream inputStream)
    {
        int pageSize = 4096;
        ByteArrayOutputStream bos = new ByteArrayOutputStream(pageSize);
        byte[] pageData = new byte[pageSize];

        try {
            int sizeRead;
            while ((sizeRead = inputStream.read(pageData)) > 0) {
                bos.write(pageData, 0, sizeRead);
            }
        } catch (IOException e) {
            e.printStackTrace();
            bos = null;
        }

        return bos;
    }

    public static String cloneUriImage(Context context, Uri image, String name, BitmapFactory.Options options) {
        String path = "";

        try {
            InputStream inputStream = context.getContentResolver().openInputStream(image);
            ByteArrayOutputStream bos = readInputStream(inputStream);

            FileOutputStream fos = context.openFileOutput(name, Context.MODE_PRIVATE);
            fos.write(bos.toByteArray(), 0, bos.size());
            fos.close();

            String imagePath = context.getFilesDir() + "/" + name;
            File file = new File(imagePath);
            URLConnection connection = file.toURI().toURL().openConnection();
            String mimeType = connection.getContentType();
            if (mimeType.compareToIgnoreCase("image/png") == 0) {
                imagePath += ".png";
            } else {
                imagePath += ".jpg";
            }
            if (file.renameTo(new File(imagePath))) {
                path = imagePath;

                if (options != null) {
                    options.inJustDecodeBounds = true;

                    FileInputStream fis = new FileInputStream(imagePath);
                    BitmapFactory.decodeStream(fis, null, options);
                    fis.close();
                }
            } else {
                Log.e(MiggyConst.APP_TAG, "Rename failed!");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return path;
    }
}

package com.jordan.textrender.activity;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Calendar;
import java.util.Locale;

import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.fragment.app.Fragment;

import com.github.chrisbanes.photoview.PhotoView;
import com.jordan.textrender.R;
import com.jordan.textrender.service.RenderService;

public class RenderFragment extends Fragment {
	public RenderFragment() {
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		return inflater.inflate(R.layout.fragment_render, container, false);
	}

	@Override
	public void onStart() {
		super.onStart();
	}

	@Override
	public void onStop() {
		super.onStop();
	}

	public void update() {
		PhotoView iv = requireActivity().findViewById(R.id.imageView);
		if (iv != null) {
			Matrix currentMatrix = new Matrix();
			iv.getSuppMatrix(currentMatrix);
			iv.setImageBitmap(RenderService.getRenderBitmap());
			iv.setSuppMatrix(currentMatrix);
		}
	}

	private String getCurrentTimeString() {
		int yyyy = Calendar.getInstance().get(Calendar.YEAR);
		int MM = Calendar.getInstance().get(Calendar.MONTH) + 1;
		int dd = Calendar.getInstance().get(Calendar.DAY_OF_MONTH);
		int hh = Calendar.getInstance().get(Calendar.HOUR_OF_DAY);
		int mm = Calendar.getInstance().get(Calendar.MINUTE);
		int ss = Calendar.getInstance().get(Calendar.SECOND);

		return String.format(Locale.getDefault(), "%04d%02d%02d-%02d%02d%02d", yyyy, MM, dd, hh, mm, ss);
	}

	// view image in Photos viewer, can be shared from there
	private void viewImage(Uri imagePath) {
		Intent sharingIntent = new Intent(Intent.ACTION_VIEW);
		sharingIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		sharingIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
		sharingIntent.setDataAndType(imagePath, "image/*");
		startActivity(sharingIntent);
	}

	public void saveRenderedImage() {
		if (!RenderService.isRendering() && RenderService.getRenderBitmap() != null) {
			try {
				String fileName = "Render-" + getCurrentTimeString() + ".png";

				ContentValues values = new ContentValues();
				values.put(MediaStore.Images.Media.DISPLAY_NAME, fileName);
				values.put(MediaStore.Images.Media.MIME_TYPE, "image/jpg");
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
					values.put(MediaStore.MediaColumns.RELATIVE_PATH, "Pictures/");
				} else {
					File directory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
					File file = new File(directory, fileName);
					values.put(MediaStore.MediaColumns.DATA, file.getAbsolutePath());
				}

				Uri uri = requireActivity().getContentResolver().insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
				try (OutputStream output = requireActivity().getContentResolver().openOutputStream(uri)) {
					RenderService.getRenderBitmap().compress(Bitmap.CompressFormat.PNG, 100, output);
					output.close();
					output.flush();

					Toast.makeText(getContext(), "Image saved", Toast.LENGTH_SHORT).show();
				}

				viewImage(uri);
			} catch (IOException e) {
				Toast.makeText(getContext(), "Error: " + e.getLocalizedMessage(), Toast.LENGTH_SHORT).show();
			}
		}
	}
}

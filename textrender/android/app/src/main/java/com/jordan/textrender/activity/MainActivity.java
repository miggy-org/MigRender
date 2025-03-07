package com.jordan.textrender.activity;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import com.jordan.textrender.MiggyConst;
import com.jordan.textrender.Preview;
import com.jordan.textrender.R;
import com.jordan.textrender.dialog.SettingsDialog;
import com.jordan.textrender.json.BackgroundConfig;
import com.jordan.textrender.json.PrefsConfig;
import com.jordan.textrender.json.SettingsConfig;
import com.jordan.textrender.json.RenderConfig;
import com.jordan.textrender.service.RenderService;

import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends AppCompatActivity {
	private static final int GENERIC_OPTIONS = 1;

	private RenderFragment fragRender;
	private Timer textChangedTimer;

	private RenderConfig renderConfig;
	private BackgroundConfig backgroundConfig;

	private boolean inTabletMode() {
		return (findViewById(R.id.renderFrame) != null);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		if (savedInstanceState == null) {
			FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
			ft.add(R.id.container, new PlaceholderFragment());
			if (inTabletMode()) {
				fragRender = new RenderFragment();
				ft.add(R.id.renderFrame, fragRender);
			}
			ft.commit();

		}

		//if (!inTabletMode()) {
		//	setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		//}

		checkPrefsVersion();
	}

	private void checkPrefsVersion() {
		boolean resetAllPrefs = true;

		SharedPreferences prefs = getSharedPreferences(MiggyConst.PREFS_KEY, Context.MODE_PRIVATE);
		String jsonPrefsConfig = prefs.getString(MiggyConst.OPT_PREFS_CONFIG, "");
		if (!jsonPrefsConfig.isEmpty()) {
			PrefsConfig prefsConfig = PrefsConfig.loadFromJson(jsonPrefsConfig, new PrefsConfig());
			if (prefsConfig.prefsVersion.compareTo(MiggyConst.PREFS_VERSION) == 0) {
				resetAllPrefs = false;
			}
		}

		if (resetAllPrefs) {
			PrefsConfig prefsConfig = new PrefsConfig();
			prefsConfig.prefsVersion = MiggyConst.PREFS_VERSION;
			prefs.edit()
					.putString(MiggyConst.OPT_PREFS_CONFIG, prefsConfig.saveToJson())
					.putString(MiggyConst.OPT_CONFIG, "")
					.putString(MiggyConst.OPT_BACKGROUND, "")
					.putString(MiggyConst.OPT_SETTINGS, "")
					.apply();
		}
	}

	private void schedulePreview(int delay) {
		if (textChangedTimer != null)
			textChangedTimer.cancel();

		if (findViewById(R.id.preview) != null) {
			textChangedTimer = new Timer();
			textChangedTimer.schedule(new TimerTask() {
				@Override
				public void run() {
					textChangedTimer = null;
					if (!RenderService.isRendering()) {
						runOnUiThread(() -> doPreview());
					}
				}
			}, delay);
		}
	}

	private class PreviewWatcher implements TextWatcher {
		private final int delay;

		PreviewWatcher() {
			this.delay = 100;
		}

		PreviewWatcher(int delay) {
			this.delay = delay > 0 ? delay : 100;
		}

		@Override
		public void beforeTextChanged(CharSequence s, int start, int count, int after) {
		}

		@Override
		public void onTextChanged(CharSequence s, int start, int before, int count) {
		}

		@Override
		public void afterTextChanged(Editable s) {
			schedulePreview(delay);
		}
	}
	private final PreviewWatcher delayedWatcher = new PreviewWatcher(2000);

	@Override
	protected void onStart() {
		super.onStart();

		onOptionsOK();

		EditText et = findViewById(R.id.textEdit);
		et.setText(renderConfig.text);
		et.addTextChangedListener(delayedWatcher);
	}

	@Override
	protected void onStop() {
		((TextView) findViewById(R.id.textEdit)).removeTextChangedListener(delayedWatcher);

		super.onStop();
	}

	@Override
	protected void onPause() {
		super.onPause();

		RenderService.setUpdateHandler(this, null);
	}

	@Override
	protected void onResume() {
		super.onResume();

		RenderService.setUpdateHandler(this, isComplete ->
			runOnUiThread(() -> {
				if (fragRender != null) {
					fragRender.update();
				}

				if (isComplete) {
					Button btn = findViewById(R.id.renderBtn);
					if (btn != null) {
						btn.setText(getString(R.string.render));
					}
				}
			}));
	}

	@Override
	public boolean onCreateOptionsMenu(@NonNull Menu menu) {

		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			DialogFragment dlg = SettingsDialog.newInstance();
			dlg.show(getSupportFragmentManager(), "settings");
			return true;
		} else if (id == R.id.action_save_image) {
			fragRender.saveRenderedImage();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	private void formatButtonText(Button btn, String text, int idPrefix) {
		if (idPrefix != 0) {
			btn.setText(String.format(Locale.getDefault(), "%1$s  %2$s", getString(idPrefix), text));
		} else {
			btn.setText(text);
		}
	}

	public void onOptionsOK() {
		renderConfig = RenderConfig.loadFromPrefs(this, new RenderConfig());
		if (renderConfig.text.isEmpty()) {
			renderConfig.text = getString(R.string.default_text);
		}
		backgroundConfig = BackgroundConfig.loadFromPrefs(this, new BackgroundConfig());

		formatButtonText(findViewById(R.id.fontBtn),
				getString(MiggyConst.fontOptions.getOptionNameId(renderConfig.fontId)), R.string.font_prefix);
		formatButtonText(findViewById(R.id.matBtn),
				getString(MiggyConst.materialOptions.getOptionNameId(renderConfig.textMaterialId)), R.string.material_prefix);
		formatButtonText(findViewById(R.id.backDropBtn),
				getString(MiggyConst.backdropOptions.getOptionNameId(renderConfig.backdropId)), R.string.backdrop_prefix);
		formatButtonText(findViewById(R.id.lightingBtn),
				getString(MiggyConst.lightingOptions.getOptionNameId(renderConfig.lightingId)), R.string.lighting_prefix);
		formatButtonText(findViewById(R.id.orientBtn),
				getString(MiggyConst.cameraOptions.getOptionNameId(renderConfig.cameraId)), R.string.camera_prefix);
		//formatButtonText(findViewById(R.id.resolutionBtn),
		//		getString(MiggyConst.resolutionOptions.getOption(renderConfig.resolutionId).getNameId()), 0);

		schedulePreview(100);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
		super.onActivityResult(requestCode, resultCode, data);

		if (requestCode == GENERIC_OPTIONS && resultCode == RESULT_OK) {
			onOptionsOK();
		}
	}

	public void onChangeFont(View v) {
		Intent intent = new Intent(this, OptionsActivity.FontOptionsActivity.class);
		startActivityForResult(intent, GENERIC_OPTIONS);
	}
	
	public void onChangeMaterial(View v) {
		Intent intent = new Intent(this, OptionsActivity.MaterialOptionsActivity.class);
		startActivityForResult(intent, GENERIC_OPTIONS);
	}
	
	public void onChangeBackdrop(View v) {
		Intent intent = new Intent(this, OptionsActivity.BackdropOptionsActivity.class);
		startActivityForResult(intent, GENERIC_OPTIONS);
	}
	
	public void onChangeLighting(View v) {
		Intent intent = new Intent(this, OptionsActivity.LightingOptionsActivity.class);
		startActivityForResult(intent, GENERIC_OPTIONS);
	}
	
	public void onChangeOrientation(View v)	{
		Intent intent = new Intent(this, OptionsActivity.CameraOptionsActivity.class);
		startActivityForResult(intent, GENERIC_OPTIONS);
	}

	public void onChangeResolution(View v) {
		Intent intent = new Intent(this, BackgroundActivity.class);
		startActivity(intent);
	}

	private void doPreview() {
		MainPreview preview = findViewById(R.id.preview);
		if (preview != null) {
			if (inTabletMode()) {
				preview.setVisibility(View.VISIBLE);
				findViewById(R.id.renderFrame).setVisibility(View.GONE);
			}

			EditText editText = findViewById(R.id.textEdit);
			if (editText != null) {
				renderConfig.text = editText.getText().toString();
				renderConfig.saveToPrefs(this);
			}

			preview.startPreview(renderConfig, backgroundConfig);
		}
	}

	public void onRender(View v) {
		if (fragRender != null && RenderService.isRendering()) {
			RenderService.signalAbortRender();
		} else {
			EditText et = findViewById(R.id.textEdit);
			String textToRender = et.getText().toString();
			if (textToRender.length() == 0)
				Toast.makeText(this, "Enter some text!", Toast.LENGTH_SHORT).show();
			else if (textToRender.length() > 50)
				Toast.makeText(this, "Too long!", Toast.LENGTH_SHORT).show();
			else
			{
				if (!inTabletMode()) {
					Intent intent = new Intent(this, RenderActivity.class);
					startActivity(intent);
				} else if (fragRender != null) {
					findViewById(R.id.preview).setVisibility(View.GONE);
					findViewById(R.id.renderFrame).setVisibility(View.VISIBLE);

					SettingsConfig settingsConfig = SettingsConfig.loadFromPrefs(this, new SettingsConfig());
					RenderService.startRender(this, settingsConfig, renderConfig, backgroundConfig);

					Button btn = findViewById(R.id.renderBtn);
					if (btn != null) {
						btn.setText(getString(R.string.abort));
					}
				}
			}
		}
	}
	
	/**
	 * A placeholder fragment containing a simple view.
	 */
	public static class PlaceholderFragment extends Fragment {

		@Override
		public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
			return inflater.inflate(R.layout.fragment_main, container, false);
		}
	}

	public static class MainPreview extends Preview {

		public MainPreview(@NonNull Context context) {
			super(context);
		}

		public MainPreview(@NonNull Context context, @Nullable AttributeSet attrs) {
			super(context, attrs);
		}

		public MainPreview(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
			super(context, attrs, defStyleAttr);
		}

		public void startPreview(RenderConfig config, BackgroundConfig background) {
			this.queueRender(() -> {
				this.init(config, null);

				render.setBackground(background);
				render.setEnvironment(getContext(), config);

				return true;
			});
		}
	}
}

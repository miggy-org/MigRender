package com.jordan.textrender.activity;

import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import androidx.appcompat.app.AppCompatActivity;

import com.jordan.textrender.R;
import com.jordan.textrender.json.BackgroundConfig;
import com.jordan.textrender.json.RenderConfig;
import com.jordan.textrender.json.SettingsConfig;
import com.jordan.textrender.service.RenderService;

public class RenderActivity extends AppCompatActivity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_render);

		if (savedInstanceState == null) {
			Bundle args = new Bundle();
			args.putInt("InRenderActivity", 1);

			RenderFragment fragRender = new RenderFragment();
			fragRender.setArguments(args);
			getSupportFragmentManager().beginTransaction()
					.add(R.id.container, fragRender).commit();

			if (!RenderService.isRendering()) {
				RenderConfig config = RenderConfig.loadFromPrefs(this, new RenderConfig());
				SettingsConfig settingsConfig = SettingsConfig.loadFromPrefs(this, new SettingsConfig());
				BackgroundConfig background = BackgroundConfig.loadFromPrefs(this, new BackgroundConfig());

				RenderService.startRender(this, settingsConfig, config, background);
			}
		}
	}

	@Override
	protected void onPause() {
		super.onPause();

		RenderService.setUpdateHandler(this, null);
	}

	@Override
	protected void onResume() {
		super.onResume();

		final RenderFragment fragRender = (RenderFragment) getSupportFragmentManager().findFragmentById(R.id.container);
		if (fragRender != null) {
			fragRender.update();

			RenderService.setUpdateHandler(this, isComplete -> runOnUiThread(() -> fragRender.update()));
		}
	}

	@Override
	public void onBackPressed() {
		super.onBackPressed();

		RenderService.signalAbortRender();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.render, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_save_image) {
			final RenderFragment fragRender = (RenderFragment) getSupportFragmentManager().findFragmentById(R.id.container);
			if (fragRender != null) {
				fragRender.saveRenderedImage();
			}
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
}

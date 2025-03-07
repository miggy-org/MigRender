package com.jordan.textrender.dialog;

import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.CheckBox;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;

import com.jordan.textrender.R;
import com.jordan.textrender.json.SettingsConfig;

public class SettingsDialog extends DialogFragment implements OnClickListener {
	CheckBox checkShadows;
	CheckBox checkSoftShadows;
	CheckBox checkAutoReflect;
	CheckBox checkSuperSample;
	CheckBox checkMultiCore;

	// creates an instance of this dialog
	public static SettingsDialog newInstance()
	{
		return new SettingsDialog();
	}
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		View v = inflater.inflate(R.layout.fragment_settings, container, false);
		
		// set the title string
		getDialog().setTitle(getString(R.string.action_settings));
		
		SettingsConfig settingsConfig = SettingsConfig.loadFromPrefs(getActivity(), new SettingsConfig());

		checkShadows = (CheckBox) v.findViewById(R.id.checkShadows);
		if (checkShadows != null)
		{
			checkShadows.setChecked(settingsConfig.shadows);
			checkShadows.setOnClickListener(this);
		}

		checkSoftShadows = (CheckBox) v.findViewById(R.id.checkSoftShadows);
		if (checkSoftShadows != null)
		{
			checkSoftShadows.setChecked(settingsConfig.softShadows);
			checkSoftShadows.setEnabled(checkShadows.isChecked());
		}

		checkAutoReflect = (CheckBox) v.findViewById(R.id.checkAutoReflect);
		if (checkAutoReflect != null)
			checkAutoReflect.setChecked(settingsConfig.autoReflect);

		checkSuperSample = (CheckBox) v.findViewById(R.id.checkSuperSample);
		if (checkSuperSample != null)
			checkSuperSample.setChecked(settingsConfig.superSample);

		checkMultiCore = (CheckBox) v.findViewById(R.id.checkMulticore);
		if (checkMultiCore != null)
			checkMultiCore.setChecked(settingsConfig.multiCore);

		return v;
	}

	@Override
	public void onDismiss(@NonNull DialogInterface dialog) {
		SettingsConfig settingsConfig = new SettingsConfig();
		settingsConfig.shadows = checkShadows.isChecked();
		settingsConfig.softShadows = checkSoftShadows.isChecked();
		settingsConfig.autoReflect = checkAutoReflect.isChecked();
		settingsConfig.superSample = checkSuperSample.isChecked();
		settingsConfig.multiCore = checkMultiCore.isChecked();
		settingsConfig.saveToPrefs(getActivity());

		super.onDismiss(dialog);
	}
	
	@Override
	public void onClick(View v) {
		if (checkSoftShadows != null) {
			boolean useShadows = ((CheckBox) v).isChecked();
			checkSoftShadows.setEnabled(useShadows);
			if (!useShadows)
				checkSoftShadows.setChecked(false);
		}
	}
}

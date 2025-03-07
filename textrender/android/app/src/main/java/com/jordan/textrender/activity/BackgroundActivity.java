package com.jordan.textrender.activity;

import android.content.Context;
import android.content.Intent;
import android.graphics.PorterDuff;
import android.net.Uri;
import android.os.Bundle;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.Spinner;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import com.jordan.textrender.MiggyConst;
import com.jordan.textrender.MiggyUtils;
import com.jordan.textrender.Options;
import com.jordan.textrender.Preview;
import com.jordan.textrender.R;
import com.jordan.textrender.json.BackgroundConfig;
import com.jordan.textrender.json.RenderConfig;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.Locale;

import top.defaults.colorpicker.ColorPickerPopup;

public class BackgroundActivity extends AppCompatActivity {
    private final int PICK_IMAGE = 1;
    private final double INT_TO_POLAR_COLOR_CONVERT = 80.0;
    private final double INT_TO_EQUATOR_COLOR_CONVERT = 255.0;

    private BackgroundPreview preview;
    private BackgroundConfig config;
    private RenderConfig renderConfig;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_background);

        if (savedInstanceState == null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.add(R.id.container, new BackgroundActivity.PlaceholderFragment());
            ft.commit();
        }

        preview = findViewById(R.id.preview);

        config = BackgroundConfig.loadFromPrefs(this, new BackgroundConfig());
        renderConfig = RenderConfig.loadFromPrefs(this, new RenderConfig());
    }

    private void saveConfig() {
        config.saveToPrefs(this);
    }

    private void updatePreview() {
        if (preview != null) {
            preview.startPreview(config, renderConfig);
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        Spinner spinner = findViewById(R.id.spinnerColors);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
                R.array.number_of_colors, R.layout.spinner_layout);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);

        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                int chosenNumberOfColors = (position == 0 ? 1 : 3);
                if (config.type == BackgroundConfig.Type.COLORS && chosenNumberOfColors != config.numberOfColors) {
                    config.numberOfColors = chosenNumberOfColors;
                    saveConfig();

                    updateControls();
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        updateControls();
    }

    private void updateControls() {
        if (config.type == BackgroundConfig.Type.COLORS) {
            ((RadioButton) findViewById(R.id.radioColors)).setChecked(true);
        } else if (config.type == BackgroundConfig.Type.IMAGE) {
            ((RadioButton) findViewById(R.id.radioImage)).setChecked(true);
        } else {
            ((RadioButton) findViewById(R.id.radioTransparent)).setChecked(true);
        }

        findViewById(R.id.selectImage).setVisibility(
                (config.type == BackgroundConfig.Type.IMAGE) ? View.VISIBLE : View.INVISIBLE);
        findViewById(R.id.selectScale).setVisibility(
                (config.type == BackgroundConfig.Type.IMAGE) ? View.VISIBLE : View.INVISIBLE);
        findViewById(R.id.spinnerColors).setVisibility(
                (config.type == BackgroundConfig.Type.COLORS) ? View.VISIBLE : View.INVISIBLE);
        findViewById(R.id.colorTop).setVisibility(
                (config.type == BackgroundConfig.Type.COLORS) ? View.VISIBLE : View.INVISIBLE);
        findViewById(R.id.colorMiddle).setVisibility(
                (config.type == BackgroundConfig.Type.COLORS) ? View.VISIBLE : View.INVISIBLE);
        findViewById(R.id.colorBottom).setVisibility(
                (config.type == BackgroundConfig.Type.COLORS) ? View.VISIBLE : View.INVISIBLE);

        findViewById(R.id.colorTop).getBackground().setColorFilter(
                MiggyUtils.toIntColor(config.top, INT_TO_POLAR_COLOR_CONVERT), PorterDuff.Mode.MULTIPLY);
        findViewById(R.id.colorMiddle).getBackground().setColorFilter(
                MiggyUtils.toIntColor(config.middle, INT_TO_EQUATOR_COLOR_CONVERT), PorterDuff.Mode.MULTIPLY);
        findViewById(R.id.colorBottom).getBackground().setColorFilter(
                MiggyUtils.toIntColor(config.bottom, INT_TO_POLAR_COLOR_CONVERT), PorterDuff.Mode.MULTIPLY);

        if (config.type == BackgroundConfig.Type.COLORS) {
            Spinner spinner = findViewById(R.id.spinnerColors);
            spinner.setSelection(config.numberOfColors == 1 ? 0 : 1);

            boolean showTop = false;
            boolean showMiddle = false;
            boolean showBottom = false;
            if (config.numberOfColors == 1) {
                showMiddle = true;
            } else if (config.numberOfColors == 2) {
                showTop = showBottom = true;
            } else {
                showTop = showMiddle = showBottom = true;
            }
            findViewById(R.id.colorTop).setVisibility(showTop ? View.VISIBLE : View.INVISIBLE);
            findViewById(R.id.colorMiddle).setVisibility(showMiddle ? View.VISIBLE : View.INVISIBLE);
            findViewById(R.id.colorBottom).setVisibility(showBottom ? View.VISIBLE : View.INVISIBLE);
        }

        Button resolutionButton = findViewById(R.id.sizeBottom);
        Options.Resolution res = MiggyConst.resolutionOptions.getOption(renderConfig.resolutionId);
        String text = String.format(Locale.getDefault(), "%1$s [%2$dx%3$d]",
                getString(res.getNameId()), res.getWidth(), res.getHeight());
        resolutionButton.setText(text);

        updatePreview();
    }

    public void onRadioButtonClicked(View view) {
        if (view.getId() == R.id.radioColors) {
            config.type = BackgroundConfig.Type.COLORS;
        } else if (view.getId() == R.id.radioImage) {
            config.type = BackgroundConfig.Type.IMAGE;
        } else {
            config.type = BackgroundConfig.Type.TRANSPARENT;
        }

        saveConfig();
        updateControls();
    }

    public void onScaleClicked(View view) {
        Button btn = (Button) view;
        if (config.scale == BackgroundConfig.ImageScale.STRETCH) {
            config.scale = BackgroundConfig.ImageScale.SCALE_TO_FIT;
            btn.setText(R.string.scale_to_fit);
        } else if (config.scale == BackgroundConfig.ImageScale.SCALE_TO_FIT) {
            config.scale = BackgroundConfig.ImageScale.SCALE_TO_FILL;
            btn.setText(R.string.scale_to_fill);
        } else {
            config.scale = BackgroundConfig.ImageScale.STRETCH;
            btn.setText(R.string.stretch);
        }

        saveConfig();
        updatePreview();
    }

    public void onImageClicked(View view) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("image/*");
        startActivityForResult(intent, PICK_IMAGE);
    }

    public void onSizeClicked(View view) {
        ArrayList<Integer> resolutions = MiggyConst.resolutionOptions.getKeys();
        int index = resolutions.indexOf(renderConfig.resolutionId);
        index = (index >= resolutions.size() - 1 ? 0 : index + 1);
        renderConfig.resolutionId = resolutions.get(index);
        renderConfig.saveToPrefs(this);

        updateControls();
    }

    public void onColorClicked(View view) {
        final double[] chosenColor;
        final double convert;
        if (view.getId() == R.id.colorTop) {
            chosenColor = config.top;
            convert = INT_TO_POLAR_COLOR_CONVERT;
        } else if (view.getId() == R.id.colorBottom) {
            chosenColor = config.bottom;
            convert = INT_TO_POLAR_COLOR_CONVERT;
        } else {
            chosenColor = config.middle;
            convert = INT_TO_EQUATOR_COLOR_CONVERT;
        }

        int initialColor = MiggyUtils.toIntColor(chosenColor, convert);
        new ColorPickerPopup.Builder(this).initialColor(initialColor)
                .enableBrightness(true)
                .enableAlpha(false)
                .okTitle(getString(R.string.ok))
                .cancelTitle(getString(R.string.cancel))
                .showIndicator(false)
                .showValue(false)
                .build().show(new ColorPickerPopup.ColorPickerObserver() {
                    @Override
                    public void onColorPicked(int color) {
                        MiggyUtils.fromIntColor(chosenColor, color, convert);
                        saveConfig();
                        updateControls();
                    }
                });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == RESULT_OK && data != null) {
            if (requestCode == PICK_IMAGE) {
                config.imagePath = MiggyUtils.cloneUriImage(this, data.getData(), "bgimage", null);

                //Log.i("TEST", "Path: " + config.imagePath + ", Size: " + totalSize);
                saveConfig();
                updatePreview();
            }
        }
    }

    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            return inflater.inflate(R.layout.fragment_background, container, false);
        }
    }

    public static class BackgroundPreview extends Preview {

        public BackgroundPreview(@NonNull Context context) {
            super(context);
        }

        public BackgroundPreview(@NonNull Context context, @Nullable AttributeSet attrs) {
            super(context, attrs);
        }

        public BackgroundPreview(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
        }

        public void startPreview(BackgroundConfig config, RenderConfig renderConfig) {
            this.queueRender(() -> {
                this.init(renderConfig, null);

                this.render.setBackground(config);

                return true;
            });
        }
    }
}

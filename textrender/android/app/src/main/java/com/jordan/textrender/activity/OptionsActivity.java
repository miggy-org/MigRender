package com.jordan.textrender.activity;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import android.content.Context;
import android.content.Intent;
import android.graphics.BitmapFactory;
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
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.jordan.textrender.MiggyConst;
import com.jordan.textrender.MiggyUtils;
import com.jordan.textrender.Options;
import com.jordan.textrender.Preview;
import com.jordan.textrender.R;
import com.jordan.textrender.json.SubOption;
import com.jordan.textrender.json.RenderConfig;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URLConnection;
import java.util.List;

import top.defaults.colorpicker.ColorPickerPopup;

public abstract class OptionsActivity extends AppCompatActivity implements AdapterView.OnItemClickListener {
    private final int PICK_IMAGE = 1;

    private ListView listView;
    private ItemAdapter adapter;
    private boolean showFontControls;

    protected final Options<?> coreOptions;

    protected OptionsPreview preview;
    protected RenderConfig renderConfig;

    abstract int getChosenOption();
    abstract void updateConfig(int chosenId);
    abstract void updatePreview();

    protected OptionsActivity(Options<?> options) {
        this.coreOptions = options;
        this.showFontControls = false;
    }

    protected OptionsActivity(Options<?> options, boolean showFontControls) {
        this.coreOptions = options;
        this.showFontControls = showFontControls;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_options);

        if (savedInstanceState == null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            ft.add(R.id.container, new OptionsActivity.PlaceholderFragment());
            ft.commit();
        }

        renderConfig = RenderConfig.loadFromPrefs(this, new RenderConfig());

        preview = findViewById(R.id.optionsPreview);
    }

    @Override
    protected void onStart() {
        super.onStart();

        listView = findViewById(R.id.listView);
        listView.setOnItemClickListener(this);

        // load the list of options as strings
        adapter = new ItemAdapter(this, coreOptions);
        listView.setAdapter(adapter);

        findViewById(R.id.fontControls).setVisibility(showFontControls ? View.VISIBLE : View.GONE);
        if (showFontControls) {
            CheckBox boldBox = findViewById(R.id.boldCheck);
            boldBox.setChecked(renderConfig.bold);
            CheckBox italicBox = findViewById(R.id.italicCheck);
            italicBox.setChecked(renderConfig.italic);
        }

        updatePreview();

        Button btnOK = findViewById(R.id.ok);
        btnOK.setOnClickListener(v1 -> {
            renderConfig.saveToPrefs(this);
            setResult(RESULT_OK);
            finish();
        });

        Button btnCancel = findViewById(R.id.cancel);
        btnCancel.setOnClickListener(v1 -> {
            setResult(RESULT_CANCELED);
            finish();
        });
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        updateConfig((int) listView.getItemAtPosition(position));
        adapter.notifyDataSetChanged();

        updatePreview();
    }

    public void onBold(View view) {
        renderConfig.bold = ((CheckBox) view).isChecked();
        updatePreview();
    }

    public void onItalic(View view) {
        renderConfig.italic = ((CheckBox) view).isChecked();
        updatePreview();
    }

    class ItemAdapter extends ArrayAdapter<Integer> {
        private final Options<?> options;

        public ItemAdapter(@NonNull Context context, Options<?> options) {
            super(context, R.layout.list_item_option, options.getKeys());
            this.options = options;
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
            final int optionId = options.getKeys().get(position);

            LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View row = (convertView == null ? inflater.inflate(R.layout.list_item_option, parent, false) : convertView);
            TextView text = row.findViewById(R.id.tvItem);
            text.setText(getString(options.getOptionNameId(optionId)));

            ImageView check = row.findViewById(R.id.ivCheck);
            if (options.getKeys().get(position) == getChosenOption()) {
                check.setVisibility(View.VISIBLE);
            } else {
                check.setVisibility(View.INVISIBLE);
            }

            final Button colorButton = row.findViewById(R.id.color);
            if ((coreOptions.getOptionFlags(optionId) & (Options.FLAG_COLOR_DIFFUSE | Options.FLAG_COLOR_SPECULAR)) > 0) {
                colorButton.setVisibility(View.VISIBLE);
                colorButton.setEnabled(options.getKeys().get(position) == getChosenOption());

                SubOption option = renderConfig.optionMap.get(optionId);
                colorButton.getBackground().setColorFilter(
                        (option != null ? option.chosenColor : 0xFF808080), PorterDuff.Mode.MULTIPLY);

				colorButton.setOnClickListener(v -> {
					final SubOption subOption = renderConfig.optionMap.get(optionId);

					new ColorPickerPopup.Builder(getContext())
							.initialColor((subOption != null ? subOption.chosenColor : 0xFF808080))
							.enableBrightness(true)
							.enableAlpha(false)
							.okTitle(getString(R.string.ok))
							.cancelTitle(getString(R.string.cancel))
							.showIndicator(false)
							.showValue(false)
							.build().show(new ColorPickerPopup.ColorPickerObserver() {
								@Override
								public void onColorPicked(int color) {
									if (subOption != null) {
                                        subOption.chosenColor = color;
									} else {
										SubOption newOption = new SubOption();
										newOption.chosenColor = color;
										renderConfig.optionMap.put(optionId, newOption);
									}

                                    colorButton.getBackground().setColorFilter(color, PorterDuff.Mode.MULTIPLY);
                                    updatePreview();
								}
							});
				});
            } else {
                colorButton.setVisibility(View.INVISIBLE);
            }

            final Button imageButton = row.findViewById(R.id.image);
            if ((coreOptions.getOptionFlags(optionId) & Options.FLAG_IMAGE) > 0) {
                imageButton.setVisibility(View.VISIBLE);
                imageButton.setEnabled(options.getKeys().get(position) == getChosenOption());

                imageButton.setOnClickListener(v -> {
                    Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                    intent.setType("image/*");
                    startActivityForResult(intent, PICK_IMAGE);
                });
            } else {
                imageButton.setVisibility(View.INVISIBLE);
            }

            return row;
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == RESULT_OK && data != null) {
            if (requestCode == PICK_IMAGE) {
                SubOption subOption = renderConfig.optionMap.get(getChosenOption());
                if (subOption == null) {
                    subOption = new SubOption();
                    renderConfig.optionMap.put(getChosenOption(), subOption);
                }

                BitmapFactory.Options options = new BitmapFactory.Options();
                subOption.chosenImagePath = MiggyUtils.cloneUriImage(this, data.getData(), "userimage", options);
                subOption.chosenImageWidth = options.outWidth;
                subOption.chosenImageHeight = options.outHeight;

                updatePreview();
            }
        }
    }

    public static class OptionsPreview extends Preview {

        public OptionsPreview(@NonNull Context context) {
            super(context);
        }

        public OptionsPreview(@NonNull Context context, @Nullable AttributeSet attrs) {
            super(context, attrs);
        }

        public OptionsPreview(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
            super(context, attrs, defStyleAttr);
        }

        public void startPreview(String text, int fontId, int matId, int backdropId, int lightingId, int cameraId, RenderConfig config) {
            this.queueRender(() -> {
                this.init(config, null);

                RenderConfig newConfig = new RenderConfig();
                newConfig.text = text;
                newConfig.bold = config.bold;
                newConfig.italic = config.italic;
                newConfig.fontId = fontId;
                newConfig.textMaterialId = matId;
                newConfig.backdropId = backdropId;
                newConfig.lightingId = lightingId;
                newConfig.cameraId = cameraId;
                newConfig.optionMap = config.optionMap;

                return render.setEnvironment(getContext(), newConfig);
            });
        }
    }

    /**
     * A placeholder fragment containing a simple view.
     */
    public static class PlaceholderFragment extends Fragment {

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            return inflater.inflate(R.layout.fragment_options, container, false);
        }
    }

    public static class FontOptionsActivity extends OptionsActivity {
        public FontOptionsActivity() {
            super(MiggyConst.fontOptions, true);
        }

        int getChosenOption() {
            return renderConfig.fontId;
        }

        void updateConfig(int chosenId) {
            renderConfig.fontId = chosenId;
        }

        void updatePreview() {
            preview.startPreview("ABC", renderConfig.fontId, renderConfig.textMaterialId, 0, R.string.lighting_many, R.string.orientation_lookup, renderConfig);
        }
    }

    public static class MaterialOptionsActivity extends OptionsActivity {
        public MaterialOptionsActivity() {
            super(MiggyConst.materialOptions);
        }

        int getChosenOption() {
            return renderConfig.textMaterialId;
        }

        void updateConfig(int chosenId) {
            renderConfig.textMaterialId = chosenId;
        }

        void updatePreview() {
            preview.startPreview("ABC", renderConfig.fontId, renderConfig.textMaterialId, 0, R.string.lighting_many, R.string.orientation_lookup, renderConfig);
        }
    }

    public static class BackdropOptionsActivity extends OptionsActivity {
        public BackdropOptionsActivity() {
            super(MiggyConst.backdropOptions);
        }

        int getChosenOption() {
            return renderConfig.backdropId;
        }

        void updateConfig(int chosenId) {
            renderConfig.backdropId = chosenId;
        }

        void updatePreview() {
            preview.startPreview("", 0, 0, renderConfig.backdropId, R.string.lighting_many, R.string.orientation_lookup, renderConfig);
        }
    }

    public static class LightingOptionsActivity extends OptionsActivity {
        public LightingOptionsActivity() {
            super(MiggyConst.lightingOptions);
        }

        int getChosenOption() {
            return renderConfig.lightingId;
        }

        void updateConfig(int chosenId) {
            renderConfig.lightingId = chosenId;
        }

        void updatePreview() {
            preview.startPreview("ABC", renderConfig.fontId, renderConfig.textMaterialId, renderConfig.backdropId, renderConfig.lightingId, R.string.orientation_lookup, renderConfig);
        }
    }

    public static class CameraOptionsActivity extends OptionsActivity {
        public CameraOptionsActivity() {
            super(MiggyConst.cameraOptions);
        }

        int getChosenOption() {
            return renderConfig.cameraId;
        }

        void updateConfig(int chosenId) {
            renderConfig.cameraId = chosenId;
        }

        void updatePreview() {
            preview.startPreview("ABC", renderConfig.fontId, renderConfig.textMaterialId, renderConfig.backdropId, renderConfig.lightingId, renderConfig.cameraId, renderConfig);
        }
    }
}

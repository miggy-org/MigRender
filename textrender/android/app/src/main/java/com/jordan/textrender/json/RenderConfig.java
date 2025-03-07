package com.jordan.textrender.json;

import com.jordan.textrender.MiggyConst;

import java.util.HashMap;

public class RenderConfig extends BaseConfig {
    public String text;
    public boolean bold;
    public boolean italic;

    public int fontId;
    public int textMaterialId;
    public int backdropId;
    public int lightingId;
    public int cameraId;
    public int resolutionId;

    public HashMap<Integer, SubOption> optionMap;

    public RenderConfig() {
        text = "";
        bold = false;
        italic = false;

        fontId = MiggyConst.fontOptions.getDefaultId();
        textMaterialId = MiggyConst.materialOptions.getDefaultId();
        backdropId = MiggyConst.backdropOptions.getDefaultId();
        lightingId = MiggyConst.lightingOptions.getDefaultId();
        cameraId = MiggyConst.cameraOptions.getDefaultId();
        resolutionId = MiggyConst.resolutionOptions.getDefaultId();

        optionMap = new HashMap<>();
    }

    protected String getPrefsKey() {
        return MiggyConst.OPT_CONFIG;
    }
}

package com.jordan.textrender.json;

import com.jordan.textrender.MiggyConst;

public class SettingsConfig extends BaseConfig {
    public boolean shadows;
    public boolean softShadows;
    public boolean autoReflect;
    public boolean superSample;
    public boolean multiCore;

    public SettingsConfig() {
        shadows = true;
        softShadows = false;
        autoReflect = true;
        superSample = false;
        multiCore = true;
    }

    protected String getPrefsKey() {
        return MiggyConst.OPT_SETTINGS;
    }
}

package com.jordan.textrender.json;

import com.jordan.textrender.MiggyConst;

public class PrefsConfig extends BaseConfig{
    public String prefsVersion;

    public PrefsConfig() {
        prefsVersion = "";
    }

    @Override
    protected String getPrefsKey() {
        return MiggyConst.OPT_PREFS_CONFIG;
    }
}

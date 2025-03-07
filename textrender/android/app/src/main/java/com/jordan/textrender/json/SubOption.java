package com.jordan.textrender.json;

public class SubOption extends BaseConfig {
    public int chosenColor;
    public String chosenImagePath;
    public int chosenImageWidth;
    public int chosenImageHeight;

    @Override
    protected String getPrefsKey() {
        // stored in UserConfig, so no prefs key
        return null;
    }
}

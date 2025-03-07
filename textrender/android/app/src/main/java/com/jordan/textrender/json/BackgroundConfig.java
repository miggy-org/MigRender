package com.jordan.textrender.json;

import com.jordan.textrender.MiggyConst;

public class BackgroundConfig extends BaseConfig {
    public enum Type {
        COLORS, IMAGE, TRANSPARENT
    }
    public Type type;

    // colors
    public int numberOfColors;  // 1 - 3
    public double[] top;
    public double[] middle;
    public double[] bottom;

    // image
    public String imagePath;
    public enum ImageScale {
        STRETCH, SCALE_TO_FIT, SCALE_TO_FILL
    }
    public ImageScale scale;

    public BackgroundConfig() {
        type = Type.COLORS;
        numberOfColors = 3;
        top = new double[] { 1.0, 0.0, 0.0, 1.0 };
        middle = new double[] { 0.0, 0.0, 0.2, 1.0 };
        bottom = new double[] { 0.0, 1.0, 0.0, 1.0 };
        imagePath = "";
        scale = ImageScale.STRETCH;
    }

    protected String getPrefsKey() {
        return MiggyConst.OPT_BACKGROUND;
    }
}

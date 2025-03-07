package com.jordan.textrender;

import java.io.FileDescriptor;

import android.graphics.Bitmap;

public class MiggyInterface
{
	// loads native libraries on startup
    static {
        System.loadLibrary("miggy");
    }

    public interface IMiggyCallback {
    	boolean miggyRenderStarted();
    	boolean miggyRenderUpdate(int percent);
    	void miggyRenderComplete();

        void miggyRenderAborted();
    }

    public static class ObjParams {
        public double[] diff;
        public double[] spec;
        public double[] refl;
        public double[] refr;
        public double[] glow;
        public double index;
        public boolean autoReflect;
        public boolean autoRefract;

        public ObjParams(double[] diff, double[] spec, double[] refl, double[] refr, double[] glow,
                         double index, boolean autoReflect, boolean autoRefract) {
            this.diff = diff;
            this.spec = spec;
            this.refl = refl;
            this.refr = refr;
            this.glow = glow;
            this.index = index;
            this.autoReflect = autoReflect;
            this.autoRefract = autoRefract;
        }
    }

    public static class MatrixOps {
        public double[] scale;
        public double[] rotate;
        public double[] translate;

        public MatrixOps(double[] scale, double[] rotate, double[] translate) {
            this.scale = scale;
            this.rotate = rotate;
            this.translate = translate;
        }
    }

    public static native boolean Init(boolean superSample, boolean doShadows, boolean softShadows, boolean autoReflect);

    public static native boolean SetBackground(double[] n, double[] e, double[] s);
    public static native boolean SetBackgroundImage(String imagePath, String resize);
    public static native boolean SetText(String text, String suffix, String script, byte[] pkgData, ObjParams params);
    public static native boolean SetBackdrop(byte[] mdlData, ObjParams params, MatrixOps ops);
    public static native boolean SetLighting(byte[] mdlData, double[] a);
    public static native boolean SetOrientation(MatrixOps ops);

    public static native boolean LoadImage(String slot, FileDescriptor fd, long off, long len, boolean createAlpha);
    public static native boolean LoadImagePath(String slot, String imagePath, boolean createAlpha);

    public static native boolean Render(Bitmap bmp, boolean multiCore, MiggyInterface.IMiggyCallback callback);
    public static native boolean Abort();
}

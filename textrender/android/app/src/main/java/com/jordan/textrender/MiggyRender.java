package com.jordan.textrender;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.graphics.Bitmap;
import android.util.Pair;

import com.jordan.textrender.json.BackgroundConfig;
import com.jordan.textrender.json.SettingsConfig;
import com.jordan.textrender.json.SubOption;
import com.jordan.textrender.json.RenderConfig;

import java.io.ByteArrayOutputStream;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

public class MiggyRender implements MiggyInterface.IMiggyCallback {
    private MiggyInterface.IMiggyCallback callback;
    private MiggyInterface.IMiggyCallback abortCallback;
    private boolean multiCore;
    private boolean inRender;
    private boolean abortRender;

    private final HashMap<String, Pair<Integer, byte[]>> cachedResources;

    private static MiggyRender singleton = null;
    public static MiggyRender getInstance() {
        if (singleton == null) {
            singleton = new MiggyRender();
        }
        return singleton;
    }

    private MiggyRender() {
        cachedResources = new HashMap<>();
    }

    public boolean isRendering() {
        return inRender;
    }

    public void signalAbortRender(MiggyInterface.IMiggyCallback callback)	{
        if (isRendering()) {
            abortRender = true;
            abortCallback = callback;

            MiggyInterface.Abort();
        }
    }

    private String loadString(Context context, int idResource) {
        InputStream is = context.getResources().openRawResource(idResource);
        ByteArrayOutputStream bos = MiggyUtils.readInputStream(is);
        return (bos != null ? bos.toString() : null);
    }

    private byte[] loadBinary(Context context, int idResource) {
        InputStream is = context.getResources().openRawResource(idResource);
        ByteArrayOutputStream bos = MiggyUtils.readInputStream(is);
        return (bos != null ? bos.toByteArray() : null);
    }

    private byte[] loadBinaryCached(Context context, Options.Package pkg, String resourceKey) {
        if (pkg == null || pkg.getPackageId() == 0) {
            return null;
        }
        int idResource = pkg.getPackageId();

        Pair<Integer, byte[]> resourcePair = cachedResources.get(resourceKey);
        if (resourcePair == null || resourcePair.first != idResource || resourcePair.second == null) {
            resourcePair = new Pair<>(idResource, loadBinary(context, pkg.getPackageId()));
            cachedResources.put(resourceKey, resourcePair);
        }
        return resourcePair.second;
    }

    private boolean loadImage(Context context, int idImage, String slot, boolean createAlpha) {
        AssetFileDescriptor afd = context.getResources().openRawResourceFd(idImage);
        if (afd != null) {
            FileDescriptor fd = afd.getFileDescriptor();
            int off = (int) afd.getStartOffset();
            int len = (int) afd.getLength();

            return MiggyInterface.LoadImage(slot, fd, off, len, createAlpha);
        }

        return false;
    }

    private boolean loadTextures(Context context, Options.Texture[] textures) {
        if (textures != null) {
            for (Options.Texture texture : textures) {
                if (texture.getTextureId() != 0 &&
                        !loadImage(context, texture.getTextureId(), texture.getSlot(), texture.getAlpha()))
                    return false;
            }
        }
        return true;
    }

    public boolean initRender(boolean shadows, boolean softShadows, boolean autoReflect, boolean superSample, boolean multiCore) {
        this.multiCore = multiCore;
        return MiggyInterface.Init(superSample, shadows, softShadows, autoReflect);
    }

    public boolean initRender(SettingsConfig settingsConfig) {
        this.multiCore = settingsConfig.multiCore;
        return MiggyInterface.Init(settingsConfig.superSample, settingsConfig.shadows, settingsConfig.softShadows, settingsConfig.autoReflect);
    }

    private double[] getChosenColor(Options.Material material, RenderConfig config, int optionId, int flag) {
        double[] color = null;
        if ((material.getFlags() & flag) > 0 && config.optionMap != null) {
            SubOption subConfig = config.optionMap.get(optionId);
            if (subConfig != null) {
                color = new double[4];
                MiggyUtils.fromIntColor(color, subConfig.chosenColor);
            }
        }
        if (color == null) {
            color = (flag == Options.FLAG_COLOR_SPECULAR ? material.getSpecular() : material.getDiffuse());
        }
        return color;
    }

    public boolean setText(Context context, RenderConfig config) {
        if (config == null) {
            return false;
        }

        if (!config.text.isEmpty() && config.fontId != 0 && config.textMaterialId != 0) {
            Options.Package pkg = MiggyConst.fontOptions.getOption(config.fontId);
            if (pkg == null) {
                return false;
            }
            Options.FontMaterial material = MiggyConst.materialOptions.getOption(config.textMaterialId);
            if (material == null) {
                return false;
            }

            double[] diffuse = getChosenColor(material, config, config.textMaterialId, Options.FLAG_COLOR_DIFFUSE);

            byte[] binFontData = loadBinaryCached(context, pkg, "text");
            MiggyInterface.ObjParams params = new MiggyInterface.ObjParams(
                    diffuse, material.getSpecular(), null, material.getRefract(), null,
                    material.getRefractIndex(), material.getAutoReflect(), material.getAutoRefract()
            );
            String suffix = "";
            if (config.bold) {
                suffix += "b";
            }
            if (config.italic) {
                suffix += "i";
            }
            String script = loadString(context, material.getScriptId());
            if (binFontData == null ||
                    !MiggyInterface.SetText(
                            config.text, suffix, script, binFontData, params)) {
                return false;
            }

            return loadTextures(context, material.getTextures());
        }

        return true;
    }

    public boolean setBackdrop(Context context, RenderConfig config) {
        if (config == null) {
            return false;
        }

        if (config.backdropId != 0) {
            Options.Backdrop bkd = MiggyConst.backdropOptions.getOption(config.backdropId);
            if (bkd == null) {
                return false;
            }
            byte[] binBackdropData = loadBinaryCached(context, bkd.getPkg(), "backdrop");
            if (binBackdropData == null) {
                // no backdrop is ok
                return true;
            }

            Options.Material mat = bkd.getMat();
            double[] diffuse = getChosenColor(mat, config, config.backdropId, Options.FLAG_COLOR_DIFFUSE);
            double[] specular = getChosenColor(mat, config, config.backdropId, Options.FLAG_COLOR_SPECULAR);

            double[] scale = null;
            if ((mat.getFlags() & Options.FLAG_IMAGE) > 0) {
                SubOption subOption = config.optionMap.get(config.backdropId);
                if (subOption != null && subOption.chosenImagePath != null) {
                    if (MiggyInterface.LoadImagePath("user-image", subOption.chosenImagePath, false)) {
                        double aspect = subOption.chosenImageWidth / (double) subOption.chosenImageHeight;
                        if (aspect > 1) {
                            scale = new double[] { 1, 1 / aspect, 1 };
                        } else if (subOption.chosenImageHeight > subOption.chosenImageWidth) {
                            scale = new double[] { aspect, 1, 1 };
                        }
                    } else {
                        return false;
                    }
                }
            }

            MiggyInterface.ObjParams params = new MiggyInterface.ObjParams(
                    diffuse, specular, null, mat.getRefract(), null,
                    mat.getRefractIndex(), mat.getAutoReflect(), mat.getAutoRefract()
            );
            MiggyInterface.MatrixOps ops = new MiggyInterface.MatrixOps(
                    scale, null, null
            );
            if (!MiggyInterface.SetBackdrop(binBackdropData, params, ops)) {
                return false;
            }

            return loadTextures(context, bkd.getMat().getTextures());
        }

        return true;
    }

    public boolean setLighting(Context context, int lightingId) {
        Options.Lighting lighting = MiggyConst.lightingOptions.getOption(lightingId);
        if (lighting == null) {
            return false;
        }
        byte[] binLightsData = loadBinaryCached(context, lighting.getPackage(), "lights");
        return MiggyInterface.SetLighting(binLightsData, lighting.getAmbient());
    }

    public boolean setOrientation(Context context, int cameraId) {
        Options.Camera camera = MiggyConst.cameraOptions.getOption(cameraId);
        if (camera != null) {
            MiggyInterface.MatrixOps ops = new MiggyInterface.MatrixOps(
                    null, camera.getRotate(), camera.getTranslate()
            );
            MiggyInterface.SetOrientation(ops);
        }
        return true;
    }

    // loads text, backdrop, lighting and camera in one step
    public boolean setEnvironment(Context context, RenderConfig config) {
        if (!setText(context, config)) {
            return false;
        }

        if (!setBackdrop(context, config)) {
            return false;
        }

        if (!setLighting(context, config.lightingId)) {
            return false;
        }

        if (!setOrientation(context, config.cameraId)) {
            return false;
        }

        return true;
    }

    public boolean setBackground(BackgroundConfig config) {
        if (config == null) {
            config = new BackgroundConfig();
        }

        if (config.type == BackgroundConfig.Type.COLORS) {
            if (config.numberOfColors == 1) {
                MiggyInterface.SetBackground(config.middle, config.middle, config.middle);
            } else if (config.numberOfColors == 2) {
                double[] middle = {
                        (config.top[0] + config.bottom[0]) / 2,
                        (config.top[1] + config.bottom[1]) / 2,
                        (config.top[2] + config.bottom[2]) / 2,
                        1
                };
                MiggyInterface.SetBackground(config.top, middle, config.bottom);
            } else {
                MiggyInterface.SetBackground(config.top, config.middle, config.bottom);
            }
        } else if (config.type == BackgroundConfig.Type.IMAGE) {
            String resize = "stretch";
            if (config.scale == BackgroundConfig.ImageScale.SCALE_TO_FILL) {
                resize = "fill";
            } else if (config.scale == BackgroundConfig.ImageScale.SCALE_TO_FIT) {
                resize = "fit";
            }
            MiggyInterface.SetBackgroundImage(config.imagePath, resize);
        } else {
            double[] t = { 0, 0, 0, 0 };
            MiggyInterface.SetBackground(t, t, t);
        }

        return true;
    }

    public boolean startRender(Bitmap theBmp, MiggyInterface.IMiggyCallback callback) {
        this.callback = callback;
        return MiggyInterface.Render(theBmp, multiCore, this);
    }

    @Override
    public boolean miggyRenderStarted() {
        inRender = true;

        boolean ret = (callback == null || callback.miggyRenderStarted());
        return (ret && !abortRender);
    }

    @Override
    public boolean miggyRenderUpdate(int percent) {
        boolean ret = (callback == null || callback.miggyRenderUpdate(percent));
        return (ret && !abortRender);
    }

    @Override
    public void miggyRenderComplete() {
        inRender = false;
        abortRender = false;

        if (callback != null) {
            callback.miggyRenderComplete();
            callback = null;
        }

        if (abortCallback != null) {
            abortCallback.miggyRenderComplete();
            abortCallback = null;
        }
    }

    @Override
    public void miggyRenderAborted() {
        inRender = false;
        abortRender = false;

        if (callback != null) {
            callback.miggyRenderAborted();
            callback = null;
        }

        if (abortCallback != null) {
            abortCallback.miggyRenderAborted();
            abortCallback = null;
        }
    }
}

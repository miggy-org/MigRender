package com.jordan.textrender;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.HashMap;

// class for specifying options
public class Options<T extends Options.BaseOption> {
    public static final int FLAG_NONE  = 0x0;
    public static final int FLAG_COLOR_DIFFUSE = 0x1;
    public static final int FLAG_COLOR_SPECULAR = 0x2;
    public static final int FLAG_IMAGE = 0x4;

    protected ArrayList<Integer> orderedKeys;
    protected HashMap<Integer, T> optionMap;
    protected int defaultId;

    protected Options() {
        orderedKeys = new ArrayList<>();
        optionMap = new HashMap<>();
    }

    protected void addOption(Integer id, Integer nameId, @NonNull T option) {
        orderedKeys.add(id);
        optionMap.put(id, option);
        option.setNameId(nameId);
    }

    public ArrayList<Integer> getKeys() {
        return orderedKeys;
    }

    public int getDefaultId() {
        return defaultId;
    }

    public T getOption(int optionId) {
        if (optionMap.containsKey(optionId)) {
            return optionMap.get(optionId);
        }
        return optionMap.get(defaultId);
    }

    public int getOptionNameId(int optionId) {
        if (optionMap.containsKey(optionId)) {
            return optionMap.get(optionId).getNameId();
        }
        return optionMap.get(defaultId).getNameId();
    }

    public int getOptionFlags(int optionId) {
        return FLAG_NONE;
    }

    public static class BaseOption {
        private int nameId;

        public int getNameId() {
            return nameId;
        }

        public void setNameId(int nameId) {
            this.nameId = nameId;
        }
    }

    public static class Package extends BaseOption {
        private final int idPackage;

        public Package(int idPackage) {
            this.idPackage = idPackage;
        }

        public int getPackageId() {
            return idPackage;
        }
    }

    public static class Texture extends BaseOption {
        private final int idTexture;
        private final String slot;
        private final boolean alpha;

        public Texture(int idTexture, String slot, boolean alpha) {
            this.idTexture = idTexture;
            this.slot = slot;
            this.alpha = alpha;
        }

        public int getTextureId() {
            return idTexture;
        }

        public String getSlot() {
            return slot;
        }

        public boolean getAlpha() {
            return alpha;
        }
    }

    public static class Material extends BaseOption {
        private final Texture[] textures;
        private final double[] diffuse;
        private final double[] specular;
        private final double[] refract;
        private final double refractIndex;
        private final boolean autoReflect;
        private final boolean autoRefract;
        private final int flags;

        public Material(Texture[] textures) {
            this.textures = textures;
            this.diffuse = null;
            this.specular = null;
            this.refract = null;
            this.refractIndex = 1.0;
            this.autoReflect = false;
            this.autoRefract = false;
            this.flags = FLAG_NONE;
        }

        public Material(Texture[] textures, double[] diffuse, double[] specular, boolean autoReflect, int flags) {
            this.textures = textures;
            this.diffuse = diffuse;
            this.specular = specular;
            this.refract = null;
            this.refractIndex = 1.0;
            this.autoReflect = autoReflect;
            this.autoRefract = false;
            this.flags = flags;
        }

        public Material(Texture[] textures, double[] diffuse, double[] specular, double[] refract, double refractIndex, boolean autoReflect, boolean autoRefract, int flags) {
            this.textures = textures;
            this.diffuse = diffuse;
            this.specular = specular;
            this.refract = refract;
            this.refractIndex = refractIndex;
            this.autoReflect = autoReflect;
            this.autoRefract = autoRefract;
            this.flags = flags;
        }

        public Texture[] getTextures() {
            return textures;
        }

        public double[] getDiffuse() {
            return diffuse;
        }

        public double[] getSpecular() {
            return specular;
        }

        public double[] getRefract() {
            return refract;
        }

        public double getRefractIndex() {
            return refractIndex;
        }

        public boolean getAutoReflect() {
            return autoReflect;
        }

        public boolean getAutoRefract() {
            return autoRefract;
        }

        public int getFlags() {
            return flags;
        }
    }

    public static class FontMaterial extends Material {
        private final int scriptId;

        public FontMaterial(Material mat) {
            super(mat.textures, mat.diffuse, mat.specular, mat.refract, mat.refractIndex, mat.autoReflect, mat.autoRefract, mat.flags);
            this.scriptId = R.raw.font_simple;
        }

        public FontMaterial(Material mat, int scriptId) {
            super(mat.textures, mat.diffuse, mat.specular, mat.refract, mat.refractIndex, mat.autoReflect, mat.autoRefract, mat.flags);
            this.scriptId = scriptId;
        }

        public int getScriptId() {
            return scriptId;
        }
    }

    public static class Backdrop extends BaseOption {
        private final Package pkg;
        private final Material mat;

        public Backdrop(Package pkg, Material mat) {
            this.pkg = pkg;
            this.mat = mat;
        }

        public Package getPkg() {
            return pkg;
        }

        public Material getMat() {
            return mat;
        }
    }

    public static class Camera extends BaseOption {
        private final double[] rotate;
        private final double[] translate;

        public Camera(double[] rotate, double[] translate) {
            this.rotate = rotate;
            this.translate = translate;
        }

        public double[] getRotate() {
            return rotate;
        }

        public double[] getTranslate() {
            return translate;
        }
    }

    public static class Lighting extends BaseOption {
        private final Package pkg;
        private final double[] ambient;

        public Lighting(Package pkg) {
            this.pkg = pkg;
            this.ambient = null;
        }

        public Lighting(Package pkg, double[] ambient) {
            this.pkg = pkg;
            this.ambient = ambient;
        }

        public Package getPackage() {
            return pkg;
        }

        public double[] getAmbient() {
            return ambient;
        }
    }

    public static class Resolution extends BaseOption {
        private final int width;
        private final int height;

        public Resolution(int width, int height) {
            this.width = width;
            this.height = height;
        }

        public int getWidth() {
            return width;
        }

        public int getHeight() {
            return height;
        }
    }

    public static class PackageOptions extends Options<Package> {
    }

    public static class FontOptions extends PackageOptions {
        public FontOptions() {
            final int BaseId = 1000;

            int currId = BaseId;
            addOption(++currId, R.string.font_arial, new Package(R.raw.arial));
            addOption(++currId, R.string.font_calibri, new Package(R.raw.calibri));
            addOption(++currId, R.string.font_footlight, new Package(R.raw.footlight));
            addOption(++currId, R.string.font_french, new Package(R.raw.french));
            addOption(++currId, R.string.font_harlow, new Package(R.raw.harlow));
            addOption(++currId, R.string.font_informal, new Package(R.raw.informal));
            addOption(++currId, R.string.font_kunstler, new Package(R.raw.kunstler));
            addOption(++currId, R.string.font_magneto, new Package(R.raw.magneto));
            addOption(++currId, R.string.font_modern, new Package(R.raw.modern));
            addOption(++currId, R.string.font_oldenglish, new Package(R.raw.oldenglish));
            addOption(++currId, R.string.font_pristina, new Package(R.raw.pristina));
            addOption(++currId, R.string.font_ravie, new Package(R.raw.ravie));
            addOption(++currId, R.string.font_stencil, new Package(R.raw.stencil));
            addOption(++currId, R.string.font_times, new Package(R.raw.times));
            addOption(++currId, R.string.font_trebuchet, new Package(R.raw.trebuchet));
            addOption(++currId, R.string.font_vivaldi, new Package(R.raw.vivaldi));
            defaultId = BaseId + 14;
        }
    }

    public static class MaterialOptions extends Options<FontMaterial> {
        public MaterialOptions() {
            final int BaseId = 2000;

            int currId = BaseId;
            addOption(++currId, R.string.material_brick,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.brick, "texture1", false)
                                    }
                            )
                    )
            );
            addOption(++currId, R.string.material_fancy1,
                    new FontMaterial(
                            new Material(
                                new Texture[] {
                                        new Texture(R.raw.reflect2, "reflect", false)
                                },
                                new double[] { 1.0, 1.0, 1.0 },
                                new double[] { 0.2, 0.2, 0.2 },
                                false,
                                Options.FLAG_COLOR_DIFFUSE
                            ),
                            R.raw.font_complex1
                    )
            );
            addOption(++currId, R.string.material_fancy2,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.marble1, "texture1", false),
                                            new Texture(R.raw.reflect2, "reflect", false)
                                    },
                                    null,
                                    null,
                                    false,
                                    Options.FLAG_NONE
                            ),
                            R.raw.font_complex2
                    )
            );
            addOption(++currId, R.string.material_marble1,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.marble1, "texture1", false),
                                            new Texture(R.raw.reflect1, "reflect", false)
                                    }
                            )
                    )
            );
            addOption(++currId, R.string.material_marble2,
                    new FontMaterial(
                        new Material(
                                new Texture[] {
                                        new Texture(R.raw.marble2, "texture1", false),
                                        new Texture(R.raw.reflect2, "reflect", false)
                                }
                        )
                    )
            );
            addOption(++currId, R.string.material_metal,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.reflect1, "reflect", false)
                                    },
                                    null,
                                    new double[] { 1.0, 1.0, 1.0 },
                                    false,
                                    Options.FLAG_NONE
                            ),
                            R.raw.font_metal
                    )
            );
            addOption(++currId, R.string.plastic,
                    new FontMaterial(
                            new Material(
                                    null,
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 0.2, 0.2, 0.2 },
                                    false,
                                    Options.FLAG_COLOR_DIFFUSE
                            )
                    )
            );
            addOption(++currId, R.string.shiny_plastic,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.reflect1, "reflect", false)
                                    },
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 0.6, 0.6, 0.6 },
                                    false,
                                    Options.FLAG_COLOR_DIFFUSE
                            )
                    )
            );
            addOption(++currId, R.string.material_wood,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.wood, "texture1", false)
                                    }
                            )
                    )
            );
            addOption(++currId, R.string.test,
                    new FontMaterial(
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.wood, "texture1", false),
                                            new Texture(R.raw.stucco, "texture2", false),
                                            new Texture(R.raw.reflect2, "reflect", false)
                                    },
                                    null,
                                    null,
                                    false,
                                    Options.FLAG_NONE
                            ),
                            R.raw.font_accent
                    )
            );
            defaultId = BaseId + 8;
        }

        @Override
        public int getOptionFlags(int optionId) {
            return getOption(optionId).getFlags();
        }
    }

    public static class BackdropOptions extends Options<Backdrop> {
        private static final Package metal = new Package(R.raw.backdrop_metal);
        private static final Package slab = new Package(R.raw.backdrop_slab);
        private static final Package smoothSlab = new Package(R.raw.backdrop_smoothslab);
        private static final Package frame = new Package(R.raw.backdrop_frame);

        public BackdropOptions() {
            final int BaseId = 3000;

            int currId = BaseId;
            addOption(++currId, R.string.backdrop_none,
                    new Backdrop(null, null)
            );
            addOption(++currId, R.string.backdrop_brick,
                    new Backdrop(slab,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.brick, "bd-texture1", true)
                                    },
                                    null,
                                    new double[] { 0.2, 0.2, 0.2 },
                                    false,
                                    Options.FLAG_NONE
                            )
                    )
            );
            addOption(++currId, R.string.glass,
                    new Backdrop(smoothSlab,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.reflect1, "bd-reflect", true)
                                    },
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 0.8, 0.8, 0.8, 0.3 },
                                    1.2,
                                    true,
                                    true,
                                    Options.FLAG_COLOR_DIFFUSE
                            )
                    )
            );
            addOption(++currId, R.string.backdrop_gold,
                    new Backdrop(metal,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.reflect2, "bd-reflect", true)
                                    },
                                    null,
                                    new double[] { 1.0, 0.85, 0.0 },
                                    true,
                                    Options.FLAG_NONE
                            )
                    )
            );
            addOption(++currId, R.string.backdrop_marble,
                    new Backdrop(smoothSlab,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.marble2, "bd-texture1", true)
                                    },
                                    null,
                                    new double[] { 0.5, 0.5, 0.5 },
                                    true,
                                    Options.FLAG_NONE
                            )
                    )
            );
            addOption(++currId, R.string.backdrop_frame,
                    new Backdrop(frame,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.wood, "bd-texture1", true)
                                    },
                                    null,
                                    new double[] { 0.5, 0.5, 0.5 },
                                    false,
                                    Options.FLAG_IMAGE
                            )
                    )
            );
            addOption(++currId, R.string.bd_plastic,
                    new Backdrop(slab,
                            new Material(
                                    null,
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 1, 1, 1 },
                                    false,
                                    Options.FLAG_COLOR_DIFFUSE
                            )
                    )
            );
            addOption(++currId, R.string.bd_shiny_plastic,
                    new Backdrop(metal,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.reflect1, "bd-reflect", true)
                                    },
                                    new double[] { 0.5, 0.5, 0.5 },
                                    new double[] { 1, 1, 1 },
                                    true,
                                    Options.FLAG_COLOR_SPECULAR
                            )
                    )
            );
            addOption(++currId, R.string.backdrop_wood,
                    new Backdrop(smoothSlab,
                            new Material(
                                    new Texture[] {
                                            new Texture(R.raw.wood, "bd-texture1", true)
                                    },
                                    null,
                                    new double[] { 0.5, 0.5, 0.5 },
                                    false,
                                    Options.FLAG_NONE
                            )
                    )
            );
            defaultId = BaseId + 5;
        }

        @Override
        public int getOptionFlags(int optionId) {
            Backdrop backdrop = getOption(optionId);
            return (backdrop != null && backdrop.getMat() != null ? backdrop.getMat().getFlags() : FLAG_NONE);
        }
    }

    public static class LightingOptions extends Options<Lighting> {
        public LightingOptions() {
            final int BaseId = 4000;

            int currId = BaseId;
            addOption(++currId, R.string.lighting_none, new Lighting(null, new double[] { 1, 1, 1 }));
            addOption(++currId, R.string.lighting_single, new Lighting(new Package(R.raw.lights_single)));
            addOption(++currId, R.string.lighting_many, new Lighting(new Package(R.raw.lights_many)));
            defaultId = BaseId + 3;
        }
    }

    public static class CameraOptions extends Options<Camera> {
        public CameraOptions() {
            final int BaseId = 5000;

            int currId = BaseId;
            addOption(++currId, R.string.orientation_straight,
                    new Camera(null, null)
            );
            addOption(++currId, R.string.orientation_straight_up,
                    new Camera(
                            new double[] { 0, 0, 0 },
                            new double[] { 0, 2, 0 }
                    )
            );
            addOption(++currId, R.string.orientation_straight_down,
                    new Camera(
                            new double[] { 0, 0, 0 },
                            new double[] { 0, -2, 0 }
                    )
            );
            addOption(++currId, R.string.orientation_lookup,
                    new Camera(
                            new double[] { -50, 0, 0 },
                            new double[] { 0, 0, 0 }
                    )
            );
            addOption(++currId, R.string.orientation_lookup_above,
                    new Camera(
                            new double[] { -50, 0, 0 },
                            new double[] { 0, 3, 0 }
                    )
            );
            addOption(++currId, R.string.orientation_lookup_below,
                    new Camera(
                            new double[] { -50, 0, 0 },
                            new double[] { 0, -2.5, 0 }
                    )
            );
            addOption(++currId, R.string.orientation_angled,
                    new Camera(
                            new double[] { -50, 0, 30 },
                            new double[] { 0.5, 2, 0 }
                    )
            );
            defaultId = BaseId + 4;
        }
    }

    public static class ResolutionOptions extends Options<Resolution> {
        public ResolutionOptions() {
            final int BaseId = 6000;

            int currId = BaseId;
            addOption(++currId, R.string.resolution_small, new Resolution(640, 480));
            addOption(++currId, R.string.resolution_medium, new Resolution(1280, 960));
            addOption(++currId, R.string.resolution_large, new Resolution(2560, 1920));
            addOption(++currId, R.string.resolution_hd, new Resolution(1280, 720));
            addOption(++currId, R.string.resolution_fhd, new Resolution(1920, 1080));
            addOption(++currId, R.string.resolution_4k, new Resolution(3840, 2160));
            defaultId = BaseId + 2;
        }
    }
}

using System;
using System.Collections.Generic;
using System.Text;

namespace TextRender
{
    class MiggyParams
    {
        public static String[] fontNames = {
                                         "Arial",
                                         "Calibri",
                                         "Footlight",
                                         "French Script",
                                         "Harlow Solid",
                                         "Informal Roman",
                                         "Kunstler Script",
                                         "Magneto",
                                         "Modern",
                                         "Old English",
                                         "Pristina",
                                         "Ravie",
                                         "Stencil",
                                         "Times New Roman",
                                         "Trebuchet MS",
                                         "Vivaldi"
                                     };

        public static String[] textMaterials = {
                                         "Brick",
                                         "Marble 1",
                                         "Marble 2",
                                         "Wood"
                                     };

        public static String[] backdropNames = {
                                         "None",
                                         "Brick",
                                         "Gold",
                                         "Marble",
                                         "Wood"
                                     };

        public static String[] lightingNames = {
                                         "None",
                                         "Single",
                                         "Many"
                                     };

        public static String[] cameraNames = {
                                         "Straight",
                                         "Lookup",
                                         "Angled"
                                     };

        public static String fontModel(String fontName)
        {
            String model = "";
            if (fontName == fontNames[0])
                model = "Models\\arial.pkg";
            else if (fontName == fontNames[1])
                model = "Models\\calibri.pkg";
            else if (fontName == fontNames[2])
                model = "Models\\footlight.pkg";
            else if (fontName == fontNames[3])
                model = "Models\\french.pkg";
            else if (fontName == fontNames[4])
                model = "Models\\harlow.pkg";
            else if (fontName == fontNames[5])
                model = "Models\\informal.pkg";
            else if (fontName == fontNames[6])
                model = "Models\\kunstler.pkg";
            else if (fontName == fontNames[7])
                model = "Models\\magneto.pkg";
            else if (fontName == fontNames[8])
                model = "Models\\modern.pkg";
            else if (fontName == fontNames[9])
                model = "Models\\oldenglish.pkg";
            else if (fontName == fontNames[10])
                model = "Models\\pristina.pkg";
            else if (fontName == fontNames[11])
                model = "Models\\ravie.pkg";
            else if (fontName == fontNames[12])
                model = "Models\\stencil.pkg";
            else if (fontName == fontNames[13])
                model = "Models\\times.pkg";
            else if (fontName == fontNames[14])
                model = "Models\\trebuchet.pkg";
            else if (fontName == fontNames[15])
                model = "Models\\vivaldi.pkg";
            return model;
        }

        public static void textTextures(String matName, ref String texture, ref String reflect)
        {
            if (matName == textMaterials[0])
            {
                texture = "Images\\brick.jpg";
            }
            else if (matName == textMaterials[1])
            {
                texture = "Images\\marble1.jpg";
                reflect = "Images\\reflect1.jpg";
            }
            else if (matName == textMaterials[2])
            {
                texture = "Images\\marble2.jpg";
                reflect = "Images\\reflect2.jpg";
            }
            else if (matName == textMaterials[3])
            {
                texture = "Images\\wood.jpg";
            }
        }

        public static String backdropModel(String backdropName)
        {
            String model = "";
            if (backdropName == backdropNames[1])
            {
                model = "Models\\backdrop-slab.mdl";
            }
            else if (backdropName == backdropNames[2])
            {
                model = "Models\\backdrop-metal.mdl";
            }
            else if (backdropName == backdropNames[3])
            {
                model = "Models\\backdrop-smoothslab.mdl";
            }
            else if (backdropName == backdropNames[4])
            {
                model = "Models\\backdrop-slab.mdl";
            }
            return model;
        }

        public static void backdropTextures(String backdropName, ref String texture, ref String reflect)
        {
            if (backdropName == backdropNames[1])
            {
                texture = "Images\\brick.jpg";
            }
            else if (backdropName == backdropNames[2])
            {
                reflect = "Images\\reflect2.jpg";
            }
            else if (backdropName == backdropNames[3])
            {
                texture = "Images\\marble2.jpg";
            }
            else if (backdropName == backdropNames[4])
            {
                texture = "Images\\wood.jpg";
            }
        }

        public static void backdropParams(String backdropName, ref double specR, ref double specG, ref double specB, ref bool autoReflect)
        {
            if (backdropName == backdropNames[1])
            {
                specR = specG = specB = 0.2;
                autoReflect = false;
            }
            else if (backdropName == backdropNames[2])
            {
                specR = 1.0;
                specG = 0.85;
                specB = 0.0;
                autoReflect = true;
            }
            else if (backdropName == backdropNames[3])
            {
                specR = specG = specB = 0.5;
                autoReflect = true;
            }
            else if (backdropName == backdropNames[4])
            {
                specR = specG = specB = 0.5;
                autoReflect = false;
            }
        }

        public static String lightingModel(String lightingName)
        {
            String model = "";
            if (lightingName == lightingNames[1])
                model = "Models\\single-light.mdl";
            else if (lightingName == lightingNames[2])
                model = "Models\\many-lights.mdl";
            return model;
        }

        public static void orientParams(String orientName, ref double rX, ref double rY, ref double rZ, ref double tX, ref double tY, ref double tZ)
        {
            if (orientName == cameraNames[1])
            {
                rX = -50;
            }
            else if (orientName == cameraNames[2])
            {
                rX = -50;
                rZ = 30;
                tX = 0.5;
                tY = 2;
            }
        }
    }
}

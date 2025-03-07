#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <map>

#include <android/bitmap.h>
#include <android/log.h>

// MigRender headers
#include "core/model.h"
#include "core/fileio.h"
#include "core/jpegio.h"
#include "core/android.h"
#include "core/package.h"
#include "core-ext/parser.h"

using namespace std;
using namespace MigRender;

// enables multi-threaded rendering support
#define USE_MULTI_CORE

// globals for this module
CModel theModel;
CGrp* pMainGrp;

// initialize the model
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_Init( JNIEnv* env, jclass clazz,
		jboolean superSample, jboolean doShadows, jboolean softShadows, jboolean autoReflect )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::Init()");

    theModel.Initialize();

    // create the top level group that will hold the lights, text and backdrop
    pMainGrp = theModel.GetSuperGroup().CreateSubGroup();

    // init camera
    theModel.GetCamera().GetTM().RotateY(180).Translate(0, 0, 15);

    // default background colors
    COLOR cn(1.0, 0.0, 0.0, 1.0);
    COLOR ce(0.0, 0.0, 0.2, 1.0);
    COLOR cs(0.0, 1.0, 0.0, 1.0);
    theModel.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

    // default ambient light
    COLOR ambient(0.35, 0.35, 0.35);
    theModel.SetAmbientLight(ambient);

    // super-sampling
    theModel.SetSampling(superSample ? SuperSample::X5 : SuperSample::X1);

    // rendering flags
    dword rendFlags = REND_NONE;
    if (doShadows)
        rendFlags |= REND_AUTO_SHADOWS;
    if (doShadows && softShadows)
        rendFlags |= REND_SOFT_SHADOWS;
    if (autoReflect)
        rendFlags |= (REND_AUTO_REFLECT | REND_AUTO_REFRACT);
    theModel.SetRenderQuality(rendFlags);

    return true;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetBackground(JNIEnv *env, jclass clazz,
        jdoubleArray n, jdoubleArray e, jdoubleArray s)
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetBackground()");
    if (env->GetArrayLength(n) < 4 || env->GetArrayLength(e) < 4 || env->GetArrayLength(s) < 4) {
        __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
        return false;
    }
    double* nPtr = env->GetDoubleArrayElements(n, NULL);
    double* ePtr = env->GetDoubleArrayElements(e, NULL);
    double* sPtr = env->GetDoubleArrayElements(s, NULL);

    COLOR cn(nPtr[0], nPtr[1], nPtr[2], nPtr[3]);
    COLOR ce(ePtr[0], ePtr[1], ePtr[2], ePtr[3]);
    COLOR cs(sPtr[0], sPtr[1], sPtr[2], sPtr[3]);
    theModel.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

    env->ReleaseDoubleArrayElements(n, nPtr, 0);
    env->ReleaseDoubleArrayElements(e, ePtr, 0);
    env->ReleaseDoubleArrayElements(s, sPtr, 0);

    return true;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetBackgroundImage(JNIEnv *env, jclass clazz,
        jstring imagePath, jstring resize)
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetBackgroundImage()");
    const char* ppath = env->GetStringUTFChars(imagePath, NULL);
    if (ppath != NULL)
    {
        string bgtitle = theModel.GetImageMap().LoadImageFile(ppath, ImageFormat::None, "bgimage");
        if (!bgtitle.empty()) {
            ImageResize reSize = ImageResize::Stretch;
            const char* presize = env->GetStringUTFChars(resize, NULL);
            if (presize != NULL) {
                if (strcmp(presize, "fit") == 0) {
                    reSize = ImageResize::ScaleToFit;
                } else if (strcmp(presize, "fill") == 0) {
                    reSize = ImageResize::ScaleToFill;
                }
            }

            COLOR black(0, 0, 0, 1);
            theModel.GetBackgroundHandler().SetBackgroundColors(black, black, black);
            theModel.GetBackgroundHandler().SetBackgroundImage(bgtitle, reSize);
        }
    }
    return true;
}

struct ObjParams {
    COLOR* diff;
    COLOR* spec;
    COLOR* refl;
    COLOR* refr;
    COLOR* glow;
    double index;
    bool autoReflect;
    bool autoRefract;

    ObjParams() {
        diff = NULL;
        spec = NULL;
        refl = NULL;
        refr = NULL;
        glow = NULL;
        index = 1.0;
        autoReflect = false;
        autoRefract = false;
    }

    ~ObjParams() {
        if (diff != NULL) delete diff;
        if (spec != NULL) delete spec;
        if (refl != NULL) delete refl;
        if (refr != NULL) delete refr;
        if (glow != NULL) delete glow;
    }
};

static bool ConvertObjParams(JNIEnv* env, jobject params, ObjParams& objParams) {
    jclass classParams = env->GetObjectClass(params);
    jfieldID diffFieldID = env->GetFieldID(classParams, "diff", "[D");
    jfieldID specFieldID = env->GetFieldID(classParams, "spec", "[D");
    jfieldID reflFieldID = env->GetFieldID(classParams, "refl", "[D");
    jfieldID refrFieldID = env->GetFieldID(classParams, "refr", "[D");
    jfieldID glowFieldID = env->GetFieldID(classParams, "glow", "[D");
    jfieldID indexFieldID = env->GetFieldID(classParams, "index", "D");
    jfieldID autoReflectFieldID = env->GetFieldID(classParams, "autoReflect", "Z");
    jfieldID autoRefractFieldID = env->GetFieldID(classParams, "autoRefract", "Z");

    // get the diffuse color
    auto diffArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, diffFieldID));
    if (diffArray != NULL) {
        if (env->GetArrayLength(diffArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* diffPtr = env->GetDoubleArrayElements(diffArray, NULL);
        objParams.diff = new COLOR(diffPtr[0], diffPtr[1], diffPtr[2], 1.0);
        env->ReleaseDoubleArrayElements(diffArray, diffPtr, 0);
    }
    // get the specular color
    auto specArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, specFieldID));
    if (specArray != NULL) {
        if (env->GetArrayLength(specArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* specPtr = env->GetDoubleArrayElements(specArray, NULL);
        objParams.spec = new COLOR(specPtr[0], specPtr[1], specPtr[2], 1.0);
        env->ReleaseDoubleArrayElements(specArray, specPtr, 0);
    }
    // get the reflection color
    auto reflArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, reflFieldID));
    if (reflArray != NULL) {
        if (env->GetArrayLength(reflArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* reflPtr = env->GetDoubleArrayElements(reflArray, NULL);
        objParams.refl = new COLOR(reflPtr[0], reflPtr[1], reflPtr[2], 1.0);
        env->ReleaseDoubleArrayElements(reflArray, reflPtr, 0);
    }
    // get the refraction color
    auto refrArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, refrFieldID));
    if (refrArray != NULL) {
        if (env->GetArrayLength(refrArray) < 4) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* refrPtr = env->GetDoubleArrayElements(refrArray, NULL);
        objParams.refr = new COLOR(refrPtr[0], refrPtr[1], refrPtr[2], refrPtr[3]);
        env->ReleaseDoubleArrayElements(refrArray, refrPtr, 0);
    }
    // get the glow color
    auto glowArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, glowFieldID));
    if (glowArray != NULL) {
        if (env->GetArrayLength(glowArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* glowPtr = env->GetDoubleArrayElements(glowArray, NULL);
        objParams.glow = new COLOR(glowPtr[0], glowPtr[1], glowPtr[2], 1.0);
        env->ReleaseDoubleArrayElements(glowArray, glowPtr, 0);
    }
    objParams.index = env->GetDoubleField(params, indexFieldID);
    objParams.autoReflect = env->GetBooleanField(params, autoReflectFieldID);
    objParams.autoRefract = env->GetBooleanField(params, autoRefractFieldID);
    return true;
}

struct MatrixOps {
    double* scale;
    double* rotate;
    double* translate;

    MatrixOps() {
        scale = NULL;
        rotate = NULL;
        translate = NULL;
    }

    ~MatrixOps() {
        if (scale != NULL) delete scale;
        if (rotate != NULL) delete rotate;
        if (translate != NULL) delete translate;
    }
};

static bool ConvertMatrixOps(JNIEnv* env, jobject params, MatrixOps& matrixOps) {
    jclass classParams = env->GetObjectClass(params);
    jfieldID scaleFieldID = env->GetFieldID(classParams, "scale", "[D");
    jfieldID rotateFieldID = env->GetFieldID(classParams, "rotate", "[D");
    jfieldID translateFieldID = env->GetFieldID(classParams, "translate", "[D");

    // get the scale
    auto scaleArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, scaleFieldID));
    if (scaleArray != NULL) {
        if (env->GetArrayLength(scaleArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* scalePtr = env->GetDoubleArrayElements(scaleArray, NULL);
        matrixOps.scale = new double[3] { scalePtr[0], scalePtr[1], scalePtr[2] };
        env->ReleaseDoubleArrayElements(scaleArray, scalePtr, 0);
    }
    // get the rotate
    auto rotateArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, rotateFieldID));
    if (rotateArray != NULL) {
        if (env->GetArrayLength(rotateArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* rotatePtr = env->GetDoubleArrayElements(rotateArray, NULL);
        matrixOps.rotate = new double[3] { rotatePtr[0], rotatePtr[1], rotatePtr[2] };
        env->ReleaseDoubleArrayElements(rotateArray, rotatePtr, 0);
    }
    // get the translate
    auto translateArray = reinterpret_cast<jdoubleArray>(env->GetObjectField(params, translateFieldID));
    if (translateArray != NULL) {
        if (env->GetArrayLength(translateArray) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* translatePtr = env->GetDoubleArrayElements(translateArray, NULL);
        matrixOps.translate = new double[3] { translatePtr[0], translatePtr[1], translatePtr[2] };
        env->ReleaseDoubleArrayElements(translateArray, translatePtr, 0);
    }
    return true;
}

static bool CheckMetaForAttribute(const CBaseObj& obj, const char* attribute)
{
    char szValue[20];
    if (obj.GetMetaData(attribute, szValue, sizeof(szValue)))
    {
        if (strcmp(szValue, "true") == 0)
            return true;
    }
    return false;
}

static void LoadObjParams(const ObjParams& objParams, CObj& obj) {
    if (!CheckMetaForAttribute(obj, "fixedProps"))
    {
        if (objParams.diff != NULL)
            obj.SetDiffuse(obj.GetDiffuse() * (*objParams.diff));
        if (objParams.spec != NULL)
            obj.SetSpecular(obj.GetSpecular() * (*objParams.spec));
        if (objParams.refl != NULL)
            obj.SetReflection(obj.GetReflection() * (*objParams.refl));
        if (objParams.refr != NULL)
            obj.SetRefraction(obj.GetRefraction() * (*objParams.refr), objParams.index);
        if (objParams.glow != NULL)
            obj.SetGlow(obj.GetGlow() * (*objParams.glow));

        dword flags = OBJF_SHADOW_RAY | OBJF_USE_BBOX;
        if (objParams.autoReflect)
            flags |= OBJF_REFL_RAY;
        if (objParams.autoRefract)
            flags |= OBJF_REFR_RAY;
        obj.SetObjectFlags(flags);
    }
}

static void LoadObjParams(const ObjParams& objParams, CGrp& grp)
{
    if (!CheckMetaForAttribute(grp, "fixedProps"))
    {
        for (const auto & iter : grp.GetSubGroups())
        {
            LoadObjParams(objParams, *iter);
        }
        for (const auto & iter : grp.GetObjects())
        {
            LoadObjParams(objParams, *iter);
        }
    }
}

static void LoadMatrixOps(const MatrixOps& ops, CGrp& grp)
{
    if (!CheckMetaForAttribute(grp, "suppressAspectScaling"))
    {
        if (ops.scale != NULL)
            grp.GetTM().Scale(ops.scale[0], ops.scale[1], ops.scale[2]);
        if (ops.rotate != NULL)
        {
            grp.GetTM().RotateZ(ops.rotate[2]);
            grp.GetTM().RotateX(ops.rotate[0]);
            grp.GetTM().RotateY(ops.rotate[1]);
        }
        if (ops.translate != NULL)
            grp.GetTM().Translate(ops.translate[0], ops.translate[1], ops.translate[2]);
    }
}

static CGrp* LoadTextString(CPackage& package, const char* szSuffix, const char* szScript,
                            CGrp* pgrp, const char* szText, int len, const ObjParams& objParams)
{
    // this will hold the text for this line
    CGrp* psub = pgrp->CreateSubGroup();

    int incx = 0, incy = 0;
    for (int n = 0; n < len; n++)
    {
        char item[20];
        memset(item, 0, sizeof(item));
        item[0] = szText[n];
        item[1] = '-';
        if (szSuffix != NULL && strlen(szSuffix) < 18)
            strcat(item, szSuffix);

        // we expect all objects in this package to be groups
        //if (package.GetObjectType(item) == BlockType::Polygon)
        if (package.GetObjectType(item) == BlockType::Group)
        {
            // load it
            //CPolygon* pobj = psub->CreatePolygon();
            CGrp* ptextgrp = psub->CreateSubGroup();
            package.LoadObject(item, ptextgrp);

            if (szScript != NULL && strlen(szScript) > 0)
            {
                // helps modulate the uv coordinates of the texture mappings
                int xtxtdiv = 2, ytxtdiv = 2;
                UVC uvmin, uvmax;
                uvmin.u = (double) (n%xtxtdiv) / xtxtdiv;
                uvmin.v = (double) ((n/ytxtdiv)%ytxtdiv) / ytxtdiv;
                uvmax.u = uvmin.u + (1.0/xtxtdiv);
                uvmax.v = uvmin.v + (1.0/ytxtdiv);

                CParser parser(&theModel);
                parser.SetEnvVarModelRef<CGrp>("grp", ptextgrp);
                parser.SetEnvVarModelRef<CPolygon>("poly", (CPolygon*) ptextgrp->GetObjects()[0].get());
                parser.SetEnvVar<UVC>("uvmin", uvmin);
                parser.SetEnvVar<UVC>("uvmax", uvmax);
                parser.ParseCommandScriptString(szScript);
            }

            LoadObjParams(objParams, *ptextgrp);

            // translate the polygon to the next position in the line
            ptextgrp->GetTM().Translate(incx, incy, 0);

            // use meta-data to determine the next position
            char buf[16];
            ptextgrp->GetMetaData("cellinc", buf, sizeof(buf));
            char* pcomma = strchr(buf, ',');
            if (pcomma != NULL)
            {
                *pcomma = '\0';
                incx += atoi(buf);
                incy += atoi(++pcomma);
            }
        }
    }

    return psub;
}

static void CenterAndScaleGroups(vector<CGrp*>& listOfGroups)
{
    // using bounding boxes, center the text
    double totalHeight = 0, totalScale = 0;
    for (int i = 0; i < (int) listOfGroups.size(); i++)
    {
        CBoundBox bbox;
        if (listOfGroups[i]->ComputeBoundBox(bbox))
        {
            CPt center = bbox.GetCenter();
            CPt minpt = bbox.GetMinPt();
            CPt maxpt = bbox.GetMaxPt();
            double width = maxpt.x - minpt.x;
            double scale = min(10 / width, 0.1);
            if (totalScale == 0 || scale < totalScale)
                totalScale = scale;

            listOfGroups[i]->GetTM().Translate(-center.x, -center.y, -center.z);
            //listOfGroups[i]->GetTM().Scale(scale, scale, scale);

            totalHeight += (bbox.GetMaxPt().y - bbox.GetMinPt().y);
        }
    }

    // scale it to a common scale factor
    for (int i = 0; i < (int) listOfGroups.size(); i++)
        listOfGroups[i]->GetTM().Scale(totalScale, totalScale, totalScale);

    // account for 2 or 3 lines of text, that's all we will handle
    if (listOfGroups.size() == 2)
    {
        listOfGroups[0]->GetTM().Translate(0, 0.30*totalHeight*totalScale, 0);
        listOfGroups[1]->GetTM().Translate(0, -0.30*totalHeight*totalScale, 0);
    }
    else if (listOfGroups.size() == 3)
    {
        listOfGroups[0]->GetTM().Translate(0, 0.35*totalHeight*totalScale, 0);
        listOfGroups[2]->GetTM().Translate(0, -0.35*totalHeight*totalScale, 0);
    }
}

// load a text string using a binary package
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetText( JNIEnv* env, jclass clazz,
                                                   jstring text, jstring suffix, jstring script,
                                                   jbyteArray pkgData, jobject params )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetText()");

    // open the package file
    //jint len = env->GetArrayLength(pkgData);
    jbyte* pdata = env->GetByteArrayElements(pkgData, 0);
    CMemFile memFile((byte*) pdata);
    CPackage package;
    int nchars = package.OpenPackage(&memFile);
    if (nchars == 0)
        return false;

    // the suffix refers to the lookup table in the package, can be NULL
    const char* psuffix = (suffix != NULL ? env->GetStringUTFChars(suffix, NULL) : NULL);

    // the script is used to process the loaded text objects, can be NULL
    const char* pscript = (script != NULL ? env->GetStringUTFChars(script, NULL) : NULL);

    // get the string characters from Java
    //jint textlen = env->GetStringLength(text);
    const char* ptext = env->GetStringUTFChars(text, NULL);
    if (ptext != NULL)
    {
        vector<CGrp*> listOfGroups;

        // copy to a temporary buffer
        char szText[80];
        strcpy(szText, ptext);

        ObjParams objParams;
        if (ConvertObjParams(env, params, objParams)) {
            // we will break up the string into multiple lines if it's too long and there are spaces
            int len = (int) strlen(szText);
            int minThreshold = len/3;
            if (len > minThreshold)
            {
                char szLine[80];
                szLine[0] = 0;

                char* context;
                char* szTok = strtok_r(szText, " -\n", &context);
                while (szTok != NULL) {
                    if (szLine[0] != 0) {
                        strcat(szLine, " ");
                    }
                    strcat(szLine, szTok);

                    len = (int) strlen(szLine);
                    if (len >= minThreshold) {
                        CGrp* psub = LoadTextString(package, psuffix, pscript, pMainGrp, szLine, len, objParams);
                        if (psub != NULL)
                            listOfGroups.push_back(psub);

                        szLine[0] = 0;
                        len = 0;
                    }

                    szTok = strtok_r(NULL, " -\n", &context);
                }

                if (len > 0) {
                    CGrp* psub = LoadTextString(package, psuffix, pscript, pMainGrp, szLine, len, objParams);
                    if (psub != NULL)
                        listOfGroups.push_back(psub);
                }
            }
            else
            {
                // less than a certain minimum and we just keep it all on one line
                CGrp* psub = LoadTextString(package, psuffix, pscript, pMainGrp, szText, len, objParams);
                if (psub != NULL)
                    listOfGroups.push_back(psub);
            }
        }

        // center and scale the lines of text
        CenterAndScaleGroups(listOfGroups);

        env->ReleaseStringUTFChars(text, ptext);
    }

    return true;
}

// load a backdrop using a binary model
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetBackdrop( JNIEnv* env, jclass clazz,
                                                       jbyteArray mdlData, jobject params, jobject ops )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetBackdrop()");

    bool bRet = false;

    //jint len = env->GetArrayLength(mdlData);
    jbyte* pdata = env->GetByteArrayElements(mdlData, 0);
    CMemFile memFile((byte*) pdata);
    if (memFile.OpenFile(false, MigType::Object))
    {
        //if (memFile.ReadNextBlockType() == BlockType::Polygon)
        if (memFile.ReadNextBlockType() == BlockType::Group)
        {
            //CPolygon* ppoly = pMainGrp->CreatePolygon();
            CGrp* pnewgrp = pMainGrp->CreateSubGroup();
            pnewgrp->Load(memFile);

            ObjParams objParams;
            if (ConvertObjParams(env, params, objParams)) {
                LoadObjParams(objParams, *pnewgrp);
                bRet = true;
            }

            MatrixOps matrixOps;
            if (ConvertMatrixOps(env, ops, matrixOps)) {
                LoadMatrixOps(matrixOps, *pnewgrp);
            }
        }

        memFile.CloseFile();
    }

    return bRet;
}

// load a lighting scheme using a binary model
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetLighting( JNIEnv* env, jclass clazz,
		jbyteArray mdlData, jdoubleArray a )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetLighting()");

    bool bRet = true;

    if (mdlData != NULL) {
        bRet = false;

        //jint len = env->GetArrayLength(mdlData);
        jbyte* pdata = env->GetByteArrayElements(mdlData, 0);
        CMemFile memFile((byte*) pdata);
        if (memFile.OpenFile(false, MigType::Object))
        {
            if (memFile.ReadNextBlockType() == BlockType::Group)
            {
                CGrp* plightgrp = pMainGrp->CreateSubGroup();
                plightgrp->Load(memFile);
                bRet = true;
            }

            memFile.CloseFile();
        }
    }

    if (a != NULL) {
        if (env->GetArrayLength(a) < 3) {
            __android_log_print(ANDROID_LOG_ERROR, "MIGGY-NDK", "Invalid array length");
            return false;
        }
        double* aPtr = env->GetDoubleArrayElements(a, NULL);
        COLOR ambient(aPtr[0], aPtr[1], aPtr[2]);
        env->ReleaseDoubleArrayElements(a, aPtr, 0);
        theModel.SetAmbientLight(ambient);
    }

    return bRet;
}

// set the main group orientation
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_SetOrientation( JNIEnv* env, jclass clazz,
                                                          jobject ops )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::SetOrientation()");

    pMainGrp->GetTM().SetIdentity();

    MatrixOps matrixOps;
    if (ConvertMatrixOps(env, ops, matrixOps)) {
        LoadMatrixOps(matrixOps, *pMainGrp);
    }

    return true;
}

// load an image into a slot within the image map
extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_LoadImage( JNIEnv* env, jclass clazz,
		jstring slot, jobject fdSys, jlong off, jlong len, jboolean createAlpha )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::LoadImage()");

    bool bRet = false;

    jclass fdClass = env->FindClass("java/io/FileDescriptor");
    if (fdClass != NULL)
    {
        //jclass fdClassRef = (jclass) env->NewGlobalRef(fdClass);

        jfieldID fdClassDescriptorFieldID = env->GetFieldID(fdClass, "descriptor", "I");
        if (fdClassDescriptorFieldID != NULL && fdSys != NULL)
        {
            jint fd = env->GetIntField(fdSys, fdClassDescriptorFieldID);
            int myfd = dup(fd);

            FILE* pFile = fdopen(myfd, "rb");
            if (pFile != NULL)
            {
                fseek(pFile, off, SEEK_SET);

                CJPEGLoader jpegLoader;
                const char* ptitle = env->GetStringUTFChars(slot, NULL);
                if (theModel.GetImageMap().LoadImageFile(pFile, jpegLoader, ptitle,
                                                         (createAlpha ? ImageFormat::RGBA : ImageFormat::RGB)).length() > 0)
                {
                    bRet = true;

                    // create an alpha channel from the image colors, if necessary
                    if (createAlpha)
                    {
                        CImageBuffer* pmap = theModel.GetImageMap().GetImage(ptitle);
                        if (pmap != NULL)
                        {
                            pmap->FillAlphaFromColors();
                            pmap->NormalizeAlpha();
                        }
                        else
                            bRet = false;
                    }
                }
                env->ReleaseStringUTFChars(slot, ptitle);

                fclose(pFile);
            }
        }
    }

    return bRet;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_LoadImagePath(JNIEnv *env, jclass clazz,
                                                        jstring slot, jstring imagePath, jboolean createAlpha)
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::LoadImagePath()");
    bool bRet = false;

    const char* ppath = env->GetStringUTFChars(imagePath, NULL);
    if (ppath != NULL)
    {
        const char* pslot = env->GetStringUTFChars(slot, NULL);
        if (pslot != NULL) {
            string title = theModel.GetImageMap().LoadImageFile(ppath,
                                                                (createAlpha ? ImageFormat::RGBA : ImageFormat::RGB), pslot);
            if (!title.empty()) {
                bRet = true;

                // create an alpha channel from the image colors, if necessary
                if (createAlpha)
                {
                    CImageBuffer* pmap = theModel.GetImageMap().GetImage(title);
                    if (pmap != NULL)
                    {
                        pmap->FillAlphaFromColors();
                        pmap->NormalizeAlpha();
                    }
                    else
                        bRet = false;
                }
            }

            env->ReleaseStringUTFChars(slot, pslot);
        }

        env->ReleaseStringUTFChars(imagePath, ppath);
    }

    return bRet;
}

static JavaVM* _javaVM;
static jmethodID _startedMethodID;
static jmethodID _updateMethodID;
static jmethodID _completedMethodID;
static jmethodID _abortedMethodID;
static jobject _callbackObj;
static jobject _bitmap;
static CAndroidBitmapTarget* pbmTarget;
static bool _renderActive;
static bool _renderAborted;
static pthread_cond_t cvComplete = PTHREAD_COND_INITIALIZER;

class CTrackProgress : public CRenderProgress
{
public:
    double percent;

public:
    virtual bool PreRender(int nthreads) { return true; }
    virtual bool LineComplete(double percent) { this->percent = percent; return true; }
    virtual void PostRender(bool success) { }
};
CTrackProgress trackProgress;

int getThreadCount()
{
#ifdef USE_MULTI_CORE
    return sysconf( _SC_NPROCESSORS_ONLN );
#else
    return 1;
#endif // USE_MULTI_CORE
}

void *workerThreadFunc(void *pArg)
{
    long nthread = (long) pArg;
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "Worker thread started (%d)", (int) nthread);

    theModel.DoRender((int) nthread, &trackProgress);

    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "Worker thread complete (%d)", (int) nthread);
    return NULL;
}

void *updateUIThreadFunc(void *pArg)
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "Update thread started");

    JNIEnv* env;
    if (_javaVM->AttachCurrentThread(&env, NULL) == 0)
    {
        CAndroidBitmapTarget* bmpt = (CAndroidBitmapTarget*) pArg;

        pthread_mutex_t uiMutex = PTHREAD_MUTEX_INITIALIZER;
        while (_renderActive)
        {
            struct timeval tp;
            gettimeofday(&tp, NULL);
            struct timespec ts;
            ts.tv_sec = tp.tv_sec + 1;  // one second in the future
            ts.tv_nsec = tp.tv_usec*1000;

            // wait for the period above, or signal that the render is complete
            pthread_mutex_lock(&uiMutex);
            pthread_cond_timedwait(&cvComplete, &uiMutex, &ts);
            pthread_mutex_unlock(&uiMutex);

            if (_renderActive) {
                if (bmpt != NULL)
                    bmpt->UnlockPixels(true, env);
                bool ok = false;
                if (env->GetObjectRefType(_callbackObj) != JNIInvalidRefType) {
                    ok = env->CallBooleanMethod(_callbackObj, _updateMethodID, (int) trackProgress.percent);
                }
                if (bmpt != NULL)
                    bmpt->LockPixels(true, env);

                if (!ok) {
                    _renderAborted = true;
                    if (bmpt != NULL)
                        bmpt->Abort();
                }
            }
        }

        _javaVM->DetachCurrentThread();
    }

    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "Update thread complete");
    return NULL;
}

void *renderThreadFunc(void *pArg)
{
    JNIEnv* env;
    if (_javaVM->AttachCurrentThread(&env, NULL) == 0)
    {
        // signal the UI that the render is starting
        if (env->CallBooleanMethod(_callbackObj, _startedMethodID))
        {
            pbmTarget = new CAndroidBitmapTarget(env, _bitmap);

            // set the camera viewport
            REND_INFO rinfo = pbmTarget->GetRenderInfo();
            double aspect = (rinfo.width / (double) rinfo.height);
            double minvp = 10;
            double ulen = (aspect > 1 ? minvp*aspect : minvp);
            double vlen = (aspect < 1 ? minvp/aspect : minvp);
            double dist = 15;
            theModel.GetCamera().SetViewport(ulen, vlen, dist);

            // start the render
            theModel.SetRenderTarget(pbmTarget);

            // get the thread count (should be one per physical processor)
            int nthreads = (((bool) pArg) ? getThreadCount() : 1);
            __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "Thread count is %d", nthreads);
            if (nthreads > 0)
            {
                // signal pre-render
                theModel.PreRender(nthreads);
                _renderActive = true;
                _renderAborted = false;

                // create the worker threads
                pthread_t* workerThreadRefs = new pthread_t[nthreads];
                for (long i = 0; i < nthreads; i++)
                    pthread_create(&workerThreadRefs[i], NULL, workerThreadFunc, (void*) i);

                // create the UI update thread
                pthread_t uiThreadRef;
                pthread_create(&uiThreadRef, NULL, updateUIThreadFunc, (void*) pbmTarget);

                // wait for the worker threads to all finish
                for (int j = 0; j < nthreads; j++)
                    pthread_join(workerThreadRefs[j], NULL);

                // signal end of render
                _renderActive = false;
                pthread_cond_signal(&cvComplete);
                theModel.PostRender(true);

                // wait for the UI thread to finish
                pthread_join(uiThreadRef, NULL);
            }

            // signal the UI that the render is complete or aborted
            if (env->GetObjectRefType(_callbackObj) != JNIInvalidRefType) {
                if (!_renderAborted) {
                    env->CallVoidMethod(_callbackObj, _completedMethodID);
                } else {
                    env->CallVoidMethod(_callbackObj, _abortedMethodID);
                }
            }

            CAndroidBitmapTarget* plocal = pbmTarget;
            pbmTarget = NULL;
            delete plocal;
        }

        env->DeleteGlobalRef(_bitmap);
        env->DeleteGlobalRef(_callbackObj);
        _javaVM->DetachCurrentThread();
    }

    return NULL;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_Render( JNIEnv* env, jclass clazz,
		jobject bitmap, jboolean multiCore, jobject callbackObj )
{
    __android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "MiggyInterface::Render()");

    jboolean ret = false;
    if (env->GetJavaVM(&_javaVM) == 0)
    {
        jclass classID = env->FindClass("com/jordan/textrender/MiggyInterface$IMiggyCallback");
        if (classID != NULL)
        {
            _startedMethodID = (env)->GetMethodID(classID, "miggyRenderStarted", "()Z");
            _updateMethodID = (env)->GetMethodID(classID, "miggyRenderUpdate", "(I)Z");
            _completedMethodID = (env)->GetMethodID(classID, "miggyRenderComplete", "()V");
            _abortedMethodID = (env)->GetMethodID(classID, "miggyRenderAborted", "()V");
            if (_startedMethodID != NULL && _updateMethodID != NULL && _completedMethodID != NULL && _abortedMethodID != NULL)
            {
                _callbackObj = env->NewGlobalRef(callbackObj);
                _bitmap = env->NewGlobalRef(bitmap);

                pthread_t threadRef;
                if (!pthread_create(&threadRef, NULL, renderThreadFunc, (void*) (int64_t) multiCore))
                    ret = true;
            }
        }
    }

    return ret;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_jordan_textrender_MiggyInterface_Abort( JNIEnv* env, jclass clazz )
{
    if (pbmTarget != NULL) {
        _renderAborted = true;
        pbmTarget->Abort();
    }

    return true;
}

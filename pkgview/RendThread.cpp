#include "StdAfx.h"
#include "Reg.h"

#include "rendthread.h"
#include "model.h"
#include "bmpio.h"
#include "fileio.h"
#include "package.h"
#include "parser.h"

#define USE_MULTI_CORE

using namespace MigRender;

static void CreateBackdrop(CModel& model, CGrp* pgrp)
{
	/*{
		CPolygon* ppoly = pgrp->CreatePolygon();
		ppoly->SetDiffuse(COLOR(0.7, 0.7, 0.7));
		ppoly->SetSpecular(COLOR(0.5, 0.5, 0.5));
		ppoly->SetBoundBox(true);
		ppoly->SetObjectFlags(OBJF_SHADOW_RAY | OBJF_REFL_RAY);

		int p1 = ppoly->LoadLatticePt(CPt(-7, -3, 0));
		int p2 = ppoly->LoadLatticePt(CPt( 7, -3, 0));
		int p3 = ppoly->LoadLatticePt(CPt( 7,  3, 0));
		int p4 = ppoly->LoadLatticePt(CPt(-7,  3, 0));

		ppoly->AddPolyCurve(CUnitVector(0, 0, 1), PCF_NEWPLANE, NULL, NULL, 0);
		ppoly->AddPolyCurveIndex(p1);
		ppoly->AddPolyCurveIndex(p2);
		ppoly->AddPolyCurveIndex(p3);
		ppoly->AddPolyCurveIndex(p4);
		ppoly->LoadComplete();
		ppoly->Extrude(1, false);

		CImageMap* pimages = model.GetImageMap();
		string dmap = model.GetImageMap().LoadImageFile("..\\images\\marble2.jpg", RGBA);

		CImageBuffer* pmap = model.GetImageMap().GetImage(dmap);
		pmap->FillAlphaFromColors();
		pmap->NormalizeAlpha();

		int mapping1 = ppoly->AddColorMap(Diffuse, TXTF_RGB, dmap);
		int mapping2 = ppoly->AddBumpMap(TXTF_ALPHA | TXTF_INVERT, dmap, 1);

		CPt center(0, 0, 1);
		ppoly->ApplyMapping(Extrusion, Diffuse, mapping1, true, &center, NULL);
		ppoly->ApplyMapping(Extrusion, Bump, mapping2, true, &center, NULL);

		CMatrix* ptm = ppoly->GetTM();
		ptm->Translate(0, 0, -2);
	}*/

	{
		CPolygon* ppoly = pgrp->CreatePolygon();
		ppoly->SetDiffuse(COLOR(0.2, 0.2, 0.2));
		ppoly->SetSpecular(COLOR(1.0, 0.85, 0.0));
		ppoly->SetReflection(COLOR(0.3, 0.3, 0.3));
		ppoly->SetBoundBox(true);
		ppoly->SetObjectFlags(OBJF_SHADOW_RAY | OBJF_REFL_RAY);

		int p1 = ppoly->LoadLatticePt(CPt(-7, -3, 0));
		int p2 = ppoly->LoadLatticePt(CPt( 7, -3, 0));
		int p3 = ppoly->LoadLatticePt(CPt( 7,  3, 0));
		int p4 = ppoly->LoadLatticePt(CPt(-7,  3, 0));

		ppoly->AddPolyCurve(CUnitVector(0, 0, 1), PCF_NEWPLANE, NULL, NULL, 0);
		ppoly->AddPolyCurveIndex(p1);
		ppoly->AddPolyCurveIndex(p2);
		ppoly->AddPolyCurveIndex(p3);
		ppoly->AddPolyCurveIndex(p4);
		ppoly->LoadComplete();
		//ppoly->Extrude(1, false);
		double ang[8] = { 160, 140, 120, 100, 80, 60, 40, 20 };
		double len[8] = { 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2 };
		ppoly->Extrude(ang, len, 8, true);

		string rmap = model.GetImageMap().LoadImageFile("..\\images\\reflect2.jpg");
		ppoly->AddReflectionMap(TXTF_RGB, rmap);

		ppoly->GetTM().Translate(0, 0, -2);
	}
}

static void CreateLights(CGrp* pgrp)
{
	CPtLight* plit = pgrp->CreatePointLight();
	plit->SetColor(COLOR(0.4, 0.4, 0.4));
	plit->SetOrigin(CPt(5, 8, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true, true, 3);

	plit = pgrp->CreatePointLight();
	plit->SetColor(COLOR(0.3, 0.3, 0.3));
	plit->SetOrigin(CPt(-5, 0, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true, true, 3);

	plit = pgrp->CreatePointLight();
	plit->SetColor(COLOR(0.3, 0.3, 0.3));
	plit->SetOrigin(CPt(-3, 8, 5));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true, true, 3);
}

static void LoadAsset(CGrp* pgrp, const char* path)
{
	if (path[0] != 0)
	{
		CGrp* pnewgrp = pgrp->CreateSubGroup();
		CBinFile binFile(path);
		if (binFile.OpenFile(false, MigType::Object))
		{
			if (binFile.ReadNextBlockType() == BlockType::Group)
				pnewgrp->Load(binFile);
			binFile.CloseFile();
		}
	}
}

static void LoadOrientation(CModel& model, CGrp* pgrp)
{
	CParser parser(&model);
	parser.SetEnvVarModelRef<CGrp>("grp", pgrp);
	parser.ParseCommandScript("..\\pkgview\\orient.txt");
}

static CGrp* LoadTextString(CPackage& package, CModel& model, CGrp* pgrp, const char* szText, int len, const char* szSuffix, const char* szScript)
{
	// this will hold the text
	CGrp* psub = pgrp->CreateSubGroup();

	int incx = 0, incy = 0;
	for (int n = 0; n < len; n++)
	{
		char item[24];
		memset(item, 0, sizeof(item));
		item[0] = szText[n];
		item[1] = '-';
		if (szSuffix[0])
			strcat_s(item, szSuffix);

		//if (package.GetObjectType(item) == BlockType::Polygon)
		if (package.GetObjectType(item) == BlockType::Group)
		{
			//CPolygon* pobj = psub->CreatePolygon();
			CGrp* pobj = psub->CreateSubGroup();
			package.LoadObject(item, pobj);

			// helps modulate the uv coordinates of the texture mappings
			int xtxtdiv = 2, ytxtdiv = 2;
			UVC uvmin, uvmax;
			uvmin.u = (double)(n % xtxtdiv) / xtxtdiv;
			uvmin.v = (double)((n / ytxtdiv) % ytxtdiv) / ytxtdiv;
			uvmax.u = uvmin.u + (1.0 / xtxtdiv);
			uvmax.v = uvmin.v + (1.0 / ytxtdiv);

			CParser parser(&model);
			parser.SetEnvVarModelRef<CGrp>("grp", pobj);
			parser.SetEnvVarModelRef<CPolygon>("poly", (CPolygon*)pobj->GetObjects()[0].get());
			parser.SetEnvVar<UVC>("uvmin", uvmin);
			parser.SetEnvVar<UVC>("uvmax", uvmax);
			parser.ParseCommandScript(szScript);

			pobj->GetTM().Translate(incx, incy, 0);

			char buf[16], *context;
			pobj->GetMetaData("cellinc", buf, sizeof(buf));
			incx += atoi(strtok_s(buf, ",", &context));
			incy += atoi(strtok_s(NULL, ",", &context));
		}
	}

	return psub;
}

static void CenterAndScaleGroup(CGrp* group)
{
	CBoundBox bbox;
	if (group->ComputeBoundBox(bbox))
	{
		CPt center = bbox.GetCenter();
		CPt minpt = bbox.GetMinPt();
		CPt maxpt = bbox.GetMaxPt();
		double width = maxpt.x - minpt.x;
		double scale = min(10 / width, 0.1);

		group->GetTM()
			.Translate(-center.x, -center.y, -center.z)
			.Scale(scale, scale, 2 * scale);
	}
}

static void CenterAndScaleGroups(vector<CGrp*>& listOfGroups)
{
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

			listOfGroups[i]->GetTM()
				.Translate(-center.x, -center.y, -center.z);
				//.Scale(scale, scale, scale);

			totalHeight += (bbox.GetMaxPt().y - bbox.GetMinPt().y);
		}
	}

	for (int i = 0; i < (int) listOfGroups.size(); i++)
		listOfGroups[i]->GetTM().Scale(totalScale, totalScale, totalScale);

	if (listOfGroups.size() == 2)
	{
		listOfGroups[0]->GetTM().Translate(0, 0.25*totalHeight*totalScale, 0);
		listOfGroups[1]->GetTM().Translate(0, -0.25*totalHeight*totalScale, 0);
	}
	else if (listOfGroups.size() == 3)
	{
		listOfGroups[0]->GetTM().Translate(0, 0.35*totalHeight*totalScale, 0);
		listOfGroups[2]->GetTM().Translate(0, -0.35*totalHeight*totalScale, 0);
	}
}

static void NormalizeAlpha(CImageBuffer* pmap)
{
	if (pmap)
	{
		pmap->FillAlphaFromColors();
		pmap->NormalizeAlpha();
	}
}

static void LoadPackageText(CModel& model, REND_THREAD_DATA* pdata)
{
	CBinFile binFile(pdata->szPackage);
	CPackage package;
	int nchars = package.OpenPackage(&binFile);

	// create the top level group that will hold the lights, text and backdrop
	CGrp* pgrp = model.GetSuperGroup().CreateSubGroup();
	//CreateLights(pgrp);
	LoadAsset(pgrp, pdata->szLights);

	vector<CGrp*> listOfGroups;
	int len = strlen(pdata->szText);
	if (len > 8)
	{
		char* context;
		char* szTok = strtok_s(pdata->szText, " -\n", &context);
		while (szTok != NULL)
		{
			len = strlen(szTok);
			CGrp* psub = LoadTextString(package, model, pgrp, szTok, len, pdata->szLookupSuffix, pdata->szScript);
			if (psub != NULL)
				listOfGroups.push_back(psub);
			szTok = strtok_s(NULL, " -\n", &context);
		}
	}
	else
	{
		CGrp* psub = LoadTextString(package, model, pgrp, pdata->szText, len, pdata->szLookupSuffix, pdata->szScript);
		if (psub != NULL)
			listOfGroups.push_back(psub);
	}

	CenterAndScaleGroups(listOfGroups);

	//CreateBackdrop(model, pgrp);
	LoadAsset(pgrp, pdata->szBackdrop);

	if (pdata->szTexture1[0])
		model.GetImageMap().LoadImageFile(pdata->szTexture1, ImageFormat::RGB, "texture1");
	if (pdata->szTexture2[0])
		model.GetImageMap().LoadImageFile(pdata->szTexture2, ImageFormat::RGB, "texture2");
	if (pdata->szReflect[0])
		model.GetImageMap().LoadImageFile(pdata->szReflect, ImageFormat::RGB, "reflect");
	if (pdata->szBackdropTexture1[0])
	{
		string imap = model.GetImageMap().LoadImageFile(pdata->szBackdropTexture1, ImageFormat::RGBA, "bd-texture1");
		NormalizeAlpha(model.GetImageMap().GetImage(imap));
	}
	if (pdata->szBackdropTexture2[0])
	{
		string imap = model.GetImageMap().LoadImageFile(pdata->szBackdropTexture2, ImageFormat::RGBA, "bd-texture2");
		NormalizeAlpha(model.GetImageMap().GetImage(imap));
	}
	if (pdata->szBackdropReflect[0])
		model.GetImageMap().LoadImageFile(pdata->szBackdropReflect, ImageFormat::RGB, "bd-reflect");
	if (pdata->szUserImage[0])
		model.GetImageMap().LoadImageFile(pdata->szUserImage, ImageFormat::RGB, "user-image");

	//pgrp->GetTM()->RotateZ(30);
	//pgrp->GetTM()->RotateX(-50);
	//pgrp->GetTM()->RotateY(75);
	//pgrp->GetTM()->Translate(0.5, 2, 0);
	LoadOrientation(model, pgrp);
}

struct WORKER_THREAD_DATA
{
	CModel* pmodel;
	int nthread;
};

DWORD WINAPI WorkerThreadProc(LPVOID lpParam)
{
	WORKER_THREAD_DATA* pdata = (WORKER_THREAD_DATA*) lpParam;
	pdata->pmodel->DoRender(pdata->nthread);
	delete pdata;
	return 1;
}

static int GetThreadCount()
{
#ifdef USE_MULTI_CORE
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int nthreads = (int) sysinfo.dwNumberOfProcessors;
	if (nthreads < 1)
		return 1;
	return nthreads;
#else
	return 1;
#endif // USE_MULTI_CORE
}

DWORD WINAPI RendThreadProc(LPVOID lpParam)
{
	REND_THREAD_DATA* pdata = (REND_THREAD_DATA*) lpParam;
	DWORD start = GetTickCount();

	CModel model;
	CParser parser(&model, NULL, pdata->pbmp->GetRenderInfo());
	if (parser.ParseCommandScript("..\\pkgview\\setup.txt"))
	{
		//model.SetSampling(X1);
		model.SetRenderQuality(REND_AUTO_REFLECT | REND_AUTO_REFRACT | REND_AUTO_SHADOWS);

		LoadPackageText(model, pdata);

		model.SetRenderTarget((CRenderTarget*)pdata->pbmp);

		PostMessage(pdata->hdlg, WM_REND_START, 0, 0);

		int nthreads = GetThreadCount();
		if (nthreads > 1)
		{
			model.PreRender(nthreads);

			HANDLE* hThreads = new HANDLE[nthreads];
			for (int i = 0; i < nthreads; i++)
			{
				WORKER_THREAD_DATA* pdata = new WORKER_THREAD_DATA;
				pdata->pmodel = &model;
				pdata->nthread = i;
				hThreads[i] = CreateThread(NULL, 0, WorkerThreadProc, (LPVOID)pdata, 0, NULL);
			}
			::WaitForMultipleObjects(nthreads, hThreads, TRUE, INFINITE);

			model.PostRender(true);
		}
		else
			model.DoRenderSimple();
	}

	DWORD end = GetTickCount();
	PostMessage(pdata->hdlg, WM_REND_FINISH, (end - start), 0);

	delete pdata;
	return 1;
}

static string GetPackageTitle(const string& package)
{
	string title = package;
	size_t lastSlash = title.rfind('\\');
	if (lastSlash != string::npos)
		title = title.substr(lastSlash + 1);
	size_t dot = title.find('.');
	return (dot != string::npos ? title.substr(0, dot) : title);
}

static string GeneratePrefixSring(const string& prefix)
{
	char first = prefix[0];
	if (first >= 'A' && first <= 'Z')
		return "upr" + prefix;
	else if (first >= 'a' && first <= 'z')
		return "low" + prefix;
	else if (first >= '0' && first <= '9')
		return "num" + prefix;
	return "spc" + to_string((int) first);
}

static bool ProcessBatchPackage(const string& package, BATCH_THREAD_DATA* pdata)
{
	// create one directory per package (lots of potential images)
	string newDir = pdata->dest + GetPackageTitle(package) + "\\";
	if (!CreateDirectoryA(newDir.c_str(), NULL))
	{
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
			return false;
	}

	// load texture maps
	CModel model;
	if (pdata->szTexture1[0])
		model.GetImageMap().LoadImageFile(pdata->szTexture1, ImageFormat::RGB, "texture1");
	if (pdata->szTexture2[0])
		model.GetImageMap().LoadImageFile(pdata->szTexture2, ImageFormat::RGB, "texture2");
	if (pdata->szReflect[0])
		model.GetImageMap().LoadImageFile(pdata->szReflect, ImageFormat::RGB, "reflect");

	// open the package
	CBinFile binFile(package.c_str());
	CPackage packageFile;
	if (packageFile.OpenPackage(&binFile) == 0)
		return false;

	// for each prefix
	vector<string>::iterator prefixIter = pdata->prefixes.begin();
	while (prefixIter != pdata->prefixes.end())
	{
		// for each suffix
		vector<string>::iterator suffixIter = pdata->suffixes.begin();
		while (suffixIter != pdata->suffixes.end())
		{
			// for each font script
			vector<string>::iterator fontScriptIter = pdata->fontScripts.begin();
			while (fontScriptIter != pdata->fontScripts.end())
			{
				// for each setup (told you there would be lots of images)
				for (size_t setupIndex = 0; setupIndex < pdata->setupScripts.size(); setupIndex++)
				{
					// clear the model (keep the images)
					model.Delete(false);

					// compose the image name
					string output = newDir + "prv-" + GeneratePrefixSring(*prefixIter);
					if (!(*suffixIter).empty())
						output += "-" + *suffixIter;
					if (pdata->setupScripts.size() > 1)
						output += "-" + to_string(setupIndex + 1);
					output += ".bmp";

					// create the render target image
					CBMPTarget bmp;
					bmp.Init(pdata->width, pdata->height, 24, output.c_str());
					model.SetRenderTarget((CRenderTarget*)&bmp);

					// create the top level group that will hold everything
					CGrp* pgrp = model.GetSuperGroup().CreateSubGroup();

					// load the setup
					CParser parser(&model);
					parser.SetEnvVarModelRef<CGrp>("grp", pgrp);
					if (!parser.ParseCommandScript(pdata->setupScripts[setupIndex]))
						return false;

					// load the object
					string key = *prefixIter + "-" + *suffixIter;
					//if (package.GetObjectType(item) == BlockType::Polygon)
					if (packageFile.GetObjectType(key.c_str()) != BlockType::Group)
						return false;

					//CPolygon* pobj = psub->CreatePolygon();
					CGrp* pobj = pgrp->CreateSubGroup();
					packageFile.LoadObject(key.c_str(), pobj);

					// helps modulate the uv coordinates of the texture mappings
					int index = setupIndex;
					int xtxtdiv = 2, ytxtdiv = 2;
					UVC uvmin, uvmax;
					uvmin.u = (double)(index % xtxtdiv) / xtxtdiv;
					uvmin.v = (double)((index / ytxtdiv) % ytxtdiv) / ytxtdiv;
					uvmax.u = uvmin.u + (1.0 / xtxtdiv);
					uvmax.v = uvmin.v + (1.0 / ytxtdiv);

					// font post process script
					parser.SetEnvVarModelRef<CGrp>("grp", pobj);
					parser.SetEnvVarModelRef<CPolygon>("poly", (CPolygon*)pobj->GetObjects()[0].get());
					parser.SetEnvVar<UVC>("uvmin", uvmin);
					parser.SetEnvVar<UVC>("uvmax", uvmax);
					parser.ParseCommandScript(*fontScriptIter);

					// center it
					CenterAndScaleGroup(pobj);

					// render
					model.SetRenderQuality(REND_NONE);
					model.DoRenderSimple();

					// abort will be signalled through the otherwise unused on-screen bitmap
					if (pdata->pbmp->IsStopped())
						return false;
				}

				fontScriptIter++;
			}

			suffixIter++;
		}

		prefixIter++;
	}

	return true;
}

DWORD WINAPI BatchThreadProc(LPVOID lpParam)
{
	BATCH_THREAD_DATA* pdata = (BATCH_THREAD_DATA*)lpParam;
	DWORD start = GetTickCount();

	PostMessage(pdata->hdlg, WM_REND_START, 0, 0);

	vector<string>::iterator iter = pdata->packages.begin();
	while (iter != pdata->packages.end())
	{
		if (!ProcessBatchPackage(*iter, pdata))
			break;
		iter++;
	}

	DWORD end = GetTickCount();
	PostMessage(pdata->hdlg, WM_REND_FINISH, (end - start), 0);
	return 1;
}

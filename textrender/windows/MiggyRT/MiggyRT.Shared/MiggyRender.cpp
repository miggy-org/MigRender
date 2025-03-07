#include "pch.h"
#include "MiggyRender.h"
#include <ppltasks.h>
#include <concurrent_vector.h>
#include <thread>

#include "fileio.h"
#include "jpegio.h"
#include "bmpio.h"

using namespace concurrency;
using namespace MiggyRT;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;

// not re-entrant, and copy the result immediately
static const char* _ConvertString(String^ str)
{
	static char szTmp[256];
	size_t converted;
	wcstombs_s(&converted, szTmp, str->Data(), 256);
	return szTmp;
}

MiggyRender::MiggyRender()
	: pMainGrp(NULL), threadCount(1)
{
}

bool MiggyRender::Init(int threads, bool superSample, bool doShadows, bool softShadows, bool autoReflect)
{
	theModel.Initialize();
	threadCount = threads;

	// create the top level group that will hold the lights, text and backdrop
	CGrp* pgrp = theModel.GetSuperGroup();
	pMainGrp = pgrp->CreateSubGroup();

	// init camera
	CCamera* pcam = theModel.GetCamera();
	CMatrix* pmat = pcam->GetTM();
	pmat->RotateY(180);
	pmat->Translate(0, 0, 15);

	// TODO: someday we should provide an interface for setting the background colors and/or images
	COLOR cn(1.0, 0.0, 0.0, 1.0);
	COLOR ce(0.0, 0.0, 0.2, 1.0);
	COLOR cs(0.0, 1.0, 0.0, 1.0);
	CBackground* pbg = theModel.GetBackgroundHandler();
	pbg->SetBackgroundColors(cn, ce, cs);

	// TODO: ditto for ambient light
	COLOR ambient(0.35, 0.35, 0.35);
	theModel.SetAmbientLight(ambient);

	// super-sampling
	theModel.SetSampling(superSample ? SSMPL_5X : SSMPL_1X);

	// rendering flags
	dword rendFlags = REND_NONE;
	if (doShadows)
		rendFlags |= REND_AUTO_SHADOWS;
	if (doShadows && softShadows)
		rendFlags |= REND_SOFT_SHADOWS;
	if (autoReflect)
		rendFlags |= REND_AUTO_REFLECT;
	theModel.SetRenderQuality(rendFlags);

	return true;
}

static CGrp* LoadTextString(CPackage& package, CGrp* pgrp, const char* szText, int len)
{
	// this will hold the text for this line
	CGrp* psub = pgrp->CreateSubGroup();

	int incx = 0, incy = 0;
	for (int n = 0; n < len; n++)
	{
		char item[8];
		memset(item, 0, sizeof(item));
		item[0] = szText[n];
		item[1] = '-';

		// we expect all objects in this package to be polygons
		if (package.GetObjectType(item) == BLOCK_TYPE_POLYGON)
		{
			// load it
			CPolygon* ppoly = psub->CreatePolygon();
			if (package.LoadObject(item, ppoly))
			{
				// translate the polygon to the next position in the line
				CMatrix* pmat = ppoly->GetTM();
				pmat->Translate(incx, incy, 0);

				// use meta-data to determine the next position
				char buf[16];
				ppoly->GetMetaData("cellinc", buf, sizeof(buf));
				char* pcomma = strchr(buf, ',');
				if (pcomma != NULL)
				{
					*pcomma = '\0';
					incx += atoi(buf);
					incy += atoi(++pcomma);
				}
			}
		}
	}

	return psub;
}

static void CenterAndScaleGroups(vector<CGrp*>& listOfGroups)
{
	// using bounding boxes, center the text
	double totalHeight = 0, totalScale = 0;
	for (int i = 0; i < (int)listOfGroups.size(); i++)
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

			CMatrix* pmat = listOfGroups[i]->GetTM();
			pmat->Translate(-center.x, -center.y, -center.z);
			//pmat->Scale(scale, scale, 2*scale);

			totalHeight += (bbox.GetMaxPt().y - bbox.GetMinPt().y);
		}
	}

	// scale it to a common scale factor
	for (int i = 0; i < (int)listOfGroups.size(); i++)
		listOfGroups[i]->GetTM()->Scale(totalScale, totalScale, 2 * totalScale);

	// account for 2 or 3 lines of text, that's all we will handle
	if (listOfGroups.size() == 2)
	{
		listOfGroups[0]->GetTM()->Translate(0, 0.25*totalHeight*totalScale, 0);
		listOfGroups[1]->GetTM()->Translate(0, -0.25*totalHeight*totalScale, 0);
	}
	else if (listOfGroups.size() == 3)
	{
		listOfGroups[0]->GetTM()->Translate(0, 0.35*totalHeight*totalScale, 0);
		listOfGroups[2]->GetTM()->Translate(0, -0.35*totalHeight*totalScale, 0);
	}
}

bool MiggyRender::SetText(String^ text, String^ pkgData)
{
	// open the package file
	CBinFile binFile(_ConvertString(pkgData));
	CPackage package;
	int nchars = package.OpenPackage(&binFile);
	if (nchars == 0)
		return false;

	if (text->Length() > 0 && text->Length() < 80)
	{
		vector<CGrp*> listOfGroups;

		// copy to a temporary buffer
		char szText[80];
		strcpy_s(szText, _ConvertString(text));

		// we will break up the string into multiple lines if it's too long and there are spaces
		int len = strlen(szText);
		if (len > 8)
		{
			// create a list of groups, one for each line
			char* context;
			char* szTok = strtok_s(szText, " -\n", &context);
			while (szTok != NULL)
			{
				len = strlen(szTok);
				CGrp* psub = LoadTextString(package, pMainGrp, szTok, len);
				if (psub != NULL)
					listOfGroups.push_back(psub);

				szTok = strtok_s(NULL, " -\n", &context);
			}
		}
		else
		{
			// less than a certain minimum and we just keep it all on one line
			CGrp* psub = LoadTextString(package, pMainGrp, szText, len);
			if (psub != NULL)
				listOfGroups.push_back(psub);
		}

		// center and scale the lines of text
		CenterAndScaleGroups(listOfGroups);
	}

	return (text->Length() > 0);
}

bool MiggyRender::SetBackdrop(String^ mdlData, double specR, double specG, double specB, bool autoReflect)
{
	bool bRet = false;

	CBinFile binFile(_ConvertString(mdlData));
	if (binFile.OpenFile(false, MIG_TYPE_OBJECT))
	{
		if (binFile.ReadNextBlockType() == BLOCK_TYPE_POLYGON)
		{
			CPolygon* ppoly = pMainGrp->CreatePolygon();
			if (ppoly->Load(binFile))
			{
				ppoly->SetSpecular(COLOR(specR, specG, specB));
				ppoly->SetObjectFlags(autoReflect ? (OBJF_SHADOW_RAY | OBJF_REFL_RAY) : OBJF_SHADOW_RAY);
				bRet = true;
			}
		}

		binFile.CloseFile();
	}

	return bRet;
}

bool MiggyRender::SetLighting(String^ mdlData)
{
	bool bRet = false;

	CBinFile binFile(_ConvertString(mdlData));
	if (binFile.OpenFile(false, MIG_TYPE_OBJECT))
	{
		if (binFile.ReadNextBlockType() == BLOCK_TYPE_GROUP)
		{
			CGrp* plightgrp = pMainGrp->CreateSubGroup();
			if (plightgrp->Load(binFile))
				bRet = true;
		}

		binFile.CloseFile();
	}

	return bRet;
}

bool MiggyRender::SetOrientation(double rZ, double rX, double rY, double tX, double tY, double tZ)
{
	CMatrix* ptm = pMainGrp->GetTM();
	ptm->SetIdentity();

	ptm->RotateZ(rZ);
	ptm->RotateX(rX);
	ptm->RotateY(rY);
	ptm->Translate(tX, tY, tZ);

	return true;
}

bool MiggyRender::LoadImage(String^ slot, String^ imgData, bool createAlpha)
{
	bool bRet = false;

	FILE* pFile = NULL;
	if (fopen_s(&pFile, _ConvertString(imgData), "rb") == 0)
	{
		const char* pslot = _ConvertString(slot);

		CImageMap* pimages = theModel.GetImageMap();
		if (pimages->LoadImageFile(pFile, IFILE_JPEG, pslot, (createAlpha ? IFMT_RGBA : IFMT_RGB)).length() > 0)
		{
			bRet = true;

			// create an alpha channel from the image colors, if necessary
			if (createAlpha)
			{
				CImageBuffer* pmap = pimages->GetImage(pslot);
				if (pmap != NULL)
				{
					pmap->FillAlphaFromColors();
					pmap->NormalizeAlpha();
				}
				else
					bRet = false;
			}
		}

		fclose(pFile);
	}

	return bRet;
}

IAsyncOperation<String^>^ MiggyRender::RenderToTempFile(int width, int height)
{
	// BMP dimensions must be divisible by 4
	width -= width % 4;
	height -= height % 4;

	return create_async([this, width, height]()
	{
		CBMPTarget bmTarget;
		char pathStr[256];
		sprintf_s(pathStr, "%s\\blah.bmp", _ConvertString((Windows::Storage::ApplicationData::Current)->TemporaryFolder->Path));
		bmTarget.Init(width, height, 24, pathStr);

		// set the camera viewport
		REND_INFO rinfo = bmTarget.GetRenderInfo();
		double aspect = (rinfo.width / (double)rinfo.height);
		double minvp = 10;
		double ulen = (aspect > 1 ? minvp*aspect : minvp);
		double vlen = (aspect < 1 ? minvp / aspect : minvp);
		double dist = 15;
		CCamera* pcam = theModel.GetCamera();
		pcam->SetViewport(ulen, vlen, dist);

		// start the render
		theModel.SetRenderTarget(&bmTarget);
		theModel.DoRenderSimple();

		return ref new Platform::String(L"ms-appdata:///temp/blah.bmp");
	});
}

struct WORKER_THREAD_ARGS
{
	CModel* pmodel;
	int nthread;
};

static void workerThreadFunc(WORKER_THREAD_ARGS* pArg)
{
	//WORKER_THREAD_ARGS* pargs = (WORKER_THREAD_ARGS*)pArg;
	pArg->pmodel->DoRender(pArg->nthread);
	delete pArg;
}

IAsyncOperation<bool>^ MiggyRender::Render(MiggySurface^ surfaceHint)
{
	IFMT fmt = IFMT_BGRA;
	int width, height;
	if (surfaceHint != nullptr)
	{
		fmt = surfaceHint->GetPreferredImageFormat(surfaceHint->GetPixelFormat());
		width = surfaceHint->GetWidth();
		height = surfaceHint->GetHeight();
	}

	return create_async([this, width, height, fmt]()
	{
		if (!bufTarget.Init(width, height, fmt))
			return false;

		// set the camera viewport
		double aspect = (width / (double)height);
		double minvp = 10;
		double ulen = (aspect > 1 ? minvp*aspect : minvp);
		double vlen = (aspect < 1 ? minvp / aspect : minvp);
		double dist = 15;
		CCamera* pcam = theModel.GetCamera();
		pcam->SetViewport(ulen, vlen, dist);

		// start the render
		theModel.SetRenderTarget(&bufTarget);

		// get the thread count (should be one per physical processor)
		if (threadCount > 0)
		{
			// signal pre-render
			theModel.PreRender(threadCount);

			// create the worker threads
			std::thread** threads = new std::thread*[threadCount];
			for (int i = 0; i < threadCount; i++)
			{
				WORKER_THREAD_ARGS* pArgs = new WORKER_THREAD_ARGS;
				pArgs->pmodel = &theModel;
				pArgs->nthread = i;
				threads[i] = new std::thread(workerThreadFunc, pArgs);
			}

			// wait for the worker threads to all finish
			for (int j = 0; j < threadCount; j++)
			{
				threads[j]->join();
			}

			// signal end of render
			theModel.PostRender(true);
		}

		return true;
	});
}

void MiggyRender::AbortRender()
{
	theModel.SignalRenderAbort();
}

bool MiggyRender::CopyToSurface(MiggySurface^ surface)
{
	bool ret = surface->BeginDraw();
	if (ret)
	{
		ret = surface->CopyImage(&bufTarget);
		surface->EndDraw();
	}

	return ret;
}

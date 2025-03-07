#include "StdAfx.h"
#include "Reg.h"

#include "migutil.h"
#include "rendthread.h"
#include "GLTest.h"

#include "model.h"
#include "bmpio.h"
#include "fileio.h"
#include "package.h"

#include "animmanage.h"
#include "parser.h"
#include "jsonio.h"
#include "targets.h"

using namespace std;
using namespace MigRender;

static void TestSpheres(CModel& model)
{
	//string map1 = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGBA);
	//string map2 = model.GetImageMap().LoadImageFile("..\\images\\reflect2.jpg");

	//CImageBuffer* pimg = pimages->GetImage(map1);
	//pimg->FillAlphaFromTransparentColor(10, 149, 218, 20, 20, 20, 0);
	//pimg->FillAlphaFromColors();
	//pimg->NormalizeAlpha();

	/*CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(1, 1, 1));
	CUnitVector dir(0, 1, 0);
	dir.Normalize();
	plit->SetDirection(dir);
	plit->SetShadowCaster(true);*/

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, -10, 0));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	/*CSpotLight* plit = model.GetSuperGroup().CreateSpotLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(1, -10, 0));
	CUnitVector dir(0, 1, 0);
	dir.Normalize();
	plit->SetDirection(dir);
	plit->SetConcentration(10);
	plit->SetShadowCaster(true);*/

	CSphere* psphere = NULL;
	psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(-4, 0, -4));
	psphere->SetRadius(3);
	psphere->SetDiffuse(COLOR(0, 0, 0));
	psphere->SetSpecular(COLOR(0.7, 0.7, 0.7));
	//psphere->SetBoundBox(true);

	psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(0, 0, 0));
	psphere->SetRadius(1);
	psphere->SetDiffuse(COLOR(0, 0, 0.7));
	//psphere->SetDiffuse(COLOR(1.0, 1.0, 1.0));
	psphere->SetSpecular(COLOR(0.3, 0.3, 0.3));
	//psphere->SetRefraction(COLOR(1.0, 1.0, 1.0, 0.0));
	//psphere->SetReflection(COLOR(0.3, 0.3, 0.3));
	//psphere->SetObjectFlags(OBJF_SHADOW_RAY | OBJF_USE_BBOX);
	//psphere->AddColorMap(TextureMapType::Diffuse, TXTF_RGB, map1);
	//psphere->AddColorMap(TextureMapType::Specular, TXTF_ALPHA, map1);
	//psphere->AddColorMap(TextureMapType::Refraction, TXTF_ALPHA, map1);
	//psphere->AddReflectionMap(TXTF_RGB, map2);
	//psphere->AddBumpMap(TXTF_ALPHA, map1, 1, 0);

	CMatrix& mat = psphere->GetTM();
	mat.Scale(3.0, 3.0, 3.0);
	mat.Translate(1, 4, 0);

	psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(3, -2, 2));
	psphere->SetRadius(2);
	psphere->SetDiffuse(COLOR(0.5, 0, 0.5));
	psphere->SetSpecular(COLOR(0.2, 0.2, 0.2));
	//psphere->SetBoundBox(true);

	/*CGrp* psub = model.GetSuperGroup().CreateSubGroup();
	for (int i = 0; i < 3; i++)
	{
		CSphere* psphere = psub->CreateSphere();
		psphere->SetOrigin(CPt(0, 0, 0));
		psphere->SetRadius(1);
		psphere->SetDiffuse(COLOR(0, 0, 0.7));
		psphere->SetSpecular(COLOR(0, 0, 0));

		CMatrix* pmat = psphere->GetTM();
		pmat->Translate(-3 + 3*i, 0, 0);
	}
	CMatrix* pmat = psub->GetTM();
	pmat->RotateZ(30);*/
}

static void TestPolygons(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 5));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	ppoly->SetSpecular(COLOR(0.1, 0.1, 0.1));
	ppoly->SetBoundBox(true);

	CPt pts[12];
	pts[0].SetPoint(-2, -2,  0);
	pts[1].SetPoint( 2, -2,  0);
	pts[2].SetPoint(-2,  2,  0);
	pts[3].SetPoint( 2,  2,  0);
	pts[4].SetPoint(-1, -1,  0);
	pts[5].SetPoint( 1, -1,  0);
	pts[6].SetPoint(-1,  1,  0);
	pts[7].SetPoint( 1,  1,  0);
	pts[8].SetPoint( 2, -2,  2);
	pts[9].SetPoint( 2,  2,  2);
	pts[10].SetPoint(-2,  2,  2);
	pts[11].SetPoint(-2, -2,  2);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	int inds[4] = { 0, 1, 3, 2 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, NULL, 4);
	int hole[4] = { 4, 5, 7, 6 };
	ppoly->AddPolyCurve(norm, PCF_NONE, hole, NULL, 4);

	norm.SetPoint(1, 0, 0);
	norm.Normalize();
	int side1[4] = { 1, 3, 9, 8 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, side1, NULL, 4);

	norm.SetPoint(0, 1, 0);
	norm.Normalize();
	int side2[4] = { 3, 2, 10, 9 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, side2, NULL, 4);

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	mat.RotateX(-30);
	mat.RotateY(30);
	mat.Translate(1, 1, 0);
}

static void TestSmoothPolygons(CModel& model)
{
	CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetDirection(CUnitVector(0, 0, -1));
	plit->SetShadowCaster(true);

	/*CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);*/

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	//ppoly->SetSpecular(COLOR(0.1, 0.1, 0.1));
	ppoly->SetObjectFlags(OBJF_REFL_RAY | OBJF_REFR_RAY | OBJF_USE_BBOX);

	CPt pts[8];
	pts[0].SetPoint(-4, -3,  0);
	pts[1].SetPoint( 4, -3,  0);
	pts[2].SetPoint(-4,  3,  0);
	pts[3].SetPoint( 4,  3,  0);
	pts[4].SetPoint(-1, -1,  0);
	pts[5].SetPoint( 1, -1,  0);
	pts[6].SetPoint(-1,  1,  0);
	pts[7].SetPoint( 1,  1,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE | PCF_VERTEXNORMS | PCF_CULL, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(0);
	ppoly->AddPolyCurveIndex(1);
	ppoly->AddPolyCurveIndex(3);
	ppoly->AddPolyCurveIndex(2);

	norm.SetPoint(-1,  0, 1); norm.Normalize();
	ppoly->AddPolyCurveNormal(norm);
	norm.SetPoint( 1,  0, 1); norm.Normalize();
	ppoly->AddPolyCurveNormal(norm);
	norm.SetPoint( 1,  0, 1); norm.Normalize();
	ppoly->AddPolyCurveNormal(norm);
	norm.SetPoint(-1,  0, 1); norm.Normalize();
	ppoly->AddPolyCurveNormal(norm);

	norm.SetPoint(0, 0, 1);	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_CULL | PCF_CW, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(4);
	ppoly->AddPolyCurveIndex(6);
	ppoly->AddPolyCurveIndex(7);
	ppoly->AddPolyCurveIndex(5);

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.RotateX(-30);
	mat.RotateY(30);
	mat.RotateZ(30);
	//mat.Translate(4, 0, 0);
}

static CPt ToPoint(const POINTFX& pt)
{
	return CPt(pt.x.value + pt.x.fract/65536.0, pt.y.value + pt.y.fract/65536.0, 0);
}

static bool CmpPoint(const POINTFX& pt1, const POINTFX& pt2)
{
	return (!memcmp(&pt1, &pt2, sizeof(POINTFX)));
}

static int LoadQSpline(CPolygon* ppoly, int curpt, const CPt& pta, const CPt& ptb, const CPt& ptc, bool addlast)
{
	double fineness = 3;
	double inc = 1.0 / fineness;

	int cnt = 0;
	for (double t = inc; t <= 1.0; t += inc)
	{
		CPt newpt = (pta - ptb*2 + ptc)*t*t + (ptb*2 - pta*2)*t + pta;
		if (t + inc <= 1.0 || addlast)
		{
			ppoly->LoadLatticePt(newpt);
			ppoly->AddPolyCurveIndex(curpt++);
			cnt++;
		}
	}

	return cnt;
}

static CGrp* TestGlyph(CModel& model, LPCTSTR font, LPCTSTR str, LPCSTR postScript, bool fit)
{
	CGrp* pgrp = NULL;

	HWND hwnd = GetDesktopWindow();
	HDC hDC = GetDC(hwnd);
	HDC memDC = CreateCompatibleDC(hDC);
	ReleaseDC(hwnd, hDC);

	int nHeight = 50;
	int nWidth = 0;
	int nEscapement = 0;
	int nOrientation = 0;
	int nWeight = FW_NORMAL;
	BOOL bItalic = FALSE;
	BOOL bUnderline = FALSE;
	BOOL bStrikeout = FALSE;
	DWORD dwCharSet = DEFAULT_CHARSET;
	DWORD dwOutputPrecision = OUT_DEFAULT_PRECIS;
	DWORD dwClipPrecision = CLIP_DEFAULT_PRECIS;
	DWORD dwQuality = PROOF_QUALITY;
	DWORD dwPitchAndFamily = VARIABLE_PITCH;
	HFONT hFont = CreateFont(nHeight, nWidth, nEscapement, nOrientation, nWeight,
		bItalic, bUnderline, bStrikeout,
		dwCharSet, dwOutputPrecision, dwClipPrecision, dwQuality, dwPitchAndFamily, font);
	if (hFont)
	{
		HGDIOBJ hOld = SelectObject(memDC, hFont);

	    MAT2 m;
		memset(&m, 0, sizeof(MAT2));
		m.eM11.value = 1;
		m.eM22.value = 1;

		GLYPHMETRICS gm;
		DWORD size = GetGlyphOutline(memDC, str[0], GGO_NATIVE, &gm, 0, NULL, &m);
		if (size != GDI_ERROR)
		{
			//pgrp = model.GetSuperGroup();
			pgrp = model.GetSuperGroup().CreateSubGroup();
			CPolygon* ppoly = pgrp->CreatePolygon();
			CUnitVector norm(0, 0, 1);
			//norm.Normalize();

			if (size > 0)
			{
				vector<byte> bytes(size);
				DWORD ret = GetGlyphOutline(memDC, str[0], GGO_NATIVE, &gm, size, bytes.data(), &m);

				CPt pta, ptb, ptc;
				int curpt = 0;
				DWORD cnt = 0;
				while (cnt < size)
				{
					TTPOLYGONHEADER* ptth = (TTPOLYGONHEADER*) &bytes[cnt];
					cnt += sizeof(TTPOLYGONHEADER);

					ppoly->AddPolyCurve(norm, (curpt == 0 ? PCF_NEWPLANE : PCF_NONE), NULL, NULL, 0);

					pta = ToPoint(ptth->pfxStart);
					ppoly->LoadLatticePt(pta);
					ppoly->AddPolyCurveIndex(curpt++);

					DWORD local = sizeof(TTPOLYGONHEADER);
					while (local < ptth->cb)
					{
						TTPOLYCURVE* pttc = (TTPOLYCURVE*) &bytes[cnt];
						cnt += sizeof(TTPOLYCURVE) + (pttc->cpfx-1)*sizeof(POINTFX);
						local += sizeof(TTPOLYCURVE) + (pttc->cpfx-1)*sizeof(POINTFX);

						if (pttc->wType == TT_PRIM_LINE)
						{
							for (int i = 0; i < pttc->cpfx; i++)
							{
								// update current point
								pta = ToPoint(pttc->apfx[i]);
								ppoly->LoadLatticePt(pta);
								ppoly->AddPolyCurveIndex(curpt++);
							}
						}
						else if (pttc->wType == TT_PRIM_QSPLINE)
						{
							for (int i = 0; i < pttc->cpfx - 1; i++)
							{
								ptb = ToPoint(pttc->apfx[i]);
								if (i < pttc->cpfx - 2)
								{
									// if not on last spline, compute C
									CPt tmp = ToPoint(pttc->apfx[i+1]);
									ptc = (ptb + tmp) / 2;
								}
								else
								{
									// else, next point is C
									ptc = ToPoint(pttc->apfx[i+1]);
								}

								// run through the spline adding points
								//bool addlast = (local < ptth->cb || i < pttc->cpfx - 2);
								bool addlast = true;
								if (local >= ptth->cb && i == pttc->cpfx - 2 && CmpPoint(pttc->apfx[i+1], ptth->pfxStart))
									addlast = false;
								curpt += LoadQSpline(ppoly, curpt, pta, ptb, ptc, true);

								// update current point
								pta = ptc;
							}
						}
						else if (pttc->wType == TT_PRIM_CSPLINE)
						{
							// not supported yet
						}
					}
				}

				ppoly->LoadComplete();

				// TrueType fonts are always CW (with CCW holes), so invert clockedness
				ppoly->SetClockedness(true, true);
				ppoly->SetDiffuse(COLOR(1, 1, 1));
				ppoly->SetObjectFlags(OBJF_USE_BBOX);

				if (postScript != NULL && postScript[0] != 0)
				{
					CParser parser(&model);
					parser.SetEnvVarModelRef<CGrp>("grp", pgrp);
					parser.SetEnvVarModelRef<CPolygon>("poly", ppoly);
					parser.ParseCommandScript(postScript);
				}

				if (fit)
				{
					CMatrix& mat = pgrp->GetTM();
					mat.Translate(-14.5, -16, -2.5);
					mat.Scale(0.2, 0.2, 0.2);
					//mat.RotateY(20);
					//mat.RotateX(-10);
				}
			}
			else
			{
				ppoly->SetObjectFlags(OBJF_INVISIBLE);
			}

			// add meta-data for the glyph metrics
			char buf[80];
			sprintf_s(buf, 80, "%d,%d", gm.gmBlackBoxX, gm.gmBlackBoxY);
			ppoly->AddMetaData("blackbox", buf);
			sprintf_s(buf, 80, "%d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
			ppoly->AddMetaData("origin", buf);
			sprintf_s(buf, 80, "%d,%d", gm.gmCellIncX, gm.gmCellIncY);
			ppoly->AddMetaData("cellinc", buf);
		}

		SelectObject(memDC, hOld);
		DeleteObject(hFont);
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);

	DeleteDC(memDC);
	return pgrp;
}

static void TestGlyph(CModel& model)
{
	static TCHAR font[] = _T("Arial");
	static TCHAR c = _T('e');

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(5, 2, 10));
	plit->SetHighlight(100);

	CGrp* pgrp = TestGlyph(model, font, (LPCTSTR)&c, "glyph-test.txt", true);
	if (pgrp)
	{
		//CPolygon* pnew = pgrp->CreatePolygon();

		//double ang[5] = { 150, 120, 90, 60, 30 };
		//double len[5] = { 0.5, 0.5, 3, 0.5, 0.5 };
		//double ang[3] = { 120, 90, 45 };
		//double len[3] = { 0.5, 3, 0.5 };
		//ppoly->Extrude(ang, len, _countof(ang), true);
		//ppoly->Extrude(1, true);
		//ppoly->DupExtrudedFacePlate(pnew, true);

		//ppoly->SetDiffuse(COLOR(0, 0, 1));
		//pnew->SetDiffuse(COLOR(1, 0, 0));

		//*pnew = *ppoly;
		//pnew->GetTM()->Translate(0, 0, 2);
		//if (ppoly->DupExtrudedFacePlate(pnew, true))
		{
			//pnew->Extrude(1, true);
			//pnew->SetDiffuse(COLOR(0.0, 0.0, 0.0));
			//pnew->SetSpecular(COLOR(0.8, 0.8, 0.8));
			//pnew->SetReflection(COLOR(1.0, 1.0, 1.0));
			//pnew->SetObjectFlags(OBJF_USE_BBOX);

			//CImageMap* pimages = model.GetImageMap();
			//string rmap = pimages->LoadImageFile("..\\images\\reflect2.jpg");
			//pnew->AddReflectionMap(TXTF_RGB, rmap);
		}

		pgrp->GetTM().RotateY(30);
		pgrp->GetTM().RotateX(45);
	}
}

static void TestGlitch(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(0.8, 0.8, 0.8));
	plit->SetOrigin(CPt(2, 2, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	ppoly->SetObjectFlags(OBJF_REFL_RAY | OBJF_REFR_RAY | OBJF_USE_BBOX);

	CPt pts[4];
	pts[0].SetPoint( 0,  0,  0);
	pts[1].SetPoint( 4,  0,  0);
	pts[2].SetPoint( 2,  2,  0);
	pts[3].SetPoint( 6,  2,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	int inds[4] = { 0, 1, 3, 2 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, NULL, 4);
	ppoly->LoadComplete();

	//double ang[13] = { 90, 84, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30 };
	//double len[13] = { 1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1 };
	double ang[1] = { 45 };
	double len[1] = { 1 };
	ppoly->Extrude(ang, len, 1, true);

	/*CPt pts[3];
	pts[0].SetPoint( 0,  0,  0);
	pts[1].SetPoint( 1,  2,  0);
	pts[2].SetPoint(-1,  2,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	int inds[3] = { 0, 1, 2 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, 3);
	ppoly->LoadComplete();*/

	CMatrix& mat = ppoly->GetTM();
	//mat.Scale(2, 2, 2);
	//mat.RotateX(-45);
	//mat.RotateY(30);
	//mat.RotateZ(30);
}

static void TestExtrude(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 10));
	plit->SetHighlight(100);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	//ppoly->SetSpecular(COLOR(0.1, 0.1, 0.1));
	ppoly->SetBoundBox(true);

	CPt pts[3];
	pts[0].SetPoint( 0,  0,  0);
	pts[1].SetPoint( 4,  0,  0);
	pts[2].SetPoint( 0,  4,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	int inds[4] = { 0, 1, 2 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, NULL, 3);

	ppoly->LoadComplete();

	ppoly->SetClockedness(true, false);
	double ang[5] = { 90, 80, 70, 60, 50 };
	double len[5] = { 1, 0.2, 0.2, 0.2, 0.2 };
	ppoly->Extrude(ang, len, 5, true);
	//ppoly->Extrude(2, true);

	CMatrix& mat = ppoly->GetTM();
	mat.Translate(-2, -2, 0);
	mat.RotateX(-45);
	mat.RotateY(30);
}

static void TestImage(CModel& model)
{
	string title = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGB);
	//string title = model.GetImageMap().LoadImageFile("..\\images\\bliss.bmp");

	model.GetBackgroundHandler().SetBackgroundImage(title, ImageResize::Stretch);

	//CMatrix* pmat = pbg->GetTM();
	//pmat->RotateZ(30);
	//pmat->Scale(2, 1, 1);
}

static void TestTexturing(CModel& model)
{
	COLOR cn(1.0, 0.0, 0.0, 1.0);
	COLOR ce(0.0, 0.0, 0.2, 1.0);
	COLOR cs(0.0, 1.0, 0.0, 1.0);
	model.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

	string map1 = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGBA);
	//string map1 = model.GetImageMap().LoadImageFile("..\\images\\sample-transparent.png", ImageFormat::RGBA);
	//string map2 = model.GetImageMap().LoadImageFile("..\\images\\reflect2.jpg", GreyScale);

	CImageBuffer* pimg = model.GetImageMap().GetImage(map1);
	//pimg->FillAlphaFromTransparentColor(0, 0, 0, 25, 25, 25, 1);
	pimg->FillAlphaFromColors();
	//pimg->NormalizeAlpha();

	/*CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetDirection(CUnitVector(0, 0, -1));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);*/

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(-5, 5, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.8, 0.8, 0.8));
	ppoly->SetSpecular(COLOR(0.2, 0.2, 0.2));
	ppoly->SetRefraction(COLOR(1.0, 1.0, 1.0, 1.0));
	ppoly->SetObjectFlags(OBJF_USE_BBOX);

	CPt pts[4];
	pts[0].SetPoint(-4, -3,  0);
	pts[1].SetPoint( 4, -3,  0);
	pts[2].SetPoint(-4,  3,  0);
	pts[3].SetPoint( 4,  3,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	UVC uvs[4];
	uvs[0].u = 0; uvs[0].v = 0;
	uvs[1].u = 1; uvs[1].v = 0;
	uvs[2].u = 0; uvs[2].v = 1;
	uvs[3].u = 1; uvs[3].v = 1;
	TextureMapType map1type = TextureMapType::Diffuse;
	TextureMapType map2type = TextureMapType::Specular;
	ppoly->AddColorMap(map1type, TextureMapOp::Multiply, TXTF_RGB, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddColorMap(map2type, TXTF_ALPHA, map1, uvs, sizeof(uvs) / sizeof(UVC));
	ppoly->AddBumpMap(TXTF_ALPHA, map1, 2, 0, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddTransparencyMap(TXTF_ALPHA, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//int imap1 = ppoly->AddColorMap(map1type, TXTRF_RGB, map1);
	//int imap2 = ppoly->AddColorMap(map2type, TXTRF_RGB, map2);

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(0);
	ppoly->AddPolyCurveIndex(1);
	ppoly->AddPolyCurveIndex(3);
	ppoly->AddPolyCurveIndex(2);

	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(0, 0));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(1, 0));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(1, 1));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(0, 1));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(0, 0));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(1, 0));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(1, 1));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(0, 1));

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.Scale(1.5, 1.5, 1);
	mat.RotateX(-20);
	//mat.RotateY(30);
	//mat.RotateZ(30);
	//mat.Translate(0, 0, -10);
}

static void TestTransparentPNG(CModel& model)
{
	COLOR cn(1.0, 0.0, 0.0, 1.0);
	COLOR ce(0.0, 0.0, 0.2, 1.0);
	COLOR cs(0.0, 1.0, 0.0, 1.0);
	model.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

	string map1 = model.GetImageMap().LoadImageFile("..\\images\\sample-transparent.png", ImageFormat::RGBA);

	/*CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetDirection(CUnitVector(0, 0, -1));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);*/

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(-5, 5, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.8, 0.8, 0.8));
	ppoly->SetSpecular(COLOR(0.2, 0.2, 0.2));
	ppoly->SetRefraction(COLOR(1.0, 1.0, 1.0, 1.0));
	ppoly->SetObjectFlags(OBJF_REFR_RAY | OBJF_USE_BBOX);

	CPt pts[4];
	pts[0].SetPoint(-4, -3, 0);
	pts[1].SetPoint(4, -3, 0);
	pts[2].SetPoint(-4, 3, 0);
	pts[3].SetPoint(4, 3, 0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	UVC uvs[4];
	uvs[0].u = 0; uvs[0].v = 0;
	uvs[1].u = 1; uvs[1].v = 0;
	uvs[2].u = 0; uvs[2].v = 1;
	uvs[3].u = 1; uvs[3].v = 1;
	TextureMapType map1type = TextureMapType::Diffuse;
	TextureMapType map2type = TextureMapType::Specular;
	ppoly->AddColorMap(map1type, TextureMapOp::Multiply, TXTF_RGB, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddColorMap(map2type, TXTF_ALPHA, map1, uvs, sizeof(uvs) / sizeof(UVC));
	ppoly->AddTransparencyMap(TXTF_ALPHA, map1, uvs, sizeof(uvs) / sizeof(UVC));

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(0);
	ppoly->AddPolyCurveIndex(1);
	ppoly->AddPolyCurveIndex(3);
	ppoly->AddPolyCurveIndex(2);

	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(0, 0));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(1, 0));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(1, 1));
	//ppoly->AddPolyCurveMapCoord(map1type, imap1, UVC(0, 1));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(0, 0));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(1, 0));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(1, 1));
	//ppoly->AddPolyCurveMapCoord(map2type, imap2, UVC(0, 1));

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.Scale(1.5, 1.5, 1);
	mat.RotateX(-20);
	//mat.RotateY(30);
	//mat.RotateZ(30);
	//mat.Translate(0, 0, -10);
}

static void TestTransparency(CModel& model)
{
	COLOR cn(1.0, 0.0, 0.0, 1.0);
	COLOR ce(0.0, 0.0, 0.2, 1.0);
	COLOR cs(0.0, 1.0, 0.0, 1.0);
	model.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

	string map1 = model.GetImageMap().LoadImageFile("..\\images\\earth.jpg", ImageFormat::RGBA);
	//string map1 = model.GetImageMap().LoadImageFile("..\\images\\sample-transparent.png", RGBA);
	//string map2 = model.GetImageMap().LoadImageFile("..\\images\\reflect2.jpg");

	CImageBuffer* pimg = model.GetImageMap().GetImage(map1);
	pimg->FillAlphaFromTransparentColor(10, 149, 218, 50, 50, 50, 0);
	//pimg->FillAlphaFromColors();
	//pimg->NormalizeAlpha();

	/*CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(1, 1, 1));
	CUnitVector dir(0, 1, 0);
	dir.Normalize();
	plit->SetDirection(dir);
	plit->SetShadowCaster(true);*/

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 20));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	CSphere* psphere = NULL;
	psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(0, 0, 0));
	psphere->SetRadius(1);
	psphere->SetDiffuse(COLOR(1.0, 1.0, 1.0));
	psphere->SetSpecular(COLOR(0.3, 0.3, 0.3));
	psphere->SetRefraction(COLOR(1.0, 1.0, 1.0, 1.0));
	//psphere->SetReflection(COLOR(0.3, 0.3, 0.3));
	psphere->SetObjectFlags(OBJF_REFR_RAY | OBJF_USE_BBOX);
	psphere->AddColorMap(TextureMapType::Diffuse, TextureMapOp::Multiply, TXTF_RGB, map1);
	//psphere->AddColorMap(Specular, TXTF_ALPHA, map1);
	//psphere->AddColorMap(Refraction, TXTF_RGB, map1);
	//psphere->AddReflectionMap(TXTF_RGB, map2);
	psphere->AddTransparencyMap(TXTF_ALPHA, map1);
	psphere->AddBumpMap(TXTF_ALPHA, map1, 2, 0);

	CMatrix& mat = psphere->GetTM();
	mat.Scale(3.0, 3.0, 3.0);
	//mat.Translate(1, 4, 0);
}

static void TestGlow(CModel& model)
{
	string map1 = model.GetImageMap().LoadImageFile("..\\images\\reflect2.jpg", ImageFormat::GreyScale);

	//CImageBuffer* pimg = model.GetImageMap().GetImage(map1);
	//pimg->FillAlphaFromColors();
	//pimg->NormalizeAlpha();

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(-5, 5, 10));
	plit->SetHighlight(100);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.5, 0.5, 0.5));
	//ppoly->SetSpecular(COLOR(0.2, 0.2, 0.2));
	//ppoly->SetRefraction(COLOR(1.0, 1.0, 1.0, 0.0));
	ppoly->SetGlow(COLOR(1.0, 0.0, 0.0));
	ppoly->SetObjectFlags(OBJF_USE_BBOX);

	CPt pts[4];
	pts[0].SetPoint(-4, -3,  0);
	pts[1].SetPoint( 4, -3,  0);
	pts[2].SetPoint(-4,  3,  0);
	pts[3].SetPoint( 4,  3,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	UVC uvs[4];
	uvs[0].u = 0; uvs[0].v = 0;
	uvs[1].u = 1; uvs[1].v = 0;
	uvs[2].u = 0; uvs[2].v = 1;
	uvs[3].u = 1; uvs[3].v = 1;
	TextureMapType map1type = TextureMapType::Glow;
	TextureMapType map2type = TextureMapType::Specular;
	ppoly->AddColorMap(map1type, TextureMapOp::Multiply, TXTF_RGB, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddColorMap(map2type, TXTF_ALPHA, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddBumpMap(TXTF_ALPHA, map1, 2, 0, uvs, sizeof(uvs) / sizeof(UVC));

	CUnitVector norm(0, 0, 1);
	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(0);
	ppoly->AddPolyCurveIndex(1);
	ppoly->AddPolyCurveIndex(3);
	ppoly->AddPolyCurveIndex(2);

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.Scale(1.5, 1.5, 1);
	mat.RotateX(-20);
	//mat.RotateY(30);
	//mat.RotateZ(30);
	//mat.Translate(0, -2, 0);
}

static void TestAutoBump(CModel& model)
{
	string map1 = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGBA);

	CImageBuffer* pimg = model.GetImageMap().GetImage(map1);
	pimg->FillAlphaFromColors();
	pimg->NormalizeAlpha();

	CDirLight* plit = model.GetSuperGroup().CreateDirectionalLight();
	plit->SetColor(COLOR(0.7, 0.7, 0.7));
	CUnitVector dir(0, 0, -1);
	dir.Normalize();
	plit->SetDirection(dir);

	CSphere* psphere = NULL;
	psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(0, 2, -2));
	psphere->SetRadius(2);
	psphere->SetDiffuse(COLOR(0, 0, 1));
	psphere->SetBoundBox(true);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(1.0, 1.0, 1.0));
	ppoly->SetSpecular(COLOR(1.0, 1.0, 1.0));
	ppoly->SetObjectFlags(OBJF_REFL_RAY | OBJF_USE_BBOX);

	CPt pts[4];
	//pts[0].SetPoint(-4, -3,  0);
	//pts[1].SetPoint( 4, -3,  0);
	//pts[2].SetPoint(-4,  3,  0);
	//pts[3].SetPoint( 4,  3,  0);
	pts[0].SetPoint(-5,  0,  4);
	pts[1].SetPoint( 5,  0,  4);
	pts[2].SetPoint(-5,  0, -4);
	pts[3].SetPoint( 5,  0, -4);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	UVC uvs[4];
	uvs[0].u = 0; uvs[0].v = 0;
	uvs[1].u = 1; uvs[1].v = 0;
	uvs[2].u = 0; uvs[2].v = 1;
	uvs[3].u = 1; uvs[3].v = 1;
	ppoly->AddColorMap(TextureMapType::Diffuse, TextureMapOp::Multiply, TXTF_RGB, map1, uvs, sizeof(uvs) / sizeof(UVC));
	//ppoly->AddColorMap(Specular, TXTF_ALPHA|TXTF_INVERT, map1, uvs, sizeof(uvs) / sizeof(UVC));
	ppoly->AddBumpMap(TXTF_NONE, "", 2, 0);

	CUnitVector norm(0, 1, 0);
	norm.Normalize();
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
	ppoly->AddPolyCurveIndex(0);
	ppoly->AddPolyCurveIndex(1);
	ppoly->AddPolyCurveIndex(3);
	ppoly->AddPolyCurveIndex(2);

	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.Scale(1.5, 1.5, 1);
	mat.RotateX(25);
	//mat.RotateY(30);
	//mat.RotateZ(30);
	mat.Translate(0, -2, 0);
}

static void TestSoftShadows(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(5, 5, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true, true, 1);

	CPolygon* ppoly = NULL;
	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.7, 0.7));
	ppoly->SetObjectFlags(OBJF_SHADOW_RAY | OBJF_USE_BBOX);

	CPt pts[4];
	pts[0].SetPoint(-5, -4,  0);
	pts[1].SetPoint( 5, -4,  0);
	pts[2].SetPoint(-5,  4,  0);
	pts[3].SetPoint( 5,  4,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	int inds[4] = { 0, 1, 3, 2 };
	ppoly->AddPolyCurve(CUnitVector(0, 0, 1), PCF_NEWPLANE, inds, NULL, 4);
	ppoly->LoadComplete();

	ppoly->GetTM().Translate(0, 0, -5);

	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	ppoly->SetObjectFlags(OBJF_SHADOW_CASTER | OBJF_USE_BBOX);

	pts[0].SetPoint( 0, -2,  0);
	pts[1].SetPoint( 3, -2,  0);
	pts[2].SetPoint( 0,  2,  0);
	pts[3].SetPoint( 3,  2,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	ppoly->AddPolyCurve(CUnitVector(0, 0, 1), PCF_NEWPLANE, inds, NULL, 4);
	ppoly->LoadComplete();

	CMatrix& mat = ppoly->GetTM();
	//mat.RotateZ(20);
	//mat.RotateY(20);
	mat.Translate(0, 0, 2);
}

static void CreateLights(CGrp& grp)
{
	CPtLight* plit = grp.CreatePointLight();
	plit->SetColor(COLOR(0.4, 0.4, 0.4));
	plit->SetOrigin(CPt(5, 8, 5));
	plit->SetHighlight(100);
	//plit->SetShadowCaster(false);

	plit = grp.CreatePointLight();
	plit->SetColor(COLOR(0.3, 0.3, 0.3));
	plit->SetOrigin(CPt(-5, 0, 5));
	plit->SetHighlight(100);
	//plit->SetShadowCaster(false);

	plit = grp.CreatePointLight();
	plit->SetColor(COLOR(0.3, 0.3, 0.3));
	plit->SetOrigin(CPt(-3, 8, 0));
	plit->SetHighlight(100);
	//plit->SetShadowCaster(false);
}

static CPolygon* CreateBackdrop(CModel& model, CGrp* pgrp)
{
	CPolygon* ppoly = pgrp->CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.7, 0.7));
	ppoly->SetSpecular(COLOR(0.3, 0.3, 0.3));
	ppoly->SetBoundBox(true);
	ppoly->SetObjectFlags(OBJF_ALL_RAYS);
	//ppoly->SetObjectFlags(OBJF_SHADOW_RAY);

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

	string dmap = model.GetImageMap().LoadImageFile("..\\images\\marble2.jpg", ImageFormat::RGBA);

	CImageBuffer* pmap = model.GetImageMap().GetImage(dmap);
	pmap->FillAlphaFromColors();
	pmap->NormalizeAlpha();

	int mapping1 = ppoly->AddColorMap(TextureMapType::Diffuse, TextureMapOp::Multiply, TXTF_RGB, dmap);
	int mapping2 = ppoly->AddBumpMap(TXTF_ALPHA | TXTF_INVERT, dmap, 1);

	CPt center(0, 0, 1);
	ppoly->ApplyMapping(TextureMapWrapType::Extrusion, TextureMapType::Diffuse, mapping1, true, &center, NULL);
	ppoly->ApplyMapping(TextureMapWrapType::Extrusion, TextureMapType::Bump, mapping2, true, &center, NULL);

	ppoly->GetTM().Translate(0, 0, -2);

	return ppoly;
}

static double ApplySpline(double t, double a, double b, double c)
{
	return (a - b*2 + c)*t*t + (b*2 - a*2)*t + a;
}

static double GetFrame(int frame, int total, double bias)
{
	double ret = frame/(double) total;
	if (ret < 0)
		ret = 0;
	if (ret > 1)
		ret = 1;
	return ApplySpline(ret, 0, bias, 1);
}

static void TestModelSave(CModel& model)
{
	char file[80];
	strcpy_s(file, 80, "..\\models\\test.mdl");

	CBinFile fileobj(file);
	model.Save(fileobj);
}

static void TestModelLoad(CModel& model)
{
	char file[80];
	strcpy_s(file, 80, "..\\models\\test.mdl");

	CBinFile fileobj(file);
	model.Load(fileobj);
}

static void TestPackageCreate(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 5));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	string map = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGBA);

	CPackage package;
	package.StartNewPackage("..\\models\\test.pkg");

	CPolygon* ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
	ppoly->SetObjectFlags(OBJF_NONE);

	CPt pts[4];
	pts[0].SetPoint(-4,  0,  0);
	pts[1].SetPoint(-2,  0,  0);
	pts[2].SetPoint(-4,  2,  0);
	pts[3].SetPoint(-2,  2,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	CUnitVector norm(0, 0, 1);
	int inds[4] = { 0, 1, 3, 2 };
	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, NULL, 4);
	ppoly->LoadComplete();

	package.AddObject("first", ppoly);

	ppoly = model.GetSuperGroup().CreatePolygon();
	ppoly->SetDiffuse(COLOR(0.0, 0.7, 0.0));
	ppoly->SetObjectFlags(OBJF_NONE);

	pts[0].SetPoint(-1,  0,  0);
	pts[1].SetPoint( 1,  0,  0);
	pts[2].SetPoint(-1,  2,  0);
	pts[3].SetPoint( 1,  2,  0);
	ppoly->LoadLattice(sizeof(pts) / sizeof(CPt), pts);

	ppoly->AddPolyCurve(norm, PCF_NEWPLANE, inds, NULL, 4);
	ppoly->LoadComplete();

	UVC uvs[4];
	uvs[0].u = 0; uvs[0].v = 0;
	uvs[1].u = 1; uvs[1].v = 0;
	uvs[2].u = 0; uvs[2].v = 1;
	uvs[3].u = 1; uvs[3].v = 1;
	ppoly->AddColorMap(TextureMapType::Diffuse, TextureMapOp::Multiply, TXTF_RGB, map, uvs, sizeof(uvs) / sizeof(UVC));

	package.AddObject("second", ppoly);

	CSphere* psphere = model.GetSuperGroup().CreateSphere();
	psphere->SetOrigin(CPt(3, 1, 0));
	psphere->SetRadius(1);
	psphere->SetDiffuse(COLOR(0, 0, 0.7));

	package.AddObject("third", psphere);

	package.CompleteNewPackage();
}

static void TestPackageImport(CModel& model)
{
	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(1, 1, 1));
	plit->SetOrigin(CPt(2, 2, 5));
	plit->SetHighlight(100);
	plit->SetShadowCaster(true);

	string map = model.GetImageMap().LoadImageFile("..\\images\\wood.jpg", ImageFormat::RGBA);

	CBinFile binFile("..\\models\\test.pkg");
	CPackage package;
	package.OpenPackage(&binFile);

	char name[] = "second";
	BlockType bt = package.GetObjectType(name);
	if (bt == BlockType::Sphere)
	{
		CSphere* psphere = model.GetSuperGroup().CreateSphere();
		package.LoadObject(name, psphere);
	}
	else if (bt == BlockType::Polygon)
	{
		CPolygon* ppoly = model.GetSuperGroup().CreatePolygon();
		package.LoadObject(name, ppoly);
	}

	package.ClosePackage();
}

static void TestTextPackageCreate(CModel& model, REND_TREAD_DATA* pdata)
{
	model.SetRenderTarget((CRenderTarget*) pdata->ptarget);
	//CGrp* pgrp = model.GetSuperGroup();

	CPackage package;
	package.StartNewPackage("..\\models\\test.pkg");

	TCHAR font[] = _T("Arial");
	for (TCHAR c = _T('A'); c != _T(' ') + 1; c++)
	{
		CGrp* pgrp = TestGlyph(model, font, (LPCTSTR) &c, NULL, false);
		if (pgrp)
		{
			char name[2];
			name[0] = (char) c;
			name[1] = 0;
			package.AddObject(name, pgrp);
		}

		CMatrix& mat = model.GetSuperGroup().GetTM();
		mat.Translate(-14.5, -16, -2.5);
		mat.Scale(0.2, 0.2, 0.2);
		mat.RotateY(20);
		mat.RotateX(-10);

		CPtLight* plit = model.GetSuperGroup().CreatePointLight();
		plit->SetColor(COLOR(0.8, 0.8, 0.8));
		plit->SetOrigin(CPt(5, 2, 10));
		plit->SetHighlight(100);
		plit->SetShadowCaster(false);

		PostMessage(pdata->hdlg, WM_REND_START, 0, 0);
		model.DoRenderSimple();
		PostMessage(pdata->hdlg, WM_REND_FINISH, 0, 0);

		model.Delete();
		if (c == _T('Z'))
			c = _T('a') - 1;
		else if (c == _T('z'))
			c = _T('0') - 1;
		else if (c == _T('9'))
			c = _T(' ') - 1;
	}

	package.CompleteNewPackage();
}

static void TestTextPackage(CModel& model)
{
	CBinFile binFile("..\\models\\times.pkg");
	CPackage package;
	int nchars = package.OpenPackage(&binFile);

	CPtLight* plit = model.GetSuperGroup().CreatePointLight();
	plit->SetColor(COLOR(0.8, 0.8, 0.8));
	plit->SetOrigin(CPt(5, 2, 10));
	plit->SetHighlight(100);
	plit->SetShadowCaster(false);

	CGrp* psub = model.GetSuperGroup().CreateSubGroup();

	char text[] = "hello world";
	int incx = 0, incy = 0;
	for (int n = 0; n < (int) strlen(text); n++)
	{
		char item[2];
		item[0] = text[n];
		item[1] = 0;

		if (package.GetObjectType(item) == BlockType::Polygon)
		{
			CPolygon* ppoly = psub->CreatePolygon();
			package.LoadObject(item, ppoly);
			ppoly->GetTM().Translate(incx, incy, 0);

			char buf[16], *context;
			ppoly->GetMetaData("cellinc", buf, sizeof(buf));
			incx += atoi(strtok_s(buf, ",", &context));
			incy += atoi(strtok_s(NULL, ",", &context));
		}
	}

	CBoundBox bbox;
	if (psub->ComputeBoundBox(bbox))
	{
		CPt center = bbox.GetCenter();
		CPt minpt = bbox.GetMinPt();
		CPt maxpt = bbox.GetMaxPt();
		double width = maxpt.x - minpt.x;
		double scale = 10 / width;

		CMatrix& mat = psub->GetTM();
		mat.Translate(-center.x, -center.y, -center.z);
		mat.Scale(scale, scale, 2*scale);
		mat.RotateX(-30);
	}
}

static bool TestScript(CParser& parser, CModel& model, HWND hWnd)
{
	bool returnValue = false;
	try
	{
		returnValue = parser.ParseCommandScript("$SCRIPTS\\script.txt");
	}
	catch (const mig_exception& e)
	{
		MessageBoxA(hWnd, e.what(), "Script Error", MB_OK);
	}
	return returnValue;
}

static bool SetupRender(REND_TREAD_DATA* pdata, CModel& model)
{
	CParser parser(&model, NULL, pdata->ptarget->GetRenderInfo());

	//if (!parser.ParseCommandScript("$SCRIPTS\\setup.txt"))
	{
		model.GetCamera().GetTM()
			.SetIdentity()
			.RotateY(180)
			.Translate(0, 0, 15);

		REND_INFO rinfo = pdata->ptarget->GetRenderInfo();
		int width = rinfo.width;
		int height = rinfo.height;
		double aspect = (width / (double)height);
		double minvp = 10; //GetScriptFloat(_T("camera"), _T("minvp"), 10);
		double ulen = (aspect > 1 ? minvp * aspect : minvp);
		double vlen = (aspect < 1 ? minvp / aspect : minvp);
		double dist = 15; //GetScriptFloat(_T("camera"), _T("dist"), 15);
		model.GetCamera().SetViewport(ulen, vlen, dist);

		COLOR cn(1.0, 0.0, 0.0, 1.0);
		COLOR ce(0.0, 0.0, 0.2, 1.0);
		COLOR cs(0.0, 1.0, 0.0, 1.0);
		model.GetBackgroundHandler().SetBackgroundColors(cn, ce, cs);

		COLOR ambient(0.3, 0.3, 0.3);
		model.SetAmbientLight(ambient);

		model.SetSampling(SuperSample::X1);
	}

	bool doRender = true;
	//TestSpheres(model);
	//TestPolygons(model);
	//TestGlyph(model);
	//TestGlitch(model);
	//TestSmoothPolygons(model);
	//TestExtrude(model);
	//TestImage(model);
	//TestTexturing(model);
	//TestTransparentPNG(model);
	//TestTransparency(model);
	//TestGlow(model);
	//TestAutoBump(model);
	//TestSoftShadows(model);

	//TestModelSave(model);
	//TestModelLoad(model);

	//TestPackageCreate(model);
	//TestPackageImport(model);
	//TestTextPackageCreate(model, pdata);
	//TestTextPackage(model);
	doRender = TestScript(parser, model, pdata->hdlg);

	return doRender;
}

// returns frame count, or 1 if no animation, 0 on error
static int SetupGLRender(REND_TREAD_DATA* pdata, CModel& model, CAnimManager& animManager)
{
	int frames = 1;

	CParser parser(&model, &animManager, pdata->ptarget->GetRenderInfo());
	try
	{
		parser.ParseCommandScript("$SCRIPTS\\gl-test.txt");
		if (!parser.GetEnvVar<int>("frames", frames))
			frames = 1;
	}
	catch (const mig_exception& e)
	{
		MessageBoxA(pdata->hdlg, e.what(), "Script Error", MB_OK);
		return 0;
	}

	return frames;
}

static void TestMovie(CModel& model, const REND_INFO& rinfo)
{
	model.SetSampling(SuperSample::Object);

	// create the top level group that will hold the lights, text and backdrop
	CreateLights(model.GetSuperGroup());
	CGrp* pgrp = model.GetSuperGroup().CreateSubGroup();

	// this will hold the text
	CGrp* psub = pgrp->CreateSubGroup();

	// background slab
	CPolygon* pbd = CreateBackdrop(model, pgrp);

	// text
	CBinFile binFile("..\\models\\times.pkg");
	CPackage package;
	int nchars = package.OpenPackage(&binFile);
	char text[8] = "JODI";
	bool bold = false;
	bool italic = false;

	int incxs[8], incys[8];
	memset(incxs, 0, 8 * sizeof(int));
	memset(incys, 0, 8 * sizeof(int));
	CMatrix* ptxtmats[8];
	memset(ptxtmats, 0, 8 * sizeof(CMatrix*));

	int incx = 0, incy = 0;
	for (int n = 0; n < (int)strlen(text); n++)
	{
		char item[8];
		memset(item, 0, sizeof(item));
		item[0] = text[n];
		item[1] = '-';
		if (bold)
		{
			item[2] = 'b';
			if (italic)
				item[3] = 'i';
		}
		else if (italic)
		{
			item[2] = 'i';
		}

		if (package.GetObjectType(item) == BlockType::Group)
		{
			CGrp* ptext = psub->CreateSubGroup();
			package.LoadObject(item, ptext);

			LPCSTR szScript = "glyph-simple.txt";
			if (szScript != NULL)
			{
				CParser parser(&model);
				parser.SetEnvVarModelRef<CGrp>("grp", ptext);
				parser.SetEnvVarModelRef<CPolygon>("poly", (CPolygon*)ptext->GetObjects().at(0).get());
				parser.ParseCommandScript(szScript);
			}

			CMatrix& mat = ptext->GetTM();
			mat.Translate(incx, incy, 0);
			incxs[n] = incx;
			incys[n] = incy;

			char buf[16], * context;
			ptext->GetMetaData("cellinc", buf, sizeof(buf));
			incx += atoi(strtok_s(buf, ",", &context));
			incy += atoi(strtok_s(NULL, ",", &context));

			//ppoly->SetReflection(COLOR(0.5, 0.5, 0.5));
			ptxtmats[n] = &mat;
		}
	}
	nchars = (int)strlen(text);

	//CImageMap* pimages = model.GetImageMap();
	//pimages->LoadImageFile("..\\images\\wood.jpg", RGB, "texture1");
	//pimages->LoadImageFile("..\\images\\reflect2.jpg", RGB, "reflect");

	CBoundBox bbox;
	if (psub->ComputeBoundBox(bbox))
	{
		CPt center = bbox.GetCenter();
		CPt minpt = bbox.GetMinPt();
		CPt maxpt = bbox.GetMaxPt();
		double width = maxpt.x - minpt.x;
		double scale = 10 / width;

		CMatrix& mat = psub->GetTM();
		mat.Translate(-center.x, -center.y, -center.z);
		mat.Scale(scale, scale, scale);
	}

	// run the animation
	char file[80];
	CMatrix& mat = pgrp->GetTM();
	for (int frame = 0; frame < 120; frame++)
	{
		mat.SetIdentity();
		mat.RotateZ(30);
		mat.RotateX(-89.0 + 29.0 * GetFrame(frame, 120, 3));
		mat.Translate(0.5, 2, 0);

		for (int n = 0; n < nchars; n++)
		{
			ptxtmats[n]->SetIdentity();
			ptxtmats[n]->Translate(incxs[n], incys[n], 60 - 60 * (GetFrame(frame - n * 6, 60, 0.95)));
		}

		sprintf_s(file, 80, "..\\movies\\test%03d.bmp", frame + 1);
		CBMPTarget bmp;
		bmp.Init(rinfo.width, rinfo.height, 24, file);
		model.SetRenderTarget((CRenderTarget*)&bmp);
		model.DoRenderSimple();
	}

	TCHAR src[80], dst[80];
	swprintf_s(src, 80, _T("..\\movies\\test%03d.bmp"), 120);
	for (int frame = 120; frame < 150; frame++)
	{
		swprintf_s(dst, 80, _T("..\\movies\\test%03d.bmp"), frame + 1);
		CopyFile(src, dst, FALSE);
	}
}

struct WORKER_THREAD_DATA
{
	CModel* pmodel;
	int nthread;
};

DWORD WINAPI WorkerThreadProc(LPVOID lpParam)
{
	WORKER_THREAD_DATA* pdata = (WORKER_THREAD_DATA*)lpParam;
	pdata->pmodel->DoRender(pdata->nthread);
	delete pdata;
	return 1;
}

//#define USE_MULTI_CORE

static int GetThreadCount()
{
#ifdef USE_MULTI_CORE
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int nthreads = (int)sysinfo.dwNumberOfProcessors;
	if (nthreads < 1)
		return 1;
	return nthreads;
#else
	return 1;
#endif // USE_MULTI_CORE
}

static void ExecuteMultiCoreRender(CModel& model)
{
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

static void TestAnimation(CModel& model, CRenderTarget* ptarget, HWND hwnd)
{
	CAnimManager animManager;

	CParser parser;
	parser.Init(&model, &animManager);

	try
	{
		parser.ParseCommandScript("$SCRIPTS\\movie-test.txt");
	}
	catch (const mig_exception& e)
	{
		MessageBoxA(hwnd, e.what(), "Script Error", MB_OK);
		return;
	}

	// use script suggested parameters, if available
	int frames, width, height;
	if (!parser.GetEnvVar<int>("frames", frames))
		frames = 30;
	if (!parser.GetEnvVar<int>("width", width))
		width = 320;
	if (!parser.GetEnvVar<int>("height", height))
		height = 240;

	// either render frames to the window, or make bitmaps
	bool suppressBitmaps = false;
	parser.GetEnvVar<bool>("suppressBitmaps", suppressBitmaps);

	animManager.StartPlayback(model, frames);
	for (int frame = 1; frame < frames + 1; frame++)
	{
		animManager.GoToFrame(frame);

		if (!suppressBitmaps)
		{
			char file[80];
			sprintf_s(file, 80, "..\\movies\\test%03d.bmp", frame);
			CBMPTarget bmp;
			bmp.Init(width, height, 24, file);

			/*CBufferedTarget buffer;
			buffer.SetTarget(static_cast<CRenderTarget*>(&bmp));

			CSplitterTarget splitter;
			splitter.AddTarget(static_cast<CRenderTarget*>(&buffer));

			CStretchTarget stretcher;
			stretcher.SetTarget(ptarget);
			stretcher.SetSourceRendInfo(bmp.GetRenderInfo());
			splitter.AddTarget(static_cast<CRenderTarget*>(&stretcher));*/

			CSplitterTarget splitter;
			splitter.AddTarget(static_cast<CRenderTarget*>(&bmp));

			CBufferedTarget buffer;
			buffer.SetTarget(static_cast<CRenderTarget*>(&splitter));

			CStretchTarget stretcher;
			stretcher.SetTarget(ptarget);
			stretcher.SetSourceRendInfo(bmp.GetRenderInfo());
			splitter.AddTarget(static_cast<CRenderTarget*>(&stretcher));

			model.SetRenderTarget(static_cast<CRenderTarget*>(&buffer));
			ExecuteMultiCoreRender(model);
		}
		else
		{
			model.SetRenderTarget(ptarget);
			ExecuteMultiCoreRender(model);
		}

		if (ShouldAbortRender())
			break;
	}
	animManager.FinishPlayback();

	// confirm the model is back to normal
	if (ptarget != NULL)
	{
		model.SetRenderTarget(ptarget);
		ExecuteMultiCoreRender(model);
	}
}

// controls either static single frame tests, or animation tests
bool _isAnimation = false;
bool _isGLTest = true;

DWORD WINAPI RendThreadProc(LPVOID lpParam)
{
	REND_TREAD_DATA* pdata = (REND_TREAD_DATA*) lpParam;
	DWORD dwElapsed = 0;

	PostMessage(pdata->hdlg, WM_REND_START, pdata->preview, 0);
	DWORD start = GetTickCount();
	LPARAM suppressInvalidate = 0;

	CEnv::GetEnv().SetHomePath("..");
	{
		CModel model;
		if (!_isAnimation)
		{
			if (_isGLTest)
			{
				CAnimManager animManager;
				int frames = SetupGLRender(pdata, model, animManager);
				if (frames > 0)
				{
					if (pdata->preview)
					{
						GLTest(model, animManager, pdata->hrend, frames);
						suppressInvalidate = 1;
					}
					else
					{
						model.SetRenderTarget((CRenderTarget*)pdata->ptarget);
						model.DoRenderSimple();
					}
				}
			}
			else
			{
				if (SetupRender(pdata, model))
				{
					model.SetRenderTarget((CRenderTarget*)pdata->ptarget);
					model.DoRenderSimple();
				}
			}
		}
		else
		{
			// default movie quality is low, script can override
			model.SetRenderQuality(REND_NONE);
			TestAnimation(model, pdata->ptarget, pdata->hdlg);
		}
	}

	dwElapsed = (GetTickCount() - start);
	PostMessage(pdata->hdlg, WM_REND_FINISH, dwElapsed, suppressInvalidate);
	delete pdata;
	return 1;
}

const TCHAR* RendPixelDebug(REND_TREAD_DATA* pdata, int x, int y)
{
	static TCHAR szErr[MAX_PATH];
	szErr[0] = 0;

	CModel model;
	if (SetupRender(pdata, model))
	{
		model.SetRenderTarget((CRenderTarget*)pdata->ptarget);
		COLOR col = model.DoRenderPixel(x, y);
		_stprintf_s(szErr, _T("DoRenderPixel() completed: %f %f %f %f"), col.r, col.g, col.b, col.a);
	}

	return szErr;
}

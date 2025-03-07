#include "StdAfx.h"

#include "createthread.h"
#include "model.h"
#include "bmpio.h"
#include "fileio.h"
#include "package.h"
#include "parser.h"

using namespace MigRender;

static const string _GroupName = "grp";
static const string _OutputFileName = "outputFileName";

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

static void ConvertAndSendError(HWND hWnd, const string& error)
{
	TCHAR* pupdate = new TCHAR[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, error.c_str(), -1, pupdate, MAX_PATH);
	PostMessage(hWnd, WM_CREATE_UPDATE, (WPARAM)pupdate, 0);
}

static CGrp* ProcessGlyph(CModel& model, CREATE_FONT_DATA* pdata, char theChar, UVC* uvmin, UVC* uvmax, string& suffix)
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
	int nWeight = pdata->nWeight;
	BOOL bItalic = pdata->bItalic;
	BOOL bUnderline = pdata->bUnderline;
	BOOL bStrikeout = FALSE;
	DWORD dwCharSet = DEFAULT_CHARSET;
	DWORD dwOutputPrecision = OUT_DEFAULT_PRECIS;
	DWORD dwClipPrecision = CLIP_DEFAULT_PRECIS;
	DWORD dwQuality = PROOF_QUALITY;
	DWORD dwPitchAndFamily = VARIABLE_PITCH;
	HFONT hFont = CreateFont(nHeight, nWidth, nEscapement, nOrientation, nWeight,
		bItalic, bUnderline, bStrikeout,
		dwCharSet, dwOutputPrecision, dwClipPrecision, dwQuality, dwPitchAndFamily, pdata->szFont);
	if (hFont)
	{
		HGDIOBJ hOld = SelectObject(memDC, hFont);

	    MAT2 m;
		memset(&m, 0, sizeof(MAT2));
		m.eM11.value = 1;
		m.eM22.value = 1;

		GLYPHMETRICS gm;
		DWORD size = GetGlyphOutline(memDC, theChar, GGO_NATIVE, &gm, 0, NULL, &m);
		if (size != GDI_ERROR)
		{
			//pgrp = model.GetSuperGroup;
			pgrp = model.GetSuperGroup().CreateSubGroup();
			CPolygon* ppoly = pgrp->CreatePolygon();
			CUnitVector norm(0, 0, 1);
			//norm.Normalize();

			if (size > 0)
			{
				LPBYTE bytes = new BYTE[size];
				DWORD ret = GetGlyphOutline(memDC, theChar, GGO_NATIVE, &gm, size, bytes, &m);

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
								curpt += LoadQSpline(ppoly, curpt, pta, ptb, ptc, addlast);

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

				// TrueType fonts are alwas CW (with CCW holes), so invert clockedness
				// TODO: in rare instances, this isn't true, see 'H' in Times New Roman
				ppoly->SetClockedness(true, true);

				char szScript[MAX_PATH];
				WideCharToMultiByte(CP_UTF8, 0, pdata->szScript, -1, szScript, _countof(szScript), NULL, NULL);

				try
				{
					CParser parser(&model);
					parser.SetEnvVarModelRef<CGrp>("grp", pgrp);
					parser.SetEnvVarModelRef<CPolygon>("poly", ppoly);
					parser.SetEnvVar<UVC>("uvmin", *uvmin);
					parser.SetEnvVar<UVC>("uvmax", *uvmax);
					if (parser.ParseCommandScript(szScript))
						parser.GetEnvVar<string>("suffix", suffix);
				}
				catch (const mig_exception& e)
				{
					ConvertAndSendError(pdata->hdlg, e.what());
					return NULL;
				}

				/*if (bevel)
				{
					double ang[6] = { 90, 75, 60, 45, 30, 15 };
					double len[6] = { 4, 0.3, 0.3, 0.3, 0.3, 0.3 };
					ppoly->Extrude(ang, len, 6, true);
				}
				else
					ppoly->Extrude(4, true);

				ppoly->SetDiffuse(COLOR(0.7, 0.0, 0.0));
				ppoly->SetObjectFlags(OBJF_USE_BBOX);

				if (map)
				{
					//CImageMap* pimages = model.GetImageMap();
					//string dmap = pimages->LoadImageFile("c:\\source code\\migrender\\images\\wood.jpg", RGBA);
					//string rmap = pimages->LoadImageFile("c:\\source code\\migrender\\images\\reflect2.jpg");
					string dmap = "texture";
					string rmap = "reflect";

					//CImageBuffer* pmap = pimages->GetImage(dmap);
					//pmap->FillAlphaFromColors();
					//pmap->NormalizeAlpha();

					ppoly->SetDiffuse(COLOR(1, 1, 1));
					ppoly->SetSpecular(COLOR(0.3, 0.3, 0.3));
					ppoly->SetReflection(COLOR(1.0, 1.0, 1.0));
					int mapping1 = ppoly->AddColorMap(Diffuse, TXTF_RGB, dmap);
					//int mapping2 = ppoly->AddBumpMap(TXTF_ALPHA, dmap);
					ppoly->AddReflectionMap(TXTF_RGB, rmap);

					CPt center(15, 15, -5);
					ppoly->ApplyMapping(Extrusion, Diffuse, mapping1, true, &center, NULL, uvmin, uvmax);
					//ppoly->ApplyMapping(TMAPW_BUMP, Specular, mapping2, true, &center, NULL);
				}*/

				delete bytes;
			}
			else
			{
				ppoly->SetObjectFlags(OBJF_INVISIBLE);
			}

			// add meta-data for the glyph metrics
			char buf[80];
			sprintf_s(buf, 80, "%d,%d", gm.gmBlackBoxX, gm.gmBlackBoxY);
			pgrp->AddMetaData("blackbox", buf);
			sprintf_s(buf, 80, "%d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
			pgrp->AddMetaData("origin", buf);
			sprintf_s(buf, 80, "%d,%d", gm.gmCellIncX, gm.gmCellIncY);
			pgrp->AddMetaData("cellinc", buf);
		}

		SelectObject(memDC, hOld);
		DeleteObject(hFont);
	}
	else
		MessageBeep(MB_ICONEXCLAMATION);

	DeleteDC(memDC);
	return pgrp;
}

static BOOL FontPackageCreateInner(CPackage& package, CREATE_FONT_DATA* pdata)
{
	CModel model;

	vector<char> items;
	for (char c = 'a'; c != 'z' + 1; c++)
		items.push_back(c);
	for (char c = 'A'; c != 'Z' + 1; c++)
		items.push_back(c);
	for (char c = '0'; c != '9' + 1; c++)
		items.push_back(c);
	items.push_back(' ');
	items.push_back('!');
	items.push_back('@');
	items.push_back('#');
	items.push_back('$');
	items.push_back('%');
	items.push_back('^');
	items.push_back('&');
	items.push_back('*');
	items.push_back('(');
	items.push_back(')');
	items.push_back('+');
	items.push_back('-');
	items.push_back('=');
	items.push_back('_');
	items.push_back('[');
	items.push_back(']');
	items.push_back('\\');
	items.push_back('|');
	items.push_back('{');
	items.push_back('}');
	items.push_back(';');
	items.push_back(':');
	items.push_back('\'');
	items.push_back('\"');
	items.push_back(',');
	items.push_back('.');
	items.push_back('<');
	items.push_back('>');
	items.push_back('/');
	items.push_back('?');
	items.push_back('`');
	items.push_back('~');

	string suffix;
	UVC uvmin, uvmax;
	for (int n = 0; n < (int) items.size(); n++)
	{
		uvmin.u = (double) (n%pdata->xtxtdiv) / pdata->xtxtdiv;
		uvmin.v = (double) ((n/pdata->ytxtdiv)%pdata->ytxtdiv) / pdata->ytxtdiv;
		uvmax.u = uvmin.u + (1.0/pdata->xtxtdiv);
		uvmax.v = uvmin.v + (1.0/pdata->ytxtdiv);

		char c = items[n];
		CGrp* pgrp = ProcessGlyph(model, pdata, c, &uvmin, &uvmax, suffix);
		if (pgrp)
		{
			string key = std::string(1, c) + "-";
			if (pdata->nWeight == FW_BOLD)
				key += "b";
			if (pdata->bItalic)
				key += "i";
			if (pdata->bUnderline)
				key += "u";
			if (!suffix.empty())
				key += "-" + suffix;
			package.AddObject(key.c_str(), pgrp);
		}
		else
			return FALSE;

		model.Delete();
		if (c == _T('Z'))
			c = _T('a') - 1;
		else if (c == _T('z'))
			c = _T(' ') - 1;
	}

	return TRUE;
}

static BOOL FontPackageCreate(CPackage& package, CREATE_FONT_DATA* pdata)
{
	if (pdata->bComplete)
	{
		pdata->nWeight = FW_NORMAL; pdata->bItalic = FALSE; pdata->bUnderline = FALSE;
		if (!FontPackageCreateInner(package, pdata))
			return FALSE;
		pdata->nWeight = FW_BOLD; pdata->bItalic = FALSE; pdata->bUnderline = FALSE;
		if (!FontPackageCreateInner(package, pdata))
			return FALSE;
		pdata->nWeight = FW_NORMAL; pdata->bItalic = TRUE; pdata->bUnderline = FALSE;
		if (!FontPackageCreateInner(package, pdata))
			return FALSE;
		pdata->nWeight = FW_BOLD; pdata->bItalic = TRUE; pdata->bUnderline = FALSE;
		if (!FontPackageCreateInner(package, pdata))
			return FALSE;
	}
	else
	{
		if (!FontPackageCreateInner(package, pdata))
			return FALSE;
	}

	return TRUE;
}

static BOOL FontPackageCreate(CREATE_FONT_DATA* pdata)
{
	char szOutput[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, pdata->szOutput, -1, szOutput, _countof(szOutput), NULL, NULL);

	CPackage package;
	package.StartNewPackage(szOutput);
	BOOL ok = FontPackageCreate(package, pdata);
	if (ok)
		package.CompleteNewPackage();
	return ok;
}

DWORD WINAPI CreateFontProc(LPVOID lpParam)
{
	CREATE_FONT_DATA* pdata = (CREATE_FONT_DATA*) lpParam;

	PostMessage(pdata->hdlg, WM_CREATE_START, 0, 0);
	BOOL ok = FontPackageCreate(pdata);
	PostMessage(pdata->hdlg, WM_CREATE_FINISH, ok, 0);

	delete pdata;
	return 1;
}

static void ExecuteScript(HWND hWnd, const char* script)
{
	CModel model;
	CGrp* pgrp = model.GetSuperGroup().CreateSubGroup();

	try
	{
		CParser parser(&model);
		parser.SetEnvVarModelRef<CGrp>(_GroupName, pgrp);
		if (parser.ParseCommandScript(script))
		{
			string path;
			if (parser.GetEnvVar<string>(_OutputFileName, path))
			{
				CBinFile binFile(path.c_str());
				if (binFile.OpenFile(true, MigType::Object))
				{
					pgrp->Save(binFile);
					binFile.CloseFile();
				}
			}
		}
	}
	catch (const mig_exception& e)
	{
		TCHAR* pupdate = new TCHAR[MAX_PATH];
		MultiByteToWideChar(CP_UTF8, 0, e.what(), -1, pupdate, MAX_PATH);
		PostMessage(hWnd, WM_CREATE_UPDATE, (WPARAM)pupdate, 0);
	}
}

DWORD WINAPI CreateAssetProc(LPVOID lpParam)
{
	CREATE_ASSET_DATA* pdata = (CREATE_ASSET_DATA*)lpParam;

	char szScript[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, pdata->szScript, -1, szScript, _countof(szScript), NULL, NULL);

	ExecuteScript(pdata->hdlg, szScript);
	return 1;
}

DWORD WINAPI CreateBatchProc(LPVOID lpParam)
{
	BOOL abort = FALSE;

	CREATE_BATCH_DATA* pdata = (CREATE_BATCH_DATA*)lpParam;
	PostMessage(pdata->hdlg, WM_CREATE_START, 0, 0);

	vector<FONT_SETTING>::iterator fontIter = pdata->fontSettings.begin();
	while (!abort && fontIter != pdata->fontSettings.end())
	{
		CPackage package;
		package.StartNewPackage(fontIter->filename.c_str());

		vector<string>::iterator iter = pdata->fontScripts.begin();
		while (!abort && iter != pdata->fontScripts.end())
		{
			CREATE_FONT_DATA fontData;
			fontData.hdlg = pdata->hdlg;
			MultiByteToWideChar(CP_UTF8, 0, fontIter->font.c_str(), -1, fontData.szFont, _countof(fontData.szFont));
			MultiByteToWideChar(CP_UTF8, 0, fontIter->filename.c_str(), -1, fontData.szOutput, _countof(fontData.szOutput));
			MultiByteToWideChar(CP_UTF8, 0, (*iter).c_str(), -1, fontData.szScript, _countof(fontData.szScript));
			fontData.xtxtdiv = fontIter->xdiv;
			fontData.ytxtdiv = fontIter->ydiv;
			fontData.nWeight = (fontIter->bold ? FW_BOLD : FW_NORMAL);
			fontData.bItalic = fontIter->italic;
			fontData.bUnderline = fontIter->underline;
			fontData.bComplete = fontIter->complete;

			TCHAR* pupdate = new TCHAR[MAX_PATH];
			swprintf_s(pupdate, MAX_PATH, _T("Processing font %s..."), fontData.szFont);
			PostMessage(pdata->hdlg, WM_CREATE_UPDATE, (WPARAM) pupdate, 0);
			if (!FontPackageCreate(package, &fontData))
				abort = TRUE;

			iter++;
		}

		if (!abort)
			package.CompleteNewPackage();

		fontIter++;
	}

	if (!abort)
	{
		TCHAR* pupdate = new TCHAR[MAX_PATH];
		swprintf_s(pupdate, MAX_PATH, _T("Processing asset scripts..."));
		PostMessage(pdata->hdlg, WM_CREATE_UPDATE, (WPARAM)pupdate, 0);

		vector<string>::iterator iter = pdata->assetScripts.begin();
		while (iter != pdata->assetScripts.end())
		{
			ExecuteScript(pdata->hdlg, (*iter).c_str());
			iter++;
		}
	}

	PostMessage(pdata->hdlg, WM_CREATE_FINISH, !abort, 0);
	delete pdata;
	return 1;
}

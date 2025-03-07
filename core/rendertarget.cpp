// rendertarget.cpp - defines the rendering target base class
//

#include "rendertarget.h"
#include <map>
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CRenderTarget
//-----------------------------------------------------------------------------

void CRenderTarget::PreRender(int nthreads)
{
}

void CRenderTarget::PostRender(bool success)
{
}

//-----------------------------------------------------------------------------
// CRenderTargetThreadSafe (NOT COMPLETE)
//-----------------------------------------------------------------------------
/*
typedef map<int, dword*> LineMap;

CRenderTargetThreadSafe::CRenderTargetThreadSafe()
{
	m_pTarget = NULL;
	m_pLineMap = NULL;
	m_isThreadSafe = false;
}

bool CRenderTargetThreadSafe::FlushLines()
{
	while (true)
	{
		LineMap::iterator iter = ((LineMap*)m_pLineMap)->find(m_nextLine);
		if (iter != ((LineMap*)m_pLineMap)->end())
		{
			dword* pline = iter->second;
			((LineMap*)m_pLineMap)->erase(iter);

			bool ok = m_pTarget->DoLine(m_nextLine, pline);
			m_nextLine += m_dir;
			delete pline;

			if (!ok)
				return false;
		}
		else
			break;
	}

	return true;
}

void CRenderTargetThreadSafe::FlushAllLines()
{
	LineMap::iterator iter = ((LineMap*)m_pLineMap)->begin();
	while (iter != ((LineMap*)m_pLineMap)->end())
	{
		dword* pline = iter->second;
		delete pline;
		iter++;
	}

	((LineMap*)m_pLineMap)->clear();
}

void CRenderTargetThreadSafe::SetTarget(CRenderTarget* ptarget)
{
	m_pTarget = ptarget;

	REND_INFO rinfo = m_pTarget->GetRenderInfo();
	//m_isThreadSafe = rinfo.isthreadsafe;
	m_nextLine = (rinfo.topdown ? rinfo.height - 1 : 0);
	m_lastLine = (rinfo.topdown ? -1 : rinfo.height);
	m_dir = (rinfo.topdown ? -1 : 1);
}

void CRenderTargetThreadSafe::PreRender(int nthreads)
{
	// we don't care about thread safety if it's not an issue
	if (nthreads == 1)
		m_isThreadSafe = true;

	if (!m_isThreadSafe)
	{
		m_pLineMap = (void*) new LineMap();
	}
}

bool CRenderTargetThreadSafe::DoLine(int y, const dword *pline)
{
	bool ret = true;

	if (m_isThreadSafe)
	{
		ret = m_pTarget->DoLine(y, pline);
		delete pline;
	}
	else
	{
		if (y == m_nextLine)
		{
			ret = m_pTarget->DoLine(y, pline);
			delete pline;
			m_nextLine += m_dir;
		}
		else
		{
			(*((LineMap*)m_pLineMap))[y] = pline;
		}

		if (ret)
			ret = FlushLines();
	}

	return ret;
}

void CRenderTargetThreadSafe::PostRender(void)
{
	FlushAllLines();
	m_pTarget = NULL;
}
*/
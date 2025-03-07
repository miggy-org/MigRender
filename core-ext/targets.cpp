// targets.cpp - common render target classes
//

#include "targets.h"
#include "../core/migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CSplitterTarget
//-----------------------------------------------------------------------------

void CSplitterTarget::AddTarget(CRenderTarget* ptarget)
{
	_targets.push_back(ptarget);
}

void CSplitterTarget::ReplaceTarget(size_t index, CRenderTarget* ptarget)
{
	if (index >= 0 && index < _targets.size())
		_targets[index] = ptarget;
}

REND_INFO CSplitterTarget::GetRenderInfo(void) const
{
	return _targets[0]->GetRenderInfo();
}

void CSplitterTarget::PreRender(int nthreads)
{
	for (const auto& target : _targets)
		target->PreRender(nthreads);
}

bool CSplitterTarget::DoLine(int y, const dword* pline)
{
	for (const auto& target : _targets)
	{
		if (!target->DoLine(y, pline))
			return false;
	}

	return true;
}

void CSplitterTarget::PostRender(bool success)
{
	for (const auto& target : _targets)
		target->PostRender(success);
}

//-----------------------------------------------------------------------------
// CStretchTarget
//-----------------------------------------------------------------------------

CStretchTarget::CStretchTarget()
	: _target(NULL), _lastY(0), _scaleX(0), _scaleY(0), _lastStretchedY(0)
{
	_srcInfo = {};
	_dstInfo = {};
}

static bool IsValidRendInfo(const REND_INFO& rinfo)
{
	return (rinfo.width > 0 && rinfo.height > 0 && (rinfo.fmt == ImageFormat::RGBA || rinfo.fmt == ImageFormat::BGRA) && !rinfo.topdown);
}

void CStretchTarget::SetTarget(CRenderTarget* ptarget)
{
	_target = ptarget;
}

void CStretchTarget::SetSourceRendInfo(const REND_INFO& rinfo)
{
	_srcInfo = rinfo;
}

REND_INFO CStretchTarget::GetRenderInfo(void) const
{
	return _srcInfo;
}

void CStretchTarget::PreRender(int nthreads)
{
	if (_target == NULL || !IsValidRendInfo(_srcInfo))
		throw prerender_exception("No stretch target, or source info is invalid");

	_dstInfo = _target->GetRenderInfo();
	_scaleX = _dstInfo.width / (double)_srcInfo.width;
	_scaleY = _dstInfo.height / (double)_srcInfo.height;
	_lastY = (_dstInfo.topdown ? _dstInfo.height : -1);

	_lastLine.resize(_srcInfo.width);

	_target->PreRender(nthreads);
}

static unsigned char BlendComponent(int v1, int v2, double factor)
{
	return (unsigned char)(v1 + (v2 - v1) * factor);
}

static dword BlendPixel(dword v1, dword v2, double factor)
{
	dword c1 = BlendComponent((v1 & 0xFF000000) >> 24, (v2 & 0xFF000000) >> 24, factor);
	dword c2 = BlendComponent((v1 & 0x00FF0000) >> 16, (v2 & 0x00FF0000) >> 16, factor);
	dword c3 = BlendComponent((v1 & 0x0000FF00) >> 8, (v2 & 0x0000FF00) >> 8, factor);
	dword c4 = BlendComponent((v1 & 0x000000FF), (v2 & 0x000000FF), factor);
	return (c1 << 24) + (c2 << 16) + (c3 << 8) + c4;
}

static dword BlendPixel(dword ul, dword ur, dword ll, dword lr, double xfactor, double yfactor)
{
	dword l = BlendPixel(ul, ll, yfactor);
	dword r = BlendPixel(ur, lr, yfactor);
	return BlendPixel(l, r, xfactor);
}

bool CStretchTarget::DoLine(int y, const dword* pline)
{
	bool ok = true;

	double stretchedY = _scaleY * y;
	if (stretchedY > _lastY)
	{
		std::vector<dword> output(_dstInfo.width);

		while (stretchedY > _lastY && ok)
		{
			double yfactor = (stretchedY == _lastStretchedY ? 0 : ((int)stretchedY - _lastStretchedY) / (stretchedY - _lastStretchedY));

			double lastX = 0;
			for (int x = 0; x < _dstInfo.width; x++)
			{
				int srcX = (int) (x / _scaleX);
				if (srcX >= _srcInfo.width - 1)
					srcX = _srcInfo.width - 2;

				double xfactor = (x == lastX ? 0 : ((int)x - lastX) / (x - lastX));
				output[x] = BlendPixel(
					pline[(int)srcX],
					pline[(int)srcX + 1],
					_lastLine[(int)srcX],
					_lastLine[(int)srcX + 1],
					xfactor, yfactor);

				lastX = x;
			}

			_lastY++;
			if (_lastY < _dstInfo.height && !_target->DoLine(_lastY, output.data()))
				ok = false;
		}
	}
	memcpy(_lastLine.data(), pline, _srcInfo.width * sizeof(dword));
	_lastStretchedY = stretchedY;

	return ok;
}

void CStretchTarget::PostRender(bool success)
{
	_lastLine.clear();

	_target->PostRender(success);
}

//-----------------------------------------------------------------------------
// CBufferedTarget
//-----------------------------------------------------------------------------

CBufferedTarget::CBufferedTarget() : _target(NULL)
{
}

void CBufferedTarget::SetTarget(CRenderTarget* ptarget)
{
	_target = ptarget;
}

// CRenderTarget
REND_INFO CBufferedTarget::GetRenderInfo(void) const
{
	return _target->GetRenderInfo();
}

void CBufferedTarget::PreRender(int nthreads)
{
	REND_INFO rinfo = GetRenderInfo();
	if (rinfo.fmt != ImageFormat::RGBA && rinfo.fmt != ImageFormat::BGRA)
		throw prerender_exception("Unsupported buffered target image format: " + std::to_string(static_cast<int>(rinfo.fmt)));
	_buffer.Init(rinfo.width, rinfo.height, rinfo.fmt);
	_buffer.PreRender(nthreads);
}

bool CBufferedTarget::DoLine(int y, const dword* pline)
{
	return _buffer.DoLine(y, pline);
}

void CBufferedTarget::PostRender(bool success)
{
	_buffer.PostRender(success);

	if (success)
	{
		_target->PreRender(1);

		REND_INFO rinfo = GetRenderInfo();
		std::vector<dword> line(rinfo.width);
		int start = (rinfo.topdown ? rinfo.height - 1 : 0);
		int end = (rinfo.topdown ? -1 : rinfo.height);
		int dir = (rinfo.topdown ? -1 : 1);
		for (int y = start; y != end; y += dir)
		{
			_buffer.GetLine(y, rinfo.fmt, (unsigned char*)line.data());
			_target->DoLine(y, line.data());
		}

		_target->PostRender(true);
	}
}

//-----------------------------------------------------------------------------
// CPassThroughTarget
//-----------------------------------------------------------------------------

CPassThroughTarget::CPassThroughTarget(void) : _target(NULL)
{
}

CPassThroughTarget::CPassThroughTarget(CRenderTarget* ptarget) : _target(ptarget)
{
}

void CPassThroughTarget::SetTarget(CRenderTarget* ptarget)
{
	_target = ptarget;
}

REND_INFO CPassThroughTarget::GetRenderInfo(void) const
{
	return _target->GetRenderInfo();
}

void CPassThroughTarget::PreRender(int nthreads)
{
	_target->PreRender(nthreads);
}

bool CPassThroughTarget::DoLine(int y, const dword* pline)
{
	return _target->DoLine(y, pline);
}

void CPassThroughTarget::PostRender(bool success)
{
	_target->PostRender(success);
}

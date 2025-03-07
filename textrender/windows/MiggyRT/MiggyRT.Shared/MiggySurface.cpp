#include "pch.h"
#include "MiggySurface.h"

#include <dxgi.h>
#include <dxgi1_2.h>

using namespace MiggyRT;

MiggySurface::MiggySurface(int pixelWidth, int pixelHeight)
	: SurfaceImageSource(pixelWidth, pixelHeight, true)
{
	// Global variable that contains the width,
	// in pixels, of the SurfaceImageSource.
	m_width = pixelWidth;
	// Global variable that contains the height, 
	// in pixels, of the SurfaceImageSource.
	m_height = pixelHeight;
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

int MiggySurface::GetWidth()
{
	return m_width;
}

int MiggySurface::GetHeight()
{
	return m_height;
}

void MiggySurface::CreateDeviceResources()
{
	// This flag adds support for surfaces with a different color channel ordering
	// from the API default. It’s required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers.
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// This array defines the set of DirectX hardware feature levels this
	// app will support. Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required
	// feature level in its description. All applications are assumed
	// to support 9.1 unless otherwise stated.
	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	// Create the Direct3D 11 API device object.
	D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		// Set this to D3D_SDK_VERSION for Windows Store apps.
		D3D11_SDK_VERSION,
		// Returns the Direct3D device created in a global var.
		&m_d3dDevice,
		nullptr,
		nullptr);

	// Get the Direct3D API device.
	ComPtr<IDXGIDevice> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	// Create the Direct2D device object and a
	// corresponding device context.
	D2D1CreateDevice(dxgiDevice.Get(), nullptr, &m_d2dDevice);
	m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);

	// Associate the DXGI device with the SurfaceImageSource.
	m_sisNative->SetDevice(dxgiDevice.Get());
}

void MiggySurface::CreateDeviceIndependentResources()
{
	// Query for ISurfaceImageSourceNative interface.
	reinterpret_cast<IUnknown*>(this)->QueryInterface(IID_PPV_ARGS(&m_sisNative));
}

D2D1_PIXEL_FORMAT MiggySurface::GetPixelFormat()
{
	D2D1_PIXEL_FORMAT fmt = D2D1::PixelFormat();
	if (m_bitmap != NULL)
		fmt = m_bitmap->GetPixelFormat();
	else
	{
		ComPtr<IDXGISurface> surface;
		RECT tempRect = { 0, 0, 1, 1 };
		POINT offset;

		// begin drawing to a fake bitmap just to determine pixel format
		HRESULT hres = m_sisNative->BeginDraw(tempRect, &surface, &offset);
		if (hres == S_OK)
		{
			ComPtr<ID2D1Bitmap1> bitmap;
			hres = m_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), nullptr, &bitmap);
			if (hres == S_OK)
				fmt = bitmap->GetPixelFormat();
			m_sisNative->EndDraw();
		}
	}
	return fmt;
}

IFMT MiggySurface::GetPreferredImageFormat(const D2D1_PIXEL_FORMAT& pfmt)
{
	IFMT fmt = IFMT_NONE;

	switch (pfmt.format)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		fmt = IFMT_BGRA;
		break;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
		fmt = IFMT_RGBA;
		break;
	}

	return fmt;
}

bool MiggySurface::BeginDraw()
{
	Windows::Foundation::Rect updateRect(0, 0, (float) m_width, (float) m_height);
	return BeginDraw(updateRect);
}

bool MiggySurface::BeginDraw(Windows::Foundation::Rect updateRect)
{
	POINT offset;
	ComPtr<IDXGISurface> surface;

	// Express target area as a native RECT type.
	RECT updateRectNative;
	updateRectNative.left = static_cast<LONG>(updateRect.Left);
	updateRectNative.top = static_cast<LONG>(updateRect.Top);
	updateRectNative.right = static_cast<LONG>(updateRect.Right);
	updateRectNative.bottom = static_cast<LONG>(updateRect.Bottom);

	// Begin drawing - returns a target surface and an offset
	// to use as the top-left origin when drawing.
	HRESULT beginDrawHR = m_sisNative->BeginDraw(updateRectNative, &surface, &offset);
	if (beginDrawHR == DXGI_ERROR_DEVICE_REMOVED ||
		beginDrawHR == DXGI_ERROR_DEVICE_RESET)
	{
		// If the device has been removed or reset, attempt to
		// re-create it and continue drawing.
		CreateDeviceResources();
		BeginDraw(updateRect);
	}
	else if (beginDrawHR != S_OK)
		return false;

	// Create render target.
	HRESULT hres = m_d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), nullptr, &m_bitmap);
	if (hres != S_OK)
	{
		m_sisNative->EndDraw();
		return false;
	}

	// Set context's render target.
	m_d2dContext->SetTarget(m_bitmap.Get());

	// Begin drawing using D2D context.
	m_d2dContext->BeginDraw();

	// Apply a clip and transform to constrain updates to the target update
	// area. This is required to ensure coordinates within the target surface
	// remain consistent by taking into account the offset returned by
	// BeginDraw, and can also improve performance by optimizing the area
	// that's drawn by D2D. Apps should always account for the offset output
	// parameter returned by BeginDraw, because it might not match the passed
	// updateRect input parameter's location.
	m_d2dContext->PushAxisAlignedClip(
		D2D1::RectF(
			static_cast<float>(offset.x),
			static_cast<float>(offset.y),
			static_cast<float>(offset.x + updateRect.Width),
			static_cast<float>(offset.y + updateRect.Height)),
		D2D1_ANTIALIAS_MODE_ALIASED);
	m_d2dContext->SetTransform(
		D2D1::Matrix3x2F::Translation(
		static_cast<float>(offset.x),
		static_cast<float>(offset.y)
	));

	return true;
}

bool MiggySurface::TestDraw(void)
{
	if (m_bitmap == NULL)
		return false;
	ID2D1Bitmap* pibmp = m_bitmap.Get();

	D2D1_SIZE_U siz{ m_width, m_height };
	D2D1_RECT_U ru;
	ru.left = 0;
	ru.right = siz.width + 1;

	UINT32 pixelWidth = 4 * siz.width;
	BYTE* pmem = new BYTE[pixelWidth];
	for (int y = 0; y < (int)siz.height; y++)
	{
		ru.top = y;
		ru.bottom = y + 1;

		for (int x = 0; x < (int)siz.width; x++)
		{
			if (y == (int)siz.height - 1 || x == (int)siz.width - 1)
			{
				pmem[4 * x + 0] = 0;
				pmem[4 * x + 1] = 0;
				pmem[4 * x + 2] = 255;
				pmem[4 * x + 3] = 255;
			}
			else
			{
				pmem[4 * x + 0] = y % 256;
				pmem[4 * x + 1] = 0;
				pmem[4 * x + 2] = y % 128;
				pmem[4 * x + 3] = 255;
			}
		}
		pibmp->CopyFromMemory(&ru, pmem, pixelWidth);
	}

	return true;
}

bool MiggySurface::CopyImage(CImageBuffer* pimg)
{
	if (pimg == NULL || m_bitmap == NULL)
		return false;
	ID2D1Bitmap* pibmp = m_bitmap.Get();

	IFMT fmt = GetPreferredImageFormat(pibmp->GetPixelFormat());
	if (fmt == IFMT_NONE)
		return false;

	D2D1_SIZE_U siz{ m_width, m_height };
	D2D1_RECT_U ru;
	ru.left = 0;
	ru.right = siz.width + 1;

	UINT32 pixelWidth = 4 * siz.width;
	BYTE* pmem = new BYTE[pixelWidth];
	for (int y = 0; y < (int)siz.height; y++)
	{
		ru.top = y;
		ru.bottom = y + 1;

		if (pimg->GetLine(y, fmt, pmem))
		{
			HRESULT hres = pibmp->CopyFromMemory(&ru, pmem, pixelWidth);
			if (hres != S_OK)
				return false;
		}
	}

	return true;
}

void MiggySurface::EndDraw()
{
	m_bitmap.Detach();

	// Remove the transform and clip applied in BeginDraw because
	// the target area can change on every update.
	m_d2dContext->SetTransform(D2D1::IdentityMatrix());
	m_d2dContext->PopAxisAlignedClip();

	// Remove the render target and end drawing.
	m_d2dContext->EndDraw();
	m_d2dContext->SetTarget(nullptr);
	m_sisNative->EndDraw();
}

#pragma once

// most of the code here shamelessly copied, though not well understood, from:
//  http://msdn.microsoft.com/en-us/magazine/jj991975.aspx

#include <wrl.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include "windows.ui.xaml.media.dxinterop.h"

#include "image.h"

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace MiggyRT
{
	public ref class MiggySurface sealed : SurfaceImageSource
	{
	private:
		Microsoft::WRL::ComPtr<ISurfaceImageSourceNative> m_sisNative;
		Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<ID2D1Device> m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_d2dContext;

		ComPtr<ID2D1Bitmap1> m_bitmap;
		int m_width;
		int m_height;

	private:
		void CreateDeviceResources();
		void CreateDeviceIndependentResources();
		bool BeginDraw(Windows::Foundation::Rect updateRect);

	internal:
		D2D1_PIXEL_FORMAT GetPixelFormat();
		IFMT GetPreferredImageFormat(const D2D1_PIXEL_FORMAT& pfmt);

		bool CopyImage(CImageBuffer* pimg);

	public:
		MiggySurface(int pixelWidth, int pixelHeight);

		int GetWidth();
		int GetHeight();

		bool BeginDraw();
		bool TestDraw();
		void EndDraw();
	};
}

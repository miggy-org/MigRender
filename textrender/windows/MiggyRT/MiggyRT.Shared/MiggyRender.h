#pragma once

#include <collection.h>

#include "model.h"
#include "package.h"
#include "MiggySurface.h"

using namespace Platform;
//using namespace Platform::Collections;
//using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation;
//using namespace Windows::UI::Core;

namespace MiggyRT
{
	public delegate void TestOpDone(int result);

	public ref class MiggyRender sealed
	{
	private:
		//Windows::UI::Core::CoreDispatcher^ m_dispatcher;
		CImageBufferTarget bufTarget;
		CModel theModel;
		CGrp* pMainGrp;
		int threadCount;

	public:
		event TestOpDone^ testOpDoneEvent;

	public:
		MiggyRender();

		bool Init(int threads, bool superSample, bool doShadows, bool softShadows, bool autoReflect);
		bool SetText(String^ text, String^ pkgData);
		bool SetBackdrop(String^ mdlData, double specR, double specG, double specB, bool autoReflect);
		bool SetLighting(String^ mdlData);
		bool SetOrientation(double rZ, double rX, double rY, double tX, double tY, double tZ);
		bool LoadImage(String^ slot, String^ imgData, bool createAlpha);

		IAsyncOperation<String^>^ RenderToTempFile(int width, int height);
		IAsyncOperation<bool>^ Render(MiggySurface^ surfaceHint);
		void AbortRender();

		bool CopyToSurface(MiggySurface^ surface);
	};
}

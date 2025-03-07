// android.cpp - defines Android specific implementations
//

#include <android/log.h>

#include "android.h"
#include "migexcept.h"

using namespace MigRender;

//-----------------------------------------------------------------------------
// CAndroidBitmapTarget
//-----------------------------------------------------------------------------

CAndroidBitmapTarget::CAndroidBitmapTarget(JNIEnv* env, jobject bitmap)
{
	_env = env;
	_bitmap = bitmap;
	_ptr = NULL;
	memset(&_abmi, 0, sizeof(AndroidBitmapInfo));

	if (AndroidBitmap_getInfo(_env, _bitmap, &_abmi) != ANDROID_BITMAP_RESULT_SUCCESS)
		throw mig_exception("Unable to get Android bitmap info");
}

CAndroidBitmapTarget::~CAndroidBitmapTarget(void)
{
	UnlockPixels();
}

REND_INFO CAndroidBitmapTarget::GetRenderInfo(void) const
{
	REND_INFO rinfo;
	rinfo.width = _abmi.width;
	rinfo.height = _abmi.height;
	rinfo.topdown = true;
	rinfo.fmt = (_abmi.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ? ImageFormat::RGBA : ImageFormat::None);
	return rinfo;
}

void CAndroidBitmapTarget::PreRender(int nthreads)
{
	pthread_mutex_init(&mutex, NULL);
	mutexActive = false;
	abortRender = false;

	CRenderTarget::PreRender(nthreads);
	LockPixels();
}

bool CAndroidBitmapTarget::DoLine(int y, const dword *pline)
{
	//__android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "DoLine(%d)", y);

	pthread_mutex_lock(&mutex);
	unsigned char* pdest = _ptr + 4*(_abmi.height - y - 1)*_abmi.width;
	memcpy(pdest, pline, 4*_abmi.width);
	pthread_mutex_unlock(&mutex);

	return (!abortRender);
}

void CAndroidBitmapTarget::PostRender(bool success)
{
	pthread_mutex_destroy(&mutex);

	UnlockPixels();
	CRenderTarget::PostRender(success);
}

bool CAndroidBitmapTarget::LockPixels(bool unlockMutex, JNIEnv* env)
{
	//__android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "LockPixels()");
	bool ret = (_ptr == NULL ? (AndroidBitmap_lockPixels((env ? env : _env), _bitmap, (void**) &_ptr) == ANDROID_BITMAP_RESULT_SUCCESS) : false);

	if (unlockMutex && mutexActive)
	{
		mutexActive = false;
		pthread_mutex_unlock(&mutex);
	}
	return ret;
}

bool CAndroidBitmapTarget::UnlockPixels(bool needMutex, JNIEnv* env)
{
	if (needMutex && !mutexActive)
	{
		pthread_mutex_lock(&mutex);
		mutexActive = true;
	}

	if (_ptr != NULL)
	{
		bool ret = (AndroidBitmap_unlockPixels((env ? env : _env), _bitmap) == ANDROID_BITMAP_RESULT_SUCCESS);
		_ptr = NULL;
		//__android_log_print(ANDROID_LOG_INFO, "MIGGY-NDK", "UnlockPixels() returning %d", (int) ret);
		return ret;
	}

	return false;
}

void CAndroidBitmapTarget::Abort()
{
	abortRender = true;
}

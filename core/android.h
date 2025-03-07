// android.h - defines Android specific implementations
//

#pragma once

#include <jni.h>
#include <pthread.h>
#include <android/bitmap.h>

#include "rendertarget.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CAndroidBitmapTarget - allows rendering directly into an Android bitmap
//-----------------------------------------------------------------------------

class CAndroidBitmapTarget :
	public CRenderTarget
{
protected:
	JNIEnv* _env;
	jobject _bitmap;
	AndroidBitmapInfo _abmi;
	unsigned char* _ptr;
	pthread_mutex_t mutex;
	bool mutexActive;
	bool abortRender;

protected:

public:
	CAndroidBitmapTarget(JNIEnv* env, jobject bitmap);
	~CAndroidBitmapTarget(void);

	virtual REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);

	bool LockPixels(bool unlockMutex = false, JNIEnv* env = NULL);
	bool UnlockPixels(bool needMutex = false, JNIEnv* env = NULL);

	void Abort();
};

_MIGRENDER_END

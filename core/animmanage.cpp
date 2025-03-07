// anim.cpp - defines animation classes
//

#include "migexcept.h"
#include "animmanage.h"
using namespace std;
using namespace MigRender;

// animation object names (identifies animation targets in scripts)
static const string AnimObjModel = "model";
static const string AnimObjCamera = "camera";
static const string AnimObjBackground = "background";

//-----------------------------------------------------------------------------
// CAnimManager
//-----------------------------------------------------------------------------

CAnimManager::CAnimManager(void) : _frames(0), _maxTime(0)
{
}

void CAnimManager::ClearAnimation(void)
{
	_records.clear();
}

void CAnimManager::AddRecord(const CAnimRecord& record)
{
	_records.push_back(record);
}

void CAnimManager::AddRecord(const string& target, const CAnimRecord& record)
{
	AddRecord(record);
	_records.back().SetObjectName(target);
}

void CAnimManager::AddRecord(const string& target, double beginTime, double endTime, const CAnimRecord& record)
{
	AddRecord(record);
	_records.back().SetObjectName(target);
	_records.back().SetTimes(beginTime, endTime);
}

void CAnimManager::AddRecord(const string& target, double beginTime, double endTime, bool applyToStart, bool reverseAnim, const CAnimRecord& record)
{
	AddRecord(record);
	_records.back().SetObjectName(target);
	_records.back().SetTimes(beginTime, endTime);
	_records.back().SetApplyToStart(applyToStart);
	_records.back().SetReverse(reverseAnim);
}

void CAnimManager::PrepareAnimObjs(CModel& model)
{
	_maxTime = 0;

	for (const auto& iter : _records)
	{
		string objName = iter.GetObjectName();
		if (objName == AnimObjModel)
		{
			_animObjs[objName] = static_cast<CAnimTarget*>(&model);
		}
		else if (objName == AnimObjCamera)
		{
			_animObjs[objName] = static_cast<CAnimTarget*>(&model.GetCamera());
		}
		else if (objName == AnimObjBackground)
		{
			_animObjs[objName] = static_cast<CAnimTarget*>(&model.GetBackgroundHandler());
		}
		else if (!objName.empty())
		{
			CAnimTarget* pobj = static_cast<CAnimTarget*>(model.GetSuperGroup().DrillDown(objName));
			if (pobj == NULL)
				throw anim_exception("Unknown object: " + objName);
			_animObjs[objName] = pobj;
		}

		if (iter.GetEndTime() > _maxTime)
			_maxTime = iter.GetEndTime();
	}
}

double CAnimManager::StartPlayback(CModel& model, int frames)
{
	PrepareAnimObjs(model);

	for (const auto& iter : _animObjs)
	{
		iter.second->PreAnimate();
	}

	_frames = frames;
	return _maxTime;
}

static double ApplySpline(double t, double a, double b, double c)
{
	return (a - b * 2 + c) * t * t + (b * 2 - a * 2) * t + a;
}

static double ApplySpline2(double t, double a, double b1, double b2, double c)
{
	double diff = c - a;
	if (t < 0.5)
		return ApplySpline(t*2, a, b1, c - 0.5*diff);
	else
		return ApplySpline((t - 0.5)*2, a + 0.5*diff, b2, c);
}

static double GetInterpolatedFactor(const CAnimRecord& record, double factor)
{
	double finalFactor = factor;
	if (record.GetInterpolation() == AnimInterpolation::Spline)
		finalFactor = ApplySpline(factor, 0, record.GetBias(), 1);
	else if (record.GetInterpolation() == AnimInterpolation::Spline2)
		finalFactor = ApplySpline2(factor, 0, record.GetBias(), record.GetBias2(), 1);
	return finalFactor;
}

template <typename T>
void CAnimManager::InterpolateAnimation(const CAnimRecord& record, const CAnimValues<T>& values, double factor)
{
	T beginValue = values.GetBeginValue();
	T endValue = values.GetEndValue();

	T diff = (record.GetReverse() ? beginValue - endValue : endValue - beginValue);
	T finalValue = (record.GetReverse() ? endValue : beginValue) + diff * GetInterpolatedFactor(record, factor);

	CAnimTarget* pobj = _animObjs[record.GetObjectName()];
	pobj->Animate(record.GetAnimType(), record.GetOperation(), &finalValue);
}

void CAnimManager::ExecuteAnimation(const CAnimRecord& record, double factor)
{
	switch (record.GetAnimType())
	{
	case AnimType::RotateX:
	case AnimType::RotateY:
	case AnimType::RotateZ:
	case AnimType::CameraDist:
	case AnimType::CameraULen:
	case AnimType::CameraVLen:
	case AnimType::LightHLit:
	case AnimType::LightSScale:
	case AnimType::LightFullDistance:
	case AnimType::LightDropDistance:
	case AnimType::ModelFade:
	case AnimType::ModelFog:
	case AnimType::ModelFogNear:
	case AnimType::ModelFogFar:
		InterpolateAnimation<double>(record, record.GetValues<double>(), factor);
		break;

	case AnimType::ColorDiffuse:
	case AnimType::ColorSpecular:
	case AnimType::ColorReflection:
	case AnimType::ColorRefraction:
	case AnimType::ColorGlow:
	case AnimType::BgColorNorth:
	case AnimType::BgColorEquator:
	case AnimType::BgColorSouth:
	case AnimType::LightColor:
	case AnimType::ModelAmbient:
		InterpolateAnimation<COLOR>(record, record.GetValues<COLOR>(), factor);
		break;

	case AnimType::Translate:
	case AnimType::Scale:
		InterpolateAnimation<CPt>(record, record.GetValues<CPt>(), factor);
		break;

	case AnimType::None:
		break;

	default:
		throw anim_exception("Unsupported animation type" + to_string(static_cast<int>(record.GetAnimType())));
	}
}

void CAnimManager::GoToFrame(int frame)
{
	if (frame <= 0)
		throw anim_exception("Requested frame is less than 1");
	if (frame > _frames)
		throw anim_exception("Requested frame exceeds animation frame count: " + to_string(frame));
	double currentTime = _maxTime * (frame / (double)_frames);

	for (const auto& iter : _animObjs)
	{
		iter.second->ResetPlayback();
	}

	//for (auto iter = _records.begin(); iter != _records.end(); iter++)
	for (const auto& iter : _records)
	{
		if (iter.GetAnimType() != AnimType::None && (iter.GetApplyToStart() || currentTime >= iter.GetBeginTime()))
		{
			double factor = 1;
			if (currentTime < iter.GetBeginTime())
				factor = 0;
			else if (iter.GetEndTime() > iter.GetBeginTime())
				factor = (currentTime - iter.GetBeginTime()) / (iter.GetEndTime() - iter.GetBeginTime());
			if (factor > 1)
				factor = 1;
			ExecuteAnimation(iter, factor);
		}
	}
}

void CAnimManager::FinishPlayback(void)
{
	for (const auto& iter : _animObjs)
	{
		iter.second->PostAnimate();
	}

	_animObjs.clear();
	_frames = 0;
	_maxTime = 0;
}

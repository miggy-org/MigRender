// anim.cpp - defines animation classes
//

#include "migexcept.h"
#include "anim.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CAnimValues
//-----------------------------------------------------------------------------

template <typename T>
CAnimValues<T>::CAnimValues()
{
}

template <typename T>
void CAnimValues<T>::SetValues(const T& beginValue, const T& endValue)
{
	_beginValue = beginValue;
	_endValue = endValue;
}

template <typename T>
const T& CAnimValues<T>::GetBeginValue(void) const
{
	return _beginValue;
}

template <typename T>
const T& CAnimValues<T>::GetEndValue(void) const
{
	return _endValue;
}

// double templates
template void CAnimValues<double>::SetValues(const double& beginValue, const double& endValue);
template const double& CAnimValues<double>::GetBeginValue(void) const;
template const double& CAnimValues<double>::GetEndValue(void) const;

// COLOR templates
template void CAnimValues<COLOR>::SetValues(const COLOR& beginValue, const COLOR& endValue);
template const COLOR& CAnimValues<COLOR>::GetBeginValue(void) const;
template const COLOR& CAnimValues<COLOR>::GetEndValue(void) const;

// CPt templates
template void CAnimValues<CPt>::SetValues(const CPt& beginValue, const CPt& endValue);
template const CPt& CAnimValues<CPt>::GetBeginValue(void) const;
template const CPt& CAnimValues<CPt>::GetEndValue(void) const;

//-----------------------------------------------------------------------------
// CAnimRecord
//-----------------------------------------------------------------------------

CAnimRecord::CAnimRecord(void) :
	_animType(AnimType::None),
	_animOperation(AnimOperation::Replace),
	_animInterpolation(AnimInterpolation::Linear),
	_applyToStart(false), _reverseAnim(false),
	_beginTime(0), _endTime(1), _bias(0.5), _bias2(0.5)
{
	// the other two have default constructors
	_dValues.SetValues(0, 0);
}

void CAnimRecord::SetObjectName(const string& objectName)
{
	_objectName = objectName;
}

void CAnimRecord::SetAnimType(AnimType animType)
{
	_animType = animType;
}

void CAnimRecord::SetOperation(AnimOperation animOperation)
{
	_animOperation = animOperation;
}

void CAnimRecord::SetInterpolation(AnimInterpolation animInterpolation)
{
	_animInterpolation = animInterpolation;
}

void CAnimRecord::SetBias(double bias)
{
	_bias = bias;
}

void CAnimRecord::SetBias2(double bias2)
{
	_bias2 = bias2;
}

void CAnimRecord::SetApplyToStart(bool applyToStart)
{
	_applyToStart = applyToStart;
}

void CAnimRecord::SetReverse(bool reverse)
{
	_reverseAnim = reverse;
}

void CAnimRecord::SetTimes(double beginTime, double endTime)
{
	if (beginTime > endTime)
		throw anim_exception("Begin time must be less than or equal to end time");
	_beginTime = beginTime;
	_endTime = endTime;
}

void CAnimRecord::SetTimes(double bothTimes)
{
	SetTimes(bothTimes, bothTimes);
}

const string& CAnimRecord::GetObjectName(void) const
{
	return _objectName;
}

AnimType CAnimRecord::GetAnimType(void) const
{
	return _animType;
}

AnimOperation CAnimRecord::GetOperation(void) const
{
	return _animOperation;
}

AnimInterpolation CAnimRecord::GetInterpolation(void) const
{
	return _animInterpolation;
}

double CAnimRecord::GetBias(void) const
{
	return _bias;
}

double CAnimRecord::GetBias2(void) const
{
	return _bias2;
}

double CAnimRecord::GetBeginTime(void) const
{
	return _beginTime;
}

bool CAnimRecord::GetApplyToStart(void) const
{
	return _applyToStart;
}

bool CAnimRecord::GetReverse(void) const
{
	return _reverseAnim;
}

double CAnimRecord::GetEndTime(void) const
{
	return _endTime;
}

template <typename T>
CAnimValues<T>& CAnimRecord::GetValues(void)
{
	throw anim_exception("Unknown animation value type");
}

template <>
CAnimValues<double>& CAnimRecord::GetValues(void)
{
	return _dValues;
}

template <>
CAnimValues<CPt>& CAnimRecord::GetValues(void)
{
	return _ptValues;
}

template <>
CAnimValues<COLOR>& CAnimRecord::GetValues(void)
{
	return _colValues;
}

template <typename T>
const CAnimValues<T>& CAnimRecord::GetValues(void) const
{
	throw anim_exception("Unknown animation value type");
}

template <>
const CAnimValues<double>& CAnimRecord::GetValues(void) const
{
	return _dValues;
}

template <>
const CAnimValues<CPt>& CAnimRecord::GetValues(void) const
{
	return _ptValues;
}

template <>
const CAnimValues<COLOR>& CAnimRecord::GetValues(void) const
{
	return _colValues;
}

template <typename T>
void CAnimRecord::SetValues(const T& beginValue, const T& endValue)
{
	GetValues<T>().SetValues(beginValue, endValue);
}

template void CAnimRecord::SetValues(const double& beginValue, const double& endValue);
template void CAnimRecord::SetValues(const CPt& beginValue, const CPt& endValue);
template void CAnimRecord::SetValues(const COLOR& beginValue, const COLOR& endValue);

template <typename T>
void CAnimRecord::SetValues(const T& bothValues)
{
	GetValues<T>().SetValues(bothValues, bothValues);
}

template void CAnimRecord::SetValues(const double& bothValues);
template void CAnimRecord::SetValues(const CPt& bothValues);
template void CAnimRecord::SetValues(const COLOR& bothValues);

// anim.h - defines animation classes
//

#pragma once

#include <vector>
#include <string>
#include <map>

#include "defines.h"
#include "vector.h"

_MIGRENDER_BEGIN

// animation types
enum class AnimType
{
	None = 0,

	// matrix ops
	Translate,
	Scale,
	RotateX,
	RotateY,
	RotateZ,
	ShearXY,
	ShearXZ,
	ShearYZ,

	// color
	ColorDiffuse,
	ColorSpecular,
	ColorReflection,
	ColorRefraction,
	ColorGlow,
	IndexRefraction,

	// background
	BgColorNorth,
	BgColorEquator,
	BgColorSouth,

	// camera
	CameraDist,
	CameraULen,
	CameraVLen,

	// lights
	LightColor,
	LightHLit,
	LightSScale,
	LightFullDistance,
	LightDropDistance,
	// TODO: origin and direction animations?  or is that covered by matrix ops?

	// model
	ModelAmbient,
	ModelFade,
	ModelFog,
	ModelFogNear,
	ModelFogFar
};

// animation operation types
enum class AnimOperation
{
	Replace,
	Sum,
	Multiply
};

// animation interpolation types
enum class AnimInterpolation
{
	Linear,
	Spline,
	Spline2
};

// all animatable objects must implement these
class CAnimTarget
{
public:
	// called once before an animation series
	virtual void PreAnimate(void) = 0;

	// called once after animation is complete
	virtual void PostAnimate(void) = 0;

	// called before each frame
	virtual void ResetPlayback(void) = 0;

	// called to execute an animation record
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue) = 0;
};

template<typename T>
class CAnimValues
{
private:
	T _beginValue;
	T _endValue;

public:
	CAnimValues();

	void SetValues(const T& beginValue, const T& endValue);
	const T& GetBeginValue(void) const;
	const T& GetEndValue(void) const;
};

class CAnimRecord
{
private:
	// name of the object (namespace notation)
	std::string _objectName;

	// animation type
	AnimType _animType;

	// operation type (not supported by all targets)
	AnimOperation _animOperation;

	// interpolation type
	AnimInterpolation _animInterpolation;

	// biases (spline interpolation only)
	double _bias;
	double _bias2;

	// flags
	bool _applyToStart;
	bool _reverseAnim;

	// times
	double _beginTime;
	double _endTime;

	// values
	CAnimValues<double> _dValues;
	CAnimValues<CPt> _ptValues;
	CAnimValues<COLOR> _colValues;

public:
	CAnimRecord(void);

	void SetObjectName(const std::string& objectName);
	void SetAnimType(AnimType animType);
	void SetOperation(AnimOperation animOperation);
	void SetInterpolation(AnimInterpolation animInterpolation);
	void SetBias(double bias);
	void SetBias2(double bias2);
	void SetApplyToStart(bool applyToStart);
	void SetReverse(bool reverse);
	void SetTimes(double beginTime, double endTime);
	void SetTimes(double bothTimes);

	const std::string& GetObjectName(void) const;
	AnimType GetAnimType(void) const;
	AnimOperation GetOperation(void) const;
	AnimInterpolation GetInterpolation(void) const;
	double GetBias(void) const;
	double GetBias2(void) const;
	bool GetApplyToStart(void) const;
	bool GetReverse(void) const;
	double GetBeginTime(void) const;
	double GetEndTime(void) const;

	template <typename T>
	void SetValues(const T& beginValue, const T& endValue);
	template <typename T>
	void SetValues(const T& bothValues);
	template <typename T>
	CAnimValues<T>& GetValues(void);
	template <typename T>
	const CAnimValues<T>& GetValues(void) const;
};

// utility functions
template <typename T>
T ComputeNewAnimValue(const T& existingColor, const T& newColor, AnimOperation opType)
{
	if (opType == AnimOperation::Multiply)
		return existingColor * newColor;
	else if (opType == AnimOperation::Sum)
		return existingColor + newColor;
	return newColor;
}

_MIGRENDER_END

// envvars.h - defines environment variables (EXPERIMENTAL)
//

#pragma once

#include <iostream>

#include "model.h"

_MIGRENDER_BEGIN

enum class EnvVar
{
	None,

	Int,
	Bool,
	Double,
	String,

	UVC,
	CPt,
	UnitVector,
	Matrix,

	GrpRef,
	SphereRef,
	PolygonRef,
	DirLightRef,
	PtLightRef,
	SpotLightRef,

	IntArray,
	DoubleArray,
	UVCArray,
	CPtArray,
	UnitVectorArray
};

/*template <class T>
class CArray
{
private:
	std::vector<T> list;

public:
	void Add(const T& newItem) { list.push_back(newItem); }
	void Clear(void) { list.clear(); }

	T* ToArray(void) { return (list.size() > 0 ? &list[0] : NULL); }
	int GetSize(void) { return list.size(); }
};*/

struct EnvironmentVariable
{
	EnvVar m_type;

	union
	{
		int m_ivalue;
		bool m_bvalue;
		double m_dvalue;

		UVC m_uvcvalue;
		CPt m_ptvalue;
		CUnitVector m_unitvectorvalue;

		CGrp* m_pgrp;
		CObj* m_pobj;
		CLight* m_plit;
	};
	std::string m_svalue;

	std::unique_ptr<CMatrix> m_matrix;

	//std::unique_ptr<CArray<int>> m_intarray;
	//std::unique_ptr<CArray<double>> m_doublearray;
	//std::unique_ptr<CArray<UVC>> m_uvcarray;
	//std::unique_ptr<CArray<CPt>> m_ptarray;
	//std::unique_ptr<CArray<CUnitVector>> m_uvctarray;

	EnvironmentVariable();
	EnvironmentVariable(int value);
	EnvironmentVariable(bool value);
	EnvironmentVariable(double value);
	EnvironmentVariable(const std::string& value);
	EnvironmentVariable(const UVC& value);
	EnvironmentVariable(const CPt& value);
	EnvironmentVariable(const CUnitVector& value);
	EnvironmentVariable(const CMatrix& value);
	EnvironmentVariable(CGrp* value);
	EnvironmentVariable(CSphere* value);
	EnvironmentVariable(CPolygon* value);
	EnvironmentVariable(CDirLight* value);
	EnvironmentVariable(CPtLight* value);
	EnvironmentVariable(CSpotLight* value);
};

class CEnvVars
{
private:
	std::map<std::string, EnvironmentVariable> m_mapVars;

public:
	template <typename T>
	void AddVar(const std::string& name, T newValue);

	EnvVar GetVarType(const std::string& name);

	template <typename T>
	T GetVar(const std::string& name);
};

_MIGRENDER_END

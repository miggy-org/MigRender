// envvars.cpp - defines environment variables
//

#include "envvars.h"
using namespace std;
using namespace MigRender;

EnvironmentVariable::EnvironmentVariable()
{
	m_type = EnvVar::None;
	m_ivalue = 0;
}

EnvironmentVariable::EnvironmentVariable(int value)
{
	m_type = EnvVar::Int;
	m_ivalue = value;
}

EnvironmentVariable::EnvironmentVariable(bool value)
{
	m_type = EnvVar::Bool;
	m_bvalue = value;
}

EnvironmentVariable::EnvironmentVariable(double value)
{
	m_type = EnvVar::Double;
	m_dvalue = value;
}

EnvironmentVariable::EnvironmentVariable(const string& value)
{
	m_type = EnvVar::String;
	m_svalue = value;
}

EnvironmentVariable::EnvironmentVariable(const UVC& value)
{
	m_type = EnvVar::UVC;
	m_uvcvalue = value;
}

EnvironmentVariable::EnvironmentVariable(const CPt& value)
{
	m_type = EnvVar::CPt;
	m_ptvalue = value;
}

EnvironmentVariable::EnvironmentVariable(const CUnitVector& value)
{
	m_type = EnvVar::UnitVector;
	m_unitvectorvalue = value;
}

EnvironmentVariable::EnvironmentVariable(const CMatrix& value)
{
	m_type = EnvVar::Matrix;
	m_matrix = std::make_unique<CMatrix>(value);
}

EnvironmentVariable::EnvironmentVariable(CGrp* value)
{
	m_type = EnvVar::GrpRef;
	m_pgrp = value;
}

EnvironmentVariable::EnvironmentVariable(CSphere* value)
{
	m_type = EnvVar::SphereRef;
	m_pobj = value;
}

EnvironmentVariable::EnvironmentVariable(CPolygon* value)
{
	m_type = EnvVar::PolygonRef;
	m_pobj = value;
}

EnvironmentVariable::EnvironmentVariable(CDirLight* value)
{
	m_type = EnvVar::DirLightRef;
	m_plit = value;
}

EnvironmentVariable::EnvironmentVariable(CPtLight* value)
{
	m_type = EnvVar::PtLightRef;
	m_plit = value;
}

EnvironmentVariable::EnvironmentVariable(CSpotLight* value)
{
	m_type = EnvVar::SpotLightRef;
	m_plit = value;
}


template <typename T>
void CEnvVars::AddVar(const string& name, T newValue)
{
	m_mapVars[name] = EnvironmentVariable(newValue);
}

template void CEnvVars::AddVar<int>(const string& name, int newValue);
template void CEnvVars::AddVar<bool>(const string& name, bool newValue);
template void CEnvVars::AddVar<double>(const string& name, double newValue);
template void CEnvVars::AddVar<string>(const string& name, string newValue);
template void CEnvVars::AddVar<UVC>(const string& name, UVC newValue);
template void CEnvVars::AddVar<CPt>(const string& name, CPt newValue);
template void CEnvVars::AddVar<CUnitVector>(const string& name, CUnitVector newValue);
template void CEnvVars::AddVar<CMatrix>(const string& name, CMatrix newValue);
template void CEnvVars::AddVar<CGrp*>(const string& name, CGrp* newValue);
template void CEnvVars::AddVar<CSphere*>(const string& name, CSphere* newValue);
template void CEnvVars::AddVar<CPolygon*>(const string& name, CPolygon* newValue);
template void CEnvVars::AddVar<CDirLight*>(const string& name, CDirLight* newValue);
template void CEnvVars::AddVar<CPtLight*>(const string& name, CPtLight* newValue);
template void CEnvVars::AddVar<CSpotLight*>(const string& name, CSpotLight* newValue);

EnvVar CEnvVars::GetVarType(const string& name)
{
	if (m_mapVars.find(name) == m_mapVars.end())
		return EnvVar::None;
	return m_mapVars[name].m_type;
}

template <typename T>
T CEnvVars::GetVar(const string& name)
{
	EnvVar type = GetVarType(name);
	if (type == EnvVar::None)
		return 0;  // exception
	return 0;
}

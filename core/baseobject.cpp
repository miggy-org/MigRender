// baseobject.cpp - defines the CBaseObj class
//

#include "migexcept.h"
#include "baseobject.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CBaseObj
//-----------------------------------------------------------------------------

CBaseObj::CBaseObj()
{
}

CBaseObj::~CBaseObj()
{
}

void CBaseObj::Duplicate(const CBaseObj& rhs)
{
	m_tm = rhs.m_tm;
	m_meta = rhs.m_meta;
}

const string& CBaseObj::GetName(void) const
{
	return m_name;
}

void CBaseObj::SetName(const string& name)
{
	m_name = name;
}

CMatrix& CBaseObj::GetTM(void)
{
	return m_tm;
}

const CMatrix& CBaseObj::GetTM(void) const
{
	return m_tm;
}

void CBaseObj::AddMetaData(const char* key, const char* val)
{
	m_meta[key] = val;
}

bool CBaseObj::GetMetaData(const char* key, char* val, int len) const
{
	auto iter = m_meta.find(key);
	if (iter == m_meta.end())
		return false;
	if (len <= (int)iter->second.length())
		return false;
	strncpy(val, iter->second.c_str(), iter->second.length() + 1);
	return true;
}

void CBaseObj::ClearMetaData(void)
{
	m_meta.clear();
}

void CBaseObj::Load(CFileBase& fobj)
{
	BlockType bt = fobj.ReadNextBlockType();
	while (bt != BlockType::EndRange)
	{
		if (bt == BlockType::MetaData)
		{
			int len = fobj.ReadNextBlock(NULL, 0);
			if (len > 0)
			{
				std::vector<char> buf(len);
				if (!fobj.ReadNextBlock(reinterpret_cast<byte*>(buf.data()), len))
					throw fileio_exception("Error reading meta-data block");
				string key = buf.data();
				string val = &buf[key.length() + 1];
				m_meta[key] = val;
			}
		}
		else if (bt == BlockType::BaseObj)
		{
			FILE_BASEOBJ fbo;
			if (!fobj.ReadNextBlock((byte*)&fbo, sizeof(FILE_BASEOBJ)))
				throw fileio_exception("Error reading base object block");
			m_name = fbo.name;
		}

		bt = fobj.ReadNextBlockType();
	}
}

void CBaseObj::Save(CFileBase& fobj)
{
	for (const auto& iter : m_meta)
	{
		int len1 = (int)iter.first.size();
		int len2 = (int)iter.second.size();
		std::vector<char> buf(len1 + len2 + 2);
		strncpy(buf.data(), iter.first.c_str(), len1 + len2 + 2);
		strncpy(&buf[len1 + 1], iter.second.c_str(), iter.second.length() + 1);

		if (!fobj.WriteDataBlock(BlockType::MetaData, reinterpret_cast<byte*>(buf.data()), len1 + len2 + 2))
			throw fileio_exception("Error writing meta-data block");
	}

	if (!m_name.empty())
	{
		FILE_BASEOBJ fbo;
		if (sizeof(fbo.name) <= m_name.length())
			throw fileio_exception("Object name too big to save");
		strncpy(fbo.name, m_name.c_str(), sizeof(fbo.name));
		if (!fobj.WriteDataBlock(BlockType::BaseObj, (byte*)&fbo, sizeof(FILE_BASEOBJ)))
			throw fileio_exception("Error saving base object block");
	}

	if (!fobj.WriteDataBlock(BlockType::EndRange, NULL, 0))
		throw fileio_exception("Error saving end-of-ranage block");
}

void CBaseObj::PreAnimate(void)
{
	m_tmOrig = m_tm;
}

void CBaseObj::PostAnimate(void)
{
	ResetPlayback();
}

void CBaseObj::ResetPlayback()
{
	m_tm = m_tmOrig;
}

void CBaseObj::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	// all matrix operations are assumed to be ANIM_OPERATION_SUM
	switch (animId)
	{
	case AnimType::Translate:
		{
			CPt* pt = (CPt*)newValue;
			m_tm.Translate(pt->x, pt->y, pt->z);
		}
		break;
	case AnimType::Scale:
		{
			CPt* pt = (CPt*)newValue;
			m_tm.Scale(pt->x, pt->y, pt->z);
		}
		break;
	case AnimType::RotateX:
		m_tm.RotateX(*((double*)newValue));
		break;
	case AnimType::RotateY:
		m_tm.RotateY(*((double*)newValue));
		break;
	case AnimType::RotateZ:
		m_tm.RotateZ(*((double*)newValue));
		break;
	default:
		throw anim_exception("Unsupported animation type: " + to_string(static_cast<int>(animId)));
	}
}

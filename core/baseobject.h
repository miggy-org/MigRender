// baseobject.h - defines the base object base class, common to all instances inside a model
//

#pragma once

#include <vector>
#include <string>
#include <map>

#include "matrix.h"
#include "fileio.h"
#include "anim.h"

typedef std::map<std::string, std::string> MetaDataMap;

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CBaseObj - object base class
//
//  Any object instantiated inside a model should derive from this class.
//-----------------------------------------------------------------------------

class CBaseObj : public CAnimTarget
{
protected:
	std::string m_name;
	CMatrix m_tm;
	MetaDataMap m_meta;

	// animation
	CMatrix m_tmOrig;

protected:
	void Duplicate(const CBaseObj& rhs);

public:
	CBaseObj();
	virtual ~CBaseObj();

	virtual ObjType GetType() const = 0;

	virtual const std::string& GetName(void) const;
	virtual void SetName(const std::string& name);

	virtual CMatrix& GetTM(void);
	virtual const CMatrix& GetTM(void) const;

	virtual void AddMetaData(const char* key, const char* val);
	virtual bool GetMetaData(const char* key, char* val, int len) const;
	virtual void ClearMetaData(void);

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	// animation
	virtual void PreAnimate(void);
	virtual void PostAnimate(void);
	virtual void ResetPlayback(void);
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue);
};

_MIGRENDER_END

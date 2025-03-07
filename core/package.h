// package.h - defines the package handler class
//

#pragma once

#include "defines.h"
#include "fileio.h"
#include "object.h"

typedef std::map<std::string, int> TOC;
typedef std::map<std::string, int>::iterator TOC_Iter;

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CPackage - class to manage packages of objects
//-----------------------------------------------------------------------------

class CPackage
{
protected:
	// used for package importation
	CFileBase* m_pfile;
	TOC m_toc;
	int m_start;

	// used for package creation
	std::string m_path;
	std::string m_tmp;
	std::vector<std::string> m_items;
	std::vector<std::string> m_objs;

public:
	CPackage(void);
	~CPackage(void);

	// import
	int OpenPackage(CFileBase* pfile);
	BlockType GetObjectType(const char* item);
	void LoadObject(const char* item, CBaseObj* pobj);
	void ClosePackage(void);

	// creation
	void SetTmpPath(const char* tmp);
	void StartNewPackage(const char* path);
	void AddObject(const char* item, CBaseObj* pobj);
	void CompleteNewPackage(void);
};

_MIGRENDER_END

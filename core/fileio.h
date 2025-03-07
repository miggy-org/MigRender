// fileio.h - defines classes for file handling
//

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "defines.h"
#include "filestructs.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CFileBase - abstract base class for file I/O
//-----------------------------------------------------------------------------

class CFileBase
{
protected:
	// used during reading and writing
	//MigType m_type;

	// used during reading
	FILE_TAG m_next;
	bool m_skip;

protected:
	virtual bool ReadHeader(MigType type);
	virtual bool WriteHeader(MigType type);

	virtual bool ReadData(byte* data, int size) = 0;
	virtual bool SeekData(int size, bool cur) = 0;
	virtual bool WriteData(const byte* data, int size) = 0;

public:
	CFileBase(void);
	virtual ~CFileBase(void);

	virtual bool OpenFile(bool write, MigType type);
	virtual bool CloseFile(void);

	virtual BlockType ReadNextBlockType(void);
	virtual BlockType GetCurrentBlockType(void) const;
	virtual int ReadNextBlock(byte* data, int size);

	virtual int GetCurPos(void) const;
	virtual bool SetCurPos(int offset);

	virtual bool WriteDataBlock(BlockType bt, const byte* data, int size);
};

//-----------------------------------------------------------------------------
// CBinFile - does binary files
//-----------------------------------------------------------------------------

class CBinFile : public CFileBase
{
protected:
	std::string m_path;
	FILE* m_fh;

protected:
	virtual bool ReadData(byte* data, int size);
	virtual bool SeekData(int size, bool cur);
	virtual bool WriteData(const byte* data, int size);

public:
	CBinFile(void);
	CBinFile(const char* path);
	virtual ~CBinFile(void);

	void Init(const char* path);
	virtual int GetCurPos(void) const;

	virtual bool OpenFile(bool write, MigType type);
	virtual bool CloseFile(void);
};

//-----------------------------------------------------------------------------
// CMemFile - does memory based files
//-----------------------------------------------------------------------------

class CMemFile : public CFileBase
{
protected:
	byte* m_ptrBuffer;
	byte* m_ptrCurr;

protected:
	virtual bool ReadData(byte* data, int size);
	virtual bool SeekData(int size, bool cur);
	virtual bool WriteData(const byte* data, int size);

public:
	CMemFile(void);
	CMemFile(byte* ptr);
	virtual ~CMemFile(void);

	void Init(byte* pdata);
	virtual int GetCurPos(void) const;

	virtual bool OpenFile(bool write, MigType type);
	virtual bool CloseFile(void);
};

_MIGRENDER_END

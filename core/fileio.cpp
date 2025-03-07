// fileio.cpp - defines the file handling classes
//

#include <string.h>

#include "fileio.h"
#include "filestructs.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CFileBase
//-----------------------------------------------------------------------------

CFileBase::CFileBase(void)
	: m_skip(false)
{
	m_next.type = BlockType::None;
	m_next.size = 0;
}

CFileBase::~CFileBase(void)
{
}

bool CFileBase::ReadHeader(MigType type)
{
	FILE_HEADER fh;
	if (!ReadData((byte*) &fh, sizeof(FILE_HEADER)))
		throw fileio_exception("Error reading file header");
	if (fh.ident != MIG_IDENT)
		throw fileio_exception("Failed MIG_IDENT check");
	if (fh.version != MIG_VERSION)
		throw fileio_exception("Failed MIG_VERSION check");
	if (fh.type != type)
		throw fileio_exception("Failed type check: " + std::to_string(static_cast<int>(fh.type)));
	return true;
}

bool CFileBase::WriteHeader(MigType type)
{
	FILE_HEADER fh;
	fh.ident = MIG_IDENT;
	fh.version = MIG_VERSION;
	fh.type = type;
	fh.reserved = 0;

	return WriteData((byte*) &fh, sizeof(FILE_HEADER));
}

bool CFileBase::OpenFile(bool write, MigType type)
{
	if (type == MigType::None)
		throw fileio_exception("Invalid file type");
	return (write ? WriteHeader(type) : ReadHeader(type));
}

bool CFileBase::CloseFile(void)
{
	return true;
}

BlockType CFileBase::ReadNextBlockType(void)
{
	if (m_skip)
	{
		if (!SeekData(m_next.size, true))
			return BlockType::None;
	}
	if (!ReadData((byte*) &m_next, sizeof(FILE_TAG)))
		return BlockType::None;
	m_skip = true;
	return m_next.type;
}

BlockType CFileBase::GetCurrentBlockType(void) const
{
	return m_next.type;
}

int CFileBase::ReadNextBlock(byte* data, int size)
{
	if (data == NULL || size == 0)
		return m_next.size;
	if (size != m_next.size)
		return 0;
	if (!ReadData(data, size))
		return 0;
	m_skip = false;
	return size;
}

int CFileBase::GetCurPos(void) const
{
	return 0;
}

bool CFileBase::SetCurPos(int offset)
{
	return SeekData(offset, false);
}

bool CFileBase::WriteDataBlock(BlockType bt, const byte* data, int size)
{
	FILE_TAG ft;
	ft.type = bt;
	ft.size = size;
	if (!WriteData((byte*) &ft, sizeof(FILE_TAG)))
		throw fileio_exception("Unable to write FILE_TAG");
	if (bt == BlockType::EndRange || bt == BlockType::EndFile)
		return true;
	return WriteData(data, size);
}

//-----------------------------------------------------------------------------
// CBinFile
//-----------------------------------------------------------------------------

CBinFile::CBinFile(void)
	: CFileBase(), m_fh(NULL)
{
}

CBinFile::CBinFile(const char* path)
	: CFileBase(), m_fh(NULL)
{
	Init(path);
}

CBinFile::~CBinFile(void)
{
	CloseFile();
}

void CBinFile::Init(const char* path)
{
	m_path = path;
}

int CBinFile::GetCurPos(void) const
{
	return ftell(m_fh);
}

bool CBinFile::OpenFile(bool write, MigType type)
{
	m_fh = fopen(m_path.c_str(), (write ? "wb" : "rb"));
	if (m_fh == NULL)
		throw fileio_exception("Unable to open file: " + m_path);
	return CFileBase::OpenFile(write, type);
}

bool CBinFile::ReadData(byte* data, int size)
{
	return (fread(data, 1, size, m_fh) == size);
}

bool CBinFile::SeekData(int size, bool cur)
{
	return (!fseek(m_fh, size, (cur ? SEEK_CUR : SEEK_SET)));
}

bool CBinFile::WriteData(const byte* data, int size)
{
	return (fwrite(data, 1, size, m_fh) == size);
}

bool CBinFile::CloseFile(void)
{
	if (m_fh)
	{
		fclose(m_fh);
		m_fh = NULL;
	}
	return CFileBase::CloseFile();
}

//-----------------------------------------------------------------------------
// CMemFile
//-----------------------------------------------------------------------------

CMemFile::CMemFile(void)
	: CFileBase(), m_ptrBuffer(NULL), m_ptrCurr(NULL)
{
}

CMemFile::CMemFile(byte* ptr)
	: CFileBase(), m_ptrBuffer(NULL), m_ptrCurr(NULL)
{
	Init(ptr);
}

CMemFile::~CMemFile(void)
{
	CloseFile();
}

void CMemFile::Init(byte* ptr)
{
	m_ptrBuffer = m_ptrCurr = ptr;
}

int CMemFile::GetCurPos(void) const
{
	return (m_ptrCurr - m_ptrBuffer);
}

bool CMemFile::OpenFile(bool write, MigType type)
{
	return (m_ptrBuffer != NULL ? CFileBase::OpenFile(write, type) : false);
}

bool CMemFile::ReadData(byte* data, int size)
{
	if (m_ptrCurr != NULL)
	{
		memcpy(data, m_ptrCurr, size);
		m_ptrCurr += size;
	}
	return (m_ptrCurr != NULL);
}

bool CMemFile::SeekData(int size, bool cur)
{
	if (m_ptrCurr != NULL)
	{
		m_ptrCurr = (cur ? m_ptrCurr + size : m_ptrBuffer + size);
	}
	return (m_ptrCurr != NULL);
}

bool CMemFile::WriteData(const byte* data, int size)
{
	memcpy(m_ptrCurr, data, size);
	m_ptrCurr += size;
	return (m_ptrCurr != NULL);
}

bool CMemFile::CloseFile(void)
{
	m_ptrBuffer = m_ptrCurr = NULL;
	return CFileBase::CloseFile();
}

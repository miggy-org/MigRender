// package.cpp - defines the package handler class
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "package.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CPackage
//-----------------------------------------------------------------------------

CPackage::CPackage(void)
	: m_start(0), m_pfile(NULL)
{
}

CPackage::~CPackage(void)
{
	for (const auto& iter : m_objs)
		remove(iter.c_str());

	ClosePackage();
}

int CPackage::OpenPackage(CFileBase* pfile)
{
	m_pfile = pfile;
	if (!m_pfile->OpenFile(false, MigType::Package))
		return 0;

	if (m_pfile->ReadNextBlockType() != BlockType::PackageTOC)
		throw fileio_exception("Failed TOC next block check");

	int size = m_pfile->ReadNextBlock(NULL, 0) / sizeof(FILE_TOC_ENTRY);
	std::vector<FILE_TOC_ENTRY> toc(size);
	if (m_pfile->ReadNextBlock(reinterpret_cast<byte*>(toc.data()), size * sizeof(FILE_TOC_ENTRY)) != size * sizeof(FILE_TOC_ENTRY))
		throw fileio_exception("Failed reading TOC entry");
	m_start = m_pfile->GetCurPos();

	for (int n = 0; n < size; n++)
		m_toc[toc[n].item] = toc[n].offset;
	return size;
}

BlockType CPackage::GetObjectType(const char* item)
{
	if (m_toc.find(item) == m_toc.end())
		return BlockType::None;

	int offset = m_toc[item] + m_start;
	if (!m_pfile->SetCurPos(offset))
		return BlockType::None;
	return m_pfile->ReadNextBlockType();
}

void CPackage::LoadObject(const char* item, CBaseObj* pobj)
{
	pobj->Load(*m_pfile);
}

void CPackage::ClosePackage(void)
{
	if (m_pfile != NULL)
		m_pfile->CloseFile();
	m_pfile = NULL;
}

void CPackage::SetTmpPath(const char* tmp)
{
	m_tmp = tmp;
}

void CPackage::StartNewPackage(const char* path)
{
	m_path = path;
	m_toc.clear();
	m_items.clear();
	m_objs.clear();
}

void CPackage::AddObject(const char* item, CBaseObj* pobj)
{
	int num = (int) m_objs.size();

	char path[256];
	sprintf(path, "%stmp_obj_%d.tmp", m_tmp.c_str(), num);
	CBinFile tmpfile(path);
	if (!tmpfile.OpenFile(true, MigType::Object))
		throw fileio_exception("Failed adding object to package");

	pobj->Save(tmpfile);
	tmpfile.CloseFile();

	m_items.push_back(item);
	m_objs.push_back(path);
}

void CPackage::CompleteNewPackage(void)
{
	int size = (int) m_objs.size();
	if (size == 0)
		throw fileio_exception("Empty packages not supported");

	struct stat buf;
	int offset = 0;
	vector<FILE_TOC_ENTRY> toc(size);
	for (int n = 0; n < size; n++)
	{
		if (sizeof(toc[n].item) <= m_items[n].length())
			throw fileio_exception("Package entry size check failed");
		strncpy(toc[n].item, m_items[n].c_str(), sizeof(toc[n].item));
		toc[n].offset = offset;

		const string& objfile = m_objs[n];
		stat(objfile.c_str(), &buf);
		offset += (int) buf.st_size - sizeof(FILE_HEADER);
	}

	CBinFile pkgfile(m_path.c_str());
	if (!pkgfile.OpenFile(true, MigType::Package))
		throw fileio_exception("Unable to open package for writing");
	if (!pkgfile.WriteDataBlock(BlockType::PackageTOC, reinterpret_cast<const byte*>(toc.data()), size * sizeof(FILE_TOC_ENTRY)))
	{
		pkgfile.CloseFile();
		remove(m_path.c_str());
		throw fileio_exception("Unable to write TOC");
	}
	pkgfile.CloseFile();

	FILE* ofh = fopen(m_path.c_str(), "ab+");
	if (ofh == NULL)
	{
		remove(m_path.c_str());
		throw fileio_exception("Unable to open package for appending");
	}

	for (int n = 0; n < size; n++)
	{
		const string& objfile = m_objs[n];
		stat(objfile.c_str(), &buf);

		FILE* ifh = fopen(objfile.c_str(), "rb");
		if (ifh == NULL)
		{
			fclose(ofh);
			remove(m_path.c_str());
			throw fileio_exception("Unable to open object to append to package");
		}

		int size = (int) buf.st_size - sizeof(FILE_HEADER);
		std::vector<byte> data(size);
		if (fseek(ifh, sizeof(FILE_HEADER), SEEK_SET) || fread(data.data(), 1, size, ifh) < (size_t)size)
		{
			fclose(ifh);
			fclose(ofh);
			remove(m_path.c_str());
			throw fileio_exception("Unable to read object header for appending");
		}
		fclose(ifh);

		if (fwrite(data.data(), 1, size, ofh) < (size_t)size)
		{
			fclose(ofh);
			remove(m_path.c_str());
			throw fileio_exception("Unable to append object to package");
		}
	}

	fclose(ofh);
}

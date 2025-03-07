// templates.h - defines any templates used in this project
//

#pragma once

#include <vector>

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CLoadArray - defines a loadable array
//-----------------------------------------------------------------------------

template<class OBJECT, class VECTOR = std::vector<OBJECT> >
class CLoadArray
{
protected:
	VECTOR m_load;
	VECTOR m_objs;

public:
	CLoadArray(void);
	CLoadArray(const CLoadArray& rhs);
	~CLoadArray(void);

	bool LoadDirect(const OBJECT* objs, int nobjs);
	bool LoadObj(const OBJECT& obj);
	bool LoadComplete(void);

	OBJECT* LockArray(int nobjs);
	void UnlockArray(void);

	void DeleteAll(void);

	int CountLoaded(void) const;
	int CountLoading(void) const;
	int Count(void) const;

	void operator=(const CLoadArray& rhs);
	OBJECT& operator[](int n);
	const OBJECT& operator[](int n) const;
};

template<class OBJECT, class VECTOR>
CLoadArray<OBJECT, VECTOR>::CLoadArray(void)
{
}

template<class OBJECT, class VECTOR>
CLoadArray<OBJECT, VECTOR>::CLoadArray(const CLoadArray& rhs)
{
	m_objs = rhs.m_objs;
	m_load = rhs.m_load;
}

template<class OBJECT, class VECTOR>
CLoadArray<OBJECT, VECTOR>::~CLoadArray(void)
{
	DeleteAll();
}

template<class OBJECT, class VECTOR>
bool CLoadArray<OBJECT, VECTOR>::LoadDirect(const OBJECT* objs, int nobjs)
{
	DeleteAll();

	if (nobjs > 0 && objs != NULL)
	{
		m_objs.resize(nobjs);
		memcpy(m_objs.data(), objs, nobjs * sizeof(OBJECT));
	}

	return true;
}

template<class OBJECT, class VECTOR>
bool CLoadArray<OBJECT, VECTOR>::LoadObj(const OBJECT& obj)
{
	m_load.push_back(obj);
	return true;
}

template<class OBJECT, class VECTOR>
bool CLoadArray<OBJECT, VECTOR>::LoadComplete(void)
{
	if (m_load.size() > 0)
	{
		for (const auto& item : m_load)
			m_objs.push_back(item);
		m_load.clear();
	}

	return true;
}

template<class OBJECT, class VECTOR>
OBJECT* CLoadArray<OBJECT, VECTOR>::LockArray(int nobjs)
{
	// note that this routine destroys existing contents of the array
	DeleteAll();

	m_objs.resize(nobjs);
	return m_objs.data();
}

template<class OBJECT, class VECTOR>
void CLoadArray<OBJECT, VECTOR>::UnlockArray(void)
{
}

template<class OBJECT, class VECTOR>
void CLoadArray<OBJECT, VECTOR>::DeleteAll(void)
{
	m_objs.clear();
	m_load.clear();
}

template<class OBJECT, class VECTOR>
int CLoadArray<OBJECT, VECTOR>::CountLoaded(void) const
{
	return static_cast<int>(m_objs.size());
}

template<class OBJECT, class VECTOR>
int CLoadArray<OBJECT, VECTOR>::CountLoading(void) const
{
	return static_cast<int>(m_load.size());
}

template<class OBJECT, class VECTOR>
int CLoadArray<OBJECT, VECTOR>::Count(void) const
{
	return static_cast<int>(m_objs.size() + m_load.size());
}

template<class OBJECT, class VECTOR>
void CLoadArray<OBJECT, VECTOR>::operator=(const CLoadArray& rhs)
{
	m_objs = rhs.m_objs;
	m_load = rhs.m_load;
}

template<class OBJECT, class VECTOR>
OBJECT& CLoadArray<OBJECT, VECTOR>::operator[](int n)
{
	int nobjs = CountLoaded();
	if (n < nobjs)
		return m_objs[n];
	n -= nobjs;
	return m_load[n];
}

template<class OBJECT, class VECTOR>
const OBJECT& CLoadArray<OBJECT, VECTOR>::operator[](int n) const
{
	int nobjs = CountLoaded();
	if (n < nobjs)
		return m_objs[n];
	n -= nobjs;
	return m_load[n];
}

_MIGRENDER_END

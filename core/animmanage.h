// anim.h - defines animation classes
//

#pragma once

#include <vector>
#include <string>
#include <map>

#include "anim.h"
#include "model.h"

_MIGRENDER_BEGIN

// animation manager
class CAnimManager
{
private:
	std::vector<CAnimRecord> _records;

	// temporary during animation
	std::map<std::string, CAnimTarget*> _animObjs;
	int _frames;
	double _maxTime;

protected:
	template <typename T>
	void InterpolateAnimation(const CAnimRecord& record, const CAnimValues<T>& values, double factor);

	void PrepareAnimObjs(CModel& model);
	void ExecuteAnimation(const CAnimRecord& record, double factor);

public:
	CAnimManager(void);

	void ClearAnimation(void);

	void AddRecord(const CAnimRecord& record);
	void AddRecord(const std::string& target, const CAnimRecord& record);
	void AddRecord(const std::string& target, double beginTime, double endTime, const CAnimRecord& record);
	void AddRecord(const std::string& target, double beginTime, double endTime, bool applyToStart, bool reverseAnim, const CAnimRecord& record);

	bool HasRecords(void) const { return !_records.empty(); }
	const std::vector<CAnimRecord>& GetList(void) const { return _records; }
	bool InPlayback(void) const { return GetFramesCount() > 0; }
	int GetFramesCount(void) const { return _frames; }

	double StartPlayback(CModel& model, int frames);
	void GoToFrame(int frame);
	void FinishPlayback(void);
};

_MIGRENDER_END

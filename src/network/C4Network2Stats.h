/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
// network statistics and information dialogs

#ifndef INC_C4Network2Stats
#define INC_C4Network2Stats

#include "game/C4Application.h"
#include "lib/StdBuf.h"

#include <algorithm>

// (int) value by time function
class C4Graph
{
public:
	typedef float ValueType;
	typedef int TimeType;

private:
	StdStrBuf szTitle;
	DWORD dwColor;

public:
	C4Graph();
	virtual ~C4Graph() { }

	void SetTitle(const char *szNewTitle) { szTitle.Copy(szNewTitle); }
	void SetColorDw(DWORD dwClr) { dwColor = dwClr; }

	// retrieve timeframe covered by backlog
	virtual TimeType GetStartTime() const = 0; // inclusively
	virtual TimeType GetEndTime() const   = 0; // exclusively

	// retrieve values within backlog timeframe - guarantueed to be working inside [GetStartTime(), GetEndTime()[
	virtual ValueType GetValue(TimeType iAtTime) const = 0;
	virtual ValueType GetMedianValue(TimeType iStartTime, TimeType iEndTime) const = 0; // median within [iStartTime, iEndTime[
	virtual ValueType GetMinValue() const = 0;
	virtual ValueType GetMaxValue() const = 0;

	// base graph has always just one series (self)
	virtual int GetSeriesCount() const = 0;
	virtual const C4Graph *GetSeries(int iIndex) const = 0;

	DWORD GetColorDw() const { return dwColor; }
	const char *GetTitle() const { return szTitle.getData(); }

	// called before drawing procedure: Make sure graph values are up-to-date
	virtual void Update() const {}

	virtual void SetAverageTime(int iToTime) = 0;
	virtual void SetMultiplier(ValueType fToVal) = 0;
};

// Standard graph: Assume constant time and one call each time frame
class C4TableGraph : public C4Graph
{
private:
	// recorded backlog
	int iBackLogLength;
	ValueType *pValues;
	ValueType fMultiplier; // multiplicative factor used for all value returns
	mutable ValueType *pAveragedValues; // equals pValues if no average is being calculated

	// currently valid backlog timeframe
	int iBackLogPos; // position of next entry to be written
	bool fWrapped; // if true, the buffer has been cycled already

	TimeType iInitialStartTime; // initial table start time; used to calc offset in buffer
	TimeType iTime; // next timeframe to be recorded
	mutable TimeType iAveragedTime; // last timeframe for which average has been calculated
	StdStrBuf szDumpFile; // if set, the buffer is dumped to file regularly
	TimeType iAvgRange;   // range used for averaging

public:
	enum { DefaultBlockLength = 256 }; // default backlog

	C4TableGraph(int iBackLogLength=DefaultBlockLength, TimeType iStartTime = 0);
	~C4TableGraph();

	// flush dump; reset time, etc
	void Reset(TimeType iToTime);

	void SetDumpFile(StdStrBuf &szFile) { szDumpFile = szFile; }

	// retrieve timeframe covered by backlog
	virtual TimeType GetStartTime() const; // inclusively
	virtual TimeType GetEndTime() const;   // exclusively

	// retrieve values within backlog timeframe - guarantueed to be working inside [GetStartTime(), GetEndTime()[
	virtual ValueType GetValue(TimeType iAtTime) const; // retrieve averaged value!
	ValueType GetAtValue(TimeType iAtTime) const; // retrieve not-averaged value
	void SetAvgValue(TimeType iAtTime, ValueType iValue) const; // overwrite avg value at time - considered const because it modifies mutable only
	virtual ValueType GetMedianValue(TimeType iStartTime, TimeType iEndTime) const; // median within [iStartTime, iEndTime[
	virtual ValueType GetMinValue() const;
	virtual ValueType GetMaxValue() const;

	// record a value for this frame; increases time by one
	void RecordValue(ValueType iValue);

	// write table to file
	virtual bool DumpToFile(const StdStrBuf &rszFilename, bool fAppend) const;

	// base graph has always just one series (self)
	virtual int GetSeriesCount() const { return 1; }
	virtual const C4Graph *GetSeries(int iIndex) const { return iIndex ? nullptr : this; }

	virtual void SetAverageTime(int iToTime);
	virtual void Update() const; // make sure average times are correctly calculated

	virtual void SetMultiplier(ValueType fToVal) { fMultiplier = fToVal; }
};

// A graph collection; grouping similar graphs
// Does not delete graph pointers
class C4GraphCollection : public C4Graph, private std::vector<C4Graph *>
{
private:
	// if 0, keep individual for each graph
	int iCommonAvgTime;
	ValueType fMultiplier;
public:
	C4GraphCollection() : iCommonAvgTime(0), fMultiplier(0) {}

	// retrieve max timeframe covered by all graphs
	virtual C4Graph::TimeType GetStartTime() const;
	virtual C4Graph::TimeType GetEndTime() const;

	// must be called on base series only!
	virtual C4Graph::ValueType GetValue(C4Graph::TimeType iAtTime) const { assert(0); return 0; }
	virtual C4Graph::ValueType GetMedianValue(C4Graph::TimeType iStartTime, C4Graph::TimeType iEndTime) const { assert(0); return 0; }

	// get overall min/max for all series
	virtual C4Graph::ValueType GetMinValue() const;
	virtual C4Graph::ValueType GetMaxValue() const;

	// retrieve child (or grandchild) graphs
	virtual int GetSeriesCount() const;
	virtual const C4Graph *GetSeries(int iIndex) const;

	void AddGraph(C4Graph *pAdd) { push_back(pAdd); if (iCommonAvgTime) pAdd->SetAverageTime(iCommonAvgTime); if (fMultiplier) pAdd->SetMultiplier(fMultiplier); }
	void RemoveGraph(const C4Graph *pRemove) { iterator i=std::find(begin(), end(), pRemove); if (i!=end()) erase(i); }

	// update all children
	virtual void Update() const;

	// force values for all children
	virtual void SetAverageTime(int iToTime);
	virtual void SetMultiplier(ValueType fToVal);
};

// network stat collection wrapper
class C4Network2Stats : private C4ApplicationSec1Timer
{
private:

	// per-frame stats
	C4TableGraph statObjCount;

	// per-second stats
	C4TableGraph statFPS;

	// overall network i/o
	C4TableGraph statNetI, statNetO;
	C4GraphCollection graphNetIO;

protected:
	C4GraphCollection statPings; // for all clients

	// per-controlframe stats
	C4GraphCollection statControls;
	C4GraphCollection statActions;

	int SecondCounter; // seconds passed in measured time by network stats module
	int ControlCounter; // control frames passed in measured time by network stats module

	friend class C4Player;
	friend class C4Network2Client;

public:
	C4Network2Stats();
	virtual ~C4Network2Stats();

	// periodic callbacks
	void ExecuteFrame();
	void ExecuteSecond();
	void ExecuteControlFrame();

	virtual void OnSec1Timer() { ExecuteSecond(); }

	C4Graph *GetGraphByName(const StdStrBuf &rszName, bool &rfIsTemp);
};

#endif // INC_C4Network2Stats

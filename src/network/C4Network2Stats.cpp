/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#include "C4Include.h"
#include "network/C4Network2Stats.h"

#include "game/C4Game.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"
#include "object/C4GameObjects.h"
#include "network/C4Network2.h"
#include "control/C4GameControl.h"

C4Graph::C4Graph()
		: szTitle(LoadResStr("IDS_NET_GRAPH")), dwColor(0x7fff0000)
{
}

C4TableGraph::C4TableGraph(int iBackLogLength, int iStartTime)
		: iBackLogLength(iBackLogLength), pValues(nullptr), fMultiplier(1), pAveragedValues(nullptr), iBackLogPos(0), fWrapped(false)
		, iInitialStartTime(iStartTime), iTime(iStartTime), iAveragedTime(iStartTime), iAvgRange(1)
{
	// create value buffer
	assert(iBackLogLength);
	pValues = pAveragedValues = new ValueType[iBackLogLength];
	*pValues = 0;
}

C4TableGraph::~C4TableGraph()
{
	// flush stuff
	Reset(-1);
	// free value buffer(s)
	if (pValues != pAveragedValues) delete [] pAveragedValues;
	delete [] pValues;
}

void C4TableGraph::Reset(TimeType iToTime)
{
	// flush buffer
	if (!!szDumpFile) DumpToFile(szDumpFile, fWrapped);
	// reset stuff
	iInitialStartTime = iTime = iToTime;
	fWrapped = false;
	iBackLogPos = 0;
	*pValues = 0;
}


// retrieve timeframe covered by backlog
C4Graph::TimeType C4TableGraph::GetStartTime() const
{
	// wrap? -> whole buffer used
	if (fWrapped) return iTime - iBackLogLength;
	// otherwise, just buffer to the start used
	return iTime - iBackLogPos;
}

C4Graph::TimeType C4TableGraph::GetEndTime() const
{
	// end time is current
	return iTime;
}

C4Graph::ValueType C4TableGraph::GetValue(TimeType iAtTime) const
{
	// must be inside buffer
	assert(Inside(iAtTime, GetStartTime(), GetEndTime()-1));
	// query it - can't be negative if inside start/end-time
	return pAveragedValues[(iAtTime - iInitialStartTime) % iBackLogLength] * fMultiplier;
}

C4Graph::ValueType C4TableGraph::GetAtValue(TimeType iAtTime) const
{
	// must be inside buffer
	assert(Inside(iAtTime, GetStartTime(), GetEndTime()-1));
	// query it - can't be negative if inside start/end-time
	return pValues[(iAtTime - iInitialStartTime) % iBackLogLength];
}

void C4TableGraph::SetAvgValue(TimeType iAtTime, ValueType iValue) const
{
	// must be inside buffer
	assert(Inside(iAtTime, GetStartTime(), GetEndTime()-1));
	// set it - can't be negative if inside start/end-time
	pAveragedValues[(iAtTime - iInitialStartTime) % iBackLogLength] = iValue;
}

C4Graph::ValueType C4TableGraph::GetMedianValue(TimeType iStartTime, TimeType iEndTime) const
{
	assert(iStartTime < iEndTime);
	// safety: Never build median if no values are recorded
	if (!iBackLogPos && !fWrapped) return 0;
	// sum up and divide in the end - let's hope this will never be called for really large values that could overflow ValueType
	ValueType iSum = GetValue(iStartTime), iNum=1;
	for (; ++iStartTime < iEndTime; ++iNum) iSum += GetValue(iStartTime);
	return iSum / iNum;
}

C4Graph::ValueType C4TableGraph::GetMinValue() const
{
	int iPos0 = iBackLogPos ? iBackLogPos-1 : iBackLogPos;
	ValueType iMinVal = pAveragedValues[iPos0];
	int i = iPos0; ValueType *p = pAveragedValues;
	while (i--) iMinVal = std::min(iMinVal, *p++);
	if (fWrapped)
	{
		i = iBackLogLength - iPos0;
		while (--i) iMinVal = std::min(iMinVal, *++p);
	}
	return iMinVal * fMultiplier;
}

C4Graph::ValueType C4TableGraph::GetMaxValue() const
{
	int iPos0 = iBackLogPos ? iBackLogPos-1 : iBackLogPos;
	ValueType iMaxVal = pAveragedValues[iPos0];
	int i = iPos0; ValueType *p = pAveragedValues;
	while (i--) iMaxVal = std::max(iMaxVal, *p++);
	if (fWrapped)
	{
		i = iBackLogLength - iPos0;
		while (--i) iMaxVal = std::max(iMaxVal, *++p);
	}
	return iMaxVal * fMultiplier;
}

void C4TableGraph::RecordValue(ValueType iValue)
{
	// rec value
	pValues[iBackLogPos] = iValue;
	// calc time
	++iTime;
	if (++iBackLogPos >= iBackLogLength)
	{
		// create dump before overwriting last buffer
		if (!!szDumpFile) DumpToFile(szDumpFile, fWrapped);
		// restart buffer
		fWrapped = true;
		iBackLogPos = 0;
	}
}

bool C4TableGraph::DumpToFile(const StdStrBuf &rszFilename, bool fAppend) const
{
	assert(!!rszFilename);
	// nothing to write?
	if (!fWrapped && !iBackLogPos) return false;
	// try append if desired; create if unsuccessful
	CStdFile out;
	if (fAppend) if (!out.Append(rszFilename.getData())) fAppend = false;
	if (!fAppend)
	{
		if (!out.Create(rszFilename.getData())) return false;
		// print header
		out.WriteString("t\tv\n\r");
	}
	// write out current timeframe
	int iEndTime = GetEndTime();
	StdStrBuf buf;
	for (int iWriteTime = GetStartTime(); iWriteTime < iEndTime; ++iWriteTime)
	{
		buf.Format("%d\t%d\n\r", (int) iWriteTime, (int) GetValue(iWriteTime));
		out.WriteString(buf.getData());
	}
	return true;
}

void C4TableGraph::SetAverageTime(int iToTime)
{
	// set new time; resetting valid, averaged range
	if (iAveragedTime == iToTime) return;
	assert(iToTime > 0);
	iAvgRange = iToTime;
	iAveragedTime = iInitialStartTime;
}

#define FORWARD_AVERAGE
#define FORWARD_AVERAGE_FACTOR 4

void C4TableGraph::Update() const
{
	// no averaging necessary?
	if (pAveragedValues == pValues)
	{
		if (iAvgRange == 1) return;
		// averaging necessary, but buffer not yet created: Create it!
		pAveragedValues = new ValueType[iBackLogLength];
	}
	// up-to-date?
	if (iAveragedTime == iTime) return;
	assert(iAveragedTime < iTime); // must not have gone back!
	// update it
	int iStartTime = GetStartTime();
#ifdef FORWARD_AVERAGE
	int iAvgFwRange = iAvgRange/FORWARD_AVERAGE_FACTOR;
#else
	int iAvgFwRange = 0;
#endif
	for (int iUpdateTime = std::max(iAveragedTime-iAvgFwRange-1, iStartTime); iUpdateTime < iTime; ++iUpdateTime)
	{
		ValueType iSum=0, iSumWeight=0, iWeight;
		for (int iSumTime = std::max(iUpdateTime - iAvgRange, iStartTime); iSumTime < std::min(iUpdateTime + iAvgFwRange+1, iTime); ++iSumTime)
		{
			iWeight = (ValueType) iAvgRange - Abs(iUpdateTime - iSumTime) + 1;
			iSum += GetAtValue(iSumTime) * iWeight;
			iSumWeight += iWeight;
		}
		SetAvgValue(iUpdateTime, iSum / iSumWeight);
	}
	// now it's all up-to-date
	iAveragedTime = iTime;
}

// --------------------------------------------------

C4Graph::TimeType C4GraphCollection::GetStartTime() const
{
	const_iterator i = begin(); if (i == end()) return 0;
	C4Graph::TimeType iTime = (*i)->GetStartTime();
	while (++i != end()) iTime = std::min(iTime, (*i)->GetStartTime());
	return iTime;
}

C4Graph::TimeType C4GraphCollection::GetEndTime() const
{
	const_iterator i = begin(); if (i == end()) return 0;
	C4Graph::TimeType iTime = (*i)->GetEndTime();
	while (++i != end()) iTime = std::max(iTime, (*i)->GetEndTime());
	return iTime;
}

C4Graph::ValueType C4GraphCollection::GetMinValue() const
{
	const_iterator i = begin(); if (i == end()) return 0;
	C4Graph::ValueType iVal = (*i)->GetMinValue();
	while (++i != end()) iVal = std::min(iVal, (*i)->GetMinValue());
	return iVal;
}

C4Graph::ValueType C4GraphCollection::GetMaxValue() const
{
	const_iterator i = begin(); if (i == end()) return 0;
	C4Graph::ValueType iVal = (*i)->GetMaxValue();
	while (++i != end()) iVal = std::max(iVal, (*i)->GetMaxValue());
	return iVal;
}

int C4GraphCollection::GetSeriesCount() const
{
	int iCount = 0;
	for (const_iterator i = begin(); i != end(); ++i) iCount += (*i)->GetSeriesCount();
	return iCount;
}

const C4Graph *C4GraphCollection::GetSeries(int iIndex) const
{
	for (const_iterator i = begin(); i != end(); ++i)
	{
		int iCnt = (*i)->GetSeriesCount();
		if (iIndex < iCnt) return (*i)->GetSeries(iIndex);
		iIndex -= iCnt;
	}
	return nullptr;
}

void C4GraphCollection::Update() const
{
	// update all child graphs
	for (const_iterator i = begin(); i != end(); ++i) (*i)->Update();
}

void C4GraphCollection::SetAverageTime(int iToTime)
{
	if ((iCommonAvgTime = iToTime))
		for (iterator i = begin(); i != end(); ++i) (*i)->SetAverageTime(iToTime);
}

void C4GraphCollection::SetMultiplier(ValueType fToVal)
{
	if ((fMultiplier = fToVal))
		for (iterator i = begin(); i != end(); ++i) (*i)->SetMultiplier(fToVal);
}


// --------------------------------------------------------------------------------

C4Network2Stats::C4Network2Stats()
{
	// init callback timer
	Application.Add(this);
	SecondCounter = 0;
	ControlCounter = 0;
	// init graphs
	statObjCount.SetTitle(LoadResStr("IDS_MSG_OBJCOUNT"));
	statFPS.SetTitle(LoadResStr("IDS_MSG_FPS"));
	statNetI.SetTitle(LoadResStr("IDS_NET_INPUT"));
	statNetI.SetColorDw(0x00ff00);
	statNetO.SetTitle(LoadResStr("IDS_NET_OUTPUT"));
	statNetO.SetColorDw(0xff0000);
	graphNetIO.AddGraph(&statNetI); graphNetIO.AddGraph(&statNetO);
	statControls.SetTitle(LoadResStr("IDS_NET_CONTROL"));
	statControls.SetAverageTime(100);
	statActions.SetTitle(LoadResStr("IDS_NET_APM"));
	statActions.SetAverageTime(100);
	for (C4Player *pPlr = ::Players.First; pPlr; pPlr = pPlr->Next) pPlr->CreateGraphs();
	C4Network2Client *pClient = nullptr;
	while ((pClient = ::Network.Clients.GetNextClient(pClient))) pClient->CreateGraphs();
}

C4Network2Stats::~C4Network2Stats()
{
	for (C4Player *pPlr = ::Players.First; pPlr; pPlr = pPlr->Next) pPlr->ClearGraphs();
	C4Network2Client *pClient = nullptr;
	while ((pClient = ::Network.Clients.GetNextClient(pClient))) pClient->ClearGraphs();
	Application.Remove(this);
}

void C4Network2Stats::ExecuteFrame()
{
	statObjCount.RecordValue(C4Graph::ValueType(::Objects.ObjectCount()));
}

void C4Network2Stats::ExecuteSecond()
{
	statFPS.RecordValue(C4Graph::ValueType(Game.FPS));
	statNetI.RecordValue(C4Graph::ValueType(::Network.NetIO.getProtIRate(P_TCP) + ::Network.NetIO.getProtIRate(P_UDP)));
	statNetO.RecordValue(C4Graph::ValueType(::Network.NetIO.getProtORate(P_TCP) + ::Network.NetIO.getProtORate(P_UDP)));
	// pings for all clients
	C4Network2Client *pClient = nullptr;
	while ((pClient = ::Network.Clients.GetNextClient(pClient))) if (pClient->getStatPing())
		{
			int iPing=0;
			C4Network2IOConnection *pConn = pClient->getMsgConn();
			if (pConn) iPing = pConn->getLag();
			pClient->getStatPing()->RecordValue(C4Graph::ValueType(iPing));
		}
	++SecondCounter;
}

void C4Network2Stats::ExecuteControlFrame()
{
	// control rate may have updated: always convert values to actions per minute
	statControls.SetMultiplier((C4Graph::ValueType) 1000 / 38 / ::Control.ControlRate);
	statActions.SetMultiplier((C4Graph::ValueType) 1000 / 38 * 60 / ::Control.ControlRate);
	// register and reset control counts for all players
	for (C4Player *pPlr = ::Players.First; pPlr; pPlr = pPlr->Next)
	{
		if (pPlr->pstatControls)
		{
			pPlr->pstatControls->RecordValue(C4Graph::ValueType(pPlr->ControlCount));
			pPlr->ControlCount = 0;
		}
		if (pPlr->pstatActions)
		{
			pPlr->pstatActions->RecordValue(C4Graph::ValueType(pPlr->ActionCount));
			pPlr->ActionCount = 0;
		}
	}
	++ControlCounter;
}

C4Graph *C4Network2Stats::GetGraphByName(const StdStrBuf &rszName, bool &rfIsTemp)
{
	// compare against default graph names
	rfIsTemp = false;
	if (SEqualNoCase(rszName.getData(), "oc")) return &statObjCount;
	if (SEqualNoCase(rszName.getData(), "fps")) return &statFPS;
	if (SEqualNoCase(rszName.getData(), "netio")) return &graphNetIO;
	if (SEqualNoCase(rszName.getData(), "pings")) return &statPings;
	if (SEqualNoCase(rszName.getData(), "control")) return &statControls;
	if (SEqualNoCase(rszName.getData(), "apm")) return &statActions;
	// no match
	return nullptr;
}

// MassGraph.SetDumpFile(StdStrBuf("C:\\test.txt"));

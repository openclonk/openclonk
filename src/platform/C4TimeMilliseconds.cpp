/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013  Tobias Zwick
 * Copyright (c) 2005, 2011  Günther Brammer
 * Copyright (c) 2012  Martin Plicht
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include "C4TimeMilliseconds.h"
#include <limits>

#ifdef _WIN32

#include <C4windowswrapper.h>
#include <mmsystem.h>

C4TimeMilliseconds C4TimeMilliseconds::Now()
{
	return C4TimeMilliseconds(timeGetTime());
}

#else

#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif

C4TimeMilliseconds C4TimeMilliseconds::Now()
{
#ifdef __APPLE__
	static time_t sec_offset;
	timeval tv;
	gettimeofday(&tv, 0);
	if (!sec_offset) sec_offset = tv.tv_sec;
	return C4TimeMilliseconds((tv.tv_sec - sec_offset) * 1000 + tv.tv_usec / 1000);
#else
	timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	static time_t sec_offset = tv.tv_sec;
	return C4TimeMilliseconds((tv.tv_sec - sec_offset) * 1000 + tv.tv_nsec / 1000000);
#endif
}

#endif

const char* C4TimeMilliseconds::AsString() const
{
	if (inf == PositiveInfinity)
	{
		return "POSITIVE INFINITY";
	}
	if (inf == NegativeInfinity)
	{
		return "NEGATIVE INFINITY";
	}
	StdStrBuf string;
	string.Format("%u:%02u:%02u:%03u:",time / 1000 / 60 / 60, (time / 1000 / 60) % 60, (time / 1000) % 60, time % 1000);
	return string.getData();
}

C4TimeMilliseconds& C4TimeMilliseconds::operator=(const C4TimeMilliseconds& rhs)
{
	time = rhs.time;
	inf = rhs.inf;
	return *this;
}

bool operator==( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )
{
	return lhs.inf == rhs.inf && 
	       lhs.time == rhs.time;
}

bool operator<( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )
{
	if (lhs.inf != C4TimeMilliseconds::NoInfinity ||
	    rhs.inf != C4TimeMilliseconds::NoInfinity)
	{
		return lhs.inf < rhs.inf;
	}
	return int32_t(lhs.time - rhs.time) < 0;
}

int32_t operator-(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs)
{
	// if infinity is set, nothing else than infinity matters (infinity + 100 == infinity)
	if (lhs.inf != C4TimeMilliseconds::NoInfinity ||
	    rhs.inf != C4TimeMilliseconds::NoInfinity)
	{
		int infinityTo = lhs.inf - rhs.inf;
		
		if (infinityTo < 0) return std::numeric_limits<int32_t>::min();
		if (infinityTo > 0) return std::numeric_limits<int32_t>::max();
		return 0;
	}
	// otherwise, as usual
	return int32_t(lhs.time - rhs.time);
}

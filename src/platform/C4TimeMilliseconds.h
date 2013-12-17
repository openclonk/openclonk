/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013, The OpenClonk Team and contributors
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
#ifndef INC_C4TimeMilliseconds
#define INC_C4TimeMilliseconds

#include "PlatformAbstraction.h"

/* Class to store times in milliseconds for measurement purposes.

   This class behaves for the most part like an unsigned integer, with the
   difference that it handles an overflow correctly. For example:
     C4TimeMilliseconds start = UINT32_MAX-10;
     C4TimeMilliseconds stop = 10;
   start < stop returns true. stop - start is 20.
   
   A commonly used operation is to measure the time difference between two
   C4TimeMilliseconds, that is why if two C4TimeMilliseconds are subtracted from another,
   the return value is of the type uint32_t.

   Otherwise, there should be no use case other than for printing, or packing/
   unpacking for network to have a uint32_t representation of this. You can use
   AsInt()/AsString() for that. */

class C4TimeMilliseconds
{
public:
	enum Infinity
	{
		NegativeInfinity = -1,
		NoInfinity = 0,
		PositiveInfinity = 1
};

private:
	uint32_t time;
	Infinity inf;

public:
	
	static C4TimeMilliseconds Now();

	C4TimeMilliseconds() : time(0), inf(NoInfinity) { }
	C4TimeMilliseconds(uint32_t millis) : time(millis), inf(NoInfinity) { }
	C4TimeMilliseconds(C4TimeMilliseconds::Infinity infinity) : time(0), inf(infinity) { }
	C4TimeMilliseconds(const C4TimeMilliseconds& rhs) : time(rhs.time), inf(rhs.inf) { }
	~C4TimeMilliseconds() { }

	/* Returns the stored time. Do not use this for comparisons because this method always
	   returns the stored time, independent of whether this variable is actually infinite. */
	uint32_t AsInt() const { return time; }
	/* Returns whether this variable is set to some infinity. This does normally mean that
	   the variable is not initialized yet. */
	bool IsInfinite() const { return inf != NoInfinity; }
	/* Returns a string representation useful for debugging and logging purposes. */
	StdCopyStrBuf AsString() const;

	C4TimeMilliseconds& operator=(const C4TimeMilliseconds& rhs);
	
	inline C4TimeMilliseconds& operator-=(const uint32_t& rhs) { time -= rhs; return *this; }
	inline C4TimeMilliseconds& operator+=(const uint32_t& rhs) { time += rhs; return *this; }

	friend bool operator==( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs );
	friend bool operator<( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs );
	friend int32_t operator-(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs);
};

bool operator==( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs );
bool operator<( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs );

inline bool operator!=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs == rhs); }
inline bool operator>( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )  { return rhs < lhs; }
inline bool operator<=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs > rhs); }
inline bool operator>=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs < rhs); }

int32_t operator-(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs);

inline C4TimeMilliseconds operator+(C4TimeMilliseconds lhs, const uint32_t& rhs)        { lhs += rhs; return lhs; }
inline C4TimeMilliseconds operator-(C4TimeMilliseconds lhs, const uint32_t& rhs)        { lhs -= rhs; return lhs; }

#endif

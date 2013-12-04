/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013  Tobias Zwick
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
   AsInt() for that. */
class C4TimeMilliseconds
{
private:
	uint32_t time;

public:
	C4TimeMilliseconds() : time(0) { }
	C4TimeMilliseconds(uint32_t millis) : time(millis) { }
	C4TimeMilliseconds(const C4TimeMilliseconds& rhs) : time(rhs.time) { }
	~C4TimeMilliseconds() { }

	uint32_t AsInt() const { return time; }

	C4TimeMilliseconds& operator=(const C4TimeMilliseconds& rhs)  { time = rhs.time; return *this; }
	
	inline C4TimeMilliseconds& operator-=(const uint32_t& rhs) { time -= rhs; return *this; }
	inline C4TimeMilliseconds& operator+=(const uint32_t& rhs) { time += rhs; return *this; }

};

inline bool operator==( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return lhs.AsInt() == rhs.AsInt(); }
inline bool operator!=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs == rhs); }
inline bool operator<( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )  { return int32_t(lhs.AsInt() - rhs.AsInt()) < 0; }
inline bool operator>( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs )  { return rhs < lhs; }
inline bool operator<=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs > rhs); }
inline bool operator>=( const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs ) { return !(lhs < rhs); }

inline uint32_t operator-(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs) { return lhs.AsInt() - rhs.AsInt(); }

inline C4TimeMilliseconds operator+(C4TimeMilliseconds lhs, const uint32_t& rhs)        { lhs += rhs; return lhs; }
inline C4TimeMilliseconds operator-(C4TimeMilliseconds lhs, const uint32_t& rhs)        { lhs -= rhs; return lhs; }

/* the following operations make no sense and should rather be not defined to
   throw a compiler error than being defined for the sake of it */

//inline uint32_t operator+(const C4TimeMilliseconds& lhs, const C4TimeMilliseconds& rhs) { return lhs.AsInt() + rhs.AsInt(); }


#endif

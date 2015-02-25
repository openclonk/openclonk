/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2015, The OpenClonk Team and contributors
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

#ifndef INC_make_unique
#define INC_make_unique

#include "config.h"

// C++14 std::make_unique
#include <memory>
#include <utility>
#ifndef HAVE_MAKE_UNIQUE
namespace detail {
template<class T>
typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0, std::unique_ptr<T>>::type
make_unique(size_t n)
{
	return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

#ifdef HAVE_VARIADIC_TEMPLATES
template<class T, class ...Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
// We have to emulate variadic templates with preprocessor trickery.
// Create a number of overloaded functions that handle this just like the
// one function above. Works for up to _VARIADIC_MAX constructor arguments
// (if you need more than 5, define that macro before including this header).
#include <boost/preprocessor/inc.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
// This is the same macro that the MSVC STL also uses when it has to emulate
// variadic templates, so it should work fine for our purposes
#ifndef _VARIADIC_MAX
#define _VARIADIC_MAX 5
#endif

#define MAKE_UNIQUE_FORWARDER(z, n, unused) std::forward<Arg ## n>(arg ## n)
#define MAKE_UNIQUE_MAKER(z, n, unused) \
	template<class T BOOST_PP_ENUM_TRAILING_PARAMS(n, class Arg)> \
	typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type make_unique(BOOST_PP_ENUM_BINARY_PARAMS(n, Arg, &&arg)) \
	{ \
		return std::unique_ptr<T>(new T(BOOST_PP_ENUM(n, MAKE_UNIQUE_FORWARDER, unused))); \
	}

BOOST_PP_REPEAT(BOOST_PP_INC(_VARIADIC_MAX), MAKE_UNIQUE_MAKER, unused)
#undef MAKE_UNIQUE_MAKER
#undef MAKE_UNIQUE_FORWARDER
#endif
}
namespace std { using ::detail::make_unique; }
#endif

#endif

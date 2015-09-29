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

template<class T, class ...Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}
namespace std { using ::detail::make_unique; }
#endif

#endif

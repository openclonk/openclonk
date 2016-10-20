/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2016, The OpenClonk Team and contributors
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

#ifndef C4PuncherHash_H
#define C4PuncherHash_H

#include <functional>
#include "network/C4NetIO.h"

namespace {

	size_t hash_combiner(size_t left, size_t right) //replacable
	{ return left*2793419347^right; }

	template<int index, class...types>
	struct hash_impl {
		size_t operator()(size_t a, const std::tuple<types...>& t) const {
			typedef typename std::tuple_element<index-1, std::tuple<types...>>::type nexttype;
			hash_impl<index-1, types...> next;
			size_t b = std::hash<nexttype>()(std::get<index-1>(t));
			return next(hash_combiner(a, b), t);
		}
	};
	template<class...types>
	struct hash_impl<1, types...> {
		size_t operator()(size_t a, const std::tuple<types...>& t) const {
			typedef typename std::tuple_element<0, std::tuple<types...>>::type nexttype;
			size_t b = std::hash<nexttype>()(std::get<0>(t));
			return hash_combiner(a, b);
		}
	};
	template<class...types>
	struct hash_impl<0, types...> {
		size_t operator()(size_t a, const std::tuple<types...>& t) const {
			return 0;
		}
	};
}

namespace std { 
	// Remove this when C++ gets proper tuple hashes. For now, http://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set will do
	template<class...types>
	struct hash<std::tuple<types...>> {
		size_t operator()(const std::tuple<types...>& t) const {
			const size_t begin = std::tuple_size<std::tuple<types...>>::value;
			return hash_impl<begin, types...>()(19739, t);
		}
	};

	template<>
	struct hash<::sockaddr_in> {
		size_t operator()(const ::sockaddr_in& addr) const {
			auto unpack = make_tuple(addr.sin_family, addr.sin_addr.s_addr, addr.sin_port);
			return hash<decltype(unpack)>()(unpack);
		}
	};
}

#endif

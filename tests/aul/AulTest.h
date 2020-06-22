/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2015-2016, The OpenClonk Team and contributors
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

// Testing C4Aul behaviour.

#include <gtest/gtest.h>
#include <iostream>

#include "script/C4Value.h"

inline std::ostream &operator<<(std::ostream &os, const C4Value &val)
{
	return os << val.GetDataString().getData();
}

class AulTest : public ::testing::Test
{
protected:
	C4Value RunCode(const std::string &code);
	C4Value RunScript(const std::string &code);
	C4Value RunExpr(const std::string &expr);

	static const C4Value C4VINT_MIN;
	static const C4Value C4VINT_MAX;

	virtual void SetUp() override;

private:
	int part_count = 0;
};

namespace aul_test {
	namespace detail {
		inline void _setItems(C4ValueArray *a, int i) {}
		template<class T0, class ...T>
		inline void _setItems(C4ValueArray *a, int i, T0 &&v, T &&...t)
		{
			a->SetItem(i, v);
			_setItems(a, ++i, std::forward<T>(t)...);
		}

		// Helper functions to create array C4Values inline
		template<class ...T>
		inline C4Value C4VArray(T &&...v)
		{
			C4ValueArray *a = new C4ValueArray(sizeof...(v));
			_setItems(a, 0, std::forward<T>(v)...);
			return C4VArray(a);
		}

		inline void _setItems(C4PropList *p) {}
		template<class T0, class T1, class ...T>
		inline void _setItems(C4PropList *p, T0 &&k, T1 &&v, T &&...t)
		{
			p->SetPropertyByS(::Strings.RegString(k), v);
			_setItems(p, std::forward<T>(t)...);
		}

		// Helper function to create proplist C4Values inline
		template<class ...T>
		inline C4Value C4VPropList(T &&...v)
		{
			static_assert(sizeof...(v) % 2 == 0, "Proplist constructor needs even number of arguments");
			C4PropList *p = C4PropList::New();
			_setItems(p, std::forward<T>(v)...);
			return C4VPropList(p);
		}
	}
}

using ::aul_test::detail::C4VArray;
using ::aul_test::detail::C4VPropList;
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

/*
 * This file serves as a quick way to check wither COMPILED_AS_C4LIBRARY is set, which is the case when compiling lib{misc,c4script}
 * This file should be included when the USE_CONSOLE define is checked. In the past, we had some header files with classes like
class Foo {
#ifndef USE_CONSOLE
	int bar;
#endif
	int blubb;
}
 * If such a header was included in the library, the class definition would look different for the library than for the openclonk-server, which could be potentially desasterous. (just think of accessing blubb, or destructors, or copying it by size, or…)
*/

#ifdef COMPILED_AS_C4LIBRARY // as (hopefully) defined in the CMakeLists.txt
#error A file that is marked as not suitable for being included in lib{misc,c4script} (probably because it checks USE_CONSOLE) has been included in their compilation. Check your header dependencies.
#endif

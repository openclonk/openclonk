/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2018, The OpenClonk Team and contributors
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

class C4PlayerScript
{
public:
	// The script functions are registered to this player prototype in the engine 
	static constexpr const char *PROTOTYPE_NAME_ENGINE = "_Player";
	// The actual player prototype is defined in script and inherits from the engine prototype
	static constexpr const char *PROTOTYPE_NAME_SCRIPT = "Player";
	static void RegisterWithEngine(C4AulScriptEngine *engine);
};

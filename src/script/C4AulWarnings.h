/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2017, The OpenClonk Team and contributors
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

// C4Aul diagnostics definitions

#pragma push_macro("DIAG")
#ifndef DIAG
#define DIAG(id, text, enabled_by_default)
#endif

// Lexer diagnostics
DIAG(invalid_escape_sequence, "unknown escape sequence '\\%1$s'", true)
DIAG(invalid_hex_escape, "'\\x' used with no following hex digits", true)

// Parser diagnostics
DIAG(type_name_used_as_par_name, "type '%1$s' used as parameter name", false)
DIAG(empty_parameter_in_call, "parameter %1$zu of call to '%2$s' is empty", false)
DIAG(empty_parameter_in_array, "array entry %1$zu is empty", false)

// Compiler diagnostics
DIAG(implicit_range_loop_var_decl, "implicit declaration of the loop variable '%1$s' in a for-in loop is deprecated", true)
DIAG(non_global_var_is_never_const, "variable '%1$s' declared as const, but non-global variables are always mutable", true)
DIAG(variable_shadows_variable, "declaration of %2$s '%1$s' shadows %3$s", true)
DIAG(redeclaration, "redeclaration of %2$s '%1$s'", true)
DIAG(undeclared_varargs, "use of '%1$s' in a function forces it to take variable arguments", true)
DIAG(arg_count_mismatch, "call to '%1$s' passes %2$zu arguments, of which only %3$zu are used", true)
DIAG(arg_type_mismatch, "parameter %2$zu of call to '%1$s' passes %3$s (%4$s expected)", true)
DIAG(empty_if, "empty controlled statement (use '{}' if this is intentional)", true)

#pragma pop_macro("DIAG")

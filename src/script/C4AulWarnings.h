/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2017-2018, The OpenClonk Team and contributors
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
DIAG(invalid_escape_sequence, "unknown escape sequence '\\%c'", true)
DIAG(invalid_hex_escape, "'\\x' used with no following hex digits", true)

// Parser diagnostics
DIAG(type_name_used_as_par_name, "type '%s' used as parameter name", false)
DIAG(empty_parameter_in_call, "parameter %u of call to '%s' is empty", false)
DIAG(empty_parameter_in_array, "array entry %u is empty", false)

// Compiler diagnostics
DIAG(implicit_range_loop_var_decl, "implicit declaration of the loop variable '%s' in a for-in loop is deprecated", true)
DIAG(non_global_var_is_never_const, "variable '%s' declared as const, but non-global variables are always mutable", true)
DIAG(variable_shadows_variable, "declaration of %s '%s' shadows %s", true)
DIAG(variable_out_of_scope, "variable '%s' used outside of its declared scope", true)
DIAG(redeclaration, "redeclaration of %s '%s'", true)
DIAG(undeclared_varargs, "use of '%s' in a function forces it to take variable arguments", true)
DIAG(arg_count_mismatch, "call to '%s' passes %u arguments, of which only %u are used", true)
DIAG(arg_type_mismatch, "parameter %u of call to '%s' passes %s (%s expected)", true)
DIAG(empty_if, "empty controlled statement (use '{}' if this is intentional)", true)
DIAG(suspicious_assignment, "suspicious assignment (was a comparison intended here?)", true)

#pragma pop_macro("DIAG")

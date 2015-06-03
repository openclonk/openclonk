/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
// user input validation functions

#include "C4Include.h"
#include <C4InputValidation.h>
#include <C4Log.h>
#include "C4Markup.h"

#include <cctype>

namespace C4InVal
{
	bool ValidateString(char *szString, ValidationOption eOption, size_t iMaxSize)
	{
		// validate in a StdStrBuf. Does one alloc and copy :(
		StdStrBuf buf; buf.Copy(szString);
		bool fInvalid = ValidateString(buf, eOption);
		if (fInvalid) SCopy(buf.getData(), szString, iMaxSize);
		return fInvalid;
	}

	bool ValidateString(StdStrBuf &rsString, ValidationOption eOption)
	{
		bool fValid = true;
		// validation depending on option
		// check min length
		if (!rsString.getLength())
		{
			// empty if not allowed?
			if (eOption != VAL_NameAllowEmpty && eOption != VAL_NameExAllowEmpty && eOption != VAL_Comment)
			{
				rsString.Copy("empty");
				fValid = false;
			}
		}
		switch (eOption)
		{
		case VAL_Filename: // regular filenames only
			// absolutely no directory traversal
			if (rsString.ReplaceChar('/', '_')) fValid = false;
			if (rsString.ReplaceChar('\\', '_')) fValid = false;

			// fallthrough to general file name validation
		case VAL_SubPathFilename: // filenames and optional subpath
			// do not traverse upwards in file hierarchy
			if (rsString.Replace("..", "__")) fValid = false;
			if (*rsString.getData() == '/' || *rsString.getData() == '\\') { *rsString.getMData() = '_'; fValid = false; }

			// fallthrough to general file name validation
		case VAL_FullPath:        // full filename paths
			// some characters are prohibited in filenames in general
			if (rsString.ReplaceChar('*', '_')) fValid = false;
			if (rsString.ReplaceChar('?', '_')) fValid = false;
			if (rsString.ReplaceChar('<', '_')) fValid = false;
			if (rsString.ReplaceChar('>', '_')) fValid = false;
			// ';' and '|' is never allowed in filenames, because it would cause problems in many engine internal file lists
			if (rsString.ReplaceChar(';', '_')) fValid = false;
			if (rsString.ReplaceChar('|', '_')) fValid = false;
			// the colon is generally prohibited except at pos 2 (C:\...), because it could lead to creation of (invisible) streams on NTFS
			if (rsString.ReplaceChar(':', '_')) fValid = false;
			if (*rsString.getData() == ':') { *rsString.getMData() = '_'; fValid = false; }
			// validate drive letter
			if (rsString.getLength()>=2 && *rsString.getPtr(1) == ':')
			{
				if (eOption != VAL_FullPath)
				{
					*rsString.getMPtr(1)='_'; fValid = false;
				}
				else if (!isalpha((unsigned char)*rsString.getData()) || (*rsString.getPtr(2)!='\\' && *rsString.getPtr(2)!='/'))
				{
					*rsString.getMData()=*rsString.getMPtr(1)='_'; fValid = false;
				}
			}
			break;

		case VAL_NameNoEmpty:
		case VAL_NameAllowEmpty:
			// no markup
			if (C4Markup::StripMarkup(&rsString)) { fValid = false; }
			// trim spaces
			if (rsString.TrimSpaces()) fValid = false;
			// min length
			if (eOption == VAL_NameNoEmpty) if (!rsString.getLength()) { fValid = false; rsString.Copy("Unknown"); }
			// max length
			if (rsString.getLength() > C4MaxName) { fValid = false; rsString.SetLength(C4MaxName); }
			break;

		case VAL_NameExNoEmpty:
		case VAL_NameExAllowEmpty:
			// trim spaces
			if (rsString.TrimSpaces()) fValid = false;
			// min length
			if (eOption == VAL_NameExNoEmpty) if (!rsString.getLength()) { fValid = false; rsString.Copy("Unknown"); }
			// max length
			if (rsString.getLength() > C4MaxLongName) { fValid = false; rsString.SetLength(C4MaxLongName); }
			break;

		case VAL_IRCName: // nickname for IRC. a-z, A-Z, _^{[]} only; 0-9|- inbetween; max 30 characters
			if (rsString.getLength() > 30) fValid = false;
			if (rsString.getLength() < 2) fValid = false;
			if (!rsString.ValidateChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_^{[]}", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_^{[]}0123456789|-")) { fValid = false; rsString.Copy("Guest"); }
			if (SEqualNoCase(rsString.getData(), "NickServ")
			    || SEqualNoCase(rsString.getData(), "ChanServ")
			    || SEqualNoCase(rsString.getData(), "MemoServ")
			    || SEqualNoCase(rsString.getData(), "OperServ")
			    || SEqualNoCase(rsString.getData(), "HelpServ")) fValid = false;
			if (!fValid) rsString.Copy("Guest");
			break;

		case VAL_IRCPass: // password for IRC; max 31 characters
			// max length; no spaces
			if (rsString.getLength() > 31) { fValid = false; rsString.SetLength(31); }
			if (rsString.getLength() < 2) { fValid = false; rsString.Copy("secret"); }
			if (rsString.ReplaceChar(' ', '_')) fValid = false;
			break;

		case VAL_IRCChannel: // IRC channel name
		{ // needed for the vector
			std::vector<StdCopyStrBuf> chans;
			StdStrBuf SplitPart;
			StdCopyStrBuf tmp; tmp.Copy(rsString);
			while(tmp.SplitAtChar(',', &SplitPart)) // Split
			{
				chans.push_back(tmp);
				tmp.Copy(SplitPart);
			}
			chans.push_back(tmp);
			rsString.Clear();
			for(std::vector<StdCopyStrBuf>::iterator it = chans.begin(); it < chans.end(); ++it) // Reassemble clean
			{
				if (it->getLength() > 32) { fValid = false; it->SetLength(32); }
				else if (it->getLength() < 2) { fValid = false; it->Clear(); }
				else if (*it->getData() != '#' && *it->getData() != '+') { fValid = false; it->InsertChar('#', 0); }
				if (it->ReplaceChar(' ', '_')) fValid = false;
				rsString.Append(*it);
				if(it+1 < chans.end() && it->getLength() > 0) rsString.Append(",");
			}
			if(rsString.getLength() < 2) rsString.Copy("#openclonk");
			break;
		}

		case VAL_Comment: // comment - just limit length
			if (rsString.getLength() > C4MaxComment) { fValid = false; rsString.SetLength(C4MaxComment); }
			break;

		default:
			assert(!"not yet implemented");
		}
		// issue warning for invalid adjustments
		if (0) if (!fValid)
		{
			const char *szOption = "unknown";
			switch (eOption)
			{
			case VAL_Filename:         szOption = "filename";         break;
			case VAL_SubPathFilename:  szOption = "(sub-)filename";   break;
			case VAL_FullPath:         szOption = "free filename";    break;
			case VAL_NameNoEmpty:      szOption = "strict name";      break;
			case VAL_NameExNoEmpty:    szOption = "name";             break;
			case VAL_NameAllowEmpty:   szOption = "strict name*";     break;
			case VAL_NameExAllowEmpty: szOption = "name*";            break;
			case VAL_IRCName:          szOption = "IRC nick";         break;
			case VAL_IRCPass:          szOption = "IRC password";     break;
			case VAL_IRCChannel:       szOption = "IRC channel";      break;
			case VAL_Comment:          szOption = "Comment";          break;
			}
			LogF("WARNING: Adjusted invalid user input for \"%s\" to \"%s\"", szOption, rsString.getData());
		}
		return !fValid;
	}

	bool ValidateInt(int32_t &riVal, int32_t iMinVal, int32_t iMaxVal)
	{
		if (riVal < iMinVal) { riVal = iMinVal; return false; }
		else if (riVal > iMaxVal) { riVal = iMaxVal; return false; }
		else return true;
	}
}

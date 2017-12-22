/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
// About/credits screen

#include "C4Include.h"
#include "gui/C4StartupAboutDlg.h"

#include "C4Version.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4UpdateDlg.h"

struct PersonList
{
	struct Entry
	{
		const char *name, *nick;
	};
	virtual void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font) = 0;
	virtual ~PersonList() { }
};

static struct DeveloperList : public PersonList
{
	std::vector<Entry> developers;

	DeveloperList(std::initializer_list<Entry> l) : developers(l) { }

	void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font)
	{
		for (auto& p : developers)
		{
			textbox->AddTextLine(FormatString("%s <c f7f76f>(%s)</c>", p.name, p.nick).getData(), &font, C4GUI_MessageFontClr, false, true);
		}
	}
}
// the following lists are all sorted by all-time commit count: git shortlog -s | sort -rn
engineAndTools =
{
	{"Sven Eberhardt", "Sven2"},
	{"Günther Brammer", "Günther"},
	{"Nicolas Hake", "Isilkor"},
	{"Armin Burgmeier", "Clonk-Karl"},
	{"Lukas Werling", "Luchs"},
	{"Julius Michaelis", "JCaesar"},
	{"Peter Wortmann", "PeterW"},
},
scriptingAndContent =
{
	{"Maikel de Vries", "Maikel"},
	{"David Dormagen", "Zapper"},
	{"Mark Haßelbusch", "Marky"},
	{"Felix Wagner", "Clonkonaut"},
	{"Bernhard Bonigl", "Boni"},
},
administration =
{
	{"Tobias Zwick", "Newton"},
},
artAndContent =
{
	{"Charles Spurrill", "Ringwaul"},
	{"Richard Gerum", "Randrian"},
	{"Timo Stabbert", "Mimmo"},
	{"Matthias Rottländer", "Matthi"},
},
musicAndSound =
{
	{"David Oerther", "ala"},
	{"Martin Strohmeier", "K-Pone"},
};

static struct ContributorList : public PersonList
{
	static const std::vector<Entry> contributorsThisRelease, contributors, packageMaintainers;

	StdStrBuf ConcatNames(const std::vector<Entry>& names)
	{
		StdStrBuf result;
		for (auto& p : names)
		{
			if (result.getLength()) result.Append(", ");
			if (p.nick)
				result.AppendFormat("%s <c f7f76f>(%s)</c>", p.name, p.nick);
			else 
				result.Append(p.name);
		}
		return result;
	}

	void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font)
	{
		StdStrBuf text;
		text = "<c ff0000>Contributors for OpenClonk 8.0:</c> ";
		text.Append(ConcatNames(contributorsThisRelease));
		textbox->AddTextLine(text.getData(), &font, C4GUI_MessageFontClr, false, true);

		text = "<c ff0000>Previous contributors:</c> ";
		text.Append(ConcatNames(contributors));
		textbox->AddTextLine(text.getData(), &font, C4GUI_MessageFontClr, false, true);

		text = "<c ff0000>Also thanks to our Linux package maintainers</c> ";
		text.Append(ConcatNames(packageMaintainers));
		text.Append(", and more");
		textbox->AddTextLine(text.getData(), &font, C4GUI_MessageFontClr, false, true);

		text = "Finally, a big thanks to Matthes Bender and all those who contributed to previous Clonk titles for the passion they put into the game and for agreeing to make Clonk open source.";
		textbox->AddTextLine(text.getData(), &font, C4GUI_MessageFontClr, false, true);
	}
} contributors;

// Sorted by commit count this release, e.g.: git shortlog -s v7.0.. | sort -rn
const std::vector<ContributorList::Entry> ContributorList::contributorsThisRelease = {
	{"George Tokmaji", "Fulgen"},
	{"Linus Heckemann", "sphalerite"},
	{"Dominik Bayerl", "Kanibal"},
	{"Armin Schäfer", nullptr},
	{"Tushar Maheshwari", nullptr},
	{"jok", nullptr},
	{"Philip Kern", "pkern"},
	{"Matthias Mailänder", nullptr},
	{"marsmoon", nullptr},
};

// First real names sorted by last name (sort -k2), then nicks (sort)
const std::vector<ContributorList::Entry> ContributorList::contributors = {
	{"Martin Adam", "Win"},
	{"Tim Blume", nullptr},
	{"Merten Ehmig", "pluto"},
	{"Florian Graier", "Nachtfalter"},
	{"Sven-Hendrik Haase", nullptr},
	{"Carl-Philip Hänsch", "Carli"},
	{"Jan Heberer", nullptr},
	{"Benjamin Herr", "Loriel"},
	{"Philip Holzmann", "Batman"},
	{"Lauri Niskanen", "Ape"},
	{"Johannes Nixdorf", "mixi"},
	{"Misty de Meo", nullptr}, // note: three part name
	{"Fabian Pietsch", nullptr},
	{"Manuel Rieke", "MrBeast"},
	{"Felix Riese", "Fungiform"},
	{"Sebastian Rühl", nullptr},
	{"Oliver Schneider", "ker"},
	{"Lorenz Schwittmann", nullptr},
	{"Alexander Semeniuk", "AlteredARMOR"},
	{"Daniel Theuke", "ST-DDT"},
	{"Andriel", nullptr},
	{"Apfelclonk", nullptr},
	{"Asmageddon", nullptr},
	{"Checkmaty", nullptr},
	{"Clonkine", nullptr},
	{"dylanstrategie", nullptr},
	{"Faby", nullptr},
	{"grgecko", nullptr},
	{"Gurkenglas", nullptr},
	{"hasufell", nullptr},
	{"Koronis", nullptr},
	{"mizipzor", nullptr},
	{"Peewee", nullptr},
	{"Pyrit", nullptr},
	{"Russell", nullptr},
	{"Stan", nullptr},
	{"TomyLobo", nullptr},
};

// Sorted alphabetically: sort -k2
const std::vector<ContributorList::Entry> ContributorList::packageMaintainers = {
	{"Benedict Etzel", "B_E"}, // Arch
	{"Linus Heckemann", "sphalerite"}, // NixOS
	{"Philip Kern", "pkern"}, // Debian
	{"Matthias Mailänder", nullptr}, // OpenSUSE
	{"Julian Ospald", "hasufell"}, // Gentoo
	{"Kevin Zeng", nullptr}, // FreeBSD
};

// ------------------------------------------------
// --- C4StartupAboutDlg

C4StartupAboutDlg::C4StartupAboutDlg() : C4StartupDlg(LoadResStr("IDS_DLG_ABOUT"))
{
	// ctor
	UpdateSize();

	CStdFont &rUseFont = ::GraphicsResource.TextFont;
	C4Rect rcClient = GetContainedClientRect();

	// bottom line buttons and copyright messages
	C4GUI::ComponentAligner caMain(rcClient, 0,0, true);
	C4GUI::ComponentAligner caButtons(caMain.GetFromBottom(caMain.GetHeight()*1/8), 0,0, false);
	C4GUI::CallbackButton<C4StartupAboutDlg> *btn;
	int32_t iButtonWidth = caButtons.GetInnerWidth() / 4;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_BACK"), caButtons.GetGridCell(0,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnBackBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_BACKMAIN"));
#ifdef WITH_AUTOMATIC_UPDATE
	AddElement(btn = new C4GUI::CallbackButton<C4StartupAboutDlg>(LoadResStr("IDS_BTN_CHECKFORUPDATES"), caButtons.GetGridCell(2,3,0,1,iButtonWidth,C4GUI_ButtonHgt,true), &C4StartupAboutDlg::OnUpdateBtn));
	btn->SetToolTip(LoadResStr("IDS_DESC_CHECKONLINEFORNEWVERSIONS"));
#endif

	AddElement(new C4GUI::Label("'Clonk' is a registered trademark of Matthes Bender.",
		caButtons.GetFromBottom(rUseFont.GetLineHeight())));

	C4GUI::ComponentAligner caDevelopers(caMain.GetFromTop(caMain.GetHeight() * 1/2), 0,0, false);
	C4GUI::ComponentAligner caContributors(caMain.GetFromTop(caMain.GetHeight()), 0,0, false);
	DrawPersonList(C4StartupAboutEngineAndTools, engineAndTools, caDevelopers.GetFromLeft(caMain.GetWidth()*1/3));
	C4GUI::ComponentAligner caDevelopersCol2(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	DrawPersonList(C4StartupAboutScriptingAndContent, scriptingAndContent, caDevelopersCol2.GetFromTop(caDevelopers.GetHeight()*2/3));
	DrawPersonList(C4StartupAboutAdministration, administration, caDevelopersCol2.GetFromTop(caDevelopers.GetHeight()*1/3));
	C4GUI::ComponentAligner caDevelopersCol3(caDevelopers.GetFromLeft(caMain.GetWidth()*1/3), 0,0, false);
	DrawPersonList(C4StartupAboutArtAndContent, artAndContent, caDevelopersCol3.GetFromTop(caDevelopers.GetHeight()*2/3));
	DrawPersonList(C4StartupAboutMusicAndSound, musicAndSound, caDevelopersCol3.GetFromTop(caDevelopers.GetHeight()*1/3));

	DrawPersonList(C4StartupAboutContributors, contributors, caContributors.GetFromTop(caContributors.GetHeight()));

}

C4StartupAboutDlg::~C4StartupAboutDlg() = default;


void C4StartupAboutDlg::DrawPersonList(int title, PersonList& persons, C4Rect& rect)
{
	CStdFont &rUseFont = ::GraphicsResource.TextFont;
	auto image = C4Startup::Get()->Graphics.fctAboutTitles.GetPhase(0, title);
	int height = 2*rUseFont.GetFontHeight(), width = std::min(image.GetWidthByHeight(height), rect.Wdt);
	auto picture = new C4GUI::Picture(C4Rect(rect.x, rect.y, width, height), true);
	AddElement(picture);
	picture->SetFacet(image);
	rect.y += height; rect.Hgt -= height;
	auto textbox = new C4GUI::TextWindow(rect, 0, 0, 0, 100, 4096, "", true, nullptr, 0, true);
	AddElement(textbox);
	textbox->SetDecoration(false, false, nullptr, true);
	persons.WriteTo(textbox, rUseFont);
	textbox->UpdateHeight();
}

void C4StartupAboutDlg::DoBack()
{
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Main);
}

void C4StartupAboutDlg::DrawElement(C4TargetFacet &cgo)
{
}

#ifdef WITH_AUTOMATIC_UPDATE
void C4StartupAboutDlg::OnUpdateBtn(C4GUI::Control *btn)
{
	C4UpdateDlg::CheckForUpdates(GetScreen());
}
#endif

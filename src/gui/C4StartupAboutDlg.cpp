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

#include <fstream>

struct PersonList
{
	struct Entry
	{
		const char *name, *nick;
	};
	virtual void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font) = 0;
	virtual std::string ToString() = 0;
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

	std::string ToString()
	{
		std::stringstream out;
		for (auto& p : developers)
		{
			out << p.name << " (" << p.nick << ")\n";
		}
		return out.str();
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
	{"Martin Plicht", "Mortimer"},
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

	std::string ConcatNames(const std::vector<Entry>& names, bool with_color)
	{
		const char *opening_tag = with_color ? "<c f7f76f>" : "";
		const char *closing_tag = with_color ? "</c>" : "";
		std::stringstream result;
		bool first = true;
		for (auto& p : names)
		{
			if (!first) result << ", ";
			first = false;
			if (p.nick)
				result << p.name << " " << opening_tag << "(" << p.nick << ")" << closing_tag;
			else 
				result << p.name;
		}
		return result.str();
	}

	template<typename Func>
	std::string WriteLines(Func f, bool with_color)
	{
		const char *opening_tag = with_color ? "<c ff0000>" : "";
		const char *closing_tag = with_color ? "</c>" : "";
		std::stringstream text;

		text << opening_tag << "Contributors for OpenClonk 8.0:" << closing_tag << " ";
		text << ConcatNames(contributorsThisRelease, with_color);
		f(text);

		text << opening_tag << "Previous contributors:" << closing_tag << " ";
		text << ConcatNames(contributors, with_color);
		f(text);

		text << opening_tag << "Also thanks to our Linux package maintainers" << closing_tag << " ";
		text << ConcatNames(packageMaintainers, with_color);
		text << ", and more";
		f(text);

		text << "Finally, a big thanks to Matthes Bender and all those who contributed to previous Clonk titles for the passion they put into the game and for agreeing to make Clonk open source.";
		f(text);

		return text.str();
	}

	void WriteTo(C4GUI::TextWindow *textbox, CStdFont &font)
	{
		WriteLines([&](std::stringstream& text)
		{
			textbox->AddTextLine(text.str().c_str(), &font, C4GUI_MessageFontClr, false, true);
			text.str("");
		}, true);
	}

	std::string ToString()
	{
		return WriteLines([&](std::stringstream& text)
		{
			text << "\n";
		}, false);
	}
} contributors;

// Sorted by commit count this release, e.g.: git shortlog -s v7.0.. | sort -rn
// Stuff from the milestone project sorted in-between as those commits usually end up squashed.
const std::vector<ContributorList::Entry> ContributorList::contributorsThisRelease = {
	{"George Tokmaji", "Fulgen"},      // 75 commits
	{"Martin Adam", "Win"},            // lava core animal
	{"Merten Ehmig", "pluto"},         // helmet
	{"Florian Graier", "Nachtfalter"}, // tree and guidepost
	{"Philip Holzmann", "Foaly"},      // "another decorative deciduous tree"
	{"Dominik Bayerl", "Kanibal"},     // 7 commits
	{"Linus Heckemann", "sphalerite"}, // 6 commits
	{"Pyrit", nullptr},                // texture to sproutberry bush
	{"Armin Schäfer", nullptr},        // 5 commits
	{"Tushar Maheshwari", nullptr},    // 4 commits
	{"jok", nullptr},                  // 3 commits
	{"Tarte", nullptr},                // fixed normal map for brick textures
	{"Philip Kern", "pkern"},          // 2 commits
	{"Arne Schauf", "NativeException"},// 1 commit
	{"Matthias Mailänder", nullptr},   // 1 commit
	{"marsmoon", nullptr},             // 1 commit
};

// First real names sorted by last name (sort -k2), then nicks (sort)
const std::vector<ContributorList::Entry> ContributorList::contributors = {
	{"Tim Blume", nullptr},
	{"Sven-Hendrik Haase", nullptr},
	{"Carl-Philip Hänsch", "Carli"},
	{"Jan Heberer", nullptr},
	{"Benjamin Herr", "Loriel"},
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
	{"Kevin Zheng", nullptr}, // FreeBSD
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

	keySaveCredits = std::make_unique<C4KeyBinding>(C4KeyCodeEx(K_S, KEYS_Control), "StartupAboutSaveCredits", KEYSCOPE_Gui,
			new C4GUI::DlgKeyCB<C4StartupAboutDlg>(*this, &C4StartupAboutDlg::SaveCredits), C4CustomKey::PRIO_CtrlOverride);
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

bool C4StartupAboutDlg::SaveCredits()
{
	std::ofstream credits("Credits.txt", std::ios::out | std::ios::trunc);
	if (!credits)
	{
		GetScreen()->ShowMessageModal("Couldn't open Credits.txt", "Credits", C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error, nullptr);
		return true;
	}

	credits << "<Engine and Tools>\n";
	credits << engineAndTools.ToString();
	
	credits << "\n<Scripting and Content>\n";
	credits << scriptingAndContent.ToString();

	credits << "\n<Art and Content>\n";
	credits << artAndContent.ToString();

	credits << "\n<Music and Sound>\n";
	credits << musicAndSound.ToString();

	credits << "\n<Administration>\n";
	credits << administration.ToString();

	credits << "\n<Special Thanks to Contributors>\n";
	credits << contributors.ToString();

	GetScreen()->ShowMessageModal("Saved to Credits.txt", "Credits", C4GUI::MessageDialog::btnOK, C4GUI::Ico_Notify, nullptr);
	return true;
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

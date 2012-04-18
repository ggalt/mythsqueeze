/***************************************************************************
 *   Copyright (C) 2010 by George Galt                                     *
 *   ggalt at users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *
 *  Many thanks to Richard Titmuss who developed the SlimProtoLib library  *
 *  SoftSqueeze and squeezeslave, from which much of the code here is      *
 *  stolen.  For more information on SoftSqueeze and squeezeslave, see     *
 *  Richard's site at: http://sourceforge.net/projects/softsqueeze         *
 *                                                                         *
 ***************************************************************************/

// C++ headers
#include <iostream>

// QT headers

// MythTV headers
#include <mythtv/mythcontext.h>
#include <mythtv/mythversion.h>
#include <mythtv/mythplugin.h>
#include <mythtv/mythpluginapi.h>

// MythUI headers
#include <mythtv/libmythui/myththemedmenu.h>
#include <mythtv/libmythui/mythmainwindow.h>
#include <mythtv/libmythui/mythuihelper.h>
#include <mythtv/libmythui/mythprogressdialog.h>

// MythSqueezeBox2 headers
#include "mythsqueeze.h".h"
#include "squeezedefines.h"
#include "squeezesettings.h"


using namespace std;

void setupKeys(void)
{
    REG_KEY( "MythSB",	"DOWNARROW",	"Down",	"Down");
    REG_KEY( "MythSB",	"UPARROW",	"Up",	"Up");
    REG_KEY( "MythSB",	"LEFTARROW",	"Left",	"Left");
    REG_KEY( "MythSB",	"RIGHTARROW",	"Right",	"Right");
    //    REG_KEY( "MythSB",	"SELECT",	"",	"Return");
    REG_KEY( "MythSB",	"ADD",	"",	"A");
    REG_KEY( "MythSB",	"REWIND",	"",	"PgUp");
    REG_KEY( "MythSB",	"FASTFORWARD",	"",	"PgDown");
    REG_KEY( "MythSB",	"STOP",	"",	"End");
    //    REG_KEY( "MythSB",	"PLAY",	"",	"Home");
    REG_KEY( "MythSB",	"PLAY",	"",	"Return");
    REG_KEY( "MythSB",	"PAUSE",	"",	"P");
    REG_KEY( "MythSB",	"MUTE",	"",	"F9");
    REG_KEY( "MythSB",	"VOLUP",	"",	"],F11");
    REG_KEY( "MythSB",	"VOLDOWN",	"",	"[,F10");
    REG_KEY( "MythSB",	"BTN0",	"0",	"0");
    REG_KEY( "MythSB",	"BTN1",	"1",	"1");
    REG_KEY( "MythSB",	"BTN2",	"2",	"2");
    REG_KEY( "MythSB",	"BTN3",	"3",	"3");
    REG_KEY( "MythSB",	"BTN4",	"4",	"4");
    REG_KEY( "MythSB",	"BTN5",	"5",	"5");
    REG_KEY( "MythSB",	"BTN6",	"6",	"6");
    REG_KEY( "MythSB",	"BTN7",	"7",	"7");
    REG_KEY( "MythSB",	"BTN8",	"8",	"8");
    REG_KEY( "MythSB",	"BTN9",	"9",	"9");
    REG_KEY( "MythSB",	"SHUFFLE",	"",	"X");
    REG_KEY( "MythSB",	"REPEAT",	"",	"R");
    REG_KEY( "MythSB",	"SLEEP",	"",	"Q");
    REG_KEY( "MythSB",	"SIZE",	"",	"T");
    REG_KEY( "MythSB",	"SEARCH",	"",	"S");
    REG_KEY( "MythSB",	"BRIGHT",	"",	"B");
    REG_KEY( "MythSB",	"NOWPLAYING",	"",	"Space");
    REG_KEY( "MythSB",  "MENU", "", "M" );
    //REG_KEY( "MythSB",	"POWER",	"",	"ESC");
}

int mythplugin_run(void)
{
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();

    MythSqueezeBox *mythsqueezebox = new MythSqueezeBox(mainStack, "MythSqueezeBox2");

    if (mythsqueezebox->Create()) {
        mainStack->AddScreen(mythsqueezebox);
        mythsqueezebox->InitPlayer();
    }

    return 0;
}

int mythplugin_config(void)
{
    SqueezeSettings settings;
    settings.exec();
    return 0;
}

int mythplugin_init(const char *libversion)
{
    if (!gContext->TestPopupVersion("MythSqueezeBox2", libversion, MYTH_BINARY_VERSION))
        return -1;

    SqueezeSettings settings;
    settings.Load();
    settings.Save();
    setupKeys();
    return 0;
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Tom Scheper - scheper@gmail.com                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
 ***************************************************************************/


#include "dlgSourceEditorArea.h"
#include "edbee/edbee.h"
#include "edbee/texteditorwidget.h"
#include "edbee/models/textdocument.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"
#include "edbee/models/textgrammar.h"
#include "edbee/models/texteditorconfig.h"

#include <QDir> // For the homePath()

dlgSourceEditorArea::dlgSourceEditorArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Setting up the edbee widget

    edbee::Edbee* ei = edbee::Edbee::instance();

    ei->autoInit();
    ei->autoShutDownOnAppExit();

    //**********
    //
    // As it is, the editor only needs one syntax file (Lua). We could offer a choice of themes,
    // but it's not really necessary just yet. People can change the theme if they really want to
    // by overwriting the Fluidvisionlet.tmTheme file with another valid textmate theme.

    // So instead of having separate theme/syntax file directories, I'm lumping them into one directory:
    // QDir::homePath() + "/.config/mudlet/edbee/"

    edbee::TextGrammarManager* grammarManager = ei->grammarManager();
    edbee::TextGrammar* grammar = grammarManager->readGrammarFile( QDir::homePath() + "/.config/mudlet/edbee/Lua.tmLanguage" );
    edbee::TextThemeManager* themeManager = ei->themeManager();

    themeManager->readThemeFile( QDir::homePath() + "/.config/mudlet/edbee/Fluidvisionlet.tmTheme" ); // Once it's read it becomes available
    edbeeEditorWidget->config()->setThemeName("Fluidvisionlet");

    edbeeEditorWidget->config()->setShowCaretOffset( true);
    edbeeEditorWidget->config()->setCaretWidth( 1);
    //edbeeEditorWidget->config()->setExtraLineSpacing(40);
    edbeeEditorWidget->config()->setSmartTab( true); // I'm not fully sure what this does, but it says "Smart" so it must be good
    edbeeEditorWidget->config()->setCaretBlinkRate( 200);

    edbeeEditorWidget->textDocument()->setLanguageGrammar( grammar );
    edbeeEditorWidget->config()->setIndentSize( 2); // 2 spaces is the Lua default
    edbeeEditorWidget->config()->setFont(QFont( "Courier New", 28)); // This is just a fallback, the actual font is set later
}

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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

#include <QDir>


//#include "THighlighter.h"


dlgSourceEditorArea::dlgSourceEditorArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    //highlighter = new THighlighter(editor->document());
    //editor->setTabStopWidth(25);

    // Setting up the edbee widget

    edbee::Edbee* ei = edbee::Edbee::instance();

    ei->autoInit();
    ei->autoShutDownOnAppExit();

    ei->setGrammarPath(  "G:\\Temp\\edbee\\syntaxfiles\\" );
    ei->setThemePath( "G:\\Temp\\edbee\\themes\\" );

    edbee::TextGrammarManager* grammarManager = ei->grammarManager();
    edbee::TextGrammar* grammar = grammarManager->readGrammarFile( QDir::homePath() + "/.config/mudlet/edbee/Lua.tmLanguage" );
    edbee::TextThemeManager* themeManager = ei->themeManager();

    edbee::TextTheme* theme = themeManager->readThemeFile( QDir::homePath() + "/.config/mudlet/edbee/Fluidvisionlet.tmTheme" );
    edbeeEditorWidget->config()->setThemeName("Fluidvisionlet");

    edbeeEditorWidget->config()->setShowCaretOffset( true);
    edbeeEditorWidget->config()->setCaretWidth( 1);
    //edbeeEditorWidget->config()->setExtraLineSpacing(40);
    edbeeEditorWidget->config()->setSmartTab( true);
    edbeeEditorWidget->config()->setCaretBlinkRate( 200);

    edbeeEditorWidget->textDocument()->setLanguageGrammar( grammar );
    edbeeEditorWidget->config()->setIndentSize( 2);
    edbeeEditorWidget->config()->setFont(QFont( "Courier New", 28));

    //edbeeEditorWidget->config()->setThemeName( QString("Monokai"));

}

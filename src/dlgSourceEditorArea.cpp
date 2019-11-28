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
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/textgrammar.h"
#include "edbee/views/texteditorscrollarea.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/texttheme.h"

dlgSourceEditorArea::dlgSourceEditorArea(QWidget* pF) : QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    // Configuring the editor widget with defaults

    edbee::TextEditorConfig* config = edbeeEditorWidget->config();

    config->beginChanges();

    config->setSmartTab(true); // enable the automatic addition of indents when inserting a newline
    config->setUseTabChar(false); // when you press Enter for a newline, pad with spaces and not tabs
    config->setCaretBlinkRate(200);

    config->setIndentSize(2); // 2 spaces is the Lua default
    config->setCaretWidth(1);

    config->endChanges();

    edbeeEditorWidget->textDocument()->setLanguageGrammar(edbee::Edbee::instance()->grammarManager()->detectGrammarWithFilename(QStringLiteral("Buck.lua")));

    // disable shadows as their purpose (notify there is more text) is performed by scrollbars already
    edbeeEditorWidget->textScrollArea()->enableShadowWidget(false);
}

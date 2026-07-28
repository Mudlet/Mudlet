#ifndef MUDLET_TCONSOLEMODEL_H
#define MUDLET_TCONSOLEMODEL_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TBuffer.h"

#include <QColor>
#include <QPoint>
#include <QString>

class Host;
class TConsole;

// libmudlet Wave 3, step 2 (de-risk spike): the per-console data model.
//
// This is the slice of former TConsole-base state that the telnet -> trigger
// pipeline drives without needing a widget: the text buffer, the cursor/prompt
// state runTriggers() updates on every line, and the fg/bg colours that
// colour-triggers match against. Extracting it is the make-or-break step of the
// libmudlet console-model split - once this lives outside the widget, Host can
// run the per-line pipeline (Host::runTriggers) against a model with no view.
//
// The main console's model is co-owned by Host (see Host::sharedMainConsoleModel)
// so the pipeline outlives the view; sub-consoles own their own model. Every
// TConsole reaches its model through TConsole::model(); the widget keeps the
// former members (buffer, mFgColor, ...) as references that alias the model, so
// the existing accesses across the codebase are preserved unchanged.
struct TConsoleModel
{
    explicit TConsoleModel(Host* pHost, TConsole* pConsole = nullptr)
    : buffer(pHost, pConsole)
    {
    }

    TBuffer buffer;
    QColor mBgColor = QColorConstants::Black;
    QColor mFgColor = QColorConstants::LightGray;
    QString mCurrentLine;
    int mEngineCursor = -1;
    QPoint mUserCursor;
    bool mIsPromptLine = false;
};

#endif // MUDLET_TCONSOLEMODEL_H

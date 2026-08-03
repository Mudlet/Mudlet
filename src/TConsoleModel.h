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

// The per-console data model: the slice of former TConsole state that the
// telnet -> trigger pipeline drives without needing a widget. That is the text
// buffer, the cursor/prompt state Host::runTriggers() updates on every line,
// and the fg/bg colours colour-triggers match against. Splitting it out of the
// widget is what lets the pipeline run with no view at all, which is the point
// of the Widgets-free core (#8681).
//
// The main console's model is co-owned by Host (see Host::sharedMainConsoleModel)
// so the pipeline outlives the view; sub-consoles own their own model. Every
// TConsole reaches its model through TConsole::model(); the widget keeps the
// former members (buffer, mFgColor, ...) as references that alias the model, so
// the existing accesses across the codebase are preserved unchanged.
struct TConsoleModel
{
    explicit TConsoleModel(Host* pHost)
    : buffer(pHost)
    {
    }

    // A copy would duplicate the whole scrollback and leave a second model
    // claiming the view the original is bound to. Deleting the copy operations
    // suppresses the implicit move ones too.
    TConsoleModel(const TConsoleModel&) = delete;
    TConsoleModel& operator=(const TConsoleModel&) = delete;

    // No 'm' prefix on purpose: TConsole::buffer aliases this one by reference and has to keep its name for the rest of the codebase, so the two match.
    TBuffer buffer;
    // Only a cache today - the view fills these in through TConsole::changeColors(); refreshing them from the Host after the profile loads moves core-side with the colour sub-PR.
    QColor mBgColor = QColorConstants::Black;
    QColor mFgColor = QColorConstants::LightGray;
    QString mCurrentLine;
    int mEngineCursor = -1;
    QPoint mUserCursor;
    bool mIsPromptLine = false;
};

#endif // MUDLET_TCONSOLEMODEL_H

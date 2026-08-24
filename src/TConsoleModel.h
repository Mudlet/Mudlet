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
#include "THyperlinkCompactManager.h"
#include "THyperlinkSelectionManager.h"
#include "THyperlinkVisibilityManager.h"

#include <QColor>
#include <QFile>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QTextStream>

class Host;

// The per-console data model: the slice of former TConsole state that the
// telnet -> trigger pipeline drives without needing a widget. That is the text
// buffer, the cursor/prompt state Host::runTriggers() updates on every line,
// the fg/bg colours colour-triggers match against, the log lifecycle the buffer
// writes through, and the OSC 8 hyperlink state the buffer translation
// registers as it goes. Splitting it out of the widget is what lets the
// pipeline run with no view at all, which is the point of the Widgets-free
// core (#8681).
//
// The main console's model is co-owned by Host (see Host::sharedMainConsoleModel)
// so the pipeline outlives the view; sub-consoles own their own model. Every
// TConsole reaches its model through TConsole::model(); the widget keeps the
// former members (buffer, mFgColor, ...) as references that alias the model, so
// the existing accesses across the codebase are preserved unchanged.
struct TConsoleModel
{
    // Defined out of line: mpHost is a QPointer, which needs Host complete, and
    // this header is included far too widely to drag Host.h along with it.
    explicit TConsoleModel(Host* pHost);

    // A copy would duplicate the whole scrollback and leave a second model
    // claiming the view the original is bound to. Deleting the copy operations
    // suppresses the implicit move ones too.
    TConsoleModel(const TConsoleModel&) = delete;
    TConsoleModel& operator=(const TConsoleModel&) = delete;

    // Lives here rather than on the main-console widget because a profile with
    // no view still has to be able to *start* a log, not just write into a
    // stream something else opened. The two parts of it that genuinely need a
    // view - announcing the change on the console and re-labelling the log
    // button - are raised as Host signals for the frontend to act on.
    // Everything it touches (the autolog sentinel, the Host's log directory and
    // filename format) is profile-wide, so it only acts on the Host's own main
    // model and returns for any other.
    void toggleLogging(bool isMessageEnabled);

    // No 'm' prefix on purpose: TConsole::buffer aliases this one by reference and has to keep its name for the rest of the codebase, so the two match.
    TBuffer buffer;
    // A QPointer because Host and view are torn down in either order: quitting
    // destroys every Host before the consoles' deferred deletes run, while
    // closing one profile deletes its console first. The view co-owns the
    // model, so it can be left holding one whose Host has gone.
    QPointer<Host> mpHost;
    // On the main console model, the profile's colours, kept there by
    // Host::refreshMainConsoleColors(); a sub-console's are its own, and no
    // trigger reads them.
    QColor mBgColor = QColorConstants::Black;
    QColor mFgColor = QColorConstants::LightGray;
    QString mCurrentLine;
    int mEngineCursor = -1;
    QPoint mUserCursor;
    bool mIsPromptLine = false;

    // The OSC 8 hyperlink managers. Concealing and revealing rewrite this
    // model's buffer, so they run with or without a view; repainting afterwards
    // is the view's job. Registering a link is not view-free yet - TBuffer
    // reaches the manager through its own console back-pointer.
    //
    // Declared after the buffer so that the manager which writes into it is
    // destroyed first - keep it that way if a field is ever added between them.
    THyperlinkCompactManager mHyperlinkCompactManager;
    THyperlinkSelectionManager mHyperlinkSelectionManager;
    THyperlinkVisibilityManager mHyperlinkVisibilityManager;

    // The log destination. TBuffer writes into mLogStream directly, and
    // TMainConsole keeps references aliasing all four.
    // mLogStream holds a bare pointer to mLogFile, so the declaration order
    // here is load-bearing: the stream has to be destroyed - and flush - before
    // the file it is writing into.
    QFile mLogFile;
    QString mLogFileName;
    QTextStream mLogStream;
    bool mLogToLogFile = false;
};

#endif // MUDLET_TCONSOLEMODEL_H

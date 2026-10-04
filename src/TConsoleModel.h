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
#include "TConsoleModelNotifier.h"
#include "THyperlinkCompactManager.h"
#include "THyperlinkSelectionManager.h"
#include "THyperlinkVisibilityManager.h"

#include <QColor>
#include <QFile>
#include <QPair>
#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <tuple>

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
    void reportFailedLogStart(const QString& path, const QString& reason);
    // The cursor scripts read and write through: moveCursor() leaves it where it was for a line
    // outside the buffer, and moveCursorEnd() puts it on the last character of the last line.
    bool moveCursor(int x, int y);
    void moveCursorEnd();
    void deleteLineAtCursor();

    // Half-open: lines(n, n) is empty. Not const because TBuffer::line() returns a mutable QString&.
    QStringList lines(int from, int to);

    // Selecting a run of the cursor's line, painting it and restoring the format, for colorizer triggers
    // and scripts. Needs no view; the painting calls return whether the buffer changed, the view's cue to
    // repaint the selected lines. They write mFormatCurrent and the selected run, not mFgColor/mBgColor
    // below (the profile's colours).
    void deselect();
    bool selectSection(int from, int to);
    void selectCurrentLine();
    // Selects the numOfMatch-th match of text on the cursor's line and returns where it starts, or
    // deselects and returns -1 when there is none.
    int selectString(const QString& text, int numOfMatch);
    // Whether the selection is still valid, its text (else why not), its start and its length. The text
    // is read from the cursor's line, not the selection's, at the selection's columns.
    std::tuple<bool, QString, int, int> selection();
    // The selected character's format, or the cursor's with no selection; first is 2 when there is none.
    QPair<quint8, TChar> textAttributes() const;
    // The character the selection starts on, or nullptr when that is off the buffer. With no selection
    // that is the buffer's first character: unlike textAttributes(), this does not fall back to the cursor.
    const TChar* selectionStartChar() const;
    void resetFormat();
    bool setSelectionFgColor(const QColor& newColor);
    bool setSelectionBgColor(const QColor& newColor);
    bool setSelectionDisplayAttributes(TChar::AttributeFlags attributes, bool enabled);
    // Makes the selected run a link, taking over the commands' Lua registry references.
    bool setLink(const QStringList& commands, const QStringList& hints, const QVector<int>& luaReferences);

    // Client output, appended at the end of the buffer and copied to --mirror. The view showing the
    // model brings the new lines into view (TConsole::showNewLines()).
    void print(const QString& msg);
    // timeStampOverride keeps the arrival time for held-back content being replayed.
    void print(const QString& msg, const QColor& fgColor, const QColor& bgColor, const QString& timeStampOverride = QString());
    void printSystemMessage(const QString& msg);

    // What printCommand() did to the buffer, which decides what the view repaints.
    struct CommandEcho
    {
        enum class Kind {
            // Nothing written, or written while triggers run, which is left to the repaint of the
            // text they run on.
            None,
            NewLines,
            // Appended to the prompt it answers, which now wraps over rows [firstLine, lastLine).
            PromptLine
        };
        Kind kind = Kind::None;
        int firstLine = 0;
        int lastLine = 0;
    };
    // Echoes a command the player sent. msg is changed to what was written, line feeds included,
    // which Host::send() then splits and sends.
    CommandEcho printCommand(QString& msg);

    // For --mirror: copies text to stdout prefixed with the profile and console names. Text may be a
    // line fragment, so what follows the last line feed is held until one arrives.
    void mirrorToStdOut(const QString& text);
    // For a complete line: TBuffer::commitLineData() passes it as sent, before triggers can gag or rewrite it.
    void mirrorLineToStdOut(const QString& line);
    // What a write at mUserCursor leaves the view to do: show the lines it
    // appended, or repaint firstLine..lastLine, which it changed in place.
    struct WriteResult
    {
        bool appended = false;
        int firstLine = -1;
        int lastLine = -1;
    };
    // echoLink() appends; the inserts go in at mUserCursor, text in
    // mFormatCurrent. While triggers run over this console's line, a write
    // there shifts their capture groups to match.
    void echoLink(const QString& text, QStringList& commands, QStringList& hints, bool useCurrentFormat, const QVector<int>& luaReferences);
    WriteResult insertLink(const QString& text, QStringList& commands, QStringList& hints, bool useCurrentFormat, const QVector<int>& luaReferences);
    WriteResult insertText(const QString& text);
    // A script's echo(), marked as echoed text, with its carriage returns dropped from text. While
    // triggers run over this console's line it goes onto that line, which is shown once they are
    // done; otherwise it is appended, and this answers true for the view to show the new lines.
    bool echo(QString& text);
    // Puts text in place of the selected run.
    void replace(const QString& text);
    // The WCAG contrast ratio, which link colours here and TConsole's scroll bar are both chosen by.
    static double contrastRatio(const QColor& first, const QColor& second);

    // No 'm' prefix on purpose: TConsole::buffer aliases this one by reference and has to keep its name for the rest of the codebase, so the two match.
    TBuffer buffer;
    // A QPointer because Host and view are torn down in either order: quitting
    // destroys every Host before the consoles' deferred deletes run, while
    // closing one profile deletes its console first. The view co-owns the
    // model, so it can be left holding one whose Host has gone.
    QPointer<Host> mpHost;
    // On the main console model, the profile's colours, kept there by
    // Host::refreshMainConsoleColors(); colour triggers set to "default" match
    // against these. A sub-console model's hold that one window's own colours
    // instead, written by TConsole::setConsoleBgColor() and read back only by
    // that console.
    QColor mBgColor = QColorConstants::Black;
    QColor mFgColor = QColorConstants::LightGray;
    QString mCurrentLine;
    int mEngineCursor = -1;
    QPoint mUserCursor;
    // The run selectSection() marks. No 'm' prefix, like buffer above: the widget's members alias these.
    QPoint P_begin;
    QPoint P_end;
    // The format text is written into the buffer with.
    TChar mFormatCurrent;
    bool mIsPromptLine = false;
    // Set while triggers run on incoming text, so that script writes treat the
    // matched line as still open - see the branches on it here, in TConsole and
    // TBuffer::addLink(). Only ever raised on the main console's model.
    bool mTriggerEngineMode = false;
    // The colours printCommand() and printSystemMessage() write in. On the main console's model
    // the command pair is the profile's, which Host keeps it in step with.
    QColor mCommandFgColor = QColor(213, 195, 0);
    QColor mCommandBgColor = QColorConstants::Black;
    QColor mSystemMessageFgColor = QColorConstants::Red;
    // Transparent so a system message blends into the console's real background
    // instead of an opaque bar; TTextEdit's selection swap and TBuffer's HTML
    // export both resolve alpha-0 against getConsoleBgColor() so the text stays
    // visible when selected and the same colour is kept in copied/exported HTML.
    QColor mSystemMessageBgColor = QColorConstants::Transparent;
    // Last pressed toolbar button's state for getButtonState(): 1 = up, 2 = down (0 invalid); a plain button
    // resets it to 1.
    int mButtonState = 1;
    // Whether the view draws each line's timestamp in a gutter to its left. NAWS
    // leaves the gutter out of the width it reports, so this has to be readable
    // with no view.
    bool mShowTimeStamps = false;

    // The width and indents wrapLine() rewraps a line to. The buffer keeps its
    // own copy, which wraps text as it arrives, so always set them through
    // these setters to keep the two the same.
    void setWrapAt(int pos)
    {
        mWrapAt = pos;
        buffer.setWrapAt(pos);
    }
    void setIndentCount(int count)
    {
        mIndentCount = count;
        buffer.setWrapIndent(count);
    }
    void setHangingIndentCount(int count)
    {
        mHangingIndentCount = count;
        buffer.setWrapHangingIndent(count);
    }
    int mWrapAt = 100;
    int mIndentCount = 0;
    int mHangingIndentCount = 0;
    // The upper pane's columns and rows, kept current by TTextEdit::reportGridSize().
    QSize mGridSize;
    // The upper pane's TTextEdit::mCursorY, which it copies out of the buffer as it repaints.
    int mUpperPaneCursorY = 0;
    bool mScrollingEnabled = true;
    QColor mBorderColor = QColorConstants::Black;
    void wrapLine(int line) { buffer.wrapLine(line, mWrapAt, mIndentCount, mHangingIndentCount); }

    // The name scripts know this console by. Only the main console, user
    // windows, miniconsoles and buffers can be addressed by scripts, so only
    // they tell scripts when their line indexes shift.
    QString mConsoleName;
    // Names the console's --mirror records, with mConsoleName.
    QString mProfileName;
    // --mirror text not yet ended by a line feed.
    QString mMirrorPendingLine;
    bool mScriptAddressable = false;
    // The line on which the current search result has been found, or the next
    // one is to start (currently only for the main console). An index into the
    // buffer, so it moves with the buffer's lines.
    int mCurrentSearchResult = 0;

    TConsoleModelNotifier mNotifier;

    // The OSC 8 hyperlink managers. Registering, concealing and revealing a
    // link are all model work, so they run with or without a view; repainting
    // afterwards is the view's job.
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
    // Path and reason of a failed start, for a caller with no console to read the report off.
    QString mLogStartFailure;
};

#endif // MUDLET_TCONSOLEMODEL_H

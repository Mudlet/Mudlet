#ifndef MUDLET_TCONSOLE_H
#define MUDLET_TCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2020 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2020 by Matthias Urlichs matthias@urlichs.de            *
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


#include "TTextCodec.h"

#include "pre_guard.h"
#include <QDataStream>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QFile>
#include <QLabel>
#include <QPointer>
#include <QTextStream>
#include <QWidget>
#include "post_guard.h"

#include <hunspell/hunspell.h>

#include <list>
#include <map>

class QCloseEvent;
class QLineEdit;
class QScrollBar;
class QToolButton;

class dlgMapper;
class Host;
class TTextEdit;
class TCommandLine;
class TDockWidget;
class TLabel;
class TSplitter;
class dlgNotepad;


class TConsole : public QWidget
{
    Q_OBJECT

public:
    enum ConsoleTypeFlag {
        UnknownType = 0x0, // Should not be encountered but left as a trap value
        CentralDebugConsole = 0x1, // One of these for whole application
        ErrorConsole = 0x2, // The bottom right corner of the Editor, one per profile
        MainConsole = 0x4, // One per profile
        SubConsole = 0x8, // Overlaid on top of MainConsole instance, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
        UserWindow = 0x10, // Floatable/Dockable console, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
        Buffer = 0x20 // Non-visible store for data that can be copied to/from other per profile TConsoles, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
    };

    Q_DECLARE_FLAGS(ConsoleType, ConsoleTypeFlag)

    Q_DISABLE_COPY(TConsole)
    explicit TConsole(Host*, ConsoleType type = UnknownType, QWidget* parent = nullptr);
    ~TConsole();

    void reset();
    void resizeConsole();
    Host* getHost();
    void replace(const QString&);
    void insertHTML(const QString&);
    void insertText(const QString&);
    void insertText(const QString&, QPoint);
    void insertLink(const QString&, QStringList&, QStringList&, QPoint, bool customFormat = false, QVector<int> luaReference = QVector<int>());
    void insertLink(const QString&, QStringList&, QStringList&, bool customFormat = false, QVector<int> luaReference = QVector<int>());
    void echoLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat = false, QVector<int> luaReference = QVector<int>());
    void copy();
    void cut();
    void paste();
    void appendBuffer();
    void appendBuffer(const TBuffer&);
    int getButtonState();
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void pasteWindow(TBuffer);
    QStringList getLines(int from, int to);
    int getLineNumber();
    int getLineCount();
    bool deleteLine(int);
    void clearSelection() const;

    int getColumnNumber();

    void setWrapAt(int pos)
    {
        mWrapAt = pos;
        buffer.setWrapAt(pos);
    }
    int getWrapAt();

    void setIndentCount(int count)
    {
        mIndentCount = count;
        buffer.setWrapIndent(count);
    }

    TLinkStore &getLinkStore() { return buffer.mLinkStore; }
    void echo(const QString&);
    bool moveCursor(int x, int y);
    int select(const QString&, int numOfMatch = 1);
    std::tuple<bool, QString, int, int> getSelection();
    void deselect();
    bool selectSection(int, int);
    void skipLine();
    void setFgColor(int, int, int);
    void setFgColor(const QColor&);
    void setBgColor(int, int, int, int);
    void setBgColor(const QColor&);
    void setScrollBarVisible(bool);
    void setHorizontalScrollBar(bool);
    void setCmdVisible(bool);
    void changeColors();
    void scrollDown(int lines);
    void scrollUp(int lines);
    void print(const QString&, QColor fgColor, QColor bgColor);
    void print(const QString& msg);
    void print(const char*);
    void printSystemMessage(const QString& msg);
    void printCommand(QString&);
    bool hasSelection();
    void moveCursorEnd();
    int getLastLineNumber();
    void refresh();
    void refreshView() const;
    void raiseMudletMousePressOrReleaseEvent(QMouseEvent*, const bool);
    bool setFontSize(int);
    bool setFont(const QString& font);
    bool setConsoleBackgroundImage(const QString&, int);
    bool resetConsoleBackgroundImage();
    void setLink(const QStringList& linkFunction, const QStringList& linkHint, const QVector<int> linkReference = QVector<int>());
    // Cannot be called setAttributes as that would mask an inherited method
    void setDisplayAttributes(const TChar::AttributeFlags, const bool);
    void showStatistics();
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void setConsoleBgColor(int, int, int, int);
// Not used:    void setConsoleFgColor(int, int, int);
    std::list<int> getFgColor();
    std::list<int> getBgColor();
    void luaWrapLine(int line);
    QString getCurrentLine();
    void selectCurrentLine();

    // Returns the size of the main buffer area (excluding the command line and toolbars).
    QSize getMainWindowSize() const;

    ConsoleType getType() const { return mType; }
    virtual void setProfileName(const QString&);
    // In the next pair of functions the first element in the return is an
    // error code:
    // 0 = Okay
    // 1 = Window not found
    // 2 = Selection not valid
    QPair<quint8, TChar> getTextAttributes() const;


    QPointer<Host> mpHost;
    // Only assigned a value for user windows:
    QPointer<TDockWidget> mpDockWidget;
    // Only on a MainConsole type instance:
    QPointer<TCommandLine> mpCommandLine;

    TBuffer buffer;
    static const QString cmLuaLineVariable;
    TTextEdit* mUpperPane;
    TTextEdit* mLowerPane;

    QToolButton* emergencyStop;
    QWidget* layer;
    QWidget* layerCommandLine;
    QHBoxLayout* layoutLayer2;
    QWidget* layerEdit;
    QColor mBgColor;
    int mButtonState;
    QColor mCommandBgColor;
    QColor mCommandFgColor;

    QString mConsoleName;
    QString mCurrentLine;
    QString mDisplayFontName;
    int mDisplayFontSize;
    QFont mDisplayFont;
    int mEngineCursor;
    QColor mFgColor;
    TChar mFormatBasic;
    TChar mFormatSystemMessage;

    int mIndentCount;
    int mMainFrameBottomHeight;
    int mMainFrameLeftWidth;
    int mMainFrameRightWidth;
    int mMainFrameTopHeight;
    int mOldX;
    int mOldY;

    TChar mFormatCurrent;
    QString mFormatSequenceRest;

    QWidget* mpBaseVFrame;
    QWidget* mpTopToolBar;
    QWidget* mpBaseHFrame;
    QWidget* mpLeftToolBar;
    QWidget* mpMainFrame;
    QWidget* mpRightToolBar;
    QWidget* mpMainDisplay;

    dlgMapper* mpMapper;

    QScrollBar* mpScrollBar;
    QScrollBar* mpHScrollBar;


    QElapsedTimer mProcessingTimer;
    bool mRecordReplay;
    QFile mReplayFile;
    QDataStream mReplayStream;

    QColor mSystemMessageBgColor;
    QColor mSystemMessageFgColor;
    bool mTriggerEngineMode;

    QPoint mUserCursor;
    int mWrapAt;
    QLineEdit* mpLineEdit_networkLatency;
    QPoint P_begin;
    QPoint P_end;
    QString mProfileName;
    TSplitter* splitter;
    bool mIsPromptLine;
    QToolButton* logButton;
    bool mUserAgreedToCloseConsole;
    QLineEdit* mpBufferSearchBox;
    QToolButton* mpBufferSearchUp;
    QToolButton* mpBufferSearchDown;
    int mCurrentSearchResult;
    QList<int> mSearchResults;
    QString mSearchQuery;
    QWidget* mpButtonMainLayer;
    int mBgImageMode;
    QString mBgImagePath;
    bool mHScrollBarEnabled;


public slots:
    void slot_searchBufferUp();
    void slot_searchBufferDown();
    void slot_toggleReplayRecording();
    void slot_stop_all_triggers(bool);
    void slot_toggleLogging();


protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;


private:
    ConsoleType mType;
    QSize mOldSize;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TConsole::ConsoleType)

#if !defined(QT_NO_DEBUG)
inline QDebug& operator<<(QDebug& debug, const TConsole::ConsoleType& type)
{
    QString text;
    QDebugStateSaver saver(debug);
    switch (type) {
    case TConsole::UnknownType:           text = QStringLiteral("Unknown"); break;
    case TConsole::CentralDebugConsole:   text = QStringLiteral("Central Debug Console"); break;
    case TConsole::ErrorConsole:          text = QStringLiteral("Profile Error Console"); break;
    case TConsole::MainConsole:           text = QStringLiteral("Profile Main Console"); break;
    case TConsole::SubConsole:            text = QStringLiteral("Mini Console"); break;
    case TConsole::UserWindow:            text = QStringLiteral("User Window"); break;
    case TConsole::Buffer:                text = QStringLiteral("Buffer"); break;
    default:
        text = QStringLiteral("Non-coded Type");
    }
    debug.nospace() << text;
    return debug;
}
#endif // !defined(QT_NO_DEBUG)

#endif // MUDLET_TCONSOLE_H


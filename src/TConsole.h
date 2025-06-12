#ifndef MUDLET_TCONSOLE_H
#define MUDLET_TCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2023 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
 *   Copyright (C) 2020 by Matthias Urlichs matthias@urlichs.de            *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPointer>
#include <QSaveFile>
#include <QVideoWidget>
#include <QWidget>
#include "post_guard.h"

#include <hunspell/hunspell.h>

#include <list>
#include <map>


enum class ControlCharacterMode {
    AsIs = 0x0,
    Picture = 0x1,
    OEM = 0x2
};

// Needed so it can be handled as a QVariant
Q_DECLARE_METATYPE(ControlCharacterMode)

class QCloseEvent;
class QLineEdit;
class QScrollBar;
class QShortcut;
class QToolButton;

class dlgMapper;
class Host;
class TTextEdit;
class TCommandLine;
class TDockWidget;
class TLabel;
class TScrollBox;
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

    enum SearchOption {
        // Unset:
        SearchOptionNone = 0x0,
        SearchOptionCaseSensitive = 0x1
    };
    Q_DECLARE_FLAGS(SearchOptions, SearchOption)

    Q_DISABLE_COPY(TConsole)
    explicit TConsole(Host*, const QString&, const ConsoleType type = UnknownType, QWidget* parent = nullptr);
    ~TConsole() override;

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
    void clear();
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
    void setCommandBgColor(const QColor&);
    void setCommandBgColor(int, int, int, int);
    void setCommandFgColor(const QColor&);
    void setCommandFgColor(int, int, int, int);
    void setScrollBarVisible(bool);
    void setHorizontalScrollBar(bool);
    void setScrolling(const bool state);
    bool getScrolling() const { return mScrollingEnabled; }
    void setCmdVisible(bool);
    void changeColors();
    void scrollDown(int lines);
    void scrollUp(int lines);
    void print(const QString& msg);
    void print(const char*);
    void print(const QString& msg, QColor fgColor, QColor bgColor);
    void printSystemMessage(const QString& msg);
    void printCommand(QString&);
    bool hasSelection();
    void moveCursorEnd();
    int getLastLineNumber();
    void refresh();
    void refreshView() const;
    void raiseMudletMousePressOrReleaseEvent(QMouseEvent*, const bool);
    bool setFontSize(int);
    bool setFontName(const QString& fontName);
    bool setConsoleBackgroundImage(const QString&, int);
    bool resetConsoleBackgroundImage();
    void setLink(const QStringList& linkFunction, const QStringList& linkHint, const QVector<int> linkReference = QVector<int>());
    // Cannot be called setAttributes as that would mask an inherited method
    void setDisplayAttributes(const TChar::AttributeFlags, const bool);
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void setConsoleBgColor(int, int, int, int);
    QColor getConsoleBgColor() const { return mBgColor; }
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
    // In the next function the first element in the return is an
    // error code:
    // 0 = Okay
    // 1 = Window not found
    // 2 = Selection not valid
    QPair<quint8, TChar> getTextAttributes() const;
    void setCaretMode(bool enabled);
    void setSearchOptions(const SearchOptions);
    void setF3SearchEnabled(const bool enabled);
    void setProxyForFocus(TCommandLine*);
    void raiseMudletSysWindowResizeEvent(const int overallWidth, const int overallHeight);
    // Raises an event if the number of lines (in the
    // (QStringList) TBuffer::lineBuffer) exceeds the number of rows in a
    // non-scrolling window:
    void handleLinesOverflowEvent(const int lineCount);
    void clearSplit();
    bool showTimeStamps() const { return mShowTimeStamps; }
    void raiseMudletResizeEvent();


    QPointer<Host> mpHost;
    // Only assigned a value for user windows:
    QPointer<TDockWidget> mpDockWidget;
    QPointer<TCommandLine> mpCommandLine;

    TBuffer buffer;
    static const QString cmLuaLineVariable;
    TTextEdit* mUpperPane = nullptr;
    TTextEdit* mLowerPane = nullptr;

    QToolButton* emergencyStop = nullptr;
    QWidget* layer = nullptr;
    QWidget* layerCommandLine = nullptr;
    QHBoxLayout* layoutLayer2 = nullptr;

    QColor mBgColor = QColorConstants::Black;
    QColor mFgColor = QColorConstants::LightGray;
    QColor mSystemMessageFgColor = QColorConstants::Red;
    QColor mCommandBgColor = QColorConstants::Black;
    QColor mSystemMessageBgColor = mBgColor;
    QColor mCommandFgColor = QColor(213, 195, 0);

    //1 = unclicked/up; 2 = clicked/down, 0 is NOT valid:
    int mButtonState = 1;

    QString mConsoleName;
    QString mCurrentLine;
    QString mDisplayFontName = qsl("Bitstream Vera Sans Mono");
    int mDisplayFontSize = 14;
    QFont mDisplayFont = QFont(mDisplayFontName, mDisplayFontSize, QFont::Normal);
    int mEngineCursor = -1;

    int mIndentCount = 0;
    int mHangingIndentCount = 0;
    QMargins mBorders;
    int mOldX = 0;
    int mOldY = 0;

    TChar mFormatCurrent;
    QString mFormatSequenceRest;

    QWidget* mpBaseVFrame = nullptr;
    QWidget* mpTopToolBar = nullptr;
    QWidget* mpBaseHFrame = nullptr;
    QWidget* mpLeftToolBar = nullptr;
    QWidget* mpMainFrame = nullptr;
    QWidget* mpRightToolBar = nullptr;
    QWidget* mpMainDisplay = nullptr;

    dlgMapper* mpMapper = nullptr;

    QScrollBar* mpScrollBar = nullptr;
    QScrollBar* mpHScrollBar = nullptr;

    QElapsedTimer mProcessingTimer;
    bool mRecordReplay = false;
    QSaveFile mReplayFile;
    QDataStream mReplayStream;

    bool mTriggerEngineMode = false;

    QPoint mUserCursor;
    int mWrapAt = 100;
    QLineEdit* mpLineEdit_networkLatency = nullptr;
    QPoint P_begin;
    QPoint P_end;
    QString mProfileName;
    TSplitter* splitter = nullptr;
    bool mIsPromptLine = false;
    QToolButton* logButton = nullptr;
    QToolButton* timeStampButton = nullptr;
    QToolButton* replayButton = nullptr;
    QLineEdit* mpBufferSearchBox = nullptr;
    QAction* mpAction_searchCaseSensitive = nullptr;
    QToolButton* mpBufferSearchUp = nullptr;
    QToolButton* mpBufferSearchDown = nullptr;
    // The line on which the current search result has been found, or the next
    // one is to start (currently only for the main console):
    int mCurrentSearchResult = 0;
    // Not used:
    // QList<int> mSearchResults;
    // The term that is currently being search for (currently only for the main
    // console):
    QString mSearchQuery;
    QWidget* mpButtonMainLayer = nullptr;
    int mBgImageMode = 0;
    QString mBgImagePath;
    bool mHScrollBarEnabled = false;
    ControlCharacterMode mControlCharacter = ControlCharacterMode::AsIs;
    QVideoWidget* mpVideoWidget = nullptr;

public slots:
    void slot_searchBufferUp();
    void slot_searchBufferDown();
    void slot_toggleReplayRecording();
    void slot_stopAllItems(bool);
    void slot_toggleLogging();
    void slot_changeControlCharacterHandling(const ControlCharacterMode);
    void slot_toggleSearchCaseSensitivity(bool);
    void slot_toggleTimeStamps(const bool);

signals:
    void resized(QResizeEvent* event);

protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragMoveEvent(QDragMoveEvent*) override;
    void dropEvent(QDropEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

    bool mAlertOnNewData = true;

private slots:
    void slot_adjustAccessibleNames();
    void slot_clearSearchResults();
    void focusOnSearchResultAndAnnounce(int searchX, int searchY);

private:
    void createSearchOptionIcon();

    ConsoleType mType = UnknownType;
    QSize mOldSize;
    SearchOptions mSearchOptions = SearchOptionNone;
    QAction* mpAction_searchOptions = nullptr;
    QIcon mIcon_searchOptions;
    bool mScrollingEnabled = true;
    bool mF3SearchEnabled = false;
    QPointer<QShortcut> mpSearchNextShortcut;
    QPointer<QShortcut> mpSearchPrevShortcut;
    // The size of the TConsole in (normal) "character" cells:
    QSize mDimensions;
    // Whether to show (a 13 character by default) timestamp to the left of
    // each line of text:
    bool mShowTimeStamps = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TConsole::ConsoleType)

#if !defined(QT_NO_DEBUG)
inline QDebug& operator<<(QDebug& debug, const TConsole::ConsoleType& type)
{
    QString text;
    const QDebugStateSaver saver(debug);
    switch (type) {
    case TConsole::UnknownType:           text = qsl("Unknown"); break;
    case TConsole::CentralDebugConsole:   text = qsl("Central Debug Console"); break;
    case TConsole::ErrorConsole:          text = qsl("Profile Error Console"); break;
    case TConsole::MainConsole:           text = qsl("Profile Main Console"); break;
    case TConsole::SubConsole:            text = qsl("Mini Console"); break;
    case TConsole::UserWindow:            text = qsl("User Window"); break;
    case TConsole::Buffer:                text = qsl("Buffer"); break;
    default:
        text = qsl("Non-coded Type");
    }
    debug.nospace() << text;
    return debug;
}
#endif // !defined(QT_NO_DEBUG)

#endif // MUDLET_TCONSOLE_H

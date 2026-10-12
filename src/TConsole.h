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
#include "TConsoleModel.h"
#include "TDebug.h"
#include "TPrintSink.h"
#include "enums.h"

#include <QElapsedTimer>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QPointer>
#include <QWidget>

#include <map>
#include <memory>
#include <vector>

class QCloseEvent;
class QHBoxLayout;
class QLineEdit;
class QScrollBar;
class QShortcut;
class QSplitter;
class QToolButton;
class QVideoWidget;

class dlgMapper;
class Host;
class TTextEdit;
class TCommandLine;
class TDockWidget;
class TLabel;
class TScrollBox;
class TSplitter;
class dlgNotepad;


// QWidget stays first so moc sees the QObject base.
class TConsole : public QWidget, public TPrintSink, public TDebug::Sink
{
    Q_OBJECT

public:
    enum ConsoleTypeFlag {
        UnknownType = 0x0,         // Should not be encountered but left as a trap value
        CentralDebugConsole = 0x1, // One of these for whole application
        ErrorConsole = 0x2,        // The bottom right corner of the Editor, one per profile
        MainConsole = 0x4,         // One per profile
        SubConsole = 0x8,          // Overlaid on top of MainConsole instance, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
        UserWindow = 0x10,         // Floatable/Dockable console, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
        Buffer = 0x20              // Non-visible store for data that can be copied to/from other per profile TConsoles, should be uniquely named in pool of SubConsole/UserWindow/Buffers AND Labels
    };
    Q_DECLARE_FLAGS(ConsoleType, ConsoleTypeFlag)

    Q_DISABLE_COPY(TConsole)
    explicit TConsole(Host*, const QString&, const ConsoleType type = UnknownType, QWidget* parent = nullptr);
    ~TConsole() override;

    void reset();
    void resizeConsole();
    Host* getHost();
    TConsoleModel& model() { return *mpModel; }
    const TConsoleModel& model() const { return *mpModel; }
    void insertText(const QString&);
    void clear();
    // The view's half of clear(), for a buffer something else has already cleared: drops the selection,
    // split and horizontal scroll, which index lines that are gone.
    void bufferCleared();
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void clearSelection() const;

    void setWrapAt(int pos) { mpModel->setWrapAt(pos); }
    int getWrapAt();

    void setIndentCount(int count) { mpModel->setIndentCount(count); }

    void setHangingIndentCount(int count) { mpModel->setHangingIndentCount(count); }

    TLinkStore& getLinkStore() { return buffer.mLinkStore; }
    bool moveCursor(int x, int y);
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

    // Model state, not view state: the main console shares Host's, so a link
    // concealed while the profile was open stays concealed once the widget has
    // gone.
    THyperlinkCompactManager& getHyperlinkCompactManager() { return mpModel->mHyperlinkCompactManager; }
    THyperlinkSelectionManager& getHyperlinkSelectionManager() { return mpModel->mHyperlinkSelectionManager; }
    THyperlinkVisibilityManager& getHyperlinkVisibilityManager() { return mpModel->mHyperlinkVisibilityManager; }

    void setCmdVisible(bool);
    void changeColors();
    void scrollDown(int lines);
    void scrollUp(int lines);
    void print(const QString& msg);
    void print(const char*);
    // timeStampOverride keeps the arrival time for held-back content being replayed.
    void print(const QString& msg, QColor fgColor, QColor bgColor, const QString& timeStampOverride = QString());
    // The repaint cues for text the model wrote: showNewLines() for lines appended to it, and
    // showCommandEcho() for whatever TConsoleModel::printCommand() did.
    void showNewLines();
    void showCommandEcho(const TConsoleModel::CommandEcho&);
    // The Central Debug Console's find bar is hidden until Ctrl+F or its context menu calls this.
    void showSearchBar();
    void printFormatted(const QString& text, const std::vector<TChar>& formatting, const TLinkStore& sourceLinkStore) override;
    void printDebugLine(const QString& text, const QColor& foreground, const QColor& background, const QString& timeStamp) override;
    void discardAll() override;
    void discardLastLine() override;
    void printSystemMessage(const QString& msg);
    void printCommand(QString&);
    int getLastLineNumber();
    void refresh();
    void refreshView() const;
    // Repaint just the lines the current selection covers, for the callers that
    // change their text where it stands instead of appending new text.
    void markSelectionDirty();
    // Repaint the given buffer lines in both panes.
    void markLinesDirty(int firstLine, int lastLine);
    void raiseMudletMousePressOrReleaseEvent(QMouseEvent*, const bool);
    void setFontSize(int);
    bool setConsoleBackgroundImage(const QString&, int);
    bool resetConsoleBackgroundImage();
    bool setWindowBackgroundImage(const QString&, int);
    bool resetWindowBackgroundImage();
    void updateMainFrameTransparency();
    // False only when a scale failed; no source or an unsized widget defers to the next resize
    bool updateWindowBackgroundCoverPixmap();
    static QRect coverSourceRect(const QSize& sourceSize, const QSize& targetSize);
    void setBorderColor(const QColor&);
    QColor borderColor() const { return mBorderColor; }
    void lowerMainDisplay();
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void setConsoleBgColor(int, int, int, int);
    QColor getConsoleBgColor() const { return mBgColor; }
    // Not used:    void setConsoleFgColor(int, int, int);
    // Returns the size of the main buffer area (excluding the command line and toolbars).
    QSize getMainWindowSize() const;
    // For a MainConsole hidden by a tab switch, which gets no resize events: NAWS-reports its restored size.
    void syncHiddenScreenDimensions();
    ConsoleType getType() const { return mType; }
    virtual void setProfileName(const QString&);
    void setCaretMode(bool enabled);
    void setSearchOptions(const enums::BufferSearchOptions);
    void setF3SearchEnabled(const bool enabled);
    void setProxyForFocus(TCommandLine*);
    void setCompactInputLine(const bool state);
    void repaintPanes() const;
    void raiseMudletSysWindowResizeEvent(const int overallWidth, const int overallHeight);
    // Raises an event if the number of lines (in the
    // (QStringList) TBuffer::lineBuffer) exceeds the number of rows in a
    // non-scrolling window:
    void handleLinesOverflowEvent(const int lineCount);
    void clearSplit();
    bool showTimeStamps() const { return mpModel->mShowTimeStamps; }
    void raiseMudletResizeEvent();
    // Shows the model's timestamp flag as it now stands.
    void applyTimeStamps();
    // This hides QWidget::setFont(const QFont&) rather than overriding it
    // (QWidget::setFont is non-virtual). The forceChange parameter is needed
    // when calling from setFontSize(...) because that modifies
    // mDisplayFontDetails before calling this, and the TFontAttributes
    // comparison would otherwise see no change:
    void setFont(const QFont&, const bool forceChange = false);


    QPointer<Host> mpHost;

    // Initialised in the constructor:
    TFontAttributes mDisplayFontDetails;

    // Only assigned a value for user windows:
    QPointer<TDockWidget> mpDockWidget;
    QPointer<TCommandLine> mpCommandLine;
    // The Central Debug Console's find bar, floating over its bottom right.
    QPointer<QWidget> mpFindBar;

    // The main console co-owns its model with Host, which runs triggers through it; sub-consoles own theirs.
    // The members below alias the model, so it must be declared first.
    std::shared_ptr<TConsoleModel> mpModel;
    TBuffer& buffer;
    TTextEdit* mUpperPane = nullptr;
    TTextEdit* mLowerPane = nullptr;

    QToolButton* emergencyStop = nullptr;
    QWidget* layer = nullptr;
    QWidget* layerCommandLine = nullptr;
    QHBoxLayout* layoutLayer2 = nullptr;

    QColor& mBgColor;
    QColor& mFgColor;
    QColor& mCommandBgColor;
    QColor& mCommandFgColor;

    int& mButtonState;

    QString& mConsoleName;
    QString& mCurrentLine;
    int& mEngineCursor;

    int& mIndentCount;
    int& mHangingIndentCount;
    QMargins mBorders;
    int mOldX = 0;
    int mOldY = 0;

    TChar& mFormatCurrent;
    QString mFormatSequenceRest;

    QWidget* mpBaseVFrame = nullptr;
    QWidget* mpTopToolBar = nullptr;
    QWidget* mpBaseHFrame = nullptr;
    QWidget* mpLeftToolBar = nullptr;
    QWidget* mpMainFrame = nullptr;
    QWidget* mpRightToolBar = nullptr;
    QWidget* mpMainDisplay = nullptr;
    QWidget* mpWindowBackground = nullptr;

    QPointer<dlgMapper> mpMapper;

    QScrollBar* mpScrollBar = nullptr;
    QScrollBar* mpHScrollBar = nullptr;

    QElapsedTimer mProcessingTimer;

    bool& mTriggerEngineMode;

    QPoint& mUserCursor;
    int& mWrapAt;
    QLineEdit* mpLineEdit_networkLatency = nullptr;
    QPoint& P_begin;
    QPoint& P_end;
    QString& mProfileName;
    TSplitter* splitter = nullptr;
    bool& mIsPromptLine;
    QToolButton* logButton = nullptr;
    QToolButton* timeStampButton = nullptr;
    QToolButton* replayButton = nullptr;
    QLineEdit* mpBufferSearchBox = nullptr;
    QAction* mpAction_searchCaseSensitive = nullptr;
    QToolButton* mpBufferSearchUp = nullptr;
    QToolButton* mpBufferSearchDown = nullptr;
    int& mCurrentSearchResult;
    // Not used:
    // QList<int> mSearchResults;
    // The term that is currently being search for (currently only for the main
    // console):
    QString mSearchQuery;
    QWidget* mpButtonMainLayer = nullptr;
    int mBgImageMode = 0;
    QString mBgImagePath;
    int mWindowBgImageMode = 0;
    QString mWindowBgImagePath;
    QPixmap mWindowBgSourcePixmap;
    bool mHScrollBarEnabled = false;
    bool& mScrollBarEnabled;
    ControlCharacterMode mControlCharacter = ControlCharacterMode::AsIs;
    QVideoWidget* mpVideoWidget = nullptr;
    QSplitter* commandSplitter = nullptr;

public slots:
    void slot_searchBufferUp();
    void slot_searchBufferDown();
    void slot_toggleReplayRecording();
    void slot_stopAllItems(bool);
    void slot_toggleLogging();
    void slot_changeControlCharacterHandling(const ControlCharacterMode);
    void slot_toggleSearchCaseSensitivity(bool);
    void slot_toggleTimeStamps(const bool);
    void slot_saveCommandSearchSettings();

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
    void slot_hyperlinkVisibilityChanged();
    void focusOnSearchResultAndAnnounce(int searchX, int searchY);
    void hideSearchBar();

private:
    void createFindBar();
    void positionFindBar();
    // MainConsole only: subtract the profile's main window borders. Height is -1 when unknown.
    int upperPaneWidthFor(const int containerWidth) const;
    int upperPaneHeightFor(const int containerHeight) const;
    void syncHostScreenDimensions(const int paneWidthPx, const int paneHeightPx);
    void createSearchOptionIcon();
    void raiseFontChangeEvent();
    void restoreCommandSearchSettings();
    void updateScrollBarStyle();

    ConsoleType mType = UnknownType;
    // the size the last resize reported to Lua
    QSize mOldSize;
    // only ever written from a size that was really measured, so what
    // getMainWindowSize() falls back to while the console is hidden or too small
    // to measure cannot be a size the window never had
    mutable QSize mLastMeasuredSize;
    enums::BufferSearchOptions mSearchOptions = enums::BufferSearchOptionNone;
    QAction* mpAction_searchOptions = nullptr;
    QIcon mIcon_searchOptions;
    bool& mScrollingEnabled;
    bool mF3SearchEnabled = false;
    QPointer<QShortcut> mpSearchNextShortcut;
    QPointer<QShortcut> mpSearchPrevShortcut;
    // The size of the TConsole in (normal) "character" cells:
    QSize mDimensions;
    // mpMainFrame's palette cannot hold this - it is rebuilt from scratch on every colour change
    QColor& mBorderColor;
    // latches the 'cover' scale failure so a resize drag does not repeat the warning
    bool mWindowBgCoverScaleFailed = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TConsole::ConsoleType)

#if !defined(QT_NO_DEBUG)
inline QDebug& operator<<(QDebug& debug, const TConsole::ConsoleType& type)
{
    QString text;
    const QDebugStateSaver saver(debug);
    // clang-format off
    switch (type) {
    case TConsole::UnknownType:
        text = qsl("Unknown");
        break;
    case TConsole::CentralDebugConsole:
        text = qsl("Central Debug Console");
        break;
    case TConsole::ErrorConsole:
        text = qsl("Profile Error Console");
        break;
    case TConsole::MainConsole:
        text = qsl("Profile Main Console");
        break;
    case TConsole::SubConsole:
        text = qsl("Mini Console");
        break;
    case TConsole::UserWindow:
        text = qsl("User Window");
        break;
    case TConsole::Buffer:
        text = qsl("Buffer");
        break;
    default:
        text = qsl("Non-coded Type");
    }
    // clang-format on
    debug.nospace() << text;
    return debug;
}
#endif // !defined(QT_NO_DEBUG)

#endif // MUDLET_TCONSOLE_H

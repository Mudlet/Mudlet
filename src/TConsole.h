#ifndef MUDLET_TCONSOLE_H
#define MUDLET_TCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016 by Ian Adkins - ieadkins@gmail.com                 *
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

#include "pre_guard.h"
#include <QDataStream>
#include <QFile>
#include <QPointer>
#include <QTextStream>
#include <QWidget>
#include "post_guard.h"

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
class TLabel;
class TSplitter;
class dlgNotepad;


class TConsole : public QWidget
{
    Q_OBJECT

    Q_DISABLE_COPY(TConsole)

public:
    TConsole(Host*, bool isDebugConsole, QWidget* parent = 0);
    void reset();
    void resetMainConsole();
    void echoUserWindow(const QString&);
    Host* getHost();
    TCommandLine* mpCommandLine;
    void replace(const QString&);
    void insertHTML(const QString&);
    void insertText(const QString&);
    void insertText(const QString&, QPoint);
    void insertLink(const QString&, QStringList&, QStringList&, QPoint, bool customFormat = false);
    void insertLink(const QString&, QStringList&, QStringList&, bool customFormat = false);
    void echoLink(const QString& text, QStringList& func, QStringList& hint, bool customFormat = false);
    void setLabelStyleSheet(std::string& buf, std::string& sh);
    void copy();
    void cut();
    void paste();
    void appendBuffer();
    void appendBuffer(TBuffer);
    int getButtonState();
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void pasteWindow(TBuffer);
    void setUserWindow();
    QStringList getLines(int from, int to);
    int getLineNumber();
    int getLineCount();
    bool deleteLine(int);
    std::list<int> getFgColor(std::string& buf);
    std::list<int> getBgColor(std::string& buf);
    void luaWrapLine(std::string& buf, int line);

    int getColumnNumber();
    void createMapper(int, int, int, int);
    void loadRawFile(std::string);

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

    void echo(const QString&);
    bool moveCursor(int x, int y);
    int select(const QString&, int numOfMatch = 1);
    void deselect();
    bool selectSection(int, int);
    void skipLine();
    void setFgColor(int, int, int);
    void setBgColor(int, int, int);
    void changeColors();
    TConsole* createBuffer(const QString& name);
    void scrollDown(int lines);
    void scrollUp(int lines);
    void print(const QString&, const QColor fgColor, const QColor bgColor);
    void print(const QString& msg);
    void print(const char*);
    void printDebug(QColor&, QColor&, const QString&);
    void printSystemMessage(const QString& msg);
    void printOnDisplay(std::string&, const bool isFromServer = false);
    void printCommand(QString&);
    bool hasSelection();
    void moveCursorEnd();
    int getLastLineNumber();
    void refresh();
    TLabel* createLabel(const QString& name, int x, int y, int width, int height, bool fillBackground);
    TConsole* createMiniConsole(const QString& name, int x, int y, int width, int height);
    bool createButton(const QString& name, int x, int y, int width, int height, bool fillBackground);
    bool raiseWindow(const QString& name);
    bool lowerWindow(const QString& name);
    bool showWindow(const QString& name);
    bool hideWindow(const QString& name);
    bool printWindow(const QString& name, const QString& text);
    bool setBackgroundImage(const QString& name, const QString& path);
    bool setBackgroundColor(const QString& name, int r, int g, int b, int alpha);
    QString getCurrentLine(std::string&);
    void selectCurrentLine(std::string&);
    bool setMiniConsoleFontSize(std::string&, int);
    void setBold(bool);
    void setLink(const QString& linkText, QStringList& linkFunction, QStringList& linkHint);
    void setItalics(bool);
    void setUnderline(bool);
    void setStrikeOut(bool);
    void finalize();
    void runTriggers(int);
    void showStatistics();
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void setConsoleBgColor(int, int, int);
    void setConsoleFgColor(int, int, int);
    std::list<int> _getFgColor();
    std::list<int> _getBgColor();
    void _luaWrapLine(int);
    QString getCurrentLine();
    void selectCurrentLine();
    bool saveMap(const QString&);
    bool loadMap(const QString&);
    bool importMap(const QString&, QString* errMsg = Q_NULLPTR);

    // Returns the size of the main buffer area (excluding the command line and toolbars).
    QSize getMainWindowSize() const;

    void toggleLogging(bool);

    QPointer<Host> mpHost;

    TBuffer buffer;
    static const QString cmLuaLineVariable;
    TTextEdit* console;
    TTextEdit* console2;
    int currentFgColorProperty;
    QToolButton* emergencyStop;
    bool isUserScrollBack;
    QWidget* layer;
    QWidget* layerCommandLine;
    QWidget* layerEdit;
    QColor mBgColor;
    int mButtonState;
    TBuffer mClipboard;
    QColor mCommandBgColor;
    QColor mCommandFgColor;

    QString mConsoleName;
    QString mCurrentLine;
    int mDeletedLines;
    QFont mDisplayFont;
    int mEngineCursor;
    QColor mFgColor;
    TChar mFormatBasic;
    TChar mFormatSystemMessage;

    int mIndentCount;
    bool mIsDebugConsole;
    bool mIsHighColorMode;
    bool mIsSubConsole;
    std::map<std::string, TLabel*> mLabelMap;
    QFile mLogFile;
    QString mLogFileName;
    QTextStream mLogStream;
    bool mLogToLogFile;
    int mMainFrameBottomHeight;
    int mMainFrameLeftWidth;
    int mMainFrameRightWidth;
    int mMainFrameTopHeight;
    int mOldX;
    int mOldY;


    TChar mFormatCurrent;
    QString mFormatSequenceRest;
    bool mHighColorModeBackground;
    bool mHighColorModeForeground;


    QWidget* mpBaseVFrame;
    QWidget* mpTopToolBar;
    QWidget* mpBaseHFrame;
    QWidget* mpLeftToolBar;
    QWidget* mpMainFrame;
    QWidget* mpRightToolBar;
    QWidget* mpMainDisplay;

    dlgMapper* mpMapper;
    dlgNotepad* mpNotePad;

    QScrollBar* mpScrollBar;


    QTime mProcessingTime;
    bool mRecordReplay;
    QFile mReplayFile;
    QDataStream mReplayStream;
    TChar mStandardFormat;
    QList<TConsole*> mSubConsoleList;
    std::map<std::string, TConsole*> mSubConsoleMap;

    QColor mSystemMessageBgColor;
    QColor mSystemMessageFgColor;
    bool mTriggerEngineMode;
    bool mUserConsole;
    QPoint mUserCursor;
    bool mWaitingForHighColorCode;
    bool mWindowIsHidden;
    int mWrapAt;
    QLineEdit* networkLatency;
    QPoint P_begin;
    QPoint P_end;
    QString profile_name;
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
    bool mSaveLayoutRequested;

signals:


public slots:
    void slot_searchBufferUp();
    void slot_searchBufferDown();
    void slot_toggleReplayRecording();
    void slot_stop_all_triggers(bool);
    void slot_toggleLogging();

    // Used by mudlet class as told by "Profile Preferences"
    // =>"Copy Map" in another profile to inform a list of
    // profiles - asynchronously - to load in an updated map
    void slot_reloadMap(QList<QString>);
};

#endif // MUDLET_TCONSOLE_H

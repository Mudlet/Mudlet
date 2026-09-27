#ifndef MUDLET_TMAINCONSOLE_H
#define MUDLET_TMAINCONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2022 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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


#include "TConsole.h"
#include <QFile>
#include <QPointer>
#include <QTextStream>
#include <QWidget>
#include <optional>
#include <utility>

#include <list>

class TAction;
class TEasyButtonBar;
class TMediaPlayer;
class TScrollBox;
class TTextBox;
class TToolBar;
class QDialog;
class QDockWidget;
class QProgressDialog;

class TMainConsole : public TConsole
{
    Q_OBJECT

public:
    explicit TMainConsole(Host*, QWidget* parent = nullptr);
    ~TMainConsole();

    void resizeEvent(QResizeEvent* event) override;
    void resetMainConsole();
    void closeEvent(QCloseEvent*) override;
    TConsole* createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height);
    TConsole* createSubConsole(const QString& name, QWidget* parent);
    bool createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height);
    bool raiseWindow(const QString& name);
    bool lowerWindow(const QString& name);
    bool showWindow(const QString& name);
    bool hideWindow(const QString& name);
    bool printWindow(const QString& name, const QString& text);
    bool clear(const QString& name);
    void setProfileName(const QString&) override;
    void selectCurrentLine(std::string&);
    std::list<int> getFgColor(QString& buf);
    std::list<int> getBgColor(QString& buf);
    QPair<quint8, TChar> getTextAttributes(const QString&) const;
    void luaWrapLine(QString& buf, int line);
    bool createBuffer(const QString& name);
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet);
    std::optional<QString> getUserWindowStyleSheet(const QString& name) const;
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text);
    std::pair<bool, QString> getUserWindowTitle(const QString& name) const;
    bool setTextFormat(const QString& name, const QColor& fgColor, const QColor& bgColor, const TChar::AttributeFlags& flags);
    bool createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough = false);
    std::pair<bool, QString> createMapper(const QString& windowname, int, int, int, int);
    std::pair<bool, QString> createCommandLine(const QString& windowname, const QString& name, int, int, int, int);
    void registerSubCommandLine(const QString& name, TCommandLine* pCommandLine);
    void deregisterSubCommandLine(TCommandLine* pCommandLine);
    std::pair<bool, QString> createTextBox(const QString& windowname, const QString& name, int, int, int, int);
    QSize getUserWindowSize(const QString& windowname) const;
    std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet);
    std::optional<QString> getCmdLineStyleSheet(const QString& name) const;
    std::pair<bool, QString> setLabelStyleSheet(const QString& name, const QString& stylesheet);
    std::optional<QString> getLabelStyleSheet(const QString& name) const;
    std::optional<QSize> getLabelSizeHint(const QString& name) const;
    std::pair<bool, QString> deleteLabel(const QString&);
    std::pair<bool, QString> deleteMiniConsole(const QString&);
    std::pair<bool, QString> deleteCommandLine(const QString&);
    std::pair<bool, QString> deleteTextBox(const QString&);
    std::pair<bool, QString> deleteScrollBox(const QString&);
    std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration);
    std::optional<QString> getLabelToolTip(const QString& name) const;
    std::pair<bool, QString> setLabelCursor(const QString& name, int shape);
    std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY);
    // Host forwards these by name, never by widget; each fails for a name that is not a label's.
    bool setLabelClickThrough(const QString& name, bool clickThrough);
    bool setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, bool underline);
    bool resetLabelLinkStyle(const QString& name);
    bool clearLabelVisitedLinks(const QString& name);
    bool showLabel(const QString& name);
    bool hideLabel(const QString& name);
    bool resizeLabel(const QString& name, int width, int height);
    bool moveLabel(const QString& name, int x, int y);
    bool reparentLabel(const QString& windowname, const QString& name, int x, int y, bool show);
    bool setLabelText(const QString& name, const QString& text);
    std::pair<bool, QString> setLabelMovie(const QString& name, const QString& moviePath);
    bool setLabelBackgroundColor(const QString& name, const QColor& color);
    std::optional<QColor> getLabelBackgroundColor(const QString& name) const;
    bool setLabelBackgroundImage(const QString& name, const QString& path);
    bool resetLabelBackgroundImage(const QString& name);
    bool setLabelSvgTint(const QString& name, const QColor& color);
    bool resetLabelSvgTint(const QString& name);
    bool setLabelSvgRotation(const QString& name, double angle);
    bool resetLabelSvgRotation(const QString& name);
    bool setLabelSvgShear(const QString& name, double shearX, double shearY);
    bool resetLabelSvgShear(const QString& name);
    bool resetLabelSvgTransform(const QString& name);
    std::optional<QRect> getLabelGeometry(const QString& name) const;
    std::optional<bool> getLabelVisible(const QString& name) const;
    std::optional<QFont> getLabelFont(const QString& name) const;
    bool setLabelFont(const QString& name, const QFont& font);
    std::optional<QString> getLabelText(const QString& name) const;
    // No value for a name that is not a label's, false for a label that is not
    // showing a movie; the movie operations below report failure for either.
    std::optional<bool> labelShowsMovie(const QString& name) const;
    bool startLabelMovie(const QString& name);
    bool pauseLabelMovie(const QString& name);
    // Also false when the movie has no such frame.
    bool setLabelMovieFrame(const QString& name, int frame);
    bool setLabelMovieSpeed(const QString& name, int percent);
    bool scaleLabelMovie(const QString& name, bool followLabelSize);
    // Not the open map, so map changes stay in this class, in step with the window registry.
    TLabel* labelWidget(const QString& name) const { return mLabelMap.value(name); }
    // All sub-console and dock map changes go through these four, keeping Host's window registry in step.
    void registerSubConsole(const QString& name, TConsole* pConsole);
    TConsole* deregisterSubConsole(const QString& name);
    void registerDockWidget(const QString& name, TDockWidget* pDockWidget);
    TDockWidget* deregisterDockWidget(const QString& name);
    TDockWidget* createUserWindow(const QString& name);
    TConsole* subConsoleWidget(const QString& name) const { return mSubConsoleMap.value(name); }
    QString subConsoleName(TConsole* pConsole) const { return mSubConsoleMap.key(pConsole); }
    TDockWidget* dockWidget(const QString& name) const { return mDockWidgetMap.value(name); }
    QStringList dockWidgetNames() const { return QStringList(mDockWidgetMap.keys()); }
    // Host forwards these by name; each also handles the name's dock, so the core needs one branch each.
    void closeSubConsole(const QString& name);
    void changeSubConsoleColors(const QString& name);
    bool showSubConsole(const QString& name);
    bool hideSubConsole(const QString& name);
    bool resizeSubConsole(const QString& name, int width, int height);
    bool moveSubConsole(const QString& name, int x, int y);
    bool reparentWindow(const QString& windowname, const QString& name, int x, int y, bool show);
    bool pasteToSubConsole(const QString& name);
    std::optional<QSize> consoleFontSize(const QString& name) const;
    bool setSubConsoleBackgroundColor(const QString& name, const QColor& color);
    bool setSubConsoleBackgroundImage(const QString& name, const QString& path, int mode);
    bool resetSubConsoleBackgroundImage(const QString& name);
    bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color);
    bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color);
    std::optional<QRect> getSubConsoleGeometry(const QString& name) const;
    std::optional<bool> getSubConsoleVisible(const QString& name) const;
    void setDockWidgetStyleSheets(const QString& styleSheet);
    void setDockLayoutChanged(const QString& name);
    bool clearDockLayoutChanged(const QString& name);
    TCommandLine* subCommandLineWidget(const QString& name) const { return mSubCommandLineMap.value(name); }
    QList<TCommandLine*> subCommandLineWidgets() const { return mSubCommandLineMap.values(); }
    void setCommandLinePlaceholderText(const QString& text);
    void updateCommandLineSpellCheck(bool enabled);
    void setCommandLineText(const QString& text);
    TCommandLine* raiseCommandLine();
    // The command line operations the core forwards to this view by name, never
    // by widget. An empty name or "main" is this console's own command line, any
    // other one made by createCommandLine() or a mini console's; each reports
    // failure for a name that is none of those.
    std::optional<QString> getCommandLineText(const QString& name) const;
    bool replaceCommandLineText(const QString& name, const QString& text);
    bool appendCommandLineText(const QString& name, const QString& text);
    bool clearCommandLine(const QString& name);
    bool selectCommandLineText(const QString& name);
    bool addCommandLineSuggestion(const QString& name, const QString& word);
    bool removeCommandLineSuggestion(const QString& name, const QString& word);
    bool clearCommandLineSuggestions(const QString& name);
    bool addCommandLineBlacklistWord(const QString& name, const QString& word);
    bool removeCommandLineBlacklistWord(const QString& name, const QString& word);
    bool clearCommandLineBlacklist(const QString& name);
    bool addCommandLineMenuItem(const QString& name, const QString& label, const QString& eventName);
    // No value for a name that is not a command line's, false for a command line
    // with no such item.
    std::optional<bool> removeCommandLineMenuItem(const QString& name, const QString& label);
    std::optional<bool> getCommandLineSavesHistory(const QString& name) const;
    bool setCommandLineSavesHistory(const QString& name, bool savesHistory);
    bool setCommandLineVisible(const QString& name, bool visible);
    // Repaints buffer lines the core changed in place in the console showing
    // this model, if one does. Taking the model the core already found, rather
    // than a name, saves a second lookup on a script's per-line calls.
    static void markWindowDirty(const TConsoleModel& model, int firstLine, int lastLine);
    // The scroll bar and scrolling operations the core forwards to this view by
    // name, never by widget. An empty name or "main" is this console, any other
    // a mini console, user window or buffer; each reports failure for a name
    // that is none of those.
    bool setWindowScrollBarVisible(const QString& name, bool visible);
    bool setWindowHorizontalScrollBarVisible(const QString& name, bool visible);
    std::optional<bool> getWindowScrollBarVisible(const QString& name);
    bool setWindowScrolling(const QString& name, bool enabled);
    std::optional<bool> getWindowScrolling(const QString& name);
    std::optional<int> getWindowScroll(const QString& name);
    // A negative line counts back from the end. One at or past the end, or
    // toEnd, puts the console back to following new lines.
    bool scrollWindowTo(const QString& name, int line, bool toEnd);
    // Whether the named console shows timestamps, found by name in the same
    // way. Only the view has this flag: the buffer records every line's time
    // either way.
    std::optional<bool> getWindowTimeStamps(const QString& name);
    // Answers whether they were shown before, and leaves them alone when that
    // is already what was asked for.
    std::optional<bool> setWindowTimeStamps(const QString& name, bool shown);
    // How many characters and lines fit in the named console's upper pane, from its size and font.
    std::optional<int> getWindowColumnCount(const QString& name);
    std::optional<int> getWindowRowCount(const QString& name);
    // The font operations, found by name in the same way. The main console's
    // font is the profile's display font, which Host keeps and tells scripts
    // about when it changes.
    std::optional<QFont> getWindowFont(const QString& name);
    std::optional<int> getWindowFontSize(const QString& name);
    // The console keeps its point size. Answers whether the font was taken
    // and, when not, why.
    std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString& name, const QString& family, QFont::Weight weight);
    bool setWindowFontSize(const QString& name, int size);
    // Creates the console's own command line the first time it is shown.
    bool setWindowCommandLineVisible(const QString& name, bool visible);
    // The link and text operations, found by name in the same way. A link's
    // commands can hold Lua registry references, which the console takes over
    // only when it is found, so hasWindow() lets a caller look for it before
    // taking any. Text or a link put into the main console's line while a
    // trigger runs over it, or a replacement there, moves the trigger's
    // captures to follow it.
    bool hasWindow(const QString& name);
    bool echoWindowLink(const QString& name, const QString& text, QStringList commands, QStringList hints, bool useCurrentFormat, const QVector<int>& luaReferences);
    bool insertWindowLink(const QString& name, const QString& text, QStringList commands, QStringList hints, bool useCurrentFormat, const QVector<int>& luaReferences);
    // Makes the console's selection a link.
    bool setWindowLink(const QString& name, const QStringList& commands, const QStringList& hints, const QVector<int>& luaReferences);
    bool insertWindowText(const QString& name, const QString& text);
    // Puts text in place of the console's selection.
    bool replaceWindowText(const QString& name, const QString& text);
    TTextBox* textBoxWidget(const QString& name) const { return mTextBoxMap.value(name); }
    // The text box operations the core forwards to this view by name, never by
    // widget; each reports failure for a name that is not a text box's.
    std::optional<QString> getTextBoxText(const QString& name) const;
    bool setTextBoxText(const QString& name, const QString& text);
    bool clearTextBox(const QString& name);
    bool setTextBoxReadOnly(const QString& name, bool readOnly);
    bool setTextBoxPlaceholder(const QString& name, const QString& text);
    bool setTextBoxStyleSheet(const QString& name, const QString& styleSheet);
    std::optional<QFont> getTextBoxFont(const QString& name) const;
    bool setTextBoxFont(const QString& name, const QFont& font);
    bool setTextBoxTabMovesFocus(const QString& name, bool tabMovesFocus);
    // QWidget state the core asks about, answered here so that it need not
    // reach this view's QWidget base.
    QPoint mousePosition() const;
    bool hasKeyboardFocus() const;
    // Shared by scroll boxes, command lines and text boxes: each is the same plain QWidget call.
    bool showPlainWindow(const QString& name);
    bool hidePlainWindow(const QString& name);
    bool resizePlainWindow(const QString& name, int width, int height);
    bool movePlainWindow(const QString& name, int x, int y);
    std::optional<QRect> getPlainWindowGeometry(const QString& name) const;
    std::optional<bool> getPlainWindowVisible(const QString& name) const;
    bool setCommandLineAction(const QString& name, const int func);
    bool resetCommandLineAction(const QString& name);
    void showStatistics();
    void showPackageDownloadProgress(const QString& title, const QString& cancelText);
    void updatePackageDownloadProgress(qint64 got, qint64 total);
    void closePackageDownloadProgress();
    void showMapTransferProgress(const QString& title, const QString& label, const QString& cancelButtonText);
    void showMapJsonProgress(const QString& title, const QString& label, const QString& cancelButtonText, int maximum);
    void setMapProgressDialogLabel(const QString& text);
    void setMapProgressDialogRange(int minimum, int maximum);
    void setMapProgressDialogValue(int value);
    void disableMapProgressDialogCancel();
    void closeMapProgressDialog();
    void createMapperDock(const QString& title, const QString& objectName);
    dlgMapper* dockedMapper() const;
    void showMapWidget();
    void dockMapWidget(Qt::DockWidgetArea area);
    std::pair<bool, QString> placeMapWidget(const QString& area, int x, int y, int width, int height);
    // The map dock's state as values, so the core never holds the widget. mapWidgetCreated() does not
    // mean on screen, which is what the four after it go by.
    bool mapWidgetCreated() const;
    bool setMapWidgetTitle(const QString& title);
    std::optional<QString> mapWidgetTitle() const;
    std::optional<QRect> mapWidgetGeometry() const;
    bool hideMapWidget();
    TToolBar* createToolBar(TAction* pAction, const QString& name);
    TEasyButtonBar* createEasyButtonBar(TAction* pRootAction, const QString& name);
    void attachEasyButtonBar(TEasyButtonBar* pBar, int location);
    void detachEasyButtonBar(TEasyButtonBar* pBar, int location);
    void dockToolBar(TToolBar* pToolBar, Qt::DockWidgetArea area);
    void undockToolBar(TToolBar* pToolBar);
    void showMapperScriptReminder();
    void showUnpackingProgress(const QString& message, const QString& title);
    void closeUnpackingProgress();
    void setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded);
    void hideVideoOutput(TMediaPlayer* player);
    void toggleLogging(bool);
    void printOnDisplay(std::string&, bool isFromServer = false);
    void finalize();
    void refreshSubconsoles();


    mutable QMap<QString, QSize> mCachedWindowSizes;
    TBuffer mClipboard;
    // The log lifecycle lives in the core console model so a profile with no
    // view can run one; these four are references aliasing the model's fields,
    // the way buffer and mFgColor alias theirs. mLogToLogFile and mLogFileName
    // are what keep TLuaInterpreter::startLogging() compiling unchanged; the
    // other two are kept so the set does not have to be reasoned about in
    // halves.
    QFile& mLogFile;
    QString& mLogFileName;
    QTextStream& mLogStream;
    bool& mLogToLogFile;
    QPointer<QProgressDialog> mpPackageDownloadProgressDialog;
    QPointer<QProgressDialog> mpMapProgressDialog;
    // Outlives Host::closeMapWidget(), which only hides it, so this being
    // non-null does not mean a map widget is on screen (see mapWidget()). Null means
    // none was made, or createMapper() took a hidden one over for an embedded mapper;
    // nothing else destroys it before ~TMainConsole().
    QPointer<QDockWidget> mpDockableMapWidget;
    QPointer<QDialog> mpUnpackingDialog;


public slots:
    // Used by mudlet class as told by "Profile Preferences"
    // =>"Copy Map" in another profile to inform a list of
    // profiles - asynchronously - to load in an updated map
    void slot_reloadMap(QList<QString>);


private slots:
    // The two halves of a logging change that need this view: the core model
    // owns everything else about it.
    void slot_loggingAnnouncement(const bool isLogging, const QString& logFileName);
    void slot_loggingStateChanged(const bool isLogging);


signals:
    // Raised when new data is incoming to trigger Alert handling in mudlet
    // class, second argument is true for a lower priority indication when
    // locally produced information is painted into main console
    void signal_newDataAlert(const QString&, bool isLowerPriorityChange = false);


private:
    void createMapProgressDialog(const QString& title, const QString& label, const QString& cancelButtonText, int minimum, int maximum);
    // Shared by reparentLabel() and reparentWindow() so they agree on what "main" means.
    QWidget* parentWidgetFor(const QString& windowname) const;
    // Resolves the three name-only kinds in the same order as the core.
    QWidget* plainWindowWidget(const QString& name) const;
    TCommandLine* commandLineNamed(const QString& name) const;
    TConsole* consoleNamed(const QString& name);
    // The single answer to "does this profile have a map widget on screen right
    // now" - null if it never opened one, put it away, or createMapper() took it over.
    //
    // isHidden() rather than a flag of our own, because the dock gets hidden by
    // paths that would not update one: its title bar close button,
    // mudlet::slot_showMapperDialog() and QMainWindow::restoreState(). Not !isVisible(),
    // which would also say "no map widget" while the main window is hidden (e.g. in the tray).
    QDockWidget* mapWidget() const;
    void registerLabelWidget(const QString& name, TLabel* pLabel);
    void deregisterLabelWidget(TLabel* pLabel);

    // With registerSubCommandLine()/deregisterSubCommandLine(), all scroll box, text box and command line
    // map changes go through these, keeping Host's window registry in step.
    void registerScrollBox(const QString& name, TScrollBox* pScrollBox);
    void deregisterScrollBox(TScrollBox* pScrollBox);
    void registerTextBox(const QString& name, TTextBox* pTextBox);
    void deregisterTextBox(TTextBox* pTextBox);

    // Private so Host's window registry stays in step with them; use the *Widget() accessors.
    QMap<QString, TLabel*> mLabelMap;
    // QPointer: a miniconsole in a user window dies with that dock unannounced, and a null entry
    // makes the dead name a lookup miss that can be reused.
    QMap<QString, QPointer<TConsole>> mSubConsoleMap;
    QMap<QString, TDockWidget*> mDockWidgetMap;
    QMap<QString, TCommandLine*> mSubCommandLineMap;
    QMap<QString, TTextBox*> mTextBoxMap;
    QMap<QString, TScrollBox*> mScrollBoxMap;

    bool mEnableClose = false;
};

#endif // MUDLET_TMAINCONSOLE_H

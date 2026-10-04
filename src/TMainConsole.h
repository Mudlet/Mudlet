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
#include "TConsoleFrontend.h"
#include <QFile>
#include <QHash>
#include <QPointer>
#include <QStack>
#include <QTextStream>
#include <QWidget>
#include <memory>
#include <optional>
#include <utility>

#include <list>

class EAction;
class TAction;
class TEasyButtonBar;
class TFlipButton;
class TMediaPlayer;
class TMxpFrameWidgets;
class TScrollBox;
class TTextBox;
class TToolBar;
class QDialog;
class QDockWidget;
class QProgressDialog;
class QTimer;

class TMainConsole : public TConsole, public TConsoleFrontend
{
    Q_OBJECT

public:
    explicit TMainConsole(Host*, QWidget* parent = nullptr);
    ~TMainConsole();

    void resizeEvent(QResizeEvent* event) override;
    void resetMainConsole() override;
    void closeEvent(QCloseEvent*) override;
    TConsole* createMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height);
    TConsole* createSubConsole(const QString& name, QWidget* parent);
    TMxpFrameWidgets& mxpFrameWidgets() { return *mpMxpFrameWidgets; }
    const TMxpFrameWidgets& mxpFrameWidgets() const { return *mpMxpFrameWidgets; }
    bool createScrollBox(const QString& windowname, const QString& name, int x, int y, int width, int height) override;
    bool raiseWindow(const QString& name) override;
    bool lowerWindow(const QString& name) override;
    bool showWindow(const QString& name);
    bool hideWindow(const QString& name);
    void setProfileName(const QString&) override;
    // What Host needs of this console's own widget, named rather than reached
    // through the QWidget API so that a view with no widget could answer too.
    bool requestClose() override;
    void requestRepaint();
    QFont displayFont() const override;
    void setProfileStyleSheet(const QString& styleSheet) override;
    void applyBorders() override;
    // TConsole's, which as non-virtual members of another base cannot implement TConsoleFrontend's themselves.
    void setConsoleBgColor(int r, int g, int b, int a) override { TConsole::setConsoleBgColor(r, g, b, a); }
    bool setConsoleBackgroundImage(const QString& imgPath, int mode) override { return TConsole::setConsoleBackgroundImage(imgPath, mode); }
    bool resetConsoleBackgroundImage() override { return TConsole::resetConsoleBackgroundImage(); }
    bool setWindowBackgroundImage(const QString& imgPath, int mode) override { return TConsole::setWindowBackgroundImage(imgPath, mode); }
    bool resetWindowBackgroundImage() override { return TConsole::resetWindowBackgroundImage(); }
    void setBorderColor(const QColor& color) override { TConsole::setBorderColor(color); }
    void changeColors() override { TConsole::changeColors(); }
    void setFont(const QFont& font) override { TConsole::setFont(font); }
    void setFont(const QFont& font, bool forceChange) { TConsole::setFont(font, forceChange); }
    void setFontSize(int size) override { TConsole::setFontSize(size); }
    void setF3SearchEnabled(bool enabled) override { TConsole::setF3SearchEnabled(enabled); }
    void setCompactInputLine(bool state) override { TConsole::setCompactInputLine(state); }
    void setCaretMode(bool enabled) override { TConsole::setCaretMode(enabled); }
    void showNewLines() override { TConsole::showNewLines(); }
    void showCommandEcho(const TConsoleModel::CommandEcho& echo) override { TConsole::showCommandEcho(echo); }
    void markSelectionDirty() override { TConsole::markSelectionDirty(); }
    void refreshView() const override { TConsole::refreshView(); }
    void restoreOwnMapper() override;
    bool createBuffer(const QString& name) override;
    std::pair<bool, QString> setUserWindowStyleSheet(const QString& name, const QString& userWindowStyleSheet) override;
    std::pair<bool, QString> setUserWindowTitle(const QString& name, const QString& text) override;
    bool createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool fillBackground, bool clickThrough) override;
    std::pair<bool, QString> createMapper(const QString& windowname, int, int, int, int) override;
    std::pair<bool, QString> createCommandLine(const QString& windowname, const QString& name, int, int, int, int) override;
    void registerSubCommandLine(const QString& name, TCommandLine* pCommandLine);
    void deregisterSubCommandLine(TCommandLine* pCommandLine);
    std::pair<bool, QString> createTextBox(const QString& windowname, const QString& name, int, int, int, int) override;
    std::pair<bool, QString> setCmdLineStyleSheet(const QString& name, const QString& styleSheet) override;
    std::pair<bool, QString> setLabelStyleSheet(const QString& name, const QString& stylesheet) override;
    std::optional<QSize> getLabelSizeHint(const QString& name) const override;
    std::pair<bool, QString> deleteLabel(const QString& name) override;
    std::pair<bool, QString> deleteMiniConsole(const QString&) override;
    std::pair<bool, QString> deleteCommandLine(const QString&) override;
    std::pair<bool, QString> deleteTextBox(const QString&) override;
    std::pair<bool, QString> deleteScrollBox(const QString&) override;
    std::pair<bool, QString> setLabelToolTip(const QString& name, const QString& text, double duration) override;
    std::pair<bool, QString> setLabelCursor(const QString& name, int shape) override;
    std::pair<bool, QString> setLabelCustomCursor(const QString& name, const QString& pixMapLocation, int hotX, int hotY) override;
    bool setLabelClickThrough(const QString& name, bool clickThrough) override;
    bool setLabelLinkStyle(const QString& name, const QString& linkColor, const QString& linkVisitedColor, bool underline) override;
    bool resetLabelLinkStyle(const QString& name) override;
    bool clearLabelVisitedLinks(const QString& name) override;
    bool showLabel(const QString& name) override;
    bool hideLabel(const QString& name) override;
    bool resizeLabel(const QString& name, int width, int height) override;
    bool moveLabel(const QString& name, int x, int y) override;
    bool reparentLabel(const QString& windowname, const QString& name, int x, int y, bool show) override;
    bool setLabelText(const QString& name, const QString& text) override;
    std::pair<bool, QString> setLabelMovie(const QString& name, const QString& moviePath) override;
    bool setLabelBackgroundColor(const QString& name, const QColor& color) override;
    std::optional<QColor> getLabelBackgroundColor(const QString& name) const override;
    bool setLabelBackgroundImage(const QString& name, const QString& path) override;
    bool resetLabelBackgroundImage(const QString& name) override;
    bool setLabelSvgTint(const QString& name, const QColor& color) override;
    bool resetLabelSvgTint(const QString& name) override;
    bool setLabelSvgRotation(const QString& name, double angle) override;
    bool resetLabelSvgRotation(const QString& name) override;
    bool setLabelSvgShear(const QString& name, double shearX, double shearY) override;
    bool resetLabelSvgShear(const QString& name) override;
    bool resetLabelSvgTransform(const QString& name) override;
    bool setLabelFont(const QString& name, const QFont& font) override;
    std::optional<bool> labelShowsMovie(const QString& name) const override;
    bool startLabelMovie(const QString& name) override;
    bool pauseLabelMovie(const QString& name) override;
    bool setLabelMovieFrame(const QString& name, int frame) override;
    bool setLabelMovieSpeed(const QString& name, int percent) override;
    bool scaleLabelMovie(const QString& name, bool followLabelSize) override;
    // Not the open map, so map changes stay in this class, in step with the window registry.
    TLabel* labelWidget(const QString& name) const { return mLabelMap.value(name); }
    // Copies a named window's current pos() and size() into Host's window registry (a label's into
    // its model), where Host::windowGeometry() reads them. Every view op that moves or resizes a
    // widget calls it, as a hidden widget gets no events to report its own.
    void reportGeometry(const QString& name);
    // As reportGeometry(), for QWidget::isVisibleTo() this console.
    void reportVisibility(const QString& name);
    // Every named window at or below pRoot, whose visibility follows its ancestors'.
    void reportVisibilityWithin(QWidget* pRoot);
    // For a container holding named windows that is not one itself, as an MXP frame's.
    void watchVisibility(QWidget* pContainer);
    // For QMainWindow::restoreState(), which places hidden docks without an event.
    void reportDockGeometry();
    // Copies getMainWindowSize() into Host's window registry, where Host::mainWindowSize() reads it.
    void reportMainWindowSize();
    // All sub-console and dock map changes go through these four, keeping Host's window registry in step.
    void registerSubConsole(const QString& name, TConsole* pConsole);
    TConsole* deregisterSubConsole(const QString& name);
    void registerDockWidget(const QString& name, TDockWidget* pDockWidget);
    TDockWidget* deregisterDockWidget(const QString& name);
    std::pair<bool, QString> openUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area) override;
    TConsole* subConsoleWidget(const QString& name) const { return mSubConsoleMap.value(name); }
    QString subConsoleName(TConsole* pConsole) const { return mSubConsoleMap.key(pConsole); }
    TDockWidget* dockWidget(const QString& name) const { return mDockWidgetMap.value(name); }
    QStringList dockWidgetNames() const { return QStringList(mDockWidgetMap.keys()); }
    // Host forwards these by name; each also handles the name's dock, so the core needs one branch each.
    void closeSubConsole(const QString& name) override;
    void changeSubConsoleColors(const QString& name) override;
    bool showSubConsole(const QString& name) override;
    bool hideSubConsole(const QString& name) override;
    bool resizeSubConsole(const QString& name, int width, int height) override;
    bool moveSubConsole(const QString& name, int x, int y) override;
    bool reparentWindow(const QString& windowname, const QString& name, int x, int y, bool show) override;
    std::optional<QSize> consoleFontSize(const QString& name) const override;
    bool setSubConsoleBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleBackgroundImage(const QString& name, const QString& path, int mode) override;
    bool resetSubConsoleBackgroundImage(const QString& name) override;
    bool setSubConsoleCommandBackgroundColor(const QString& name, const QColor& color) override;
    bool setSubConsoleCommandForegroundColor(const QString& name, const QColor& color) override;
    void setDockLayoutChanged(const QString& name) override;
    bool clearDockLayoutChanged(const QString& name) override;
    TCommandLine* subCommandLineWidget(const QString& name) const { return mSubCommandLineMap.value(name); }
    QList<TCommandLine*> subCommandLineWidgets() const { return mSubCommandLineMap.values(); }
    void setCommandLinePlaceholderText(const QString& text) override;
    void updateCommandLineSpellCheck(bool enabled) override;
    void setCommandLineText(const QString& text) override;
    // The command line the player used last for this profile, so the focus can
    // go back to it on returning to the profile.
    void recordActiveCommandLine(TCommandLine*);
    void forgetCommandLine(TCommandLine*);
    void focusActiveCommandLine() override;
    bool replaceCommandLineText(const QString& name, const QString& text) override;
    bool appendCommandLineText(const QString& name, const QString& text) override;
    bool clearCommandLine(const QString& name) override;
    bool selectCommandLineText(const QString& name) override;
    bool addCommandLineSuggestion(const QString& name, const QString& word) override;
    bool removeCommandLineSuggestion(const QString& name, const QString& word) override;
    bool clearCommandLineSuggestions(const QString& name) override;
    bool addCommandLineBlacklistWord(const QString& name, const QString& word) override;
    bool removeCommandLineBlacklistWord(const QString& name, const QString& word) override;
    bool clearCommandLineBlacklist(const QString& name) override;
    bool addCommandLineMenuItem(const QString& name, const QString& label, const QString& eventName) override;
    std::optional<bool> removeCommandLineMenuItem(const QString& name, const QString& label) override;
    bool setCommandLineSavesHistory(const QString& name, bool savesHistory) override;
    bool setCommandLineVisible(const QString& name, bool visible) override;
    // Also used by Host to announce a log change for a view not yet built
    static QString loggingAnnouncementText(const bool isLogging, const QString& logFileName);
    bool setWindowScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowHorizontalScrollBarVisible(const QString& name, bool visible) override;
    bool setWindowScrolling(const QString& name, bool enabled) override;
    bool scrollWindowTo(const QString& name, int line, bool toEnd) override;
    std::optional<std::pair<bool, QString>> setWindowFontFamily(const QString& name, const QString& family, QFont::Weight weight) override;
    bool setWindowFontSize(const QString& name, int size) override;
    bool setWindowCommandLineVisible(const QString& name, bool visible) override;
    TTextBox* textBoxWidget(const QString& name) const { return mTextBoxMap.value(name); }
    bool setTextBoxText(const QString& name, const QString& text) override;
    bool clearTextBox(const QString& name) override;
    bool setTextBoxReadOnly(const QString& name, bool readOnly) override;
    bool setTextBoxPlaceholder(const QString& name, const QString& text) override;
    bool setTextBoxStyleSheet(const QString& name, const QString& styleSheet) override;
    bool setTextBoxFont(const QString& name, const QFont& font) override;
    bool setTextBoxTabMovesFocus(const QString& name, bool tabMovesFocus) override;
    QPoint mousePosition() const override;
    bool hasKeyboardFocus() const override;
    bool showPlainWindow(const QString& name) override;
    bool hidePlainWindow(const QString& name) override;
    bool resizePlainWindow(const QString& name, int width, int height) override;
    bool movePlainWindow(const QString& name, int x, int y) override;
    bool setCommandLineAction(const QString& name, const int func) override;
    bool resetCommandLineAction(const QString& name) override;
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
    void createMapperDock(const QString& title, const QString& objectName) override;
    void showNewMapperDock() override;
    std::pair<bool, QString> placeMapWidget(const QString& area, int x, int y, int width, int height) override;
    bool mapWidgetCreated() const override;
    bool setMapWidgetTitle(const QString& title) override;
    std::optional<QString> mapWidgetTitle() const override;
    std::optional<QRect> mapWidgetGeometry() const override;
    bool hideMapWidget() override;
    void showLoadedMap() override;
    void showMapAfterFailedLoad() override;
    void showMapAtPlayerArea() override;
    bool mapperShown() const override;
    void setMapperShown(bool shown) override;
    void setMapperPanelVisible(bool visible) override;
    void setMapLargeAreaExitArrows(bool enabled) override;
    void requestMapRepaint() override;
    void requestRepaintAfterCommand() override;
    void regenerateToolBars(const std::list<TAction*>& rootActions) override;
    void regenerateEasyButtonBars(const std::list<TAction*>& rootActions) override;
    void detachActionBars(TAction* pAction) override;
    // Each action's bars, looked up by the action. An action shown as a menu on
    // another's bar (on a floating toolbar, any entry of such a menu) is
    // recorded against that bar too, so what is asked of its bar here is done
    // to the whole bar.
    bool hasEasyButtonBar(TAction* pAction) const override;
    TToolBar* actionToolBar(TAction* pAction) const;
    TEasyButtonBar* actionEasyButtonBar(TAction* pAction) const;
    void setActionToolBar(TAction* pAction, TToolBar* pToolBar);
    void setActionEasyButtonBar(TAction* pAction, TEasyButtonBar* pBar);
    void releaseParentActionBars(TAction* pOldParent, TAction* pChild) override;
    void renameActionToolBar(TAction* pAction, const QString& name) override;
    void setActionToolBarVisible(TAction* pAction, bool visible);
    void hideActionEasyButtonBar(TAction* pAction);
    // The button and the menu entry a bar draws an action as; the one each
    // replaces is deleted later.
    void replaceActionButton(TAction* pAction, TFlipButton* pButton);
    void replaceActionMenuEntry(TAction* pAction, EAction* pEntry);
    void setActionButtonChecked(TAction* pAction, bool checked);
    // Floating toolbars are the main window's children rather than this
    // console's, so the profile has to delete them itself.
    const std::list<QPointer<TToolBar>>& actionToolBars() const { return mToolBarList; }
    void deleteActionToolBars() override;
    void deleteActionToolBarsLater() override;
    // A floating toolbar that has moved or been resized since the layout was
    // last saved; committing clears the flags and answers whether any was raised.
    void setToolBarLayoutChanged(TToolBar* pToolBar);
    bool commitToolBarLayoutChanges() override;
    void discardToolBarLayoutChanges() override;
    void showMapperScriptReminder();
    void showUnpackingProgress(const QString& message, const QString& title);
    void closeUnpackingProgress();
    void setupVideoOutput(TMediaPlayer* player, bool& setupSucceeded);
    void hideVideoOutput(TMediaPlayer* player);
    void toggleLogging(bool);
    bool startIncomingText() override;
    void alertNewData() override;
    void finishIncomingText() override;
    void finalize() override;
    void refreshSubconsoles();


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
    void slot_refreshLatencyBox();


signals:
    // Raised when new data is incoming to trigger Alert handling in mudlet
    // class, second argument is true for a lower priority indication when
    // locally produced information is painted into main console
    void signal_newDataAlert(const QString&, bool isLowerPriorityChange = false);


private:
    dlgMapper* dockedMapper() const;
    void dockMapWidget(Qt::DockWidgetArea area);
    TDockWidget* createUserWindow(const QString& name);
    TToolBar* createToolBar(TAction* pAction, const QString& name);
    TEasyButtonBar* createEasyButtonBar(TAction* pRootAction, const QString& name);
    void attachEasyButtonBar(TEasyButtonBar* pBar, int location);
    void detachEasyButtonBar(TEasyButtonBar* pBar, int location);
    void dockToolBar(TToolBar* pToolBar, Qt::DockWidgetArea area);
    void undockToolBar(TToolBar* pToolBar);
    void constructToolbar(TAction* pAction, TToolBar* pToolBar);
    void constructToolbar(TAction* pA, TEasyButtonBar* pTB);
    struct ActionBars
    {
        QPointer<TToolBar> mpToolBar;
        QPointer<TEasyButtonBar> mpEasyButtonBar;
        QPointer<TFlipButton> mpButton;
        QPointer<EAction> mpMenuEntry;
    };
    ActionBars& actionBarsFor(TAction* pAction);
    TFlipButton* actionButton(TAction* pAction) const;
    EAction* actionMenuEntry(TAction* pAction) const;

    // The latency box repaints on every setText(), so a flood of packets is
    // shown at most once per pace interval - the same cap the panes paint at.
    static constexpr int csmLatencyBoxPaceMs = 16;
    QTimer* mpLatencyBoxPacer = nullptr;
    double mLatencyProcessT = 0.0;

    void createMapProgressDialog(const QString& title, const QString& label, const QString& cancelButtonText, int minimum, int maximum);
    // Shared by reparentLabel() and reparentWindow() so they agree on what "main" means.
    QWidget* parentWidgetFor(const QString& windowname) const;
    // Resolves the three name-only kinds in the same order as the core.
    QWidget* plainWindowWidget(const QString& name) const;
    void watchWindowState(const QString& name, QWidget* pWidget);
    void reportWindowState(const QStringList& names);
    std::pair<bool, QString> placeUserWindow(const QString& name, bool loadLayout, bool autoDock, const QString& area);
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
    TCommandLine* activeCommandLine();
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
    QStack<QPointer<TCommandLine>> mLastCommandLineUsed;
    QMap<QString, TTextBox*> mTextBoxMap;
    QMap<QString, TScrollBox*> mScrollBoxMap;

    // An entry lasts as long as its action, so an address a later action is
    // given never inherits its bars.
    QHash<TAction*, ActionBars> mActionBars;
    std::list<QPointer<TToolBar>> mToolBarList;
    std::list<QPointer<TEasyButtonBar>> mEasyButtonBarList;
    QList<QPointer<TToolBar>> mToolBarLayoutChanges;

    bool mEnableClose = false;
    std::unique_ptr<TMxpFrameWidgets> mpMxpFrameWidgets;
};

#endif // MUDLET_TMAINCONSOLE_H

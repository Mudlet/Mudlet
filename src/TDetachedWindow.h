#ifndef TDETACHEDWINDOW_H
#define TDETACHEDWINDOW_H

/***************************************************************************
 *   Copyright (C) 2025 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QMainWindow>
#include <QVBoxLayout>
#include <QPointer>
#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <QLabel>
#include <QMap>
#include <QStackedWidget>
#include <functional>

class TMainConsole;
class Host;
class TTabBar;

class TDetachedWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TDetachedWindow(const QString& profileName, TMainConsole* console, QWidget* parent = nullptr);
    ~TDetachedWindow();

    // Multiple profile support
    bool addProfile(const QString& profileName, TMainConsole* console);
    bool removeProfile(const QString& profileName);
    QStringList getProfileNames() const;
    QString getCurrentProfileName() const;
    TMainConsole* getCurrentConsole() const;
    TMainConsole* getConsole(const QString& profileName) const;
    int getProfileCount() const { return mProfileConsoleMap.size(); }
    
    void updateToolbarForProfile(Host* pHost);
    void setReattaching(bool reattaching) { mIsReattaching = reattaching; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void changeEvent(QEvent* event) override;

signals:
    void windowClosed(const QString& profileName);
    void reattachRequested(const QString& profileName);
    void profileDetachToWindowRequested(const QString& profileName, TDetachedWindow* targetWindow);

private slots:
    void onReattachAction();
    void showTabContextMenu(const QPoint& position);
    void closeProfile();
    void closeProfileByIndex(int index);
    void slot_tabChanged(int index);
    void slot_toggleFullScreenView();
    void slot_toggleAlwaysOnTop();
    void slot_saveProfile();
    void slot_exportProfile();
    
    // Detached window specific toolbar action slots
    void slot_connectProfile();
    void slot_disconnectProfile();
    void slot_reconnectProfile();
    void slot_closeCurrentProfile();
    void slot_showTriggerDialog();
    void slot_showAliasDialog();
    void slot_showTimerDialog();
    void slot_showActionDialog();
    void slot_showScriptDialog();
    void slot_showKeyDialog();
    void slot_showVariableDialog();
    void slot_showMapperDialog();
    void slot_showHelpDialog();
    void slot_showPreferencesDialog();
    void slot_showNotesDialog();
    void slot_showReplayDialog();
    void slot_showPackageManagerDialog();
    void slot_showModuleManagerDialog();
    void slot_showPackageExporterDialog();
    void slot_muteMedia();
    void slot_muteAPI();
    void slot_muteGame();

private:
    void setupUI();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void connectToolBarActions();
    void updateToolBarActions();
    void updateWindowTitle();
    void updateStatusBar(Host* pHost, bool isConnected, bool isConnecting);
    void updateTabIndicator(int tabIndex = -1);  // -1 means current tab
    void saveWindowGeometry();
    void restoreWindowGeometry();
    void switchToProfile(const QString& profileName);
    void checkForWindowMergeOpportunity();  // Check if this window overlaps with another detached window
    void performWindowMerge(TDetachedWindow* otherWindow);  // Automatically merge with another window
    void logWindowState(const QString& context);  // Debug method to track window state
    
    // Helper method to temporarily set the active host for actions
    void withCurrentProfileActive(const std::function<void()>& action);

    // Multiple profile data
    QMap<QString, QPointer<TMainConsole>> mProfileConsoleMap;
    QString mCurrentProfileName;
    QStackedWidget* mpConsoleContainer{nullptr};
    QVBoxLayout* mpMainLayout{nullptr};
    TTabBar* mpTabBar{nullptr};
    QToolBar* mpToolBar{nullptr};
    
    // Toolbar actions - mirroring main window
    QAction* mpActionConnect{nullptr};
    QAction* mpActionDisconnect{nullptr};
    QAction* mpActionReconnect{nullptr};
    QAction* mpActionCloseProfile{nullptr};
    QAction* mpActionTriggers{nullptr};
    QAction* mpActionAliases{nullptr};
    QAction* mpActionTimers{nullptr};
    QAction* mpActionButtons{nullptr};
    QAction* mpActionScripts{nullptr};
    QAction* mpActionKeys{nullptr};
    QAction* mpActionVariables{nullptr};
    QAction* mpActionMapper{nullptr};
    QAction* mpActionHelp{nullptr};
    QAction* mpActionOptions{nullptr};
    QAction* mpActionNotes{nullptr};
    QAction* mpActionReplay{nullptr};
    QAction* mpActionPackageManager{nullptr};
    QAction* mpActionModuleManager{nullptr};
    QAction* mpActionPackageExporter{nullptr};
    QAction* mpActionMuteMedia{nullptr};
    QAction* mpActionMuteAPI{nullptr};
    QAction* mpActionMuteGame{nullptr};
    QAction* mpActionFullScreenView{nullptr};
    
    // Toolbar buttons
    QToolButton* mpButtonConnect{nullptr};
    QToolButton* mpButtonMute{nullptr};
    QToolButton* mpButtonPackageManagers{nullptr};
    
    // Status bar
    QLabel* mpStatusBarConnectionLabel{nullptr};
    QLabel* mpStatusBarProfileLabel{nullptr};
    
    // Store window state
    QPoint mLastPosition;
    QSize mLastSize;
    
    // Flag to indicate if window is being closed due to reattachment
    bool mIsReattaching{false};
    
    // Flag to force window to stay visible when it has profiles
    bool mShouldStayVisible{true};
    
    // Flag to track if window is being minimized (to allow hiding during minimize)
    bool mIsBeingMinimized{false};
    
    // Flag to allow hiding during window flag changes (always on top, etc.)
    bool mIsChangingWindowFlags{false};
    
    // Flag to prevent duplicate reattach operations on this window
    bool mReattachInProgress{false};
};

#endif // TDETACHEDWINDOW_H

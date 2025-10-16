#ifndef MUDLET_DLGPROFILEPREFERENCES_H
#define MUDLET_DLGPROFILEPREFERENCES_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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


#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TMedia.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include "pre_guard.h"
#include "ui_profile_preferences.h"
#include <QtCore>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFontDialog>
#include <QMap>
#include <QCloseEvent>
#include "post_guard.h"

class Host;


class dlgProfilePreferences : public QDialog, public Ui::profile_preferences
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgProfilePreferences)
    explicit dlgProfilePreferences(QWidget*, Host* pHost = nullptr);
    void setTab(QString tab);

public slots:
    // Terminal colors.
    void slot_setColorBlack();
    void slot_setColorLightBlack();
    void slot_setColorRed();
    void slot_setColorLightRed();
    void slot_setColorBlue();
    void slot_setColorLightBlue();
    void slot_setColorGreen();
    void slot_setColorLightGreen();
    void slot_setColorYellow();
    void slot_setColorLightYellow();
    void slot_setColorCyan();
    void slot_setColorLightCyan();
    void slot_setColorMagenta();
    void slot_setColorLightMagenta();
    void slot_setColorWhite();
    void slot_setColorLightWhite();
    void slot_setFgColor();
    void slot_setBgColor();
    void slot_setCommandLineBgColor();
    void slot_setCommandLineFgColor();
    void slot_setCommandBgColor();
    void slot_setCommandFgColor();
    void slot_resetColors();

    // Mapper colors.
    void slot_setMapColorBlack();
    void slot_setMapColorLightBlack();
    void slot_setMapColorRed();
    void slot_setMapColorLightRed();
    void slot_setMapColorBlue();
    void slot_setMapColorLightBlue();
    void slot_setMapColorGreen();
    void slot_setMapColorLightGreen();
    void slot_setMapColorYellow();
    void slot_setMapColorLightYellow();
    void slot_setMapColorCyan();
    void slot_setMapColorLightCyan();
    void slot_setMapColorMagenta();
    void slot_setMapColorLightMagenta();
    void slot_setMapColorWhite();
    void slot_setMapColorLightWhite();
    void slot_setMapExitsColor();
    void slot_setMapBgColor();
    void slot_setMapRoomBorderColor();
    void slot_setMapInfoBgColor();
    void slot_setMapRoomCollisionBorderColor();
    void slot_setLowerLevelColor();
    void slot_setUpperLevelColor();
    void slot_resetMapColors();

    // Map.
    void slot_downloadMap();
    void slot_loadMap();
    void slot_saveMap();
    void slot_copyMap();
    void slot_chosenProfilesChanged(QAction*);
    void slot_showMapGlyphUsage();

    // Media
    void slot_purgeMediaCache();
    void slot_toggleEnableClosedCaption(const bool);

    // Log.
    void slot_setLogDir();
    void slot_resetLogDir();
    void slot_logFileNameFormatChange(int index);
    void slot_changeLogFileAsHtml(bool isHtml);

    // Save.
    void slot_saveAndClose();

    void slot_hideActionLabel();
    void slot_setEncoding(const int);

    void slot_handleHostAddition(Host*, quint8);
    void slot_handleHostDeletion(Host*);

    void slot_guiLanguageChanged(const QString&);

private slots:
    void slot_changeShowSpacesAndTabs(bool);
    void slot_changeShowLineFeedsAndParagraphs(bool);
    void slot_scriptSelected(int index);
    void slot_tabChanged(int tabIndex);
    void slot_themeSelected(int index);
    void slot_setMapSymbolFont(const QFont&);
    void slot_setMapSymbolFontStrategy(bool);
    void slot_changeShowMenuBar(int);
    void slot_changeShowToolBar(int);
    void slot_changeEditorTextOptions(const QTextOption::Flags);
    void slot_setAppearance(const enums::Appearance);
    void slot_changeShowMapAuditErrors(const bool);
    void slot_changeAutomaticUpdates(const bool);
    void slot_setToolBarIconSize(const int);
    void slot_setTreeWidgetIconSize(const int);
    void slot_changeMenuBarVisibility(const enums::controlsVisibility);
    void slot_changeToolBarVisibility(const enums::controlsVisibility);
    void slot_changeShowIconsOnMenus(const Qt::CheckState);
    void slot_changeGuiLanguage(int);
    void slot_passwordStorageLocationChanged(int);
    void slot_changePlayerRoomStyle(const int);
    void slot_setPlayerRoomPrimaryColor();
    void slot_setPlayerRoomSecondaryColor();
    void slot_setPlayerRoomOuterDiameter(const int);
    void slot_setPlayerRoomInnerDiameter(const int);
    void slot_setPostingTimeout(const double);
    void slot_changeControlCharacterHandling();
    void slot_enableDarkEditor(const QString&);
    void slot_toggleAdvertiseScreenReader(const bool);
    void slot_changeWrapAt();
    void slot_toggleUseMaxBufferSize(bool checked);
    void slot_deleteMap();
    void slot_changeLargeAreaExitArrows(const bool);
    void slot_changeInvertMapZoom(const bool);
    void slot_hidePasswordMigrationLabel();
    void slot_loadHistoryMap();
    void slot_displayFontChanged();
    void slot_displayFontSizeChanged();
    void slot_displayFontAliasingChanged();
    void slot_changeShowTabConnectionIndicators(bool state);


signals:
    void signal_themeUpdateCompleted();
    void signal_preferencesSaved();
    void signal_resetMainWindowShortcutsToDefaults();
    void preferencesClosing(const QString& profileName);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setColors();
    void setColors2();
    void setButtonAndProfileColor(QPushButton*, QColor&, bool allowAlpha = false);
    void setPlayerRoomColor(QPushButton*, QColor&);
    void setButtonColor(QPushButton*, const QColor&, const bool hasAlpha = false);
    void loadEditorTab();
    void populateThemesList();
    void populateScriptsList();
    void addTriggersToPreview(TTrigger* pTriggerParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addAliasesToPreview(TAlias* pAliasParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addTimersToPreview(TTimer* pTimerParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addActionsToPreview(TAction* pActionParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addScriptsToPreview(TScript* pScriptParent, std::vector<std::tuple<QString, QString, int>>& items);
    void addKeysToPreview(TKey* pKeyParent, std::vector<std::tuple<QString, QString, int>>& items);
    void initWithHost(Host*);
    void disableHostDetails();
    void enableHostDetails();
    void clearHostDetails();
    void disconnectHostRelatedControls();
    void generateMapGlyphDisplay();
    void generateDiscordTooltips();
    void setupPasswordsMigration();
    QString mapSaveLoadDirectory(Host* pHost);
    void loadMap(const QString&);
    void fillOutMapHistory();
    bool updateDisplayFont();
    void cancelShortcutCaptures();


    QPointer<Host> mpHost;
    QPointer<QTemporaryFile> tempThemesArchive;
    QMap<QString, QString> mSearchEngineMap;
    QPointer<QMenu> mpMenu;
    QPointer<QDialog> mpDialogMapGlyphUsage;
    QPointer<QDoubleSpinBox> mpDoubleSpinBox_mapSymbolFontFudge;
    std::unique_ptr<QTimer> hidePasswordMigrationLabelTimer;
    QMap<QString, QKeySequence*> currentShortcuts;
    QPointer<QMenu> protocolMenu;
    QPointer<QAction> mEnableGMCP;
    QPointer<QAction> mEnableMSDP;
    QPointer<QAction> mEnableMSSP;
    QPointer<QAction> mEnableMSP;
    QPointer<QAction> mEnableMXP;
    QPointer<QAction> mEnableMTTS;
    QPointer<QAction> mEnableMNES;

    QString mLogDirPath;
    // Needed to remember the state on construction so that we can sent the same
    // flag back for Host::mUseSharedDictionary even if we turn-off
    // Host::mEnableUserDictionary: - although, following review THAT has been
    // disallowed...
    bool mUseSharedDictionary = false;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H

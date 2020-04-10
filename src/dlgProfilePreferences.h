#ifndef MUDLET_DLGPROFILEPREFERENCES_H
#define MUDLET_DLGPROFILEPREFERENCES_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include "pre_guard.h"
#include "ui_profile_preferences.h"
#include <QtCore>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QMap>
#include "post_guard.h"

class Host;


class dlgProfilePreferences : public QDialog, public Ui::profile_preferences
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgProfilePreferences)
    dlgProfilePreferences(QWidget*, Host* pHost = nullptr);
    void setTab(QString tab);

public slots:
    // Fonts.
    void setFontSize();
    void setDisplayFont();
    void setCommandLineFont();

    // Terminal colors.
    void setColorBlack();
    void setColorLightBlack();
    void setColorRed();
    void setColorLightRed();
    void setColorBlue();
    void setColorLightBlue();
    void setColorGreen();
    void setColorLightGreen();
    void setColorYellow();
    void setColorLightYellow();
    void setColorCyan();
    void setColorLightCyan();
    void setColorMagenta();
    void setColorLightMagenta();
    void setColorWhite();
    void setColorLightWhite();
    void setFgColor();
    void setBgColor();
    void setCommandLineBgColor();
    void setCommandLineFgColor();
    void setCommandFgColor();
    void setCommandBgColor();
    void resetColors();

    // Mapper colors.
    void setColorBlack2();
    void setColorLightBlack2();
    void setColorRed2();
    void setColorLightRed2();
    void setColorBlue2();
    void setColorLightBlue2();
    void setColorGreen2();
    void setColorLightGreen2();
    void setColorYellow2();
    void setColorLightYellow2();
    void setColorCyan2();
    void setColorLightCyan2();
    void setColorMagenta2();
    void setColorLightMagenta2();
    void setColorWhite2();
    void setColorLightWhite2();
    void setFgColor2();
    void setBgColor2();
    void resetColors2();

    // Map.
    void downloadMap();
    void loadMap();
    void saveMap();
    void copyMap();
    void slot_chooseProfilesChanged(QAction*);
    void slot_showMapGlyphUsage();


    // Log.
    void slot_setLogDir();
    void slot_resetLogDir();
    void slot_logFileNameFormatChange(int index);
    void slot_changeLogFileAsHtml(bool isHtml);

    // Save.
    void slot_save_and_exit();

    void hideActionLabel();
    void slot_setEncoding(const int);

    void slot_handleHostAddition(Host*, quint8);
    void slot_handleHostDeletion(Host*);

    void slot_guiLanguageChanged(const QString&);

private slots:
    void slot_changeShowSpacesAndTabs(bool);
    void slot_changeShowLineFeedsAndParagraphs(bool);
    void slot_script_selected(int index);
    void slot_editor_tab_selected(int tabIndex);
    void slot_theme_selected(int index);
    void slot_setMapSymbolFont(const QFont&);
    void slot_setMapSymbolFontStrategy(bool);
    void slot_changeShowMenuBar(int);
    void slot_changeShowToolBar(int);
    void slot_changeEditorTextOptions(const QTextOption::Flags);
    void slot_changeEnableFullScreenMode(const bool);
    void slot_changeShowMapAuditErrors(const bool);
    void slot_changeAutomaticUpdates(const bool);
    void slot_setToolBarIconSize(const int);
    void slot_setTreeWidgetIconSize(const int);
    void slot_changeMenuBarVisibility(const mudlet::controlsVisibility);
    void slot_changeToolBarVisibility(const mudlet::controlsVisibility);
    void slot_changeShowIconsOnMenus(const Qt::CheckState);
    void slot_changeGuiLanguage(const QString&);
    void slot_passwords_location_changed(int);
    void slot_changePlayerRoomStyle(const int);
    void slot_setPlayerRoomPrimaryColor();
    void slot_setPlayerRoomSecondaryColor();
    void slot_setPlayerRoomOuterDiameter(const int);
    void slot_setPlayerRoomInnerDiameter(const int);

private:
    void setColors();
    void setColors2();
    void setColor(QPushButton*, QColor&);
    void setPlayerRoomColor(QPushButton*, QColor&);
    void setButtonColor(QPushButton*, const QColor&);
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
    void hidePasswordMigrationLabel();
    void setupPasswordsMigration();

    int mFontSize;
    QPointer<Host> mpHost;
    QPointer<QTemporaryFile> tempThemesArchive;
    QMap<QString, QString> mSearchEngineMap;
    QPointer<QMenu> mpMenu;
    QPointer<QDialog> mpDialogMapGlyphUsage;
    QPointer<QDoubleSpinBox> mpDoubleSpinBox_mapSymbolFontFudge;
    std::unique_ptr<QTimer> hidePasswordMigrationLabelTimer;

    QString mLogDirPath;
    // Needed to remember the state on construction so that we can sent the same
    // flag back for Host::mUseSharedDictionary even if we turn-off
    // Host::mEnableUserDictionary: - although, following review THAT has been
    // disallowed...
    bool mUseSharedDictionary;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H

#ifndef MUDLET_DLGPACKAGEMANAGER_H
#define MUDLET_DLGPACKAGEMANAGER_H

/***************************************************************************
 *   Copyright (C) 2011 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "PackageItemDelegate.h"

#include "ui_package_manager.h"
#include <QButtonGroup>
#include <QDialog>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QTextBrowser>

class Host;
class QAction;
class QHBoxLayout;
class QEvent;
class QMenu;
class QNetworkReply;
class QPushButton;
class QTabBar;
class dlgSystemMessageArea;

namespace uiDesign {
class GripSplitter;
}

enum class NavigationView { Explore, Installed, Updates };

class dlgPackageManager : public QDialog, public Ui::package_manager
{
    Q_OBJECT

    friend class PackageManagerShellTest;
    friend class PackageManagerRepositoryLinksTest;

public:
    Q_DISABLE_COPY(dlgPackageManager)
    explicit dlgPackageManager(QWidget* parent, Host*);
    bool readPackageRepositoryFile();
    void resetPackageList();
    QString removePackages(const QStringList& packageNames);

signals:
    void packageManagerClosing(const QString& profileName);

private slots:
    void slot_applyAppearance();
    void slot_installPackageFromFile();
    void slot_installPackageFromRepository();
    void slot_itemChanged(QListWidgetItem*);
    void slot_navigationButtonClicked(int buttonId);
    void slot_openBugWebsite();
    void slot_packageContextMenu(const QPoint& pos);
    void slot_openPackageWebsite();
    void slot_onIconDownloaded(QNetworkReply* reply);
    void slot_removePackages();
    void slot_searchTextChanged(const QString& searchText);
    void slot_setPackageList();
    void slot_toggleInstallRepoButton();
    void slot_toggleRemoveButton();

private:
    void applyPackageManagerShellStyle();
    void buildShell();
    // Split from the slot so a test can read the entries without running exec().
    // The caller owns the menu.
    QMenu* buildPackageContextMenu(QListWidgetItem* pItem);
    void clearPackageDetails();
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void restoreColumnSplit();
    void downloadIcon(const QString& packageName);
    void downloadRepositoryIndex();
    void fillPackageDetails(const QString& name, const QString& title, const QString& author, const QString& version);
    QListWidgetItem* installedPackageItem(const QString& packageName) const;
    bool installedPackageMatches(const QString& packageName, const QString& searchText) const;
    bool hasNewerVersion(const QString& installed, const QString& repo) const;
    QString updateVersionText(const QString& installedVersion, const QString& repoVersion) const;
    // Empty when either version is missing, rather than a dangling arrow
    QString updateRowVersion(const QString& packageName) const;
    static void setRowVersion(QListWidgetItem* pItem, const QString& version);
    bool matchesRepositoryPackage(const QString& packageName) const;
    QString packageHelpUrl(const QString& packageName) const;
    QString packageWebsiteUrl(const QString& packageName) const;
    void comeBackToFront();
    void scheduleHeadlineFit();
    void measureActionRowButtons();
    int actionRowWordedNeed() const;
    void fitActionRowToItsColumn();
    void scheduleActionRowFit();
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
    void populatePackagesWithUpdates();
    void setupNavigationButtons();
    void showDescription(const QString& markdown);
    void showPlainDescription(const QString& text);
    void relayDescription();
    void showPackageIcon(const QPixmap& face);
    void showPackagePlaceholder();
    // Keeps the whole name in mPackageNameShown, so a later re-elision never
    // eats further into a name that has already been cut
    void showPackageName(const QString& name);
    void showImportStatus(const QString& message);
    void showMissingFromIndex(const QString& packageName);
    void syncViewBar();
    void updateUpdatesBadge();

    Host* mpHost = nullptr;
    QButtonGroup* mpNavigationGroup = nullptr;
    NavigationView mCurrentView = NavigationView::Installed;
    QList<QString> mPackagesWithUpdates;
    PackageItemDelegate* mpPackageItemDelegate = nullptr;
    QHash<QString, QJsonObject> packageLookup;
    QJsonArray repositoryPackages;

    QTabBar* mpViewBar = nullptr;
    uiDesign::GripSplitter* mpSplitter = nullptr;
    bool mSplitterRestored = false;
    QHBoxLayout* mpLayout_headlineRow = nullptr;
    QString mPackageNameShown;
    struct ActionRowLink
    {
        QPushButton* pButton = nullptr;
        QString word;
        int worded = 0;
        int glyphOnly = 0;
    };
    QList<ActionRowLink> mActionRowLinks;
    bool mActionRowWordsShown = true;
    bool mActionRowFitting = false;
    bool mActionRowFitPending = false;
    dlgSystemMessageArea* mpNotice = nullptr;
    QAction* mpAction_searchGlyph = nullptr;
    QIcon mInstallGlyph;
    QIcon mUpdateGlyph;
    QString mDescription;
    bool mDescriptionIsMarkdown = true;
};

#endif

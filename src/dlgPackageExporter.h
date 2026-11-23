#ifndef MUDLET_DLGPACKAGEEXPORTER_H
#define MUDLET_DLGPACKAGEEXPORTER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019-2020, 2022 by Stephen Lyons                        *
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


#include "uiDesign.h"

#include <QDialog>
#include <QFileInfo>
#include <QPointer>
#include <QTextEdit>
#include <zip.h>

#include <atomic>
#include <memory>

class ExporterTreeDelegate;
class Host;
class QCloseEvent;
class QDropEvent;
class QGroupBox;
template <typename T>
class QFutureWatcher;
class QLabel;
class QResizeEvent;
class QShowEvent;
class TAction;
class TAlias;
class TKey;
class TScript;
class TTimer;
class TTrigger;
class dlgSystemMessageArea;

namespace uiDesign {
class ChipRow;
class GripSplitter;
}

#if defined(LIBZIP_VERSION_MAJOR) && defined(LIBZIP_VERSION_MINOR) && ((LIBZIP_VERSION_MAJOR > 1) || (LIBZIP_VERSION_MAJOR == 1) && (LIBZIP_VERSION_MINOR >= 7))
// libzip 1.7.0 supports cancelling archiving in progress
#define LIBZIP_SUPPORTS_CANCELLING TRUE
#endif

class QTreeWidget;
class QTreeWidgetItem;

namespace Ui {
class dlgPackageExporter;
}

class dlgPackageExporter : public QDialog
{
    Q_OBJECT

    friend class PackageExporterShellTest;

public:
    Q_DISABLE_COPY(dlgPackageExporter)
    explicit dlgPackageExporter(QWidget* parent, Host*);
    ~dlgPackageExporter();

    // Methods to preselect items when opened from trigger editor
    void preselectTrigger(QTreeWidgetItem* item);
    void preselectTimer(QTreeWidgetItem* item);
    void preselectAlias(QTreeWidgetItem* item);
    void preselectScript(QTreeWidgetItem* item);
    void preselectAction(QTreeWidgetItem* item);
    void preselectKey(QTreeWidgetItem* item);

    // Set module creation mode
    void setModuleCreationMode(bool isModule);
    void recurseTree(QTreeWidgetItem*, QList<QTreeWidgetItem*>&);
    void listTriggers();
    void recurseTriggers(TTrigger*, QTreeWidgetItem*);
    void listAliases();
    void recurseAliases(TAlias*, QTreeWidgetItem*);
    void listScripts();
    void recurseScripts(TScript*, QTreeWidgetItem*);
    void listKeys();
    void recurseKeys(TKey*, QTreeWidgetItem*);
    void listActions();
    void recurseActions(TAction*, QTreeWidgetItem*);
    void listTimers();
    void recurseTimers(TTimer*, QTreeWidgetItem*);
    TTrigger* triggerOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    TTimer* timerOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    TAlias* aliasOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    TAction* actionOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    TScript* scriptOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    TKey* keyOf(const QMap<QTreeWidgetItem*, int>&, QTreeWidgetItem*) const;
    static void copy_directory(const QString& fromDir, const QString& toDir, bool overwrite);
    void showRenderedDescription();
    QMap<QTreeWidgetItem*, int> triggerMap;
    QMap<QTreeWidgetItem*, int> modTriggerMap;
    QMap<QTreeWidgetItem*, int> aliasMap;
    QMap<QTreeWidgetItem*, int> modAliasMap;
    QMap<QTreeWidgetItem*, int> scriptMap;
    QMap<QTreeWidgetItem*, int> modScriptMap;
    QMap<QTreeWidgetItem*, int> keyMap;
    QMap<QTreeWidgetItem*, int> modKeyMap;
    QMap<QTreeWidgetItem*, int> actionMap;
    QMap<QTreeWidgetItem*, int> modActionMap;
    QMap<QTreeWidgetItem*, int> timerMap;
    QMap<QTreeWidgetItem*, int> modTimerMap;
    // This will hold the absolute pathFileName for the XML file that will
    // contain the Mudlet items to go into the package:
    QString mXmlPathFileName;
    QString mPlainDescription;
    QStringList mDescriptionImages;

    // Module creation mode flag
    bool mIsModuleCreationMode = false;

public slots:
    void slot_addFiles();
    void slot_exportPackage();
    void done(int result) override;

private slots:
    void slot_addDependency();
    void slot_removeDependency();
    void slot_importIcon();
    void slot_removeIcon();
    void slot_openPackageLocation();
    void slot_packageChanged(int);
    void slot_updateLocationPlaceholder();
    void slot_recountItems(QTreeWidgetItem* item);
    void slot_rightClickOnItems(const QPoint& point);
    void slot_cancelExport();
    void slot_applyAppearance();
    void slot_dependencyChipsChanged();

protected:
    bool eventFilter(QObject* obj, QEvent* evt) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildShell();
    void buildSelectionColumn();
    void buildDetailsColumn();
    void buildFoot();
    void buildIconRow();
    void buildAssetsRow();
    void applyPackageExporterShellStyle();
    void restoreColumnSplit();
    void showSelectionCount(const int itemsToExport);
    void showModeWords();
    void syncDependencyChips();
    void refreshDependencySuggestions();
    void showNotice(const QString& richText, const uiDesign::NoticeKind kind);
    void fitSavePath();
    void fitAssetsList();
    void restyleTreeIcons(const uiDesign::ThemeTokens& tokens);
    void showIconPlaceholder(const uiDesign::ThemeTokens& tokens);
    void showChosenIcon();
    void showAssetsPrompt();
    // Leaves out the export folder: that is a preference, not something filled in here
    bool formIsFresh() const;
    // Words loaded from a package do not count: only words typed here are lost for good
    bool formHasTypedWords() const;
    void startOver();
    // Callers go through here, not startOver(): it asks first when typed words would be lost
    void requestStartOver();
    void refreshStartOverButton();
    void appendToDetails(const QString&, const QString&);
    void displayResultMessage(const QString&, const bool isSuccessMessage = true);
    void uncheckAllChildren();
    int countRecursive(QTreeWidgetItem* item, int count) const;
    int countCheckedItems() const;
    void checkChildren(QTreeWidgetItem* item) const;
    // A unit-root folder named after the installed package picked to start from: XMLimport makes it again on
    // install, so it is not exported or counted itself unless a right-click puts it in whole
    bool packageFolder(const QTreeWidgetItem* pItem) const;
    void refreshFolderMarks();
    // Returns whether anything that is exported is ticked at or under pItem
    bool refreshFolderMark(QTreeWidgetItem* pItem);
    QString getActualPath() const;
    static const int isTopFolder = 1;

public:
    // On a package's own folder a right-click put in whole
    static constexpr int scmForcedInRole = Qt::UserRole + 1;

private:
    static std::pair<bool, QString> writeFileToZip(const QString& archiveFileName, const QString& fileSystemFileName, zip* archive);
    static std::pair<bool, QString> zipPackage(const QString& stagingDirName,
                                               const QString& packagePathFileName,
                                               const QString& xmlPathFileName,
                                               const QString& packageName,
                                               const QString& packageComment,
                                               std::shared_ptr<std::atomic<bool>> cancelled);
    static std::pair<bool, QString> copyAssetsToTmp(const QStringList& assetPaths, const QString& tempPath);
    QFileInfo copyIconToTmp(const QString& tempPath) const;
    void copyPackageDataToTmp(const QString& tempPath) const;
    QString normalizedHelpUrl() const;
    void writeConfigFile(const QString& stagingDirName, const QFileInfo& iconFile, const QString& packageDescription);
    void exportXml(bool& isOk,
                   QList<QTreeWidgetItem*>& trigList,
                   QList<QTreeWidgetItem*>& timerList,
                   QList<QTreeWidgetItem*>& aliasList,
                   QList<QTreeWidgetItem*>& actionList,
                   QList<QTreeWidgetItem*>& scriptList,
                   QList<QTreeWidgetItem*>& keyList);
    void markExportItems(QList<QTreeWidgetItem*>& trigList,
                         QList<QTreeWidgetItem*>& timerList,
                         QList<QTreeWidgetItem*>& aliasList,
                         QList<QTreeWidgetItem*>& actionList,
                         QList<QTreeWidgetItem*>& scriptList,
                         QList<QTreeWidgetItem*>& keyList);
    QString copyNewImagesToTmp(const QString& tempPath) const;
    static void cleanupUnusedImages(const QString& tempPath, const QString& plainDescription);
    void checkToEnableExportButton();
    void populateDependencies();
    void useIcon(const QString& fileName);
    static QString iconFileFromDrop(const QDropEvent* event);
    void setIconDropHighlight(const bool highlight);

    std::unique_ptr<Ui::dlgPackageExporter> ui;
    QPointer<Host> mpHost;
    QTreeWidget* mpExportSelection = nullptr;
    QPointer<QPushButton> mExportButton;
    QPointer<QPushButton> mCancelButton;
    QPointer<QPushButton> mCloseButton;
    QTreeWidgetItem* mpTriggers = nullptr;
    QTreeWidgetItem* mpAliases = nullptr;
    QTreeWidgetItem* mpTimers = nullptr;
    QTreeWidgetItem* mpScripts = nullptr;
    QTreeWidgetItem* mpKeys = nullptr;
    QTreeWidgetItem* mpButtons = nullptr;
    QGroupBox* mpSelectionText = nullptr;
    QString mPackageName;
    QString mPackagePath;
    QString mPackagePathFileName;
    QString mPackageIconPath;
    bool mIconDropHighlight = false;
    // The .mudlet folder of the installed package being re-exported, if any
    QString mPackageDataPath;
    QString mPackageConfig;
    QString mPackageComment;
    bool mCheckChildren = true;
    // Shared by every exporter, so that only one export runs at a time; held until the zip ends, even after a Cancel
    inline static bool mExportingPackage = false;
    static void releaseExportLock();
    static void recheckEveryExportButton();
    void endExportRun();
    // This dialog's export has put the busy cursor up and not yet taken it down
    bool mExportRunning = false;
    // Closed while its export ran, so it goes once that ends
    bool mDeleteWhenExportEnds = false;
    QFutureWatcher<std::pair<bool, QString>>* mpZipWatcher = nullptr;
    // Read on the zip's worker thread by libzip's cancel callback
    std::shared_ptr<std::atomic<bool>> mpZipCancelled;
    bool mRecountPending = false;

    QWidget* mpSelectionColumn = nullptr;
    QWidget* mpDetailsColumn = nullptr;
    QWidget* mpFoot = nullptr;
    uiDesign::GripSplitter* mpSplitter = nullptr;
    bool mSplitterRestored = false;
    // Resizing the list re-runs the column's layout, which asks for another fit
    bool mFittingAssets = false;
    ExporterTreeDelegate* mpTreeDelegate = nullptr;
    QLabel* mpLabel_selectionHeading = nullptr;
    QLabel* mpLabel_selectionCount = nullptr;
    QLabel* mpLabel_selectionHint = nullptr;
    QLabel* mpLabel_detailsHeading = nullptr;
    QLabel* mpLabel_nameCaption = nullptr;
    QLabel* mpLabel_detailsHint = nullptr;
    QLabel* mpLabel_descriptionHint = nullptr;
    QLabel* mpLabel_assetsHint = nullptr;
    QLabel* mpLabel_assetsDropHint = nullptr;
    QLabel* mpLabel_assetsEmpty = nullptr;
    QLabel* mpLabel_saveCaption = nullptr;
    QLabel* mpLabel_savePath = nullptr;
    uiDesign::ChipRow* mpChipRow_dependencies = nullptr;
    QPointer<QPushButton> mpButton_startOver;
    dlgSystemMessageArea* mpNotice = nullptr;
    QString mSavePathShown;
    QString mNoticeRichText;

    QString mFreshAuthor;
    QString mFreshVersion;
    QString mFreshDescription;
    struct LoadedWords
    {
        QString name;
        QString author;
        QString title;
        QString helpUrl;
        QString version;
        QString description;
    };
    LoadedWords mLoadedWords;
    int mPackageListWasAt = 0;

signals:
    void signal_exportLocationChanged(const QString& location);
    void packageExporterClosing(const QString& profileName);
};

class dlgPackageExporterDescription : public QTextEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(dlgPackageExporterDescription)
    explicit dlgPackageExporterDescription(QWidget* pW = nullptr);
    ~dlgPackageExporterDescription();
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;
    QSize minimumSizeHint() const override;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void restoreRenderedDescription();

    bool mShowingSourceForDrop = false;
};

#endif // MUDLET_DLGPACKAGEEXPORTER_H

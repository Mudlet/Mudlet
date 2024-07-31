#ifndef MUDLET_DLGPACKAGEEXPORTER_H
#define MUDLET_DLGPACKAGEEXPORTER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019-2020 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "Host.h"

#include "pre_guard.h"
#include <QDialog>
#include <QFileInfo>
#include <QTextEdit>
#include <zip.h>
#include "post_guard.h"
#include <zip.h>

#if defined(LIBZIP_VERSION_MAJOR) && defined(LIBZIP_VERSION_MINOR) && ((LIBZIP_VERSION_MAJOR  > 1) || (LIBZIP_VERSION_MAJOR == 1) && (LIBZIP_VERSION_MINOR >= 7))
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

public:
    Q_DISABLE_COPY(dlgPackageExporter)
    explicit dlgPackageExporter(QWidget* parent, Host*);
    ~dlgPackageExporter();
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
    static void copy_directory(const QString &fromDir, const QString &toDir, bool overwrite);
    QMap<QTreeWidgetItem*, TTrigger*> triggerMap;
    QMap<QTreeWidgetItem*, TTrigger*> modTriggerMap;
    QMap<QTreeWidgetItem*, TAlias*> aliasMap;
    QMap<QTreeWidgetItem*, TAlias*> modAliasMap;
    QMap<QTreeWidgetItem*, TScript*> scriptMap;
    QMap<QTreeWidgetItem*, TScript*> modScriptMap;
    QMap<QTreeWidgetItem*, TKey*> keyMap;
    QMap<QTreeWidgetItem*, TKey*> modKeyMap;
    QMap<QTreeWidgetItem*, TAction*> actionMap;
    QMap<QTreeWidgetItem*, TAction*> modActionMap;
    QMap<QTreeWidgetItem*, TTimer*> timerMap;
    QMap<QTreeWidgetItem*, TTimer*> modTimerMap;
    // This will hold the absolute pathFileName for the XML file that will
    // contain the Mudlet items to go into the package:
    QString mXmlPathFileName;
    QString mPlainDescription;
    QStringList mDescriptionImages;

public slots:
    void slot_addFiles();
    void slot_export_package();

private slots:
    void slot_addDependency();
    void slot_removeDependency();
    void slot_import_icon();
    void slot_openPackageLocation();
    void slot_packageChanged(int);
    void slot_updateLocationPlaceholder();
    void slot_enableExportButton(const QString &text);
    void slot_recountItems(QTreeWidgetItem *item);
    void slot_rightClickOnItems(const QPoint &point);
    void slot_cancelExport();

protected:
    bool eventFilter(QObject* obj, QEvent* evt) override;

private:
    static void appendToConfigFile(QString&, const QString&, const QString&);
    void displayResultMessage(const QString&, const bool isSuccessMessage = true);
    void uncheckAllChildren();
    int countRecursive(QTreeWidgetItem* item, int count) const;
    int countCheckedItems() const;
    void checkChildren(QTreeWidgetItem* item) const;
    QString getActualPath() const;
    static const int isTopFolder = 1;
    static std::pair<bool, QString> writeFileToZip(const QString& archiveFileName, const QString& fileSystemFileName, zip* archive);
    static std::pair<bool, QString> zipPackage(const QString& stagingDirName, const QString& packagePathFileName, const QString& xmlPathFileName, const QString& packageName, const QString& packageConfig);
    static std::pair<bool, QString> copyAssetsToTmp(const QStringList& assetPaths, const QString& tempPath);
    QFileInfo copyIconToTmp(const QString& tempPath) const;
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

    Ui::dlgPackageExporter* ui;
    QPointer<Host> mpHost;
    QTreeWidget* mpExportSelection;
    QPointer<QPushButton> mExportButton;
    QPointer<QPushButton> mCancelButton;
    QPointer<QPushButton> mCloseButton;
    QTreeWidgetItem* mpTriggers;
    QTreeWidgetItem* mpAliases;
    QTreeWidgetItem* mpTimers;
    QTreeWidgetItem* mpScripts;
    QTreeWidgetItem* mpKeys;
    QTreeWidgetItem* mpButtons;
    QGroupBox* mpSelectionText;
    QString mPackageName;
    QString mPackagePath;
    QString mPackagePathFileName;
    QString mPackageIconPath;
    QString mPackageConfig;
    bool mCheckChildren = true;
    inline static bool mExportingPackage = false;

signals:
    void signal_exportLocationChanged(const QString& location);
};

class dlgPackageExporterDescription : public QTextEdit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgPackageExporterDescription)
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 13, 0))
    Q_DISABLE_MOVE(dlgPackageExporterDescription)
#endif
    explicit dlgPackageExporterDescription(QWidget* pW = nullptr);
    ~dlgPackageExporterDescription();
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;
};

#endif // MUDLET_DLGPACKAGEEXPORTER_H

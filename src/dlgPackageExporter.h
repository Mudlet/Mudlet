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
#include <zip.h>
#include "post_guard.h"

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
    void copy_directory(const QString &fromDir, const QString &toDir, bool coverFileIfExist);
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
    void slot_recountItems();

protected:
    bool eventFilter(QObject* obj, QEvent* evt) override;

private:
    bool writeFileToZip(const QString&, const QString&, zip*);
    static void appendToConfigFile(QString&, const QString&, const QString&);
    void displayResultMessage(const QString&, const bool isSuccessMessage = true);
    void uncheckAllChildren();
    int countRecursive(QTreeWidgetItem* item, int count) const;
    int countCheckedItems() const;
    QString getActualPath() const;

    Ui::dlgPackageExporter* ui;
    QPointer<Host> mpHost;
    QTreeWidget* mpExportSelection;
    QPointer<QPushButton> mExportButton;
    QPointer<QPushButton> mCancelButton;
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
    QString mPlainDescription;

signals:
    void signal_exportLocationChanged(const QString& location);
};

#endif // MUDLET_DLGPACKAGEEXPORTER_H

#ifndef MUDLET_DLGPACKAGEEXPORTER_H
#define MUDLET_DLGPACKAGEEXPORTER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "ui_dlgPackageExporter.h"
#include <QDialog>
#include <QPushButton>
#include <QTemporaryDir>
#include "post_guard.h"

#include <zip.h>

class QTreeWidget;
class QTreeWidgetItem;


class dlgPackageExporter
: public QDialog
, public Ui::dlgPackageExporter
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

public slots:
    void slot_addFiles();
    void slot_export_package();

private:
    bool writeFileToZip(const QString&, const QString&, zip*);
    void displayResultMessage(const QString&, const bool isSuccessMessage = true);


    QPointer<Host> mpHost;
    QString mHostName;
    QPushButton* addFilesButton;
    QPushButton* exportButton;
    QString mPackageName;
    QString mZipFile;
    QString mXmlFile;

    // If true (default) use the prior:
    //     <user mudlet dir>/profiles/<profile name>/tmp/<package name>/
    // directory else use the mTempDir which is an OS provided unique per
    // instantation temporary directory THAT IS DESTROYED ALONG WITH ITS
    // CONTENTS WHEN THIS CLASS IS DESTROYED - i.e. is a "proper" temporary
    // directory...!
    bool mIsToKeepStagedFiles;
    bool mIsOverWriteEnabled;

    // The appropriate place depending on the previous bool:
    QDir mStagingDir;

    // The temporary directory (and its files) will only persist as long as the
    // dlgPackageExporter class instance does (unless autoremove is turned off
    // for debugging?)
    QTemporaryDir mTempDir;
};

#endif // MUDLET_DLGPACKAGEEXPORTER_H

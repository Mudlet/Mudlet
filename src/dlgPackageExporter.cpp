/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2020 by Stephen Lyons                        *
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


#include "dlgPackageExporter.h"
#include "ui_dlgPackageExporter.h"

#include "mudlet.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"

#include "pre_guard.h"
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMimeData>
#include "post_guard.h"

// We are now using code that won't work with really old versions of libzip:
// Unfortunately libzip 1.70 forgot to include these defines and thus broke the
// original tests:
#if defined(LIBZIP_VERSION_MAJOR) && defined(LIBZIP_VERSION_MINOR) && (LIBZIP_VERSION_MAJOR < 1) && (LIBZIP_VERSION_MINOR < 11)
#error Mudlet requires a version of libzip of at least 0.11
#endif

dlgPackageExporter::dlgPackageExporter(QWidget *parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::dlgPackageExporter)
, mpHost(pHost)
{
    ui->setupUi(this);
    ui->input->hide();
    ui->Icon->hide();

    mpExportSelection = ui->treeWidget_exportSelection;
    mpSelectionText = ui->groupBox_exportSelection;

    mpTriggers = new QTreeWidgetItem({tr("Triggers")});
    mpAliases = new QTreeWidgetItem({tr("Aliases")});
    mpTimers = new QTreeWidgetItem({tr("Timers")});
    mpScripts = new QTreeWidgetItem({tr("Scripts")});
    mpKeys = new QTreeWidgetItem({tr("Keys")});
    mpButtons = new QTreeWidgetItem({tr("Buttons")});

    mpExportSelection->addTopLevelItem(mpTriggers);
    mpExportSelection->addTopLevelItem(mpAliases);
    mpExportSelection->addTopLevelItem(mpTimers);
    mpExportSelection->addTopLevelItem(mpScripts);
    mpExportSelection->addTopLevelItem(mpKeys);
    mpExportSelection->addTopLevelItem(mpButtons);

    connect(mpExportSelection, &QTreeWidget::itemChanged, this, &dlgPackageExporter::slot_recountItems);

    // This button has the RejectRole which causes the dialog to be rejected
    // (and closed):
    mCancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);

    // This button has the ApplyRole which applies current changes but does NOT
    // cause the dialog to close:
    mExportButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    mExportButton->setText(tr("Export", "Text for button to perform the package export on the items the user has selected."));

    // reset mPackagePathFileName and mXmlPathFileName from possible previous use
    mPackagePathFileName.clear();
    mXmlPathFileName.clear();
    connect(ui->addFiles, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_addFiles);
    ui->buttonBox->addButton(mExportButton, QDialogButtonBox::ResetRole);
    connect(mExportButton, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_export_package);
    connect(ui->pushBotton_packageLocation, &QPushButton::clicked, this, &dlgPackageExporter::slot_openPackageLocation);
    connect(ui->lineEdit_packageName, &QLineEdit::textChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    connect(this, &dlgPackageExporter::signal_exportLocationChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    slot_updateLocationPlaceholder();
    connect(ui->lineEdit_packageName, &QLineEdit::textChanged, this, &dlgPackageExporter::slot_enableExportButton);
    slot_enableExportButton(QString());
    connect(ui->packageList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgPackageExporter::slot_packageChanged);
    connect(ui->addDependency, &QPushButton::clicked, this, &dlgPackageExporter::slot_addDependency);
    connect(ui->pushButton_addIcon, &QPushButton::clicked, this, &dlgPackageExporter::slot_import_icon);

    ui->listWidget_addedFiles->installEventFilter(this);
    ui->textEdit_description->installEventFilter(this);
    ui->comboBox_dependencies->installEventFilter(this);
    ui->packageList->addItem(tr("update installed package"));
    ui->DependencyList->addItem(tr("add dependencies"));
    ui->packageList->addItems(mpHost->mInstalledPackages);
    ui->DependencyList->addItems(mpHost->mInstalledPackages);

    auto modules = mpHost -> mInstalledModules;
    QMap<QString, QStringList>::const_iterator iter = modules.constBegin();
    while (iter != modules.constEnd()) {
        ui->packageList->addItem(iter.key());
        ui->DependencyList->addItem(iter.key());
        ++iter;
    }

    listTriggers();
    listAliases();
    listKeys();
    listScripts();
    listActions();
    listTimers();
}

dlgPackageExporter::~dlgPackageExporter()
{
    delete ui;
}

void dlgPackageExporter::appendToConfigFile(QString& comment, const QString& what, const QString& value)
{
    if (value.isEmpty()) {
        return;
    }
    comment.append(QStringLiteral("%1 = [[%2]]\n").arg(what, value));
}

void dlgPackageExporter::recurseTree(QTreeWidgetItem* pItem, QList<QTreeWidgetItem*>& treeList)
{
    treeList.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseTree(pItem->child(i), treeList);
    }
}

bool dlgPackageExporter::writeFileToZip(const QString& archiveFileName, const QString& fileSystemFileName, zip* archive)
{
    struct zip_source* s = zip_source_file(archive, fileSystemFileName.toUtf8().constData(), 0, -1);
    if (s == nullptr) {
        displayResultMessage(tr("Failed to open file \"%1\" to place into package. Error message was: \"%2\".",
                                // Intentional comment to separate arguments
                                "This error message will appear when a file is to be placed into the package but the code cannot open it.")
                             .arg(fileSystemFileName, zip_strerror(archive)), false);
        return false;
    }

    if (zip_file_add(archive, archiveFileName.toUtf8().constData(), s, ZIP_FL_ENC_UTF_8|ZIP_FL_OVERWRITE) == -1) {
        displayResultMessage(tr("Failed to add file \"%1\" to package \"%2\". Error message was: \"%3\".",
                                // Intentional comment to separate arguments
                                "This error message will appear when a file is to be placed into the package but cannot be done for some reason.")
                             .arg(archiveFileName, mPackagePathFileName, zip_strerror(archive)), false);
        return false;
    }

    return true;
}

void dlgPackageExporter::slot_addDependency()
{
    auto text = ui->DependencyList->currentText();
    auto index = ui->DependencyList->currentIndex();
    if (text.isEmpty() || index == 0) {
        return;
    }
    ui->comboBox_dependencies->addItem(text);
    ui->DependencyList->removeItem(index);
    ui->comboBox_dependencies->setCurrentIndex(ui->comboBox_dependencies->count() - 1);
}

void dlgPackageExporter::slot_removeDependency()
{
    auto text = ui->comboBox_dependencies->currentText();
    auto index = ui->comboBox_dependencies->currentIndex();
    if (text.isEmpty()) {
        return;
    }
    ui->DependencyList->addItem(text);
    ui->comboBox_dependencies->removeItem(index);
}

void dlgPackageExporter::slot_packageChanged(int index)
{
    QString packageName = ui->packageList->currentText();
    uncheckAllChildren();
    if (index != 0) {
        ui->lineEdit_packageName->setText(packageName);
    } else {
        ui->lineEdit_packageName->setText(mPackageName);
        return;
    }

    //check package/module items
    QTreeWidgetItem* top = mpTriggers;
    QList<QTreeWidgetItem*> trigList;
    recurseTree(top, trigList);
    for (auto item : qAsConst(trigList)) {
        if (triggerMap.contains(item) && triggerMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpTimers;
    QList<QTreeWidgetItem*> timerList;
    recurseTree(top, timerList);
    for (auto item : qAsConst(timerList)) {
        if (timerMap.contains(item) && timerMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpAliases;
    QList<QTreeWidgetItem*> aliasList;
    recurseTree(top, aliasList);
    for (auto item : qAsConst(aliasList)) {
        if (aliasMap.contains(item) && aliasMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpButtons;
    QList<QTreeWidgetItem*> actionList;
    recurseTree(top, actionList);
    for (auto item : qAsConst(actionList)) {
        if (actionMap.contains(item) && actionMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpScripts;
    QList<QTreeWidgetItem*> scriptList;
    recurseTree(top, scriptList);
    for (auto item : qAsConst(scriptList)) {
        if (scriptMap.contains(item) && scriptMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpKeys;
    QList<QTreeWidgetItem*> keyList;
    recurseTree(top, keyList);
    for (auto item : qAsConst(keyList)) {
        if (keyMap.contains(item) && keyMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }

    QString packagePath{mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName())};
    //fill package metadata
    mPackageIconPath.clear();
    QMap<QString, QString> packageInfo = mpHost->mPackageInfo.value(packageName);
    ui->lineEdit_author->setText(packageInfo.value(QStringLiteral("author")));
    QString icon{packageInfo.value(QStringLiteral("icon"))};
    if (!icon.isEmpty()) {
        mPackageIconPath = QStringLiteral("%1/%2/.mudlet/Icon/%3").arg(packagePath, packageName, icon);
        ui->Icon->show();
    } else {
        ui->Icon->hide();
    }
    QIcon myIcon(mPackageIconPath);
    ui->Icon->clear();
    ui->Icon->setPixmap(myIcon.pixmap(ui->Icon->size()));
    ui->lineEdit_title->setText(packageInfo.value(QStringLiteral("title")));
    mPlainDescription = packageInfo.value(QStringLiteral("description"));
    QString description{mPlainDescription};
    description.replace(QLatin1String("$packagePath"), QStringLiteral("%1/%2").arg(packagePath, packageName));
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 14, 0))
    ui->textEdit_description->setMarkdown(description);
#endif
    QString version = packageInfo.value(QStringLiteral("version"));
    ui->lineEdit_version->setText(version);
    QStringList dependencies = packageInfo.value(QStringLiteral("dependencies")).split(QLatin1Char(','));
    ui->comboBox_dependencies->clear();
    if (!dependencies.at(0).isEmpty()) {
        ui->comboBox_dependencies->addItems(dependencies);
    }

    //get files and folders from package
    ui->listWidget_addedFiles->clear();
    QFileInfo info(QStringLiteral("%1/%2/").arg(packagePath, packageName));
    if (!info.exists()) {
        return;
    }
    QDirIterator it(info.absoluteFilePath());
    QStringList ignore;
    ignore << QLatin1String("config.lua") << QStringLiteral("%1.xml").arg(packageName) << QLatin1String(".") << QLatin1String("..");
    while (it.hasNext()) {
        QFileInfo f(it.next());
        if (ignore.contains(f.fileName(), Qt::CaseInsensitive)) {
            continue;
        }
        ui->listWidget_addedFiles->addItem(f.absoluteFilePath());
    }
}

void dlgPackageExporter::slot_updateLocationPlaceholder()
{
    const auto packageName = ui->lineEdit_packageName->text();
    if (packageName.isEmpty()) {
        ui->lineEdit_filePath->setPlaceholderText(tr("Export to %1").arg(getActualPath()));
        return;
    }

    ui->lineEdit_filePath->setPlaceholderText(tr("Export to %1/%2.mpackage").arg(getActualPath(), packageName));
}

void dlgPackageExporter::slot_enableExportButton(const QString &text)
{
    if (text.isEmpty()) {
        mExportButton->setEnabled(false);
        return;
    }

    mExportButton->setEnabled(true);
}

void dlgPackageExporter::slot_import_icon()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Icon"), QDir::currentPath(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)"));
    if (fileName.isEmpty()) {
        return;
    }
    mPackageIconPath = fileName;
    QIcon myIcon(mPackageIconPath);
    ui->Icon->clear();
    ui->Icon->setPixmap(myIcon.pixmap(ui->Icon->size()));
    ui->Icon->show();
}

bool dlgPackageExporter::eventFilter(QObject* obj, QEvent* evt)
{
    if (evt->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        //add dependencies by writing them and pressing enter
        //delete them by pressing delete. Scroll through them by using Up and Down keys
        if (obj == ui->comboBox_dependencies) {
            if (keyEvent->key() == Qt::Key_Delete) {
                slot_removeDependency();
                return true;
            }
        }
    }

    //Focus handling returns false so that underlying class functions still work and the cursor is visible for example

    //description focus handling
    if (obj == ui->textEdit_description) {
        if (evt->type() == QEvent::FocusIn) {
            ui->textEdit_description->setPlainText(mPlainDescription);
            return false;
        }

        if (evt->type() == QEvent::FocusOut) {
            mPlainDescription = ui->textEdit_description->toPlainText();
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 14, 0))
            //$packagePath allows to put images in a package which will then be displayed in the description
            //during package creation it uses the profile folder. But once the package is created it will use
            //profile folder/packagename
            QString plainText{mPlainDescription};
            plainText.replace(QLatin1String("$packagePath"), mudlet::getMudletPath(mudlet::profileHomePath, mpHost->getName()));
            ui->textEdit_description->setMarkdown(plainText);
#endif
            return false;
        }
    }

    //added files listWidget events
    if (obj == ui->listWidget_addedFiles) {
        if (evt->type() == QEvent::DragEnter) {
            QDragEnterEvent* enterEvent = static_cast<QDragEnterEvent*>(evt);

            if (enterEvent->mimeData()->hasUrls()) {
                enterEvent->acceptProposedAction();
            }
            return true;
        }

        if (evt->type() == QEvent::Drop) {
            QDropEvent* dropEvent = static_cast<QDropEvent*>(evt);
            for (const auto& url : dropEvent->mimeData()->urls()) {
                QString fname = url.toLocalFile();
                QFileInfo info(fname);
                if (info.exists()) {
                    ui->listWidget_addedFiles->addItem(fname);
                }
            }
            return true;
        }

        if (evt->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
            if (keyEvent->key() == Qt::Key_Delete) {
                delete ui->listWidget_addedFiles->currentItem();
            }
        }
    }

    return false;
}

void dlgPackageExporter::copy_directory(const QString& fromDir, const QString& toDir, bool coverFileIfExist)
{
    QDirIterator it(fromDir);
    QDir targetDir(toDir);
    if (coverFileIfExist && targetDir.exists()) {
        targetDir.removeRecursively();
    }
    targetDir.mkdir(toDir);
    while (it.hasNext()) {
        QFileInfo f(it.next());
        if (f.fileName() == QLatin1String(".") || f.fileName() == QLatin1String("..") || f.isSymLink()) {
            continue;
        }
        if (f.isDir()) {
            copy_directory(f.filePath(), QStringLiteral("%1/%2").arg(targetDir.absolutePath(), f.fileName()), false);
        } else {
            QFile::copy(f.filePath(), targetDir.filePath(f.fileName()));
        }
    }
}

void dlgPackageExporter::slot_export_package()
{
    // The native windows dialog does not support displaying files - and as this
    // code will clobber/overwrite an existing package with the same
    // name it is highly desirable to show the files.
    // Although the Qt Documentation says only that the Windows platform needs
    // to NOT use the native dialog to show files, it has also shown to be
    // required for KDE on Linux - so has been used for all platforms:
    mPackageName = ui->lineEdit_packageName->text();
    if (mPackageName.isEmpty()) {
        displayResultMessage(tr("Please enter the package name."), false);
        return;
    }

    // if packageName changed allow to create a new package in the same path
    mPackagePathFileName = QStringLiteral("%1/%2.mpackage").arg(getActualPath(), mPackageName);

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    QString StagingDirName = QStringLiteral("%1/%2").arg(tempDir, mPackageName);
    QDir packageDir = QDir(StagingDirName);
    if (!packageDir.exists()) {
        packageDir.mkpath(StagingDirName);
    } else {
        packageDir.removeRecursively();
        packageDir.mkpath(StagingDirName);
    }

    QString tempPath = StagingDirName;
    tempPath.append("/");

    for (int i = 0; i < ui->listWidget_addedFiles->count(); ++i) {
        QListWidgetItem* item = ui->listWidget_addedFiles->item(i);
        QFileInfo itemFile(item->text());
        QString filePath = tempPath;
        filePath.append(itemFile.fileName());
        if (!itemFile.exists()) {
            continue;
        }
        if (itemFile.isFile()) {
            QFile::remove(filePath);
            QFile::copy(itemFile.absoluteFilePath(), filePath);
            continue;
        }
        if (itemFile.isDir()) {
            copy_directory(itemFile.absoluteFilePath(), filePath, true);
        }
    }

    QFileInfo iconFile(mPackageIconPath);
    if (iconFile.exists()) {
        QString iconDirName = QStringLiteral("%1.mudlet/Icon/").arg(tempPath);
        QDir iconDir = QDir(iconDirName);
        if (!iconDir.exists()) {
            iconDir.mkpath(iconDirName);
        }
        iconDirName.append(iconFile.fileName());
        QFile::copy(mPackageIconPath, iconDirName);
    }

    mXmlPathFileName = QStringLiteral("%1/%2.xml").arg(StagingDirName, mPackageName);

    QStringList dependencies;
    for (int index = 0; index < ui->comboBox_dependencies->count(); index++) {
        dependencies << ui->comboBox_dependencies->itemText(index);
    }

    mPackageConfig.clear();
    appendToConfigFile(mPackageConfig, QStringLiteral("mpackage"), mPackageName);
    appendToConfigFile(mPackageConfig, QStringLiteral("author"), ui->lineEdit_author->text());
    appendToConfigFile(mPackageConfig, QStringLiteral("icon"), iconFile.fileName());
    appendToConfigFile(mPackageConfig, QStringLiteral("title"), ui->lineEdit_title->text());
    appendToConfigFile(mPackageConfig, QStringLiteral("description"), mPlainDescription);
    appendToConfigFile(mPackageConfig, QStringLiteral("version"), ui->lineEdit_version->text());
    appendToConfigFile(mPackageConfig, QStringLiteral("dependencies"), dependencies.join(","));
    QDateTime iso8601timestamp = QDateTime::currentDateTime();
    int offset = iso8601timestamp.offsetFromUtc();
    iso8601timestamp.setOffsetFromUtc(offset);
    QDateTime iso8601time(QDateTime::currentDateTime());
    iso8601time.setTimeSpec(Qt::OffsetFromUTC);
    mPackageConfig.append(QStringLiteral("created = \"%1\"\n").arg(iso8601timestamp.toString(Qt::ISODate)));

    QString luaConfig = QStringLiteral("%1/config.lua").arg(StagingDirName);
    QFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << mPackageConfig;
        out.flush();
        configFile.close();
    }

    QFile checkWriteability(mXmlPathFileName);
    if (!checkWriteability.open(QIODevice::WriteOnly)) {
        displayResultMessage(tr("Failed to export. Could not open the folder \"%1\" for writing in. - "
                                "Do you have the necessary permissions and free disk-space to write to that folder?")
                             .arg(mXmlPathFileName), false);
        return;
    }
    checkWriteability.close();

    // This gets reset anytime that something goes wrong so that the export gets
    // aborted and shows an error message rather than an okay one:
    bool isOk = true;

    XMLexport writer(mpHost);
    //write trigs
    QTreeWidgetItem* top = mpTriggers;
    QList<QTreeWidgetItem*> trigList;
    recurseTree(top, trigList);
    for (auto item : qAsConst(trigList)) {
        if (item->checkState(0) == Qt::Unchecked && triggerMap.contains(item)) {
            triggerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && triggerMap.contains(item) && triggerMap[item]->mModuleMasterFolder) {
            triggerMap[item]->mModuleMasterFolder = false;
            modTriggerMap.insert(item, triggerMap[item]);
        }
    }
    top = mpTimers;
    QList<QTreeWidgetItem*> timerList;
    recurseTree(top, timerList);
    for (auto item : qAsConst(timerList)) {
        if (item->checkState(0) == Qt::Unchecked && timerMap.contains(item)) {
            timerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && timerMap.contains(item) && timerMap[item]->mModuleMasterFolder) {
            timerMap[item]->mModuleMasterFolder = false;
            modTimerMap.insert(item, timerMap[item]);
        }
    }
    top = mpAliases;
    QList<QTreeWidgetItem*> aliasList;
    recurseTree(top, aliasList);
    for (auto item : qAsConst(aliasList)) {
        if (item->checkState(0) == Qt::Unchecked && aliasMap.contains(item)) {
            aliasMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && aliasMap.contains(item) && aliasMap[item]->mModuleMasterFolder) {
            aliasMap[item]->mModuleMasterFolder = false;
            modAliasMap.insert(item, aliasMap[item]);
        }
    }
    top = mpButtons;
    QList<QTreeWidgetItem*> actionList;
    recurseTree(top, actionList);
    for (auto item : qAsConst(actionList)) {
        if (item->checkState(0) == Qt::Unchecked && actionMap.contains(item)) {
            actionMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && actionMap.contains(item) && actionMap[item]->mModuleMasterFolder) {
            actionMap[item]->mModuleMasterFolder = false;
            modActionMap.insert(item, actionMap[item]);
        }
    }
    top = mpScripts;
    QList<QTreeWidgetItem*> scriptList;
    recurseTree(top, scriptList);
    for (auto item : qAsConst(scriptList)) {
        if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)) {
            scriptMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && scriptMap.contains(item) && scriptMap[item]->mModuleMasterFolder) {
            scriptMap[item]->mModuleMasterFolder = false;
            modScriptMap.insert(item, scriptMap[item]);
        }
    }
    top = mpKeys;
    QList<QTreeWidgetItem*> keyList;
    recurseTree(top, keyList);
    for (auto item : qAsConst(keyList)) {
        if (item->checkState(0) == Qt::Unchecked && keyMap.contains(item)) {
            keyMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && keyMap.contains(item) && keyMap[item]->mModuleMasterFolder) {
            keyMap[item]->mModuleMasterFolder = false;
            modKeyMap.insert(item, keyMap[item]);
        }
    }

    if (!writer.exportPackage(mXmlPathFileName, false)) {
        displayResultMessage(tr("Failed to export. Could not write Mudlet items to the file \"%1\".",
                                // Intentional comment to separate arguments
                                "This error message is shown when all the Mudlet items cannot be written to the 'packageName'.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened.")
                             .arg(mXmlPathFileName), false);
        // Although we have failed, we must not just abort here. We need to reset
        // the selected "for export or not"-flags first. So note that we have failed:
        isOk = false;
        // After the following we will then drop through to the end of the
        // method to set up a means to close the dialogue after the user has
        // seen the error message...
    }

    //now fix all the stuff we weren't exporting
    //trigger, timer, alias, action, script, keys
    for (auto item : qAsConst(trigList)) {
        if (triggerMap.contains(item)) {
            triggerMap[item]->exportItem = true;
        }
        if (modTriggerMap.contains(item)) {
            modTriggerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(timerList)) {
        if (timerMap.contains(item)) {
            timerMap[item]->exportItem = true;
        }
        if (modTimerMap.contains(item)) {
            modTimerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(actionList)) {
        if (actionMap.contains(item)) {
            actionMap[item]->exportItem = true;
        }
        if (modActionMap.contains(item)) {
            modActionMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(scriptList)) {
        if (scriptMap.contains(item)) {
            scriptMap[item]->exportItem = true;
        }
        if (modScriptMap.contains(item)) {
            modScriptMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(keyList)) {
        if (keyMap.contains(item)) {
            keyMap[item]->exportItem = true;
        }
        if (modKeyMap.contains(item)) {
            modKeyMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : qAsConst(aliasList)) {
        if (aliasMap.contains(item)) {
            aliasMap[item]->exportItem = true;
        }
        if (modAliasMap.contains(item)) {
            modAliasMap[item]->mModuleMasterFolder = true;
        }
    }

    if (isOk) {
        // zip error code:
        int ze = 0;
/*
* Relevant options:
* * ZIP_CREATE creates the archive if it does not exist.
* * ZIP_TRUNCATE zaps any contents in a previously existing file.
*/
        // The pre-libzip 0.11 with the now obsolete zip_add() lacked the option
        // of the later zip_file_add() which DOES have an overwrite option:
        zip* archive = zip_open(mPackagePathFileName.toUtf8().constData(), ZIP_CREATE|ZIP_TRUNCATE, &ze);

        if (!archive) {
            // Failed to open/create archive file
            // We now use the better error handling system (not requiring a
            // previously defined finite-sized char type buffer {which obviously
            // could have string buffer overflow issues} which is available in
            // post 0.10 versions of libzip):
            zip_error_t error;
            zip_error_init_with_code(&error, ze);
            displayResultMessage(tr("Failed to open package file. Error is: \"%1\".",
                                    // Intentional comment to separate arguments
                                    "This error message is shown when the libzip library code is unable to open the file that was to be the end result of the export process. As this may be an existing file anywhere in the computer's file-system(s) it is possible that permissions on the directory or an existing file that is to be overwritten may be a source of problems here.")
                                 .arg(zip_error_strerror(&error)), false);
            zip_error_fini(&error);
            isOk = false;
            // The above flag will now cause execution to drop down to the bottom of
            // the method and set up a means to close the dialogue after the user
            // has seen the error message...

        } else {
            // Opened/created archive file successfully
/*
 * Previous code here failed if the user included sub-directories in the
 * temporary directory where the contents for the package/module was assembled -
 * as sub-directories are now correctly handled by the installer code we have
 * revised the code here to also work should the user use them. NB This is
 * typically the case where they have collections of images or sound files
 * which they wish to store in a heirarchical manner...! - Slysven
 */
#if defined(Q_OS_WIN32)
/*
 * From Qt Docs:
 * Note: On NTFS file systems, ownership and permissions checking is disabled by
 * default for performance reasons. To enable it, include the following line:
 */
            extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
/*
 * Permission checking is then turned on and off by incrementing and
 * decrementing qt_ntfs_permission_lookup by 1:
 */
            qt_ntfs_permission_lookup++;
#endif // defined(Q_OS_WIN32)
            QDirIterator itDir(StagingDirName, QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files, QDirIterator::Subdirectories);
            // relative names to use in archive:
            QStringList directoryEntries;
            // Key is relative name to use in archive
            // Value is fullName in file-system:
            QMap<QString, QString> fileEntries;
            while (itDir.hasNext() && isOk) {
                QString itEntry = itDir.next();
                Q_UNUSED(itEntry);
//              Comment out the preceding line if the following is uncommented!
//              qDebug() << " parsing entry:" << itEntry << " fileName() is:" << itDir.fileName() << " filePath() is:" << itDir.filePath();
                // QString::compare(...) returns 0 (false) if the two arguments
                // MATCH and non-0 (true) otherwise and De Morgans' Laws means
                // that the if branch should be taken if the fileName IS a Dot
                // OR IS a DotDot file...!
                if (!(  itDir.fileName().compare(QStringLiteral("."))
                     && itDir.fileName().compare(QStringLiteral("..")))) {

                     // Dot and DotDot entries are no use to us so skip them
                     continue;
                }

                QFileInfo entryInfo(itDir.fileInfo());
                if (!entryInfo.isReadable()) {
                    qWarning() << "dlgPackageExporter::slot_export_package() skipping file: "
                               << itDir.fileName()
                               << "it is NOT readable!";
                    continue;
                }

                if (entryInfo.isSymLink()) {
                    qWarning() << "dlgPackageExporter::slot_export_package() skipping file: "
                               << itDir.fileName()
                               << "it is a Symlink - avoided to prevent file-system loops!";
                    continue;
                }

                QString nameInArchive = itDir.filePath();
                nameInArchive.remove(QStringLiteral("%1/").arg(StagingDirName));

                if       (entryInfo.isDir()) {
                    directoryEntries.append(nameInArchive);
                } else if(entryInfo.isFile()) {
                    fileEntries.insert(nameInArchive, itDir.filePath());
                }
            }

#if defined(Q_OS_WIN32)
            qt_ntfs_permission_lookup--;
#endif

            if (directoryEntries.count() > 1) {
                std::sort(directoryEntries.begin(), directoryEntries.end());
            }

            QStringListIterator itDirectoryName(directoryEntries);
            while (itDirectoryName.hasNext() && isOk) {
                QString directoryName = itDirectoryName.next();
                // zip_dir_add(...) returns the index of the
                // added directory item in the archive or -1 on error:
                if (zip_dir_add(archive, directoryName.toStdString().c_str(), ZIP_FL_ENC_UTF_8) == -1) {
                    displayResultMessage(tr("Failed to add directory \"%1\" to package. Error is: \"%2\".")
                                         .arg(directoryName, zip_strerror(archive)), false);
                    zip_close(archive);
                    isOk = false;
                }
            }

            // Process the config and the file containing the Mudlet triggers,
            // etc. specially so they are inserted first and last respectively:
            if (isOk) {
                // Apparently it is permissable for there NOT to be a config.lua
                // file in the manufactured module (i.e. the user is allowed to
                // manually remove it even though we now request they do not).
                // If we enhance modules in the future to store more data in
                // that file I think this will no longer be permissable and the
                // two commented out bits of code can be restored
                if (fileEntries.contains(QStringLiteral("config.lua"))) {
                    if (!writeFileToZip(QStringLiteral("config.lua"), fileEntries.value(QStringLiteral("config.lua")), archive)) {
                        /* isOk = false; */
                    } else {
                        fileEntries.remove(QStringLiteral("config.lua"));
                    }
                }/* else {
                    displayResultMessage(tr("Required \"config.lua\" file not found to include in the package. Did you remove or rename it?"), false);
                    zip_close(archive);
                    isOk = false;
                }*/
            }

            QString xmlFileName = mPackageName;
            xmlFileName.append(QLatin1String(".xml"));
            if (isOk) {
                QMapIterator<QString, QString> itFileName(fileEntries);
                while (itFileName.hasNext() && isOk) {
                    itFileName.next();
                    if (itFileName.key() == xmlFileName) {
                        continue;
                    }

                    if (!writeFileToZip(itFileName.key(), itFileName.value(), archive)) {
                        zip_close(archive);
                        isOk = false;
                        break;
                    }
                }
            }

            if (isOk) {
                if (fileEntries.contains(xmlFileName) && fileEntries.value(xmlFileName) == mXmlPathFileName) {
                    if (!writeFileToZip(xmlFileName, mXmlPathFileName, archive)) {
                        isOk = false;
                    }

                    // If successful will get to HERE...

                } else {
                    displayResultMessage(tr("Required file \"%1\" was not found in the staging area. "
                                            "This area contains the Mudlet items chosen for the package, "
                                            "which you selected to be included in the package file. "
                                            "This suggests there may be a problem with that directory: "
                                            "\"%2\" - "
                                            "Do you have the necessary permissions and free disk-space?")
                                            .arg(mXmlPathFileName, QDir(StagingDirName).canonicalPath()), false);
                    isOk = false;
                }
            }

            if (isOk) {
                // THIS is the point that the archive gets created from the
                // source materials - it may take a short while!
                // If it fails to write out the new file 'archive' is left
                // unchanged (and we can still access it to get the error
                // details):
                // Change the cursor to a system busy one while we are working:
                QApplication::setOverrideCursor(Qt::BusyCursor);
                zip_set_archive_comment(archive, mPackageConfig.toUtf8().constData(), mPackageConfig.length());
                ze = zip_close(archive);
                QApplication::restoreOverrideCursor();
                if (ze) {
                    displayResultMessage(tr("Failed to write files into and then close the package. Error is: \"%1\".",
                                            // Intentional comment to separate arguments
                                            "This error message is displayed at the final stage of exporting a package when all the sourced files are finally put into the archive. Unfortunately this may be the point at which something breaks because a problem was not spotted/detected in the process earlier...")
                                         .arg(zip_strerror(archive)), false);
                    // In libzip 0.11 a function was added to clean up
                    // (deallocate) the memory associated with an archive
                    // - which would normally occur upon a successful close
                    // - before that version the memory just leaked away...
                    zip_discard(archive);
                    isOk = false;
                }

            } else {
                zip_discard(archive);
            }
        }
    }

    if (isOk) {
        // Success!
        displayResultMessage(tr("Package \"%1\" exported to: %2")
                             .arg(mPackageName, QStringLiteral("<a href=\"file:///%1\">%2</a>"))
                             .arg(getActualPath().toHtmlEscaped(), getActualPath().toHtmlEscaped()), true);
    } else {
        // Failed - convert cancel to a close button
        ui->buttonBox->removeButton(mCancelButton);
        ui->buttonBox->addButton(QDialogButtonBox::Close);
        connect(ui->buttonBox->button(QDialogButtonBox::Close), &QAbstractButton::clicked, this, &dlgPackageExporter::close);
    }
}

void dlgPackageExporter::slot_addFiles()
{
    QFileDialog* fDialog = new QFileDialog;
    fDialog->setFileMode(QFileDialog::Directory);
    fDialog->setOption(QFileDialog::DontUseNativeDialog);

    QStringList selectedFiles;
    //change file dialog children functions to support multiple folder+file selection
    //as qt doesn't seem to support that out of the box
    QDialogButtonBox* dialogBox = fDialog->findChild<QDialogButtonBox*>();
    QPushButton* button = dialogBox->button(QDialogButtonBox::Open);
    QListView* dialogListView = fDialog->findChild<QListView*>("listView");

    if (dialogListView) {
        dialogListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //button would be disabled if no folder is selected
        connect(dialogListView, &QListView::clicked, [=] { button->setEnabled(true); });
    }
    QTreeView* dialogTreeView = fDialog->findChild<QTreeView*>();
    if (dialogTreeView) {
        dialogTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(dialogTreeView, &QTreeView::clicked, [=] { button->setEnabled(true); });
    }
    connect(button, &QPushButton::clicked, [=] { fDialog->QDialog::accept(); });
    if (fDialog->exec()) {
        selectedFiles = fDialog->selectedFiles();
    }
    if (!selectedFiles.isEmpty()) {
        ui->listWidget_addedFiles->addItems(selectedFiles);
    }
    fDialog->deleteLater();
}

void dlgPackageExporter::slot_openPackageLocation()
{
    QString profileName(mpHost->getName());

    mPackagePath = QFileDialog::getExistingDirectory(
            nullptr, tr("Where do you want to save the package?"), mudlet::getMudletPath(mudlet::profileHomePath, profileName), QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);

    emit signal_exportLocationChanged(mPackagePath);
}

//only uncheck children
void uncheckRecursive(QTreeWidgetItem* item)
{
    item->setCheckState(0, Qt::Unchecked);
    for (int i = 0; i < item->childCount(); i++) {
        uncheckRecursive(item->child(i));
    }
}

void dlgPackageExporter::uncheckAllChildren()
{
    for (int i = 0; i < mpExportSelection->topLevelItemCount(); i++) {
        for (int j = 0; j < mpExportSelection->topLevelItem(i)->childCount(); j++) {
            uncheckRecursive(mpExportSelection->topLevelItem(i)->child(j));
        }
    }
}

int dlgPackageExporter::countRecursive(QTreeWidgetItem* item, int count) const
{
    count = count + (item->checkState(0) == Qt::Checked ? 1 : 0);
    for (int i = 0; i < item->childCount(); i++) {
        count = countRecursive(item->child(i), count);
    }
    return count;
}

int dlgPackageExporter::countCheckedItems() const
{
    int count = 0;
    for (int i = 0; i < mpExportSelection->topLevelItemCount(); i++) {
        count = count + (mpExportSelection->topLevelItem(i)->checkState(0) == Qt::Checked ? 1 : 0);
        for (int j = 0; j < mpExportSelection->topLevelItem(i)->childCount(); j++) {
            count = countRecursive(mpExportSelection->topLevelItem(i)->child(j), count);
        }
    }

    return count;
}

void dlgPackageExporter::recurseTriggers(TTrigger* trig, QTreeWidgetItem* qTrig)
{
    std::list<TTrigger*>* childList = trig->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TTrigger*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        triggerMap.insert(pItem, pChild);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qTrig->addChild(pItem);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::listTriggers()
{
    TriggerUnit* tu = mpHost->getTriggerUnit();
    std::list<TTrigger*>::const_iterator it;
    std::list<TTrigger*> tList = tu->getTriggerRootNodeList();
    QTreeWidgetItem* top = mpTriggers;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        triggerMap.insert(pItem, pChild);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::recurseAliases(TAlias* item, QTreeWidgetItem* qItem)
{
    std::list<TAlias*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TAlias*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qItem->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::listAliases()
{
    AliasUnit* tu = mpHost->getAliasUnit();
    std::list<TAlias*>::const_iterator it;
    std::list<TAlias*> tList = tu->getAliasRootNodeList();
    QTreeWidgetItem* top = mpAliases;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        top->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::recurseScripts(TScript* item, QTreeWidgetItem* qItem)
{
    std::list<TScript*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TScript*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::listScripts()
{
    ScriptUnit* tu = mpHost->getScriptUnit();
    std::list<TScript*>::const_iterator it;
    std::list<TScript*> tList = tu->getScriptRootNodeList();
    QTreeWidgetItem* top = mpScripts;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::recurseKeys(TKey* item, QTreeWidgetItem* qItem)
{
    std::list<TKey*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TKey*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TKey* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::listKeys()
{
    KeyUnit* tu = mpHost->getKeyUnit();
    std::list<TKey*>::const_iterator it;
    std::list<TKey*> tList = tu->getKeyRootNodeList();
    QTreeWidgetItem* top = mpKeys;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TKey* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::recurseActions(TAction* item, QTreeWidgetItem* qItem)
{
    std::list<TAction*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TAction*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::listActions()
{
    ActionUnit* tu = mpHost->getActionUnit();
    std::list<TAction*>::const_iterator it;
    std::list<TAction*> tList = tu->getActionRootNodeList();
    QTreeWidgetItem* top = mpButtons;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::recurseTimers(TTimer* item, QTreeWidgetItem* qItem)
{
    std::list<TTimer*>* childList = item->getChildrenList();
    if (childList->empty()) {
        return;
    }
    std::list<TTimer*>::iterator it;
    for (it = childList->begin(); it != childList->end(); ++it) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::listTimers()
{
    TimerUnit* tu = mpHost->getTimerUnit();
    std::list<TTimer*>::const_iterator it;
    std::list<TTimer*> tList = tu->getTimerRootNodeList();
    QTreeWidgetItem* top = mpTimers;
    for (it = tList.begin(); it != tList.end(); ++it) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::displayResultMessage(const QString& html, bool const isSuccessMessage)
{
    if (!isSuccessMessage) {
        // Big RED error message
        ui->infoLabel->setText(QStringLiteral("<p><font color='red'><b><big>%1</big><b></font></p>").arg(html));
        return;
    }

    // Big BLACK (Green would be hard for most common colour blind people to
    // tell from Red, and Blue would likely hide the URL) success message:
    ui->infoLabel->setText(QStringLiteral("<p><b><big>%1</big><b></p>"
                                          "<p>%2</p>")
                           .arg(html,
                                tr("Why not <a href=\"https://forums.mudlet.org/viewforum.php?f=6\">upload</a> your package for other Mudlet users?",
                                   // Intentional comment to separate arguments
                                   "Only the text outside of the 'a' (HTML anchor) tags PLUS the verb "
                                   "'upload' in between them in the source text, (associated with uploading "
                                   "the resulting package to the Mudlet forums) should be translated.")));
    ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel->setOpenExternalLinks(true);
}

void dlgPackageExporter::slot_recountItems()
{
    static bool debounce;

    if (!debounce) {
        debounce = true;
        QTimer::singleShot(0, this, [this]() {
            int itemsToExport = countCheckedItems();
            if (itemsToExport == 0) {
                mpSelectionText->setTitle(tr("Select what to export"));
            } else {
                mpSelectionText->setTitle(tr("Select what to export (%1 items)", "Package exporter selection", itemsToExport).arg(itemsToExport));
            }
            debounce = false;
        });
    }
}

QString dlgPackageExporter::getActualPath() const
{
    return mPackagePath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) : mPackagePath;
}

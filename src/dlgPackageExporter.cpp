/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2022, 2024 by Stephen Lyons                  *
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
#include <QtConcurrent>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMimeData>
#include "post_guard.h"

// We are now using code that won't work with really old versions of libzip;
// some of the error handling was improved in 1.0 . Unfortunately libzip 1.7.0
// (and one or two other recent versions) forgot to include the version defines
// and thus broke a test depending on them:
#if defined(LIBZIP_VERSION_MAJOR) && (LIBZIP_VERSION_MAJOR < 1)
#error Mudlet requires a version of libzip of at least 1.0
#endif

dlgPackageExporter::dlgPackageExporter(QWidget *parent, Host* pHost)
: QDialog(parent)
, ui(new Ui::dlgPackageExporter)
, mpHost(pHost)
{
    ui->setupUi(this);
    ui->splitter_metadataAssets->hide();
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

    mpExportSelection->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mpExportSelection, &QTreeWidget::itemChanged, this, &dlgPackageExporter::slot_recountItems);
    connect(mpExportSelection, &QTreeWidget::customContextMenuRequested, this, &dlgPackageExporter::slot_rightClickOnItems);

    mCancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    // make sure the Cancel button doesn't close the dialog
    ui->buttonBox->addButton(mCancelButton, QDialogButtonBox::ResetRole);
    mCancelButton->setVisible(false);
    mCloseButton = ui->buttonBox->button(QDialogButtonBox::Close);

    // This button has the ApplyRole which applies current changes but does NOT
    // cause the dialog to close:
    mExportButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    //: Text for button to perform the package export on the items the user has selected.
    mExportButton->setText(tr("Export"));

    // reset mPackagePathFileName and mXmlPathFileName from possible previous use
    mPackagePathFileName.clear();
    mXmlPathFileName.clear();
    connect(ui->addFiles, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_addFiles);
    connect(mExportButton, &QAbstractButton::clicked, this, &dlgPackageExporter::slot_exportPackage);
    connect(ui->pushButton_packageLocation, &QPushButton::clicked, this, &dlgPackageExporter::slot_openPackageLocation);
    connect(ui->lineEdit_packageName, &QLineEdit::textChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    connect(ui->lineEdit_author, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->lineEdit_title, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->lineEdit_version, &QLineEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(ui->textEdit_description, &QTextEdit::textChanged, this, &dlgPackageExporter::checkToEnableExportButton);
    connect(this, &dlgPackageExporter::signal_exportLocationChanged, this, &dlgPackageExporter::slot_updateLocationPlaceholder);
    slot_updateLocationPlaceholder();
    connect(ui->packageList, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlgPackageExporter::slot_packageChanged);
    connect(ui->addDependency, &QPushButton::clicked, this, &dlgPackageExporter::slot_addDependency);
    connect(ui->pushButton_addIcon, &QPushButton::clicked, this, &dlgPackageExporter::slot_importIcon);
    connect(mCancelButton, &QPushButton::clicked, this, &dlgPackageExporter::slot_cancelExport);

    ui->listWidget_addedFiles->installEventFilter(this);
    ui->comboBox_dependencies->installEventFilter(this);
    ui->textEdit_description->installEventFilter(this);

    // default package template - prevent clearing of text upon click
    dlgPackageExporter* te_parent = static_cast<dlgPackageExporter*>(topLevelWidget());
    te_parent->mPlainDescription = ui->textEdit_description->toPlainText();

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

    //: Title of the window. The %1 will be replaced by the current profile's name
    setWindowTitle(tr("Package Exporter - %1").arg(mpHost->getName()));

    // Set the previous details if saved
    QSettings settings("mudlet", "Mudlet");
    auto packageAuthor = settings.value(qsl("packageAuthor"), QString()).toString();
    if (!packageAuthor.isEmpty()) {
        ui->lineEdit_author->setText(packageAuthor);
    }

    // Ensure this dialog goes away if the Host (profile) is closed while we are
    // open - as this is parented to the mudlet instance rather than the Host
    // of the profile it would otherwise be left around if only the profile was
    // being closed:
    connect(mpHost, &QObject::destroyed, this, &dlgPackageExporter::slot_cancelExport);
    connect(mpHost, &QObject::destroyed, this, &dlgPackageExporter::close);
}

dlgPackageExporter::~dlgPackageExporter()
{
    delete ui;
}

void dlgPackageExporter::appendToDetails(const QString& what, const QString& value)
{
    if (value.isEmpty()) {
        return;
    }
    if (mPackageComment.isEmpty()) {
        // Insert a leading linefeed at the start as some zip utilities prefix
        // the first line with, say something like "Comment: " which does not
        // look so good when immediately followed by "Package name:". For
        // similar layout reasons, indent all the comment entries by, say,
        // four spaces in the next chunk of code.
        mPackageComment.append(QChar::LineFeed);
    }
    if (!what.compare(QLatin1String("mpackage"))) {
        // No point in using an internal variable name in an external, user
        // viewable aspect, use something that is more sensible:
        mPackageComment.append(qsl("    Package name: %1\n").arg(value));
    } else {
        mPackageComment.append(qsl("    %1: %2\n").arg(what, value));
    }
    mPackageConfig.append(qsl("%1 = [[%2]]\n").arg(what, value));
}

void dlgPackageExporter::recurseTree(QTreeWidgetItem* pItem, QList<QTreeWidgetItem*>& treeList)
{
    treeList.append(pItem);
    for (int i = 0; i < pItem->childCount(); ++i) {
        recurseTree(pItem->child(i), treeList);
    }
}

std::pair<bool, QString> dlgPackageExporter::writeFileToZip(const QString& archiveFileName, const QString& fileSystemFileName, zip* archive)
{
    struct zip_source* s = zip_source_file(archive, fileSystemFileName.toUtf8().constData(), 0, -1);
    if (s == nullptr) {
        return {false,
                //: This error message will appear when a file is to be placed into the package but the code cannot open it.
                tr("Failed to open file \"%1\" to place into package. Error message was: \"%2\".")
                        .arg(fileSystemFileName.toHtmlEscaped(), zip_strerror(archive))};
    }

    if (zip_file_add(archive, archiveFileName.toUtf8().constData(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) == -1) {
        zip_source_free(s);
        s = nullptr;
        return {false,
                //: This error message will appear when a file is to be placed into the package but cannot be done for some reason.
                tr("Failed to add file \"%1\" to package. Error message was: \"%3\".")
                        .arg(archiveFileName.toHtmlEscaped(), zip_strerror(archive))};
    }

    return {true, QString()};
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
    const QString packageName = ui->packageList->currentText();
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
    for (auto item : std::as_const(trigList)) {
        if (triggerMap.contains(item) && triggerMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpTimers;
    QList<QTreeWidgetItem*> timerList;
    recurseTree(top, timerList);
    for (auto item : std::as_const(timerList)) {
        if (timerMap.contains(item) && timerMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpAliases;
    QList<QTreeWidgetItem*> aliasList;
    recurseTree(top, aliasList);
    for (auto item : std::as_const(aliasList)) {
        if (aliasMap.contains(item) && aliasMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpButtons;
    QList<QTreeWidgetItem*> actionList;
    recurseTree(top, actionList);
    for (auto item : std::as_const(actionList)) {
        if (actionMap.contains(item) && actionMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpScripts;
    QList<QTreeWidgetItem*> scriptList;
    recurseTree(top, scriptList);
    for (auto item : std::as_const(scriptList)) {
        if (scriptMap.contains(item) && scriptMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    top = mpKeys;
    QList<QTreeWidgetItem*> keyList;
    recurseTree(top, keyList);
    for (auto item : std::as_const(keyList)) {
        if (keyMap.contains(item) && keyMap.value(item)->mPackageName == packageName) {
            item->setCheckState(0, Qt::Checked);
        }
    }

    const QString packagePath{mudlet::getMudletPath(enums::profileHomePath, mpHost->getName())};
    //fill package metadata
    mPackageIconPath.clear();
    QMap<QString, QString> const packageInfo = mpHost->mPackageInfo.value(packageName);
    ui->lineEdit_author->setText(packageInfo.value(qsl("author")));
    const QString icon{packageInfo.value(qsl("icon"))};
    if (!icon.isEmpty()) {
        mPackageIconPath = qsl("%1/%2/.mudlet/Icon/%3").arg(packagePath, packageName, icon);
        ui->Icon->show();
    } else {
        ui->Icon->hide();
    }
    const QIcon myIcon(mPackageIconPath);
    ui->Icon->clear();
    ui->Icon->setPixmap(myIcon.pixmap(ui->Icon->size()));
    ui->lineEdit_title->setText(packageInfo.value(qsl("title")));
    mPlainDescription = packageInfo.value(qsl("description"));
    QString description{mPlainDescription};
    description.replace(QLatin1String("$packagePath"), qsl("%1/%2").arg(packagePath, packageName));
    ui->textEdit_description->setMarkdown(description);
    const QString version = packageInfo.value(qsl("version"));
    ui->lineEdit_version->setText(version);
    const QStringList dependencies = packageInfo.value(qsl("dependencies")).split(QLatin1Char(','));
    ui->comboBox_dependencies->clear();
    if (!dependencies.at(0).isEmpty()) {
        ui->comboBox_dependencies->addItems(dependencies);
    }

    //get files and folders from package
    ui->listWidget_addedFiles->clear();
    const QFileInfo info(qsl("%1/%2/").arg(packagePath, packageName));
    if (!info.exists()) {
        return;
    }
    QDirIterator it(info.absoluteFilePath(), QDir::NoDotAndDotDot | QDir::Hidden | QDir::AllEntries);
    QStringList ignore;
    ignore << QLatin1String("config.lua") << qsl("%1.xml").arg(packageName);
    while (it.hasNext()) {
        const QFileInfo f(it.next());
        if (ignore.contains(f.fileName(), Qt::CaseInsensitive)) {
            continue;
        }
        ui->listWidget_addedFiles->addItem(f.absoluteFilePath());
    }
}

void dlgPackageExporter::slot_updateLocationPlaceholder()
{
    const auto packageName = ui->lineEdit_packageName->text();
    QString path;
    if (packageName.isEmpty()) {
        path = tr("Export to %1").arg(getActualPath());
    } else {
        path = tr("Export to %1").arg(qsl("%1/%2.mpackage").arg(getActualPath(), packageName));
    }

    checkToEnableExportButton();

    ui->lineEdit_filePath->setPlaceholderText(path);
}

void dlgPackageExporter::checkToEnableExportButton()
{
    QStringList missingFields;
    if (ui->lineEdit_packageName->text().isEmpty()) {
        //: package name will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
        missingFields << tr("package name");
    }
    // intentionally disabled for now until mpkg gains wider adoption
    // if (ui->lineEdit_author->text().isEmpty()) {
    //     //: package author will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("author");
    // }
    // if (ui->lineEdit_title->text().isEmpty()) {
    //     //: package title will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("title");
    // }
    // if (ui->lineEdit_version->text().isEmpty()) {
    //     //: package version will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("version");
    // }
    // if (ui->textEdit_description->toPlainText().isEmpty() || mExportingPackage) {
    //     //: package description will be added to other fields in the 'required fields missing: ...' tooltip when it's missing
    //     missingFields << tr("description");
    // }

    if (!missingFields.isEmpty()) {
        mExportButton->setEnabled(false);
        mExportButton->setToolTip(tr("Required field missing: %1").arg(missingFields.join(qsl(", "))));
    } else {
        mExportButton->setEnabled(!mExportingPackage);
        mExportButton->setToolTip(tr("Export package"));
    }
}

void dlgPackageExporter::slot_importIcon()
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Icon"), lastDir, tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tif *.ico *.icns)"));
    if (fileName.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(fileName).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);
    mPackageIconPath = fileName;
    const QIcon myIcon(mPackageIconPath);
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
            ui->textEdit_description->setCurrentCharFormat(QTextCharFormat());
            ui->textEdit_description->setPlainText(mPlainDescription);
            return false;
        }

        if (evt->type() == QEvent::FocusOut) {
            mPlainDescription = ui->textEdit_description->toPlainText();
            //$packagePath allows to put images in a package which will then be displayed in the description
            //during package creation it uses the profile folder. But once the package is created it will use
            //profile folder/packagename
            QString plainText{mPlainDescription};
            const QString profilePath{mudlet::getMudletPath(enums::profileHomePath, mpHost->getName())};
            //$packagePath will be replaced by the resource path if an existing package is selected
            if (ui->packageList->currentIndex() != 0) {
                const QString packageName = ui->packageList->currentText();
                plainText.replace(QLatin1String("$packagePath"), qsl("%1/%2").arg(profilePath, packageName));
            } else {
                plainText.replace(QLatin1String("$packagePath"), profilePath);
            }
            for (int i = mDescriptionImages.size() - 1; i >= 0; i--) {
                QString fname = mDescriptionImages.at(i);
                const QFileInfo info(fname);
                fname = QUrl::toPercentEncoding(fname).constData();
                plainText.replace(qsl("$%1").arg(info.fileName()), fname);
            }
            ui->textEdit_description->setMarkdown(plainText);
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
                const QString fname = url.toLocalFile();
                const QFileInfo info(fname);
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

void dlgPackageExporter::copy_directory(const QString& fromDir, const QString& toDir, bool overwrite)
{
    QDirIterator it(fromDir);
    QDir targetDir(toDir);
    if (overwrite && targetDir.exists()) {
        targetDir.removeRecursively();
    }
    targetDir.mkdir(toDir);
    while (it.hasNext()) {
        const QFileInfo f(it.next());
        if (f.fileName() == QLatin1String(".") || f.fileName() == QLatin1String("..") || f.isSymLink()) {
            continue;
        }
        if (f.isDir()) {
            copy_directory(f.filePath(), qsl("%1/%2").arg(targetDir.absolutePath(), f.fileName()), false);
        } else {
            QFile::copy(f.filePath(), targetDir.filePath(f.fileName()));
        }
    }
}

void dlgPackageExporter::slot_exportPackage()
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
    mPackagePathFileName = qsl("%1/%2.mpackage").arg(getActualPath(), mPackageName);

    // QT Docs say that QStandardPaths::writableLocation(QStandardPaths::TempLocation)
    // "Returns a directory where temporary files can be stored. The returned
    // value might be application-specific, shared among other applications for
    // this user, or even system-wide. The returned path is never empty."
    // which will return something like:
    // Windows: "C:/Users/<USER>/AppData/Local/Temp"
    // Linux: "/tmp"
    // MacOS: randomly generated by the OS
    // To avoid confusion if the user looks in that part of their file-system we
    // will append a "/mudlet" suffix so they can see that we are interested in
    // those files:
    const QString stagingDirName = qsl("%1/mudlet/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), mPackageName);
    QDir packageDir = QDir(stagingDirName);
    if (!packageDir.exists()) {
        packageDir.mkpath(stagingDirName);
    } else {
        packageDir.removeRecursively();
        packageDir.mkpath(stagingDirName);
    }

    const QString tempPath = qsl("%1/").arg(stagingDirName);

    mExportingPackage = true;
    QApplication::setOverrideCursor(Qt::BusyCursor);
    checkToEnableExportButton();

#if defined(LIBZIP_SUPPORTS_CANCELLING)
    mCancelButton->setVisible(true);
#endif

    mCloseButton->setVisible(false);
    displayResultMessage(tr("Exporting package..."), true);
    qApp->processEvents();

    auto plainDescription = copyNewImagesToTmp(tempPath);

    QStringList assetPaths;
    for (int i = 0; i < ui->listWidget_addedFiles->count(); ++i) {
        assetPaths << ui->listWidget_addedFiles->item(i)->text();
    }

    // start copying assets in the background
    auto assetsFuture = QtConcurrent::run(dlgPackageExporter::copyAssetsToTmp, assetPaths, tempPath);

    const QFileInfo iconFile = copyIconToTmp(tempPath);

    mXmlPathFileName = qsl("%1/%2.xml").arg(stagingDirName, mPackageName);
    writeConfigFile(stagingDirName, iconFile, plainDescription);

    QFile checkWriteability(mXmlPathFileName);
    if (!checkWriteability.open(QIODevice::WriteOnly)) {
        displayResultMessage(tr("Failed to export. Could not open the folder \"%1\" for writing. Do you have the necessary permissions and free disk-space to write to that folder?")
                             .arg(mXmlPathFileName.toHtmlEscaped()), false);
        assetsFuture.cancel();
        mExportingPackage = false;
        checkToEnableExportButton();
        QApplication::restoreOverrideCursor();
        return;
    }
    checkWriteability.close();

    // This gets reset anytime that something goes wrong so that the export gets
    // aborted and shows an error message rather than an okay one:
    bool isOk = true;

    QList<QTreeWidgetItem*> trigList;
    QList<QTreeWidgetItem*> timerList;
    QList<QTreeWidgetItem*> aliasList;
    QList<QTreeWidgetItem*> actionList;
    QList<QTreeWidgetItem*> scriptList;
    QList<QTreeWidgetItem*> keyList;
    exportXml(isOk, trigList, timerList, aliasList, actionList, scriptList, keyList);
    markExportItems(trigList, timerList, aliasList, actionList, scriptList, keyList);

    if (isOk) {
        // ensure assets have been copied before we start writing the zip
        // this will freeze the main thread, so it's not the perfect way - ideally
        // only start this after assets copy + xml writing is complete
        assetsFuture.waitForFinished();
        cleanupUnusedImages(tempPath, plainDescription);
        if (auto [success, message] = assetsFuture.result(); !success) {
            displayResultMessage(message, false);
            isOk = false;
        } else {
            auto future = QtConcurrent::run(dlgPackageExporter::zipPackage, stagingDirName, mPackagePathFileName, mXmlPathFileName, mPackageName, mPackageComment);
            auto watcher = new QFutureWatcher<std::pair<bool, QString>>;
            connect(watcher, &QFutureWatcher<std::pair<bool, QString>>::finished, this, [=, this]() {
                mExportingPackage = false;
                checkToEnableExportButton();

                if (auto [isOk, errorMsg] = future.result(); !isOk) {
                    displayResultMessage(errorMsg, false);
                } else {
                    displayResultMessage(tr("Package \"%1\" exported to: %2")
                                                 .arg(mPackageName.toHtmlEscaped(), qsl("<a href=\"file:///%1\">%1</a>")
                                                                            .arg(getActualPath().toHtmlEscaped())),
                                         true);
                }
                mCancelButton->setVisible(false);
                mCloseButton->setVisible(true);
                QApplication::restoreOverrideCursor();
            });
            watcher->setFuture(future);
        }
    }

    if (isOk) {
        displayResultMessage(tr("Exporting package..."), true);
    } else {
        mExportingPackage = false;
        checkToEnableExportButton();
        mCancelButton->setVisible(false);
        mCloseButton->setVisible(true);
        QApplication::restoreOverrideCursor();
    }

    // save settings for future reuse
    QSettings settings("mudlet", "Mudlet");
    settings.setValue("packageAuthor", ui->lineEdit_author->text());
}

//copy the newly-added description image files
QString dlgPackageExporter::copyNewImagesToTmp(const QString& tempPath) const
{
    QStringList newImagesList;
    //don't change the original plain description here as it may still be needed, for example if creating another package
    QString plainDescription = mPlainDescription;
    for (int i = mDescriptionImages.size() - 1; i >= 0; i--) {
        const QString fname = mDescriptionImages.at(i);
        const QFileInfo info(fname);
        if (plainDescription.contains(qsl("$%1").arg(info.fileName()))) {
            newImagesList.append(fname);
        }
    }

    if (!newImagesList.isEmpty()) {
        //Create description image dir
        const QString descriptionImagesDirName = qsl("%1.mudlet/description_images/").arg(tempPath);
        const QDir descriptionImageDir = QDir(descriptionImagesDirName);
        if (!descriptionImageDir.exists()) {
            descriptionImageDir.mkpath(descriptionImagesDirName);
        }
        for (int i = newImagesList.size() - 1; i >= 0; i--) {
            const QFileInfo imageFile(newImagesList.at(i));
            if (imageFile.exists()) {
                QString imageDir = descriptionImagesDirName;
                imageDir.append(imageFile.fileName());
                QFile::copy(imageFile.absoluteFilePath(), imageDir);
            }
            //replaces spaces with %20 in image file name to create a compatible url
            const QString imageName = QUrl::toPercentEncoding(imageFile.fileName()).constData();
            //replace temporary path with the path that is now inside the package
            plainDescription.replace(qsl("$%1").arg(imageFile.fileName()), qsl("$packagePath/.mudlet/description_images/%1").arg(imageName));
        }
    }
    return plainDescription;
}

// purge images from tmp which are no longer used by the description
void dlgPackageExporter::cleanupUnusedImages(const QString& tempPath, const QString& plainDescription)
{
    static const QRegularExpression imagesInUsePattern(R"(\$packagePath\/\.mudlet\/description_images\/(.+?)\.)");
    QStringList imagesInUse;
    QRegularExpressionMatchIterator i = imagesInUsePattern.globalMatch(plainDescription);
    while (i.hasNext()) {
        auto match = i.next();
        imagesInUse << match.captured(1).remove(QChar('\"'));
    }

    // iterate through all images in folder, if our list doesn't contain it - remove
    QDirIterator allImagesCopied(qsl("%1.mudlet/description_images").arg(tempPath), QDir::Files);
    while (allImagesCopied.hasNext()) {
        const QFileInfo copiedImage(allImagesCopied.next());
        if (!imagesInUse.contains(copiedImage.baseName())) {
            if (!QFile(copiedImage.absoluteFilePath()).remove()) {
                qDebug() << "couldn't remove unused image" << copiedImage.fileName();
            }
        }
    }
}

void dlgPackageExporter::markExportItems(QList<QTreeWidgetItem*>& trigList,
                                         QList<QTreeWidgetItem*>& timerList,
                                         QList<QTreeWidgetItem*>& aliasList,
                                         QList<QTreeWidgetItem*>& actionList,
                                         QList<QTreeWidgetItem*>& scriptList,
                                         QList<QTreeWidgetItem*>& keyList)
{ //now fix all the stuff we weren't exporting
    //trigger, timer, alias, action, script, keys
    for (auto item : std::as_const(trigList)) {
        if (triggerMap.contains(item)) {
            triggerMap[item]->exportItem = true;
        }
        if (modTriggerMap.contains(item)) {
            modTriggerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(timerList)) {
        if (timerMap.contains(item)) {
            timerMap[item]->exportItem = true;
        }
        if (modTimerMap.contains(item)) {
            modTimerMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(actionList)) {
        if (actionMap.contains(item)) {
            actionMap[item]->exportItem = true;
        }
        if (modActionMap.contains(item)) {
            modActionMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(scriptList)) {
        if (scriptMap.contains(item)) {
            scriptMap[item]->exportItem = true;
        }
        if (modScriptMap.contains(item)) {
            modScriptMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(keyList)) {
        if (keyMap.contains(item)) {
            keyMap[item]->exportItem = true;
        }
        if (modKeyMap.contains(item)) {
            modKeyMap[item]->mModuleMasterFolder = true;
        }
    }
    for (auto item : std::as_const(aliasList)) {
        if (aliasMap.contains(item)) {
            aliasMap[item]->exportItem = true;
        }
        if (modAliasMap.contains(item)) {
            modAliasMap[item]->mModuleMasterFolder = true;
        }
    }
}
void dlgPackageExporter::exportXml(bool& isOk,
                                   QList<QTreeWidgetItem*>& trigList,
                                   QList<QTreeWidgetItem*>& timerList,
                                   QList<QTreeWidgetItem*>& aliasList,
                                   QList<QTreeWidgetItem*>& actionList,
                                   QList<QTreeWidgetItem*>& scriptList,
                                   QList<QTreeWidgetItem*>& keyList)
{
    XMLexport writer(mpHost);
    //write trigs
    QTreeWidgetItem* top = mpTriggers;
    recurseTree(top, trigList);
    for (auto item : std::as_const(trigList)) {
        if (item->checkState(0) == Qt::Unchecked && triggerMap.contains(item)) {
            triggerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && triggerMap.contains(item) && triggerMap[item]->mModuleMasterFolder) {
            triggerMap[item]->mModuleMasterFolder = false;
            modTriggerMap.insert(item, triggerMap[item]);
        }
    }
    top = mpTimers;
    recurseTree(top, timerList);
    for (auto item : std::as_const(timerList)) {
        if (item->checkState(0) == Qt::Unchecked && timerMap.contains(item)) {
            timerMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && timerMap.contains(item) && timerMap[item]->mModuleMasterFolder) {
            timerMap[item]->mModuleMasterFolder = false;
            modTimerMap.insert(item, timerMap[item]);
        }
    }
    top = mpAliases;
    recurseTree(top, aliasList);
    for (auto item : std::as_const(aliasList)) {
        if (item->checkState(0) == Qt::Unchecked && aliasMap.contains(item)) {
            aliasMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && aliasMap.contains(item) && aliasMap[item]->mModuleMasterFolder) {
            aliasMap[item]->mModuleMasterFolder = false;
            modAliasMap.insert(item, aliasMap[item]);
        }
    }
    top = mpButtons;
    recurseTree(top, actionList);
    for (auto item : std::as_const(actionList)) {
        if (item->checkState(0) == Qt::Unchecked && actionMap.contains(item)) {
            actionMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && actionMap.contains(item) && actionMap[item]->mModuleMasterFolder) {
            actionMap[item]->mModuleMasterFolder = false;
            modActionMap.insert(item, actionMap[item]);
        }
    }
    top = mpScripts;
    recurseTree(top, scriptList);
    for (auto item : std::as_const(scriptList)) {
        if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)) {
            scriptMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && scriptMap.contains(item) && scriptMap[item]->mModuleMasterFolder) {
            scriptMap[item]->mModuleMasterFolder = false;
            modScriptMap.insert(item, scriptMap[item]);
        }
    }
    top = mpKeys;
    recurseTree(top, keyList);
    for (auto item : std::as_const(keyList)) {
        if (item->checkState(0) == Qt::Unchecked && keyMap.contains(item)) {
            keyMap[item]->exportItem = false;
        } else if (item->checkState(0) == Qt::Checked && keyMap.contains(item) && keyMap[item]->mModuleMasterFolder) {
            keyMap[item]->mModuleMasterFolder = false;
            modKeyMap.insert(item, keyMap[item]);
        }
    }

    if (!writer.exportPackage(mXmlPathFileName, false)) {
        //: This error message is shown when all the Mudlet items cannot be written to the 'packageName'.xml file in the base directory of the place where all the files are staged before being compressed into the package file. The full path and filename are shown in %1 to help the user diagnose what might have happened
        displayResultMessage(tr("Failed to export. Could not write Mudlet items to the file \"%1\".")
                             .arg(mXmlPathFileName.toHtmlEscaped()), false);
        // Although we have failed, we must not just abort here. We need to reset
        // the selected "for export or not"-flags first. So note that we have failed:
        isOk = false;
        // After the following we will then drop through to the end of the
        // method to set up a means to close the dialogue after the user has
        // seen the error message...
    }
}

void dlgPackageExporter::writeConfigFile(const QString& stagingDirName, const QFileInfo& iconFile, const QString& packageDescription)
{
    QStringList dependencies;
    for (int index = 0; index < ui->comboBox_dependencies->count(); index++) {
        dependencies << ui->comboBox_dependencies->itemText(index);
    }

    mPackageConfig.clear();
    mPackageComment.clear();
    appendToDetails(qsl("mpackage"), mPackageName);
    appendToDetails(qsl("author"), ui->lineEdit_author->text());
    appendToDetails(qsl("icon"), iconFile.fileName());
    appendToDetails(qsl("title"), ui->lineEdit_title->text());
    appendToDetails(qsl("description"), packageDescription);
    appendToDetails(qsl("version"), ui->lineEdit_version->text());
    appendToDetails(qsl("dependencies"), dependencies.join(","));
    QDateTime iso8601timestamp = QDateTime::currentDateTime();
    const int offset = iso8601timestamp.offsetFromUtc();
    iso8601timestamp.setOffsetFromUtc(offset);
    QDateTime iso8601time(QDateTime::currentDateTime());
    iso8601time.setTimeSpec(Qt::OffsetFromUTC);
    mPackageConfig.append(qsl("created = \"%1\"\n").arg(iso8601timestamp.toString(Qt::ISODate)));
    mPackageComment.append(qsl("    created: %1\n").arg(iso8601timestamp.toString(Qt::ISODate)));

    const QString luaConfig = qsl("%1/config.lua").arg(stagingDirName);
    QSaveFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);
        out << mPackageConfig;
        if (!configFile.commit()) {
            qDebug() << "dlgPackageExporter::writeConfigFile: error saving package config data: " << configFile.errorString();
        }
    }
}

QFileInfo dlgPackageExporter::copyIconToTmp(const QString& tempPath) const
{
    const QFileInfo iconFile(mPackageIconPath);
    if (iconFile.exists()) {
        QString iconDirName = qsl("%1.mudlet/Icon/").arg(tempPath);
        const QDir iconDir = QDir(iconDirName);
        if (!iconDir.exists()) {
            iconDir.mkpath(iconDirName);
        }
        iconDirName.append(iconFile.fileName());
        QFile::copy(mPackageIconPath, iconDirName);
    }
    return iconFile;
}

std::pair<bool, QString> dlgPackageExporter::copyAssetsToTmp(const QStringList& assetPaths, const QString& tempPath)
{
    for (const auto& assetPath : assetPaths) {
        const QFileInfo asset(assetPath);
        QString filePath = tempPath;
        filePath.append(asset.fileName());
        if (!asset.exists()) {
            return {false, tr("%1 doesn't seem to exist anymore - can you double-check it?").arg(asset.absoluteFilePath().toHtmlEscaped())};
        }
        if (asset.isFile()) {
            QFile::remove(filePath);
            if (!QFile::copy(asset.absoluteFilePath(), filePath)) {
                return {false, tr("cannot copy %1 to the temporary location %2 - can you double-check it?").arg(asset.absoluteFilePath().toHtmlEscaped(), tempPath.toHtmlEscaped())};
            }
        } else if (asset.isDir()) {
            copy_directory(asset.absoluteFilePath(), filePath, false);
        }
    }

    return {true, QString{}};
}

std::pair<bool, QString> dlgPackageExporter::zipPackage(const QString& stagingDirName, const QString& packagePathFileName, const QString& xmlPathFileName, const QString& packageName, const QString& packageComment)
{
    bool isOk = true;
    QString error;
    // zip error code:
    int ze = 0;


    // ZIP_CREATE creates the archive if it does not exist.
    // ZIP_TRUNCATE zaps any contents in a previously existing file.
    zip* archive = zip_open(packagePathFileName.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &ze);

    if (!archive) {
        zip_error_t zipError;
        zip_error_init_with_code(&zipError, ze);
        /*: This zipError message is shown when the libzip library code is unable
         * to open the file that was to be the end result of the export process.
         * As this may be an existing file anywhere in the computer's
         * file-system(s) it is possible that permissions on the directory or an
         * existing file that is to be overwritten may be a source of problems
         * here.
        */
        const QString errMsg = tr("Failed to open package file. Error is: \"%1\".")
                                 .arg(zip_error_strerror(&zipError));
        zip_error_fini(&zipError);
        return {false, errMsg};
    }
    // Opened/created archive file successfully
#if defined(Q_OS_WINDOWS)
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
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
#else
    /*
     * The prior system has been deprecated in Qt 6.6 and the following is
     * a low-level call to provide similar functionality. Ideally
     * the higher level QNtfsPermissionCheckGuard class would be used instead.
     */
    qEnableNtfsPermissionChecks();
#endif
#endif // defined(Q_OS_WINDOWS)
    QDirIterator stagingFile(stagingDirName, QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDirIterator::Subdirectories);
    // relative names to use in archive:
    QStringList directoryEntries;
    // Key is relative name to use in archive
    // Value is fullName in file-system:
    QMap<QString, QString> fileEntries;
    while (stagingFile.hasNext() && isOk) {
        const QString itEntry = stagingFile.next();
        Q_UNUSED(itEntry);
        // Dot and DotDot entries are no use to us so skip them
        if (!(stagingFile.fileName().compare(qsl(".")) && stagingFile.fileName().compare(qsl("..")))) {
            // Dot and DotDot entries are no use to us so skip them
            continue;
        }

        const QFileInfo stagingFileInfo(stagingFile.fileInfo());
        if (!stagingFileInfo.isReadable()) {
            qWarning() << "dlgPackageExporter::slot_exportPackage() skipping file: " << stagingFile.fileName() << "it is NOT readable!";
            continue;
        }

        if (stagingFileInfo.isSymLink()) {
            qWarning() << "dlgPackageExporter::slot_exportPackage() skipping file: " << stagingFile.fileName() << "it is a Symlink - avoided to prevent file-system loops!";
            continue;
        }

        QString nameInArchive = stagingFile.filePath();
        nameInArchive.remove(qsl("%1/").arg(stagingDirName));

        if (stagingFileInfo.isDir()) {
            directoryEntries.append(nameInArchive);
        } else if (stagingFileInfo.isFile()) {
            fileEntries.insert(nameInArchive, stagingFile.filePath());
        }
    }

#if defined(Q_OS_WINDOWS)
    // Turn off permission checking on NTFS file systems
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    qt_ntfs_permission_lookup--;
#else
    qDisableNtfsPermissionChecks();
#endif
#endif

    if (directoryEntries.count() > 1) {
        std::sort(directoryEntries.begin(), directoryEntries.end());
    }

    QStringListIterator itDirectoryName(directoryEntries);
    while (itDirectoryName.hasNext() && isOk) {
        const QString directoryName = itDirectoryName.next();
        // zip_dir_add(...) returns the index of the
        // added directory item in the archive or -1 on error:
        if (zip_dir_add(archive, directoryName.toStdString().c_str(), ZIP_FL_ENC_UTF_8) == -1) {
            const QString errorMsg = tr("Failed to add directory \"%1\" to package. Error is: \"%2\".").arg(directoryName.toHtmlEscaped(), zip_strerror(archive));
            zip_discard(archive);
            return {false, errorMsg};
        }
    }

    // Process the config and the file containing the Mudlet triggers,
    // etc. specially so they are inserted first and last respectively:
    if (isOk) {
        if (fileEntries.contains(qsl("config.lua"))) {
            if (!writeFileToZip(qsl("config.lua"), fileEntries.value(qsl("config.lua")), archive).first) {
                /* isOk = false; */
            } else {
                fileEntries.remove(qsl("config.lua"));
            }
        }
    }

    QString xmlFileName = packageName;
    xmlFileName.append(QLatin1String(".xml"));
    if (isOk) {
        QMapIterator<QString, QString> itFileName(fileEntries);
        while (itFileName.hasNext() && isOk) {
            itFileName.next();
            if (itFileName.key() == xmlFileName) {
                continue;
            }

            if (std::tie(isOk, error) = writeFileToZip(itFileName.key(), itFileName.value(), archive); !isOk) {
                zip_discard(archive);
                break;
            }
        }
    }

    if (isOk) {
        if (fileEntries.contains(xmlFileName) && fileEntries.value(xmlFileName) == xmlPathFileName) {
            std::tie(isOk, error) = writeFileToZip(xmlFileName, xmlPathFileName, archive);

            // If successful will get to HERE...

        } else {
            return {false,
                    tr("Required file \"%1\" was not found in the staging area. "
                       "This area contains the Mudlet items chosen for the package, "
                       "which you selected to be included in the package file. "
                       "This suggests there may be a problem with that directory: "
                       "\"%2\" - "
                       "Do you have the necessary permissions and free disk-space?")
                            .arg(xmlPathFileName, QDir(stagingDirName).canonicalPath())};
        }
    }

    if (isOk) {
        zip_set_archive_comment(archive, packageComment.toUtf8().constData(), static_cast<zip_uint16_t>(packageComment.toUtf8().length()));

#if defined(LIBZIP_SUPPORTS_CANCELLING)
        auto cancel_callback = [](zip*, void*) -> int { return !mExportingPackage; };
        zip_register_cancel_callback_with_state(archive, cancel_callback, nullptr, nullptr);
#endif

        // THIS is the point that the archive gets created from the
        // source materials - it may take a short while!
        // If it fails to write out the new file 'archive' is left
        // unchanged (and we can still access it to get the error
        // details):
        ze = zip_close(archive);
        if (ze) {
            // libzip's C interface around the error message isn't trivial - so copy it over into Qt land where things are simpler
            const QString zipError{zip_strerror(archive)};
            if (zipError == qsl("Operation cancelled")) {
                zip_discard(archive);
                return {false, tr("Export cancelled.")};
            }

            /*: This error message is displayed at the final stage of exporting
             * a package when all the sourced files are finally put into the
             * archive. Unfortunately this may be the point at which something
             * breaks because a problem was not spotted/detected in the process
             * earlier...
             */
            const QString errorMsg = tr("Failed to zip up the package. Error is: \"%1\".").arg(zipError);
            zip_discard(archive);
            // In libzip 0.11 a function was added to clean up
            // (deallocate) the memory associated with an archive
            // - which would normally occur upon a successful close
            // - before that version the memory just leaked away...
            return {false, errorMsg};
        }

    } else {
        zip_discard(archive);
    }

    return {isOk, error};
}

void dlgPackageExporter::slot_addFiles()
{
    QFileDialog* fDialog = new QFileDialog;
    fDialog->setFileMode(QFileDialog::Directory);
    fDialog->setOption(QFileDialog::DontUseNativeDialog);

    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();
    fDialog->setDirectory(lastDir);

    QStringList selectedFiles;
    //change file dialog children functions to support multiple folder+file selection
    //as qt doesn't seem to support that out of the box
    QDialogButtonBox* dialogBox = fDialog->findChild<QDialogButtonBox*>();
    QPushButton* button = dialogBox->button(QDialogButtonBox::Open);
    QListView* dialogListView = fDialog->findChild<QListView*>("listView");

    if (dialogListView) {
        dialogListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        //button would be disabled if no folder is selected
        connect(dialogListView, &QListView::clicked, this, [=, this] { button->setEnabled(true); });
    }
    QTreeView* dialogTreeView = fDialog->findChild<QTreeView*>();
    if (dialogTreeView) {
        dialogTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(dialogTreeView, &QTreeView::clicked, this, [=, this] { button->setEnabled(true); });
    }
    connect(button, &QPushButton::clicked, this, [=, this] { fDialog->QDialog::accept(); });
    if (fDialog->exec()) {
        selectedFiles = fDialog->selectedFiles();
    }
    if (!selectedFiles.isEmpty()) {
        ui->listWidget_addedFiles->addItems(selectedFiles);

        lastDir = fDialog->directory().absolutePath();
        settings.setValue("lastFileDialogLocation", lastDir);
    }
    fDialog->deleteLater();
}

void dlgPackageExporter::slot_openPackageLocation()
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    mPackagePath = QFileDialog::getExistingDirectory(
            nullptr, tr("Where do you want to save the package?"), lastDir, QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);

    if (mPackagePath.isEmpty()) {
        return;
    }
    lastDir = QFileInfo(mPackagePath).absolutePath();
    settings.setValue("lastFileDialogLocation", lastDir);
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

void dlgPackageExporter::checkChildren(QTreeWidgetItem* item) const
{
    if (!mCheckChildren) {
        return;
    }
    const QString packageName = ui->packageList->currentText();
    auto checkState = item->checkState(0);
    // Don't check top folder if it has the same name as the package
    if (item->text(0) == packageName && item->data(0, Qt::UserRole) == isTopFolder) {
        item->setCheckState(0, Qt::Unchecked);
    }

    for (int i = 0; i < item->childCount(); i++) {
        item->child(i)->setCheckState(0, checkState);
    }
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
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
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
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
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
        pItem->setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        if (pChild->isFolder() && pChild->getScript().isEmpty()) {
            pItem->setData(0, Qt::UserRole, isTopFolder);
        }
        timerMap.insert(pItem, pChild);
        top->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::displayResultMessage(const QString& html, const bool isSuccessMessage)
{
    if (!isSuccessMessage) {
        // Big RED error message
        ui->infoLabel->setText(qsl("<p><font color='red'><b><big>%1</big><b></font></p>").arg(html));
        return;
    }

    // Big BLACK (Green would be hard for most common colour blind people to
    // tell from Red, and Blue would likely hide the URL) success message:
    ui->infoLabel->setText(qsl("<p><b><big>%1</big><b></p>"
                                          "<p>%2</p>")
                           .arg(html,
                            /*:
                            Only the text outside of the 'a' (HTML anchor) tags PLUS the verb
                            'upload' in between them in the source text, (associated with uploading
                            the resulting package to the Mudlet forums) should be translated.
                            */
                                tr("Why not <a href=\"https://packages.mudlet.org/upload\">upload</a> your package for other Mudlet users?")));
    ui->infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->infoLabel->setOpenExternalLinks(true);
}

void dlgPackageExporter::slot_recountItems(QTreeWidgetItem *item)
{

    checkChildren(item);
    static bool debounce;
    if (!debounce) {
        debounce = true;
        QTimer::singleShot(0, this, [this]() {
            debounce = false;

            const int itemsToExport = countCheckedItems();
            if (itemsToExport) {
                //: This is the text shown at the top of a groupbox when there is %n (one or more) items to export in the Package exporter dialogue; the initial (and when there is no items selected) is a separate text.
                mpSelectionText->setTitle(tr("Select what to export (%n item(s))", nullptr, itemsToExport));
            } else {
                //: This is the text shown at the top of a groupbox initially and when there is NO items to export in the Package exporter dialogue.
                mpSelectionText->setTitle(tr("Select what to export"));
            }
        });
    }
}

// allow the top-folder to be force-selected for the package with right-click
// normally undesired as it'll add unnecessary nesting
void dlgPackageExporter::slot_rightClickOnItems(const QPoint& point)
{
    auto item = mpExportSelection->itemAt(point);
    if (!item || item->parent() == nullptr) {
        return;
    }
    mCheckChildren = false;
    if (item->checkState(0) == Qt::Checked) {
        item->setCheckState(0, Qt::Unchecked);
    } else {
        item->setCheckState(0, Qt::Checked);
    }
    mCheckChildren = true;
}

QString dlgPackageExporter::getActualPath() const
{
    QSettings& settings = *mudlet::getQSettings();
    QString lastDir = settings.value("lastFileDialogLocation", QDir::homePath()).toString();

    return mPackagePath.isEmpty() ? lastDir : mPackagePath;
}

void dlgPackageExporter::slot_cancelExport()
{
    mExportingPackage = false;
    checkToEnableExportButton();

    mCancelButton->setVisible(false);
    mCloseButton->setVisible(true);
}

//Description Class TextEdit
dlgPackageExporterDescription::dlgPackageExporterDescription(QWidget* pW) : QTextEdit(pW) {}
dlgPackageExporterDescription::~dlgPackageExporterDescription() {}

bool dlgPackageExporterDescription::canInsertFromMimeData(const QMimeData* source) const
{
    if (source->hasUrls()) {
        return true;
    }
    return QTextEdit::canInsertFromMimeData(source);
}

void dlgPackageExporterDescription::insertFromMimeData(const QMimeData* source)
{
    dlgPackageExporter* my_parent = static_cast<dlgPackageExporter*>(topLevelWidget());
    if (source->hasUrls()) {
        QTextCursor myCursor = textCursor();
        const int oldPos = myCursor.position();
        // Allows to insert image at cursor position if using copy/paste
        if (hasFocus()) {
            myCursor.setPosition(oldPos);
        } else {
            setPlainText(my_parent->mPlainDescription);
        }
        QStringList accepted_types;
        accepted_types << "jpeg"
                       << "jpg"
                       << "png"
                       << "gif"
                       << "bmp"
                       << "svg";
        for (const auto& url : source->urls()) {
            const QString fname = url.toLocalFile();
            const QFileInfo info(fname);
            if (info.exists() && accepted_types.contains(info.suffix().trimmed(), Qt::CaseInsensitive)) {
                if (!my_parent->mDescriptionImages.contains(fname)) {
                    my_parent->mDescriptionImages.append(fname);
                }
                const QString imgSrc = qsl("![Image]($%1)").arg(info.fileName());
                myCursor.insertText(imgSrc);
            }
        }
        my_parent->mPlainDescription = toPlainText();
        //setMarkdown so images can be seen as they will appear in the description
        if (!hasFocus()) {
            QString plainText = my_parent->mPlainDescription;
            for (int i = my_parent->mDescriptionImages.size() - 1; i >= 0; i--) {
                QString fname = my_parent->mDescriptionImages.at(i);
                const QFileInfo info(fname);
                fname = QUrl::toPercentEncoding(fname).constData();
                plainText.replace(qsl("$%1").arg(info.fileName()), fname);
            }
            setMarkdown(plainText);
        }
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}

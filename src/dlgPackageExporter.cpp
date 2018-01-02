/***************************************************************************
 *   Copyright (C) 2012-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017 by Stephen Lyons - slysven@virginmedia.com   *
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

/* NOTE:
 *
 * I18n: This sub-system does NOT respond to GUI Language change events because
 * it uses some translatable strings in QTreeWidgetItems and if those are
 * changed by a language change we would have to repopulate the entire dialog
 * - it is simpler not to change the language - and to ensure the dialogue is
 * destroyed after use so that a later call to this class after a language
 * uses what ever language is then selected.
 */

#include "dlgPackageExporter.h"


#include "mudlet.h"
#include "Host.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "XMLexport.h"

#include "pre_guard.h"
#include <QCheckBox>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include "post_guard.h"

#include <errno.h>


using namespace std;

// Creates a temporary, uniquely named directory in the system tempory directory
// area (e.g. /tmp/mudletPackageExporter123456 on linux)
// Into that directory places the <packageName>.xml file containing the Mudlet
// items, AND a config.lua file containing some Lua script that (currently just
// sets an "mpackage" variable to the string of the package name give.
// The user can then open a system file browser window at that location into
// which they can drag and drop other files and folders
// The whole lot is then combined into a single archive file (".mpackage")
// which is a zip format archive file...
dlgPackageExporter::dlgPackageExporter(QWidget *parent, Host* host)
: QDialog(parent)
, mpHost(host)
, mHostName(host->getName())
, mIsToKeepStagedFiles(true)
, mIsOverWriteEnabled(false)
, mTempDir(QStringLiteral("%1/mudletPackageExporterXXXXXX").arg(QDir::tempPath()))
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

// If debugging might want to uncomment this line to make the temporary
// directory persist beyond the lifetime of this class
//    mTempDir.setAutoRemove(false);

    textLabel_informationText->setText(QStringLiteral("<html><head/><body>%1</body></html>")
                                       .arg(tr("<p>Select the Mudlet items above that you wish to include in the package/module "
                                               "- unlike the <i>Export</i> action in the main Editor you may include different "
                                               "types of Mudlet item (<i>Aliases</i>, <i>Buttons</i>, <i>Key bindings</i> etc. "
                                               "but <b>not <i>variables</i></b>), into one unit that can be shared (and "
                                               "synchronised) between different profiles and can be shared with other Mudlet "
                                               "users.</p>"
                                               "<p>If you want to add additional files to the package e.g. images, icons, "
                                               "sounds, etc. that the above items or other packages/modules can use, then "
                                               "the <i>Add files</i> button brings up a system file explorer window pointing "
                                               "a temporary directory (which will be automatically deleted after the package"
                                               "/module has been created). Copy all the files (and folders) that you want "
                                               "to add to the package into this temporary directory and the structure will "
                                               "be replicated in the finished package/module.</p>"
                                               "<p><b>Do not move or edit the \"config.lua\" file that will be present - it "
                                               "is used internally by the Mudlet package/module system.</b></p>"
                                               "<p>When you are happy with the content click on the <i>Export</i> button "
                                               "below to start the creation of the package/module named below; a result "
                                               "message should appear shortly afterwards indicating whether the process was "
                                               "successful or not - if a temporary stage directory was selected it will "
                                               "dissappear when this dialog is closed...</p>",
                                               "Please try and preserve the HTML tags in any translation, note that </p><p> "
                                               "are used to separate adjacent paragraphs. The terms in italics (<i>...</i>) "
                                               "should reflect the translations used elsewhere and the <i>Add files</i> one "
                                               "should match the text used for a button elsewhere on the dialog/form.")));

    // The close button in the QDialogButtonBox has been "wired up" to the
    // dialog "reject" slot in the dlgPackageExporter.ui dialog/form so does not
    // have a connect(...) here.

    // The previously added "Export" button has been replaced by an "Reset"
    // buttonBox button that has the intrinsic "Reset" role but which is renamed
    // to say "Export" - note that we do not use the "Ok", "Yes" or "Accept"
    // role buttons as they all automagically cause the dialog to close and we
    // do NOT want that as we want it to stay to show the results message until
    // the close button is hit.
    exportButton = buttonBox->button(QDialogButtonBox::Reset);
    exportButton->setText(tr("&Export"));

    addFilesButton = buttonBox->button(QDialogButtonBox::Apply);
    addFilesButton->setText(tr("&Add files"));

    // Test and set if needed mudlet::mIsIconShownOnDialogButtonBoxes - if there
    // is already a Qt provided icon on a predefined button
    mudlet::self()->setIconsShownOnDialogButtonBoxes(!exportButton->icon().isNull());

    if (mudlet::self()->isIconsShownOnDialogButtonBoxes()) {
        // Provide replacement icons if needed
        exportButton->setIcon(QIcon(QStringLiteral(":/icons/application-zip.png")));
        addFilesButton->setIcon(QIcon(QStringLiteral(":/icons/insert-documents-multiple.png")));
    }

    // This widget was called filePath but it shadowed a QString class member of
    // the same name when the class was switched over to using the "Multiple
    // Inheritance" method of "Embedding of the UI Class" that we are using for
    // other forms...!
    lineEdit_filePath->hide();
    textLabel_exportLocation->hide();

    auto pD = new QDialog(this);
    pD->setWindowModality(Qt::NonModal);
    // Need to force it wide enough to show full title text.
    pD->setMinimumWidth(300);
    auto pL = new QGridLayout;
    pD->setLayout(pL);
    pD->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    pD->setContentsMargins(0, 0, 0, 0);
    pD->setWindowTitle(tr("New Module details"));
    auto pLabel_packageName = new QLabel(tr("Module name:"));
    auto pLineEdit_packageName = new QLineEdit(pD);
    pLabel_packageName->setBuddy(pLineEdit_packageName);
    pLineEdit_packageName->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                      .arg(tr("<p>What name do you wish to assign to this module?</p>"
                                              "<p><i>It will also appear in the Editor as the top level "
                                              "(brown) folder that contains each type of Mudlet item that "
                                              "you include in it.</i></p>")));

    // TODO: Add additional entry widgets for other module details here...




    auto pCheckBox_isStagingAreaPersistent = new QCheckBox(tr("Retain staged files?"), pD);
    pCheckBox_isStagingAreaPersistent->setChecked(true);
    pCheckBox_isStagingAreaPersistent->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                  .arg(tr("<p>If this checkbox is cleared then the area where the Module is assembled "
                                                          "before being <i>zipped up</i> into a single archive file will be made temporary "
                                                          "and will get cleaned up after the Module is created.</p>"
                                                          "<p><i>Leave this checked if you anticipate that you will want to revise and "
                                                          "update the Module in the future and you will want to reuse any additional "
                                                          "files again; this option being checked reproduces the behavior in previous "
                                                          "versions of Mudlet.</i></p>")));

    auto pCheckBox_isOverwritingAllowed = new QCheckBox(tr("Overwrite existing package/module?"), pD);
    pCheckBox_isOverwritingAllowed->setChecked(false);
    pCheckBox_isOverwritingAllowed->setToolTip(QStringLiteral("<html><head/><body>%1</body></html>")
                                                  .arg(tr("<p>If this checkbox is cleared then Mudlet will not allow an existing module "
                                                          "in the destination directory to be replaced by a new one with the same name.</p>"
                                                          "<p><i>You will want to have this option checked if you are revising and updating "
                                                          "a Module though you may want to ensure you have a backup copy <b>before</b> proceeding; "
                                                          "this option being cleared reproduces the behavior in previous versions of Mudlet.</i></p>")));

    auto pButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, pD);
    pL->addWidget(pLabel_packageName, 0, 0);
    pL->addWidget(pLineEdit_packageName, 0, 1);


    // TODO: Insert additional entry widgets for other module details onto form here...


    pL->addWidget(pCheckBox_isStagingAreaPersistent, 1, 0, 1, 2);
    pL->addWidget(pCheckBox_isOverwritingAllowed, 2, 0, 1, 2);
    pL->addWidget(pButtonBox, 3, 0, 1, 2);
    pD->adjustSize();

    connect(pButtonBox, SIGNAL(accepted()), pD, SLOT(accept()));
    connect(pButtonBox, SIGNAL(rejected()), pD, SLOT(reject()));

    if (pD->exec() == QDialog::Accepted) {
        mPackageName = pLineEdit_packageName->text();
        if (mPackageName.isEmpty()) {

            close();
            return;
        }

        // TODO: gather/validate other module details
        mIsOverWriteEnabled = pCheckBox_isOverwritingAllowed->isChecked();
        mIsToKeepStagedFiles = pCheckBox_isStagingAreaPersistent->isChecked();
    } else {
        close();
        return;
    }


    /*
     * Starting directory is mudlet's base one
     * From Qt Docs, note that:
     * "On Windows and OS X, this static function
     * [QFileDialog::getExistingDirectory(...)] will use the native file dialog
     * and not a QFileDialog. However, the native Windows file dialog does not
     * support displaying files in the directory chooser. You need to pass
     * DontUseNativeDialog to display files using a QFileDialog."
     *
     * Which is why I have chosen to move away from the Native one so that
     * existing packages and other files which might clash with this one will be
     * visible - Slysven
     *
     * Also:
     * "On Windows, the dialog will spin a blocking modal event loop that will
     * not dispatch any QTimers, and if parent is not 0 then it will position
     * the dialog just below the parent's title bar."
     */

    QString packagePath = QFileDialog::getExistingDirectory(this,
                                                            tr("Where do you want to save the package?"),
                                                            mudlet::getMudletPath(mudlet::mainPath),
                                                            QFileDialog::DontUseNativeDialog);

    if (packagePath.isEmpty()) {
        close();
        return;
    }

    // CHECKME: Seems pointless...
    packagePath.replace(R"(\)", "/");

    // The zip file will be in the intended destination
    mZipFile = QStringLiteral("%1/%2.mpackage").arg(packagePath, mPackageName);
    // filePath is the name of the XML file with the Mudlet items going into
    // the package/module
    mXmlFile=QStringLiteral("%1.xml").arg(mPackageName);
    lineEdit_filePath->setText(mZipFile);
    lineEdit_filePath->show();
    textLabel_exportLocation->show();

    if (mIsToKeepStagedFiles) {
        mStagingDir = mudlet::getMudletPath(mudlet::profilePackageStagingPathFileName, mHostName, mPackageName);
    } else if (mTempDir.isValid()) {
        mStagingDir = QDir(mTempDir.path());
    } else {
        qWarning() << "dlgPackageExporter::dlgPackageExporter() ERROR - a tempory staging directory was specified but was not created - aborting!";
        close();
        return;
    }

    if (!mStagingDir.exists()) {
        // This is "." on creation, but we need this instance of a QDir because
        // the QDir::mkpath(path) is NOT a static function - provided the "path"
        // argument is not a relative path then it is irrelevant  because only
        // in the relative case is it used as the point that the "path" is
        // relative to!
        QDir temp;
        temp.mkpath(mStagingDir.absolutePath());
    }

    QString luaConfig = QStringLiteral("%1/config.lua").arg(mStagingDir.path());
    QFile configFile(luaConfig);
    if (configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&configFile);

        out << QStringLiteral("mpackage = \"%1\"\n").arg(mPackageName);


        // TODO: Add other *interesting* (in a Lua variable format) data items
        // form to config.lua file...


        out.flush();
        configFile.close();
    }

    connect(addFilesButton, SIGNAL(clicked()), this, SLOT(slot_addFiles()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(slot_export_package()));

    listTriggers();
    listAliases();
    listKeys();
    listScripts();
    listActions();
    listTimers();

}

dlgPackageExporter::~dlgPackageExporter()
{
    ; // No-op now...!
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
    struct zip_source* s = zip_source_file(archive, fileSystemFileName.toStdString().c_str(), 0, -1);
    if (s == nullptr) {
        displayResultMessage(tr("Failed to source file \"%1\" to place into module (archive) file, error is: \"%3\".")
                             .arg(fileSystemFileName, QString::fromUtf8(zip_strerror(archive))), false);
        return false;
    }

#if (LIBZIP_VERSION_MAJOR >= 1)
    if (zip_file_add(archive, archiveFileName.toStdString().c_str(), s, ZIP_FL_ENC_UTF_8) == -1) {
#else
    // We were using zip_add(...) but that is obsolete (and does not necessarily
    // support UTF-8 encoded file-names)...
    if (zip_add(archive, archiveFileName.toStdString().c_str(), s) == -1) {
#endif
        displayResultMessage(tr("Failed to add file \"%1\" to module (archive) file \"%2\", error message was: \"%3\".")
                             .arg(archiveFileName, mZipFile, QString::fromUtf8(zip_strerror(archive))), false);
        return false;
    } else {
        return true;
    }
}

void dlgPackageExporter::slot_export_package()
{
    // Prevent a second call and give the button a "used" look:
    exportButton->setCheckable(true);
    exportButton->setChecked(true);
    exportButton->setEnabled(false);

    addFilesButton->setEnabled(false);

    QFile file_xml(QStringLiteral("%1/%2").arg(mStagingDir.path(), mXmlFile));
    QList<QTreeWidgetItem*> trigList;
    QList<QTreeWidgetItem*> timerList;
    QList<QTreeWidgetItem*> aliasList;
    QList<QTreeWidgetItem*> actionList;
    QList<QTreeWidgetItem*> scriptList;
    QList<QTreeWidgetItem*> keyList;

    if (file_xml.open(QIODevice::WriteOnly)) {
        XMLexport writer(mpHost);
        QTreeWidgetItem* top = nullptr;
        //write trigs
        QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Triggers", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, trigList);
            for (auto item : trigList) {
                if (item->checkState(0) == Qt::Unchecked && triggerMap.contains(item)) {
                    triggerMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && triggerMap.contains(item) && triggerMap[item]->mModuleMasterFolder) {
                    triggerMap[item]->mModuleMasterFolder = false;
                    modTriggerMap.insert(item, triggerMap[item]);
                }
            }
        }
        items = treeWidget->findItems(tr("Timers", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, timerList);
            for (auto item : timerList) {
                if (item->checkState(0) == Qt::Unchecked && timerMap.contains(item)) {
                    timerMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && timerMap.contains(item) && timerMap[item]->mModuleMasterFolder) {
                    timerMap[item]->mModuleMasterFolder = false;
                    modTimerMap.insert(item, timerMap[item]);
                }
            }
        }
        items = treeWidget->findItems(tr("Aliases", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, aliasList);
            for (auto item : aliasList) {
                if (item->checkState(0) == Qt::Unchecked && aliasMap.contains(item)) {
                    aliasMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && aliasMap.contains(item) && aliasMap[item]->mModuleMasterFolder) {
                    aliasMap[item]->mModuleMasterFolder = false;
                    modAliasMap.insert(item, aliasMap[item]);
                }
           }
        }
        items = treeWidget->findItems(tr("Buttons", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, actionList);
            for (auto item : actionList) {
                if (item->checkState(0) == Qt::Unchecked && actionMap.contains(item)) {
                    actionMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && actionMap.contains(item) && actionMap[item]->mModuleMasterFolder) {
                    actionMap[item]->mModuleMasterFolder = false;
                    modActionMap.insert(item, actionMap[item]);
                }
           }
        }
        items = treeWidget->findItems(tr("Scripts", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, scriptList);
            for (auto item : scriptList) {
                if (item->checkState(0) == Qt::Unchecked && scriptMap.contains(item)) {
                    scriptMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && scriptMap.contains(item) && scriptMap[item]->mModuleMasterFolder) {
                    scriptMap[item]->mModuleMasterFolder = false;
                    modScriptMap.insert(item, scriptMap[item]);
                }
            }
        }
        items = treeWidget->findItems(tr("Keys", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (1 of 3)"), Qt::MatchExactly, 0);
        if (items.count()) {
            top = items.first();
            recurseTree(top, keyList);
            for (auto item : keyList) {
                if (item->checkState(0) == Qt::Unchecked && keyMap.contains(item)) {
                    keyMap[item]->exportItem = false;
                } else if (item->checkState(0) == Qt::Checked && keyMap.contains(item) && keyMap[item]->mModuleMasterFolder) {
                    keyMap[item]->mModuleMasterFolder = false;
                    modKeyMap.insert(item, keyMap[item]);
                }
            }
        }

        QPair<bool, QString> results = writer.exportGenericPackage(&file_xml);
        file_xml.close();

        //now fix all the stuff we weren't exporting
        //trigger, timer, alias,action,script, keys
        for (auto item : trigList) {
            if (triggerMap.contains(item)) {
                triggerMap[item]->exportItem = true;
            }
            if (modTriggerMap.contains(item)) {
                modTriggerMap[item]->mModuleMasterFolder = true;
            }
        }
        for (auto item : timerList) {
            if (timerMap.contains(item)) {
                timerMap[item]->exportItem = true;
            }
            if (modTimerMap.contains(item)) {
                modTimerMap[item]->mModuleMasterFolder = true;
            }
        }
        for (auto item : actionList) {
            if (actionMap.contains(item)) {
                actionMap[item]->exportItem = true;
            }
            if (modActionMap.contains(item)) {
                modActionMap[item]->mModuleMasterFolder = true;
            }
        }
        for (auto item : scriptList) {
            if (scriptMap.contains(item)) {
                scriptMap[item]->exportItem = true;
            }
            if (modScriptMap.contains(item)) {
                modScriptMap[item]->mModuleMasterFolder = true;
            }
        }
        for (auto item : keyList) {
            if (keyMap.contains(item)) {
                keyMap[item]->exportItem = true;
            }
            if (modKeyMap.contains(item)) {
                modKeyMap[item]->mModuleMasterFolder = true;
            }
        }
        for (auto item : aliasList) {
            if (aliasMap.contains(item)) {
                aliasMap[item]->exportItem = true;
            }
            if (modAliasMap.contains(item)) {
                modAliasMap[item]->mModuleMasterFolder = true;
            }
        }

        bool isOk = true;
        if (!results.first) {
            // Oh dear, failed to write module XML file
            displayResultMessage(tr("Failed to create \"%1\" file containing the Mudlet items in module (archive) file \"%2\", the error message was: \"%3\".")
                                 .arg(mXmlFile, mZipFile, results.second), false);
            isOk = false;
        }

        if (isOk) {
            // zip error code:
            int ze = 0;
/*
 * Relevant options:
 * * ZIP_CREATE creates the archive if it does not exist.
 * * ZIP_EXCL causes an error if the file already exists to prevent overwriting
 * one that might actually be in use in this or another profile.
 * * ZIP_TRUNCATE zaps any contents in a previously existing file.
 */
            // Note that QString::toStdString() includes an implicit conversion
            // from QString to UTF-8 encoding for the const char * type result:

            // We previously only used ZIP_CREATE but without ZIP_TRUNCATE that
            // will always break when we used zip_add() to replace config.lua
            // because we should have used zip_replace() instead (and the 0.1x
            // versions with zip_add() only lacks the option of the later
            // zip_file_add() which DOES have an overwrite option in 1.x)
            // Give that the previous code would fail in this way I think we
            // should add the ZIP_EXCL option to reproduce that effect but now
            // be clear and obvious about it...
            zip* archive;
            if (mIsOverWriteEnabled) {
#if (LIBZIP_VERSION_MAJOR >=1 || LIBZIP_VERSION_MINOR >= 11)
                archive = zip_open(mZipFile.toStdString().c_str(), ZIP_CREATE|ZIP_TRUNCATE, &ze);
#else
                // The wanted ZIP_TRUNCATE flag is not present before 0.11 *sigh*
                // So trash any existing archive file:
                QFile oldArchiveFile(mZipFile);
                if (oldArchiveFile.exists()) {
                    // This could fail
                    if (!oldArchiveFile.remove()) {
                        displayResultMessage(tr("Failed to erase old module (archive) file do you have the correct file/directory permissions?"), false);
                        isOk = false;
                    }
                }

                if (isOk) {
                    archive = zip_open(mZipFile.toStdString().c_str(), ZIP_CREATE, &ze);
                }
#endif
            } else {
                archive = zip_open(mZipFile.toStdString().c_str(), ZIP_CREATE|ZIP_EXCL, &ze);
            }

            if (!isOk) {
                // No-op, needed to avoid either of the other branches should
                // the pre-0.11 workaround not work without a massive extra
                // if(...) which would be more noisy in git terms. 8-)
                ;
            } else if (!archive) {
                // Failed to open/create archive file
#if (LIBZIP_VERSION_MAJOR >= 1)
                // A better error handling system (not requiring a previously
                // defined finite-sized char type buffer {which obviously can
                // have string buffer overflow issues} is available in 1.x
                // versions of libzip):
                zip_error_t error;
                zip_error_init_with_code(&error, ze);
                displayResultMessage(tr("Failed to open module (archive) file, error is: \"%1\".")
                                     .arg(QString::fromUtf8(zip_error_strerror(&error))), false);
                zip_error_fini(&error);
#else
                // Uses system errno?
                char errorMessageBuffer[128];
                zip_error_to_str(errorMessageBuffer, sizeof(errorMessageBuffer), ze, errno);
                displayResultMessage(tr("Failed to open module (archive) file, error is: \"%1\".")
                                     .arg(QString::fromUtf8(errorMessageBuffer)), false);
#endif
                isOk = false;
            } else {
                // Opened/created archive file successfully
/*
 * Previous code here failed if the user included sub-directories in the
 * temporary directory where the contents for the package/module was assembled -
 * as sub-directories are now correctly handled by the installer code we needed
 * to revise the code here to also work when the user uses them. NB This is
 * typically the case where they have collections of images or sound files
 * which they wish to store in a heirachical manner...! - Slysven
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
#endif
                QDirIterator itDir(mStagingDir.path(), QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files, QDirIterator::Subdirectories);
                // relative names to use in archive:
                QStringList directoryEntries;
                // Key is relative name to use in archive
                // Value is fullName in file-system:
                QMap<QString, QString> fileEntries;
                while (itDir.hasNext() && isOk) {
                    QString itEntry = itDir.next();
                    Q_UNUSED(itEntry);
//                Comment out the preceding line if the following is uncommented!
//                qDebug() << " parsing entry:" << itEntry << " fileName() is:" << itDir.fileName() << " filePath() is:" << itDir.filePath();
                    if (!(  itDir.fileName().compare(QStringLiteral("."))
                         && itDir.fileName().compare(QStringLiteral("..")))) {

                        // Dot and DotDot entries are no use to us so skip them
                        continue;
                    }

                    QFileInfo entryInfo(itDir.fileInfo());
                    if (! entryInfo.isReadable()) {
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
                    nameInArchive.remove(QStringLiteral("%1/").arg(mStagingDir.path()));

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
                    // zip_add_dir()/zip_dir_add(...) returns the index of the
                    // added item in the archive or -1 on error:
#if (LIBZIP_VERSION_MAJOR >= 1)
                    if (zip_dir_add(archive, directoryName.toStdString().c_str(), ZIP_FL_ENC_UTF_8) == -1) {
#else
                    // using zip_dir_add(...) is obsolete (and does not necessarily
                    // support UTF-8 encoded file-names)...
                    if (zip_add_dir(archive, directoryName.toStdString().c_str()) == -1) {
#endif
                        displayResultMessage(tr("Failed to add directory \"%1\" to module (archive) file, error is: \"%2\".")
                                             .arg(directoryName, QString::fromUtf8(zip_strerror(archive))), false);
                        zip_close(archive);
                        isOk = false;
                    }
                }

                // Process the config and the file containing the Mudlet triggers,
                // etc specially so they are inserted first and last respectively:
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
                        displayResultMessage(tr("Required \"config.lua\" file not found to include in module (archive) file, did you remove or rename it?"), false);
                        zip_close(archive);
                        isOk = false;
                    }*/
                }

                if (isOk) {
                    QMapIterator<QString, QString> itFileName(fileEntries);
                    while (itFileName.hasNext() && isOk) {
                        itFileName.next();
                        if (itFileName.key() == mXmlFile) {
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
                    if (fileEntries.contains(mXmlFile)) {
                        if (!writeFileToZip(mXmlFile, fileEntries.value(mXmlFile), archive)) {
                            zip_close(archive);
                            isOk = false;
                        }

                        // If successful will get to HERE...
                    } else {
                        displayResultMessage(tr("Required \"%1\" file that contains the mudlet items chosen for the "
                                                "module was not found in the staging area from where it would be include in the "
                                                "module (archive) file - this suggests there is a problem with the "
                                                "permissions for the \"%2\" directory!")
                                                .arg(mXmlFile, mStagingDir.canonicalPath()), false);
                        zip_close(archive);
                        isOk = false;
                    }
                }

                if (isOk) {
                    // THIS is the point that the archive gets created from the
                    // source materials - it may take a short while. If it fails
                    // to write out the new file 'archive' is left unchanged
                    // (and we can still access it to get the error details):
                    ze = zip_close(archive);
                    if (ze != 0) {
                        displayResultMessage(tr("Failed to populate and then close module (archive) file, error is: \"%1\".")
                                             .arg(QString::fromUtf8(zip_strerror(archive))), false);
#if (LIBZIP_VERSION_MAJOR >= 1 || LIBZIP_VERSION_MINOR >= 11)
                        // In 0.11 a function was added to clean up (deallocate)
                        // the memory associated with an archive - which would
                        // normally occur upon a sucessful close - before then
                        // the memory leaked away...
                        zip_discard(archive);
#endif
                        isOk = false;
                    }
                }

                if (isOk) {
                    // Success!
                    displayResultMessage(tr("Exported module \"%1\".").arg(mZipFile), true);
                }
            }
        }
    }
}

void dlgPackageExporter::displayResultMessage(const QString& html, bool const isSuccessMessage)
{
    infoLabel->setText(QStringLiteral("<html><head/><body><p><font color='%1'><b><big>%2</big><b></font></p></body></html>")
                       .arg(isSuccessMessage ? QLatin1String("blue") : QLatin1String("red"), html));
}

void dlgPackageExporter::slot_addFiles()
{
    QString stagingPathUrlText = QStringLiteral("file://%1").arg(mStagingDir.path());
    QDesktopServices::openUrl(QUrl(stagingPathUrlText, QUrl::TolerantMode));
}

void dlgPackageExporter::recurseTriggers(TTrigger* trig, QTreeWidgetItem* qTrig)
{
    list<TTrigger*>* childList = trig->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TTrigger*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TTrigger* pChild = *it;
        if (pChild->isTemporary())
            continue;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        triggerMap.insert(pItem, pChild);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qTrig->addChild(pItem);
        recurseTriggers(pChild, pItem);
    }
}

void dlgPackageExporter::listTriggers()
{
    TriggerUnit* tu = mpHost->getTriggerUnit();
    list<TTrigger*>::const_iterator it;
    std::list<TTrigger*> tList = tu->getTriggerRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Triggers", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        // If the GUI language gets changed between starting to use this class
        // and this point in execution then we won't find any items (which would
        // cause a fatal crashes if we didn't do an items.count() check...)!
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TTrigger* pChild = *it;
            if (pChild->isTemporary()) {
                continue;
            }
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            top->addChild(pItem);
            triggerMap.insert(pItem, pChild);
            recurseTriggers(pChild, pItem);
        }
    }
}

void dlgPackageExporter::recurseAliases(TAlias* item, QTreeWidgetItem* qItem)
{
    list<TAlias*>* childList = item->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TAlias*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TAlias* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        qItem->addChild(pItem);
        aliasMap.insert(pItem, pChild);
        recurseAliases(pChild, pItem);
    }
}

void dlgPackageExporter::listAliases()
{
    AliasUnit* tu = mpHost->getAliasUnit();
    list<TAlias*>::const_iterator it;
    std::list<TAlias*> tList = tu->getAliasRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Aliases", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TAlias* pChild = *it;
            if (pChild->isTemporary()) {
                continue;
            }
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            top->addChild(pItem);
            aliasMap.insert(pItem, pChild);
            recurseAliases(pChild, pItem);
        }
    }
}

void dlgPackageExporter::recurseScripts(TScript* item, QTreeWidgetItem* qItem)
{
    list<TScript*>* childList = item->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TScript*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TScript* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        scriptMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseScripts(pChild, pItem);
    }
}

void dlgPackageExporter::listScripts()
{
    ScriptUnit* tu = mpHost->getScriptUnit();
    list<TScript*>::const_iterator it;
    std::list<TScript*> tList = tu->getScriptRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Scripts", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TScript* pChild = *it;
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            scriptMap.insert(pItem, pChild);
            top->addChild(pItem);
            recurseScripts(pChild, pItem);
        }
    }
}

void dlgPackageExporter::recurseKeys(TKey* item, QTreeWidgetItem* qItem)
{
    list<TKey*>* childList = item->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TKey*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TKey* pChild = *it;
        if (pChild->isTemporary())
            continue;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        keyMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseKeys(pChild, pItem);
    }
}

void dlgPackageExporter::listKeys()
{
    KeyUnit* tu = mpHost->getKeyUnit();
    list<TKey*>::const_iterator it;
    std::list<TKey*> tList = tu->getKeyRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Keys", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TKey* pChild = *it;
            if (pChild->isTemporary())
                continue;
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            keyMap.insert(pItem, pChild);
            top->addChild(pItem);
            recurseKeys(pChild, pItem);
        }
    }
}

void dlgPackageExporter::recurseActions(TAction* item, QTreeWidgetItem* qItem)
{
    list<TAction*>* childList = item->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TAction*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TAction* pChild = *it;
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        actionMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseActions(pChild, pItem);
    }
}

void dlgPackageExporter::listActions()
{
    ActionUnit* tu = mpHost->getActionUnit();
    list<TAction*>::const_iterator it;
    std::list<TAction*> tList = tu->getActionRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Buttons", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TAction* pChild = *it;
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            actionMap.insert(pItem, pChild);
            top->addChild(pItem);
            recurseActions(pChild, pItem);
        }
    }
}

void dlgPackageExporter::recurseTimers(TTimer* item, QTreeWidgetItem* qItem)
{
    list<TTimer*>* childList = item->getChildrenList();
    if (!childList->size()) {
        return;
    }
    list<TTimer*>::iterator it;
    for (it = childList->begin(); it != childList->end(); it++) {
        TTimer* pChild = *it;
        if (pChild->isTemporary()) {
            continue;
        }
        QStringList sl;
        sl << pChild->getName();
        auto pItem = new QTreeWidgetItem(sl);
        pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pItem->setCheckState(0, Qt::Unchecked);
        timerMap.insert(pItem, pChild);
        qItem->addChild(pItem);
        recurseTimers(pChild, pItem);
    }
}

void dlgPackageExporter::listTimers()
{
    TimerUnit* tu = mpHost->getTimerUnit();
    list<TTimer*>::const_iterator it;
    std::list<TTimer*> tList = tu->getTimerRootNodeList();
    QList<QTreeWidgetItem*> items = treeWidget->findItems(tr("Timers", "This text is used programmatically in the class implimentation and the associated dialog, ensure all instances have the same text (2 of 3)"), Qt::MatchExactly, 0);
    if (items.count()) {
        QTreeWidgetItem* top = items.first();
        for (it = tList.begin(); it != tList.end(); it++) {
            TTimer* pChild = *it;
            if (pChild->isTemporary()) {
                continue;
            }
            QStringList sl;
            sl << pChild->getName();
            auto pItem = new QTreeWidgetItem(sl);
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsTristate | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            pItem->setCheckState(0, Qt::Unchecked);
            timerMap.insert(pItem, pChild);
            top->addChild(pItem);
            recurseTimers(pChild, pItem);
        }
    }
}

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * The package exporter has no Lua entry point at all - installPackage() reads
 * an .mpackage but nothing in the scripting API writes one - so the only way to
 * exercise the selection tree, the staging directory and the zip writer is to
 * build the dialog and drive it.
 */

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgPackageExporter.h"
#include "mudlet.h"

#include "GroupedTest.h"

class PackageExporterTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mExportDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("PackageExporter-Test-Profile");
    const QString mLocalhost = qsl("localhost");

    const QString mTriggerName = qsl("qaExportedTrigger");
    const QString mAliasName = qsl("qaExportedAlias");
    const QString mTimerName = qsl("qaExportedTimer");
    const QString mScriptName = qsl("qaExportedScript");
    const QString mKeyName = qsl("qaExportedKey");
    const QString mActionName = qsl("qaExportedAction");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // The exporter maps its own tree items onto the profile's items, so this is
    // how a test says "tick the row for that trigger"
    template <typename T>
    QTreeWidgetItem* itemFor(const QMap<QTreeWidgetItem*, T*>& map, const QString& name) const
    {
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            if (it.value()->getName() == name) {
                return it.key();
            }
        }
        return nullptr;
    }

    // ticking through the raw pointer would crash the whole class if the row
    // were ever missing, which is exactly when a readable failure is wanted
    template <typename T>
    bool tickRow(const QMap<QTreeWidgetItem*, T*>& map, const QString& name) const
    {
        QTreeWidgetItem* row = itemFor(map, name);
        if (!row) {
            return false;
        }
        row->setCheckState(0, Qt::Checked);
        return true;
    }

    dlgPackageExporter* makeExporter()
    {
        auto* exporter = new dlgPackageExporter(nullptr, mpHost);
        exporter->show();
        return exporter;
    }

    QLineEdit* lineEdit(dlgPackageExporter* exporter, const QString& name) const
    {
        auto* widget = exporter->findChild<QLineEdit*>(name);
        Q_ASSERT(widget);
        return widget;
    }

    QPushButton* exportButton(dlgPackageExporter* exporter) const
    {
        auto* box = exporter->findChild<QDialogButtonBox*>(qsl("buttonBox"));
        Q_ASSERT(box);
        return box->button(QDialogButtonBox::Apply);
    }

    QString infoText(dlgPackageExporter* exporter) const
    {
        auto* label = exporter->findChild<QLabel*>(qsl("infoLabel"));
        Q_ASSERT(label);
        return label->text();
    }

    // displayResultMessage() paints failures in this colour and nothing else in
    // the dialog uses it, so it is the language-independent "did it fail" test
    bool reportedFailure(dlgPackageExporter* exporter) const { return infoText(exporter).contains(qsl("#FF6B6B")); }

    // The zip runs on a worker thread behind a QFutureWatcher, which hides the
    // Close button for the duration and shows it again from its finished
    // handler - so waiting for it back also guarantees that watcher has been
    // deleteLater()d rather than leaked past the dialog. The export button is
    // not usable as the signal: a successful module creation clears the package
    // name, which disables it again.
    bool waitForExportToSettle(dlgPackageExporter* exporter)
    {
        auto* box = exporter->findChild<QDialogButtonBox*>(qsl("buttonBox"));
        Q_ASSERT(box);
        auto* closeButton = box->button(QDialogButtonBox::Close);
        const bool settled = QTest::qWaitFor(
                [closeButton]() {
                    return !closeButton->isHidden();
                },
                30000);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        return settled;
    }

    // mudlet::unzip() joins its destination onto each archive entry without a
    // separator, so the trailing slash is load-bearing
    bool unpackInto(const QString& packageFile, const QTemporaryDir& destination) const { return mudlet::unzip(packageFile, qsl("%1/").arg(destination.path()), QDir(destination.path())); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mExportDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1000), "Could not connect with the host.");

        // getActualPath() falls back to this setting when the user has not
        // browsed for a location, which is the only way a test can steer where
        // a package lands without a file dialog
        mudlet::getQSettings()->setValue(qsl("lastFileDialogLocation"), mExportDir.path());

        auto* pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setRegexCodeList({qsl("^exported$")}, {REGEX_PERL});
        pTrigger->registerTrigger();
        pTrigger->setName(mTriggerName);
        QVERIFY(pTrigger->setScript(qsl("local exported = true\n")));
        pTrigger->setIsActive(true);

        auto* pAlias = new TAlias(mAliasName, mpHost);
        pAlias->setRegexCode(qsl("^exported$"));
        mpHost->getAliasUnit()->registerAlias(pAlias);
        QVERIFY(pAlias->setScript(qsl("local exported = true\n")));
        pAlias->setIsActive(true);

        auto* pTimer = new TTimer(mTimerName, QTime(0, 0, 30), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        QVERIFY(pTimer->setScript(qsl("local exported = true\n")));

        auto* pScript = new TScript(mScriptName, mpHost);
        mpHost->getScriptUnit()->registerScript(pScript);
        QVERIFY(pScript->setScript(qsl("local exported = true\n")));

        auto* pKey = new TKey(mKeyName, mpHost);
        mpHost->getKeyUnit()->registerKey(pKey);
        QVERIFY(pKey->setScript(qsl("local exported = true\n")));

        auto* pAction = new TAction(mActionName, mpHost);
        mpHost->getActionUnit()->registerAction(pAction);
        QVERIFY(pAction->setScript(qsl("local exported = true\n")));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // list*()/recurse*() walk each unit's root node list into the selection
    // tree; without them the dialog opens on six empty headings
    void test_listsEveryItemTypeInTheSelectionTree()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        auto* tree = exporter->findChild<QTreeWidget*>(qsl("treeWidget_exportSelection"));
        QVERIFY2(tree, "the exporter has no selection tree");
        QCOMPARE(tree->topLevelItemCount(), 6);

        QVERIFY2(itemFor(exporter->triggerMap, mTriggerName), "the trigger is missing from the selection tree");
        QVERIFY2(itemFor(exporter->aliasMap, mAliasName), "the alias is missing from the selection tree");
        QVERIFY2(itemFor(exporter->timerMap, mTimerName), "the timer is missing from the selection tree");
        QVERIFY2(itemFor(exporter->scriptMap, mScriptName), "the script is missing from the selection tree");
        QVERIFY2(itemFor(exporter->keyMap, mKeyName), "the key is missing from the selection tree");
        QVERIFY2(itemFor(exporter->actionMap, mActionName), "the button is missing from the selection tree");
    }

    void test_refusesToExportWithoutAName()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        QVERIFY2(tickRow(exporter->triggerMap, mTriggerName), qPrintable(qsl("%1 is missing from the selection tree").arg(mTriggerName)));
        QVERIFY2(lineEdit(exporter, qsl("lineEdit_packageName"))->text().isEmpty(), "the package name field started out filled in");

        exporter->slot_exportPackage();

        QVERIFY2(reportedFailure(exporter), "exporting with no package name did not report a problem");
        QVERIFY2(!exportButton(exporter)->isEnabled(), "the export button is offered with no package name");
    }

    void test_refusesToExportWithNothingSelected()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        const QString packageName = qsl("qaEmptyPackage");
        lineEdit(exporter, qsl("lineEdit_packageName"))->setText(packageName);
        QVERIFY2(exportButton(exporter)->isEnabled(), "naming the package did not enable the export button");

        exporter->slot_exportPackage();

        QVERIFY2(reportedFailure(exporter), "exporting an empty selection did not report a problem");
        QVERIFY2(!QFileInfo::exists(qsl("%1/%2.mpackage").arg(mExportDir.path(), packageName)), "an empty selection still wrote a package file");
    }

    // The whole staging pipeline: config.lua, the items XML, the assets copy
    // and the zip write
    void test_exportsSelectedItemsIntoAReadableMpackage()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        const QString packageName = qsl("qaFullPackage");
        const QString packageFile = qsl("%1/%2.mpackage").arg(mExportDir.path(), packageName);
        QVERIFY2(!QFileInfo::exists(packageFile), "the package file existed before the export");

        QFile asset(qsl("%1/qaAsset.txt").arg(mExportDir.path()));
        QVERIFY(asset.open(QIODevice::WriteOnly | QIODevice::Text));
        asset.write("packaged asset\n");
        asset.close();
        exporter->findChild<QListWidget*>(qsl("listWidget_addedFiles"))->addItem(asset.fileName());

        lineEdit(exporter, qsl("lineEdit_packageName"))->setText(packageName);
        lineEdit(exporter, qsl("lineEdit_author"))->setText(qsl("qaAuthor"));
        lineEdit(exporter, qsl("lineEdit_title"))->setText(qsl("qaTitle"));
        lineEdit(exporter, qsl("lineEdit_version"))->setText(qsl("9.9.9"));
        // no scheme, so normalizedHelpUrl() has to add one
        lineEdit(exporter, qsl("lineEdit_helpUrl"))->setText(qsl("wiki.mudlet.org/qa"));

        QVERIFY2(tickRow(exporter->triggerMap, mTriggerName), qPrintable(qsl("%1 is missing from the selection tree").arg(mTriggerName)));
        QVERIFY2(tickRow(exporter->aliasMap, mAliasName), qPrintable(qsl("%1 is missing from the selection tree").arg(mAliasName)));
        QVERIFY2(tickRow(exporter->timerMap, mTimerName), qPrintable(qsl("%1 is missing from the selection tree").arg(mTimerName)));
        QVERIFY2(tickRow(exporter->scriptMap, mScriptName), qPrintable(qsl("%1 is missing from the selection tree").arg(mScriptName)));
        QVERIFY2(tickRow(exporter->keyMap, mKeyName), qPrintable(qsl("%1 is missing from the selection tree").arg(mKeyName)));
        QVERIFY2(tickRow(exporter->actionMap, mActionName), qPrintable(qsl("%1 is missing from the selection tree").arg(mActionName)));

        exporter->slot_exportPackage();
        QVERIFY2(waitForExportToSettle(exporter), "the export never finished");
        QVERIFY2(!reportedFailure(exporter), qPrintable(qsl("the export reported a failure: %1").arg(infoText(exporter))));
        QVERIFY2(QFileInfo::exists(packageFile), "no package file was written");

        QTemporaryDir unpacked;
        QVERIFY(unpacked.isValid());
        QVERIFY2(unpackInto(packageFile, unpacked), "the exported package is not a readable archive");

        QFile config(qsl("%1/config.lua").arg(unpacked.path()));
        QVERIFY2(config.open(QIODevice::ReadOnly | QIODevice::Text), "the package has no config.lua");
        const QString configText = QString::fromUtf8(config.readAll());
        QVERIFY2(configText.contains(qsl("mpackage = [[%1]]").arg(packageName)), qPrintable(qsl("config.lua does not name the package: %1").arg(configText)));
        QVERIFY2(configText.contains(qsl("author = [[qaAuthor]]")), "config.lua lost the author");
        QVERIFY2(configText.contains(qsl("version = [[9.9.9]]")), "config.lua lost the version");
        QVERIFY2(configText.contains(qsl("https://wiki.mudlet.org/qa")), qPrintable(qsl("the scheme-less help URL was not normalised: %1").arg(configText)));

        QFile items(qsl("%1/%2.xml").arg(unpacked.path(), packageName));
        QVERIFY2(items.open(QIODevice::ReadOnly | QIODevice::Text), "the package has no items XML");
        const QString itemsXml = QString::fromUtf8(items.readAll());
        for (const QString& name : {mTriggerName, mAliasName, mTimerName, mScriptName, mKeyName, mActionName}) {
            QVERIFY2(itemsXml.contains(name), qPrintable(qsl("%1 was selected but is not in the exported XML").arg(name)));
        }

        QVERIFY2(QFileInfo::exists(qsl("%1/qaAsset.txt").arg(unpacked.path())), "the added asset file is not in the package");
    }

    // Unticked items must not be dragged along by the export, and their
    // exportItem flag has to be put back afterwards or the next profile save
    // writes the wrong set
    void test_leavesUnselectedItemsOutOfThePackage()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        const QString packageName = qsl("qaPartialPackage");
        lineEdit(exporter, qsl("lineEdit_packageName"))->setText(packageName);
        QVERIFY2(tickRow(exporter->scriptMap, mScriptName), qPrintable(qsl("%1 is missing from the selection tree").arg(mScriptName)));

        exporter->slot_exportPackage();
        QVERIFY2(waitForExportToSettle(exporter), "the export never finished");
        QVERIFY2(!reportedFailure(exporter), qPrintable(qsl("the export reported a failure: %1").arg(infoText(exporter))));

        QTemporaryDir unpacked;
        QVERIFY(unpacked.isValid());
        QVERIFY(unpackInto(qsl("%1/%2.mpackage").arg(mExportDir.path(), packageName), unpacked));

        QFile items(qsl("%1/%2.xml").arg(unpacked.path(), packageName));
        QVERIFY(items.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString itemsXml = QString::fromUtf8(items.readAll());
        QVERIFY2(itemsXml.contains(mScriptName), "the selected script is not in the exported XML");
        QVERIFY2(!itemsXml.contains(mTriggerName), "an unselected trigger was exported anyway");
        QVERIFY2(!itemsXml.contains(mAliasName), "an unselected alias was exported anyway");
    }

    // Module mode writes into the profile directory and installs the result,
    // which is a different tail of slot_exportPackage() than a plain export
    void test_moduleCreationModeInstallsWhatItExported()
    {
        const QString moduleName = qsl("qaCreatedModule");
        auto* exporter = makeExporter();
        // the module has to come back off the host even when an assertion below
        // ends the test early, or it is left behind for every later case
        const auto cleanup = qScopeGuard([this, exporter, moduleName]() {
            delete exporter;
            if (mpHost->mInstalledModules.contains(moduleName)) {
                mpHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI);
            }
        });

        QVERIFY2(!mpHost->mInstalledModules.contains(moduleName), "the module was already installed before the test");

        exporter->setModuleCreationMode(true);
        QCOMPARE(exportButton(exporter)->text(), qsl("Create Module"));

        lineEdit(exporter, qsl("lineEdit_packageName"))->setText(moduleName);
        QVERIFY2(tickRow(exporter->aliasMap, mAliasName), qPrintable(qsl("%1 is missing from the selection tree").arg(mAliasName)));

        exporter->slot_exportPackage();
        QVERIFY2(waitForExportToSettle(exporter), "the module export never finished");
        QVERIFY2(!reportedFailure(exporter), qPrintable(qsl("the module export reported a failure: %1").arg(infoText(exporter))));

        const QString moduleFile = qsl("%1/%2.mpackage").arg(mudlet::getMudletPath(enums::profileHomePath, mProfileName), moduleName);
        QVERIFY2(QFileInfo::exists(moduleFile), "module mode did not write the package into the profile directory");
        QVERIFY2(mpHost->mInstalledModules.contains(moduleName), "module mode exported the file but never installed it");

        QVERIFY2(mpHost->uninstallPackage(moduleName, enums::PackageModuleType::ModuleFromUI), "the module it installed could not be uninstalled again");
    }

    // preselect*() is how the editor's "create module from this item" entry
    // arrives with one row already ticked
    void test_preselectingAnEditorItemTicksItsRow()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        auto* pTrigger = mpHost->getTriggerUnit()->findTrigger(mTriggerName);
        QVERIFY(pTrigger);

        // stands in for the editor's tree item, which carries the item's ID in
        // Qt::UserRole
        QTreeWidgetItem editorItem;
        editorItem.setData(0, Qt::UserRole, pTrigger->getID());

        QTreeWidgetItem* row = itemFor(exporter->triggerMap, mTriggerName);
        QVERIFY2(row, "the trigger is missing from the selection tree");
        QCOMPARE(row->checkState(0), Qt::Unchecked);
        exporter->preselectTrigger(&editorItem);
        QCOMPARE(row->checkState(0), Qt::Checked);

        QTreeWidgetItem unknownItem;
        unknownItem.setData(0, Qt::UserRole, 0);
        auto* aliasRow = itemFor(exporter->aliasMap, mAliasName);
        QVERIFY2(aliasRow, "the alias is missing from the selection tree");
        exporter->preselectAlias(&unknownItem);
        QCOMPARE(aliasRow->checkState(0), Qt::Unchecked);
    }

    void test_dependenciesCanBeAddedAndRemoved()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        auto* available = exporter->findChild<QComboBox*>(qsl("DependencyList"));
        auto* chosen = exporter->findChild<QComboBox*>(qsl("comboBox_dependencies"));
        QVERIFY(available);
        QVERIFY(chosen);

        available->addItem(qsl("qaDependency"));
        available->setCurrentIndex(available->count() - 1);
        QCOMPARE(chosen->count(), 0);

        exporter->findChild<QPushButton*>(qsl("pushButton_addDependency"))->click();
        QCOMPARE(chosen->count(), 1);
        QCOMPARE(chosen->itemText(0), qsl("qaDependency"));

        // Delete on the chosen-dependencies box is handled by the dialog's own
        // event filter rather than by the combo box
        QKeyEvent deleteKey(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
        QVERIFY2(QCoreApplication::sendEvent(chosen, &deleteKey), "the exporter's event filter did not consume the Delete key");
        QCOMPARE(chosen->count(), 0);
    }

    // The description box accepts image files dropped onto it and rewrites them
    // as markdown references the exporter later resolves
    void test_descriptionAcceptsDroppedImageFiles()
    {
        auto* exporter = makeExporter();
        const auto cleanup = qScopeGuard([exporter]() {
            delete exporter;
        });

        auto* description = exporter->findChild<dlgPackageExporterDescription*>(qsl("textEdit_description"));
        QVERIFY(description);
        description->setPlainText(QString());

        const QString imagePath = qsl("%1/qaPicture.png").arg(mExportDir.path());
        QImage(4, 4, QImage::Format_ARGB32).save(imagePath);
        QVERIFY(QFileInfo::exists(imagePath));

        QMimeData imageDrop;
        imageDrop.setUrls({QUrl::fromLocalFile(imagePath)});
        QVERIFY2(description->canInsertFromMimeData(&imageDrop), "a dropped image was refused");
        description->insertFromMimeData(&imageDrop);

        QVERIFY2(exporter->mDescriptionImages.contains(imagePath), "the dropped image was not recorded for staging");
        QVERIFY2(exporter->mPlainDescription.contains(qsl("![Image]($qaPicture.png)")), qPrintable(qsl("no markdown reference was inserted: %1").arg(exporter->mPlainDescription)));

        QMimeData textDrop;
        textDrop.setText(qsl("just words"));
        description->insertFromMimeData(&textDrop);
        QVERIFY2(description->toPlainText().contains(qsl("just words")), "plain text pasted into the description was dropped");
    }

    void test_copyDirectoryCopiesNestedContent()
    {
        QTemporaryDir source;
        QVERIFY(source.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/nested").arg(source.path())));
        QFile nested(qsl("%1/nested/file.txt").arg(source.path()));
        QVERIFY(nested.open(QIODevice::WriteOnly | QIODevice::Text));
        nested.write("nested\n");
        nested.close();

        QTemporaryDir destination;
        QVERIFY(destination.isValid());
        const QString target = qsl("%1/copied").arg(destination.path());

        dlgPackageExporter::copy_directory(source.path(), target, true);

        QVERIFY2(QFileInfo::exists(qsl("%1/nested/file.txt").arg(target)), "copy_directory did not recurse into subdirectories");
    }
};

#include "PackageExporterTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageExporterTest)

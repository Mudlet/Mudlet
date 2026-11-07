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
 * The brown folder the editor shows for a module stands in for the module
 * itself: it is never written to the module file, so deleting or moving it
 * from the editor only loses the module's items. Item flags and the delete
 * dialog are widget state with no Lua entry point.
 */

#include <QMessageBox>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <functional>

#include "EditorMoveItemCommand.h"
#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
#include "XMLexport.h"
#include "ctelnet.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorModuleFolderTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mModuleDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgTriggerEditor* mpEditor = nullptr;
    const QString mProfileName = qsl("EditorModuleFolder-Test-Profile");
    const QString mLocalhost = qsl("localhost");
    const QString mModuleName = qsl("qaModuleFolder");
    const QString mModuleTriggerName = qsl("qaModuleTrigger");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void settleSaves()
    {
        for (int i = 0; i < 200 && (mpHost->hasPendingProfileSave() || mpHost->currentlySavingProfile()); ++i) {
            QTest::qWait(20ms);
            mpHost->waitForProfileSave();
        }
    }

    QString writeModule()
    {
        const QString path = qsl("%1/%2.xml").arg(mModuleDir.path(), mModuleName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return QString();
        }
        QTextStream out(&file);
        out << qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
	<TriggerPackage>
		<Trigger isActive="yes" isFolder="no" isTempTrigger="no" isMultiline="no" isPerlSlashGOption="no" isColorizerTrigger="no" isFilterTrigger="no" isSoundTrigger="no" isColorTrigger="no" isColorTriggerFg="no" isColorTriggerBg="no">
			<name>%1</name>
			<script></script>
			<triggerType>0</triggerType>
			<conditonLineDelta>0</conditonLineDelta>
			<mStayOpen>0</mStayOpen>
			<mCommand></mCommand>
			<packageName></packageName>
			<mFgColor>#ff0000</mFgColor>
			<mBgColor>#ffff00</mBgColor>
			<mSoundFile></mSoundFile>
			<colorTriggerFgColor>#000000</colorTriggerFgColor>
			<colorTriggerBgColor>#000000</colorTriggerBgColor>
			<regexCodeList>
				<string>qa module trigger</string>
			</regexCodeList>
			<regexCodePropertyList>
				<integer>0</integer>
			</regexCodePropertyList>
		</Trigger>
	</TriggerPackage>
	<TimerPackage />
	<AliasPackage />
	<ActionPackage />
	<ScriptPackage />
	<KeyPackage />
	<VariablePackage>
		<HiddenVariables />
	</VariablePackage>
</MudletPackage>
)")
                        .arg(mModuleTriggerName);
        out.flush();
        return path;
    }

    void installModule()
    {
        const QString path = writeModule();
        QVERIFY(!path.isEmpty());
        auto [installed, reason] = mpHost->installPackage(path, enums::PackageModuleType::ModuleFromUI);
        QVERIFY2(installed, qPrintable(reason));
        QVERIFY(mpHost->mInstalledModules.contains(mModuleName));
        settleSaves();
        mpEditor->slot_showTriggers();
        QCoreApplication::processEvents();
    }

    QTreeWidgetItem* moduleFolderItem() const
    {
        for (int i = 0, total = mpEditor->mpTriggerBaseItem->childCount(); i < total; ++i) {
            QTreeWidgetItem* pItem = mpEditor->mpTriggerBaseItem->child(i);
            if (pItem->text(0) == mModuleName) {
                return pItem;
            }
        }
        return nullptr;
    }

    QString exportedModuleXml()
    {
        const QString path = qsl("%1/module-export.xml").arg(mModuleDir.path());
        XMLexport writer(mpHost);
        writer.writeModuleXML(mModuleName);
        if (!XMLexport::saveXmlDocToFile(path, *writer.takeExportDocument())) {
            return {};
        }
        return readBack(path);
    }

    // The writers a profile save uses, without the console Host::saveProfile() would need
    QString exportedProfileXml()
    {
        const QString path = qsl("%1/profile-export.xml").arg(mModuleDir.path());
        auto writer = std::make_shared<XMLexport>(mpHost);
        if (!writer->exportPackage(path, true, true)) {
            return {};
        }
        return readBack(path);
    }

    static QString readBack(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        const QString xml = QString::fromUtf8(file.readAll());
        file.close();
        QFile::remove(path);
        return xml;
    }

    // Answers the next modal dialog with the button the chooser picks, and records its text
    void answerNextDialog(std::function<QAbstractButton*(QMessageBox*)> chooser, QString* pText)
    {
        QTimer::singleShot(200ms, qApp, [chooser, pText]() {
            auto* pBox = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!pBox) {
                return;
            }
            *pText = pBox->text();
            if (QAbstractButton* pButton = chooser(pBox)) {
                pButton->click();
            } else {
                pBox->close();
            }
        });
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mModuleDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host available for the test.");
        QSignalSpy connectedSpy(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectedSpy.wait(1s), "Could not connect with the host.");

        mudlet::self()->slot_showScriptDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor, "the editor dialog was not created");
    }

    void cleanupTestCase()
    {
        if (mpHost) {
            if (auto* pEditor = HostDialogs::of(mpHost).mpEditorDialog.data()) {
                HostDialogs::of(mpHost).mpEditorDialog = nullptr;
                delete pEditor;
            }
        }
        mpEditor = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theModuleFolderCannotBeDraggedButItsItemsCan()
    {
        installModule();
        QTreeWidgetItem* pFolder = moduleFolderItem();
        QVERIFY2(pFolder, "the module's folder is not in the triggers tree");
        QVERIFY2(!(pFolder->flags() & Qt::ItemIsDragEnabled), "the module's folder can be dragged out of place");
        QVERIFY2(pFolder->flags() & Qt::ItemIsDropEnabled, "items can no longer be dropped into the module's folder");
        QCOMPARE(pFolder->childCount(), 1);
        QVERIFY2(pFolder->child(0)->flags() & Qt::ItemIsDragEnabled, "the module's own items can no longer be dragged");
    }

    void test_deletingTheModuleFolderCanBeCancelled()
    {
        QTreeWidgetItem* pFolder = moduleFolderItem();
        QVERIFY(pFolder);
        mpEditor->treeWidget_triggers->setCurrentItem(pFolder);

        QString dialogText;
        answerNextDialog(
                [](QMessageBox* pBox) {
                    return pBox->button(QMessageBox::Cancel);
                },
                &dialogText);
        mpEditor->slot_deleteItemOrGroup();
        QCoreApplication::processEvents();

        QVERIFY2(dialogText.contains(mModuleName), qPrintable(qsl("no dialog named the module, got: \"%1\"").arg(dialogText)));
        QVERIFY2(moduleFolderItem(), "the module's folder left the tree");
        QVERIFY(mpHost->mInstalledModules.contains(mModuleName));
        QCOMPARE(mpHost->getTriggerUnit()->findItems(mModuleTriggerName, true, true).size(), 1);
    }

    void test_anItemMovedOutOfASyncedModuleIsSavedWithTheProfile()
    {
        QVERIFY(mpHost->changeModuleSync(mModuleName, QLatin1String("1")).first);
        auto restoreSync = qScopeGuard([this] {
            mpHost->changeModuleSync(mModuleName, QLatin1String("0"));
        });
        TriggerUnit* pUnit = mpHost->getTriggerUnit();
        const std::vector<int> ids = pUnit->findItems(mModuleTriggerName, true, true);
        QCOMPARE(ids.size(), 1);
        TTrigger* pTrigger = pUnit->getTrigger(ids.front());
        QVERIFY(pTrigger);
        TTrigger* pFolder = pTrigger->getParent();
        QVERIFY(pFolder && pFolder->mModuleMasterFolder);
        QVERIFY(pTrigger->moduleMember());
        // As a module made from a package's folder names that folder
        pTrigger->mPackageName = mModuleName;
        const int itemID = pTrigger->getID();
        const int folderID = pFolder->getID();
        auto putBack = qScopeGuard([=] {
            if (pTrigger->getParent() != pFolder) {
                pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
            }
        });
        const QString nameTag = qsl("<name>%1</name>").arg(mModuleTriggerName);

        // What TTreeWidget does on a drop to the top level, before it records the move
        pUnit->reParentTrigger(itemID, folderID, 0, TreeItemInsertMode::AtPosition, 0);
        EditorMoveItemCommand move(EditorViewType::cmTriggerView, itemID, folderID, 0, 0, 0, mModuleTriggerName, mpHost);
        move.redo();
        QVERIFY(!pTrigger->getParent());
        QVERIFY2(!pTrigger->moduleMember(), "an item moved out of the module is still a module member");
        QVERIFY2(pTrigger->mPackageName.isEmpty(), "an item moved out of the module still carries its name, so uninstalling it would take the item along");
        QVERIFY2(exportedProfileXml().contains(nameTag), "the profile save leaves out the item moved out of the module");
        QVERIFY2(!exportedModuleXml().contains(nameTag), "the module save still holds the item moved out of it");

        move.undo();
        QCOMPARE(pTrigger->getParent(), pFolder);
        QVERIFY2(pTrigger->moduleMember(), "an item moved back into the module is not a module member");
        QVERIFY(pFolder->moduleMember() && pFolder->mModuleMasterFolder);
        QVERIFY2(exportedModuleXml().contains(nameTag), "the module save leaves out the item moved back into it");
        QVERIFY2(!exportedProfileXml().contains(nameTag), "the profile save holds an item of the module");

        move.redo();
        QVERIFY(!pTrigger->moduleMember());
        QVERIFY(exportedProfileXml().contains(nameTag));

        move.undo();
        QVERIFY(pTrigger->moduleMember());
    }

    // The file of a module that is not synced is never rewritten, so it keeps the item and a copy in the profile would load twice
    void test_anItemMovedOutOfAnUnsyncedModuleStaysOutOfTheProfile()
    {
        TriggerUnit* pUnit = mpHost->getTriggerUnit();
        const std::vector<int> ids = pUnit->findItems(mModuleTriggerName, true, true);
        QCOMPARE(ids.size(), 1);
        TTrigger* pTrigger = pUnit->getTrigger(ids.front());
        QVERIFY(pTrigger);
        TTrigger* pFolder = pTrigger->getParent();
        QVERIFY(pFolder && pFolder->mModuleMasterFolder);
        // An earlier test took it out of a synced module, which hands it to the profile
        pTrigger->mLoadedFromModule = mModuleName;
        const int itemID = pTrigger->getID();
        const int folderID = pFolder->getID();
        auto putBack = qScopeGuard([=] {
            if (pTrigger->getParent() != pFolder) {
                pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
            }
        });
        const QString nameTag = qsl("<name>%1</name>").arg(mModuleTriggerName);

        pUnit->reParentTrigger(itemID, folderID, 0, TreeItemInsertMode::AtPosition, 0);
        EditorMoveItemCommand move(EditorViewType::cmTriggerView, itemID, folderID, 0, 0, 0, mModuleTriggerName, mpHost);
        move.redo();
        QVERIFY(!pTrigger->getParent());
        QVERIFY(pTrigger->moduleMember());
        QVERIFY(!exportedProfileXml().contains(nameTag));

        // Moving it again leaves no module, so it still belongs to the one whose file holds it
        pUnit->reParentTrigger(itemID, 0, 0, TreeItemInsertMode::AtPosition, 1);
        QVERIFY(pTrigger->moduleMember());
        QVERIFY(!exportedProfileXml().contains(nameTag));

        move.undo();
        QCOMPARE(pTrigger->getParent(), pFolder);
        QVERIFY(pTrigger->moduleMember());
    }

    void test_aProfileItemPassingThroughTheModuleIsSavedWithTheProfile()
    {
        TriggerUnit* pUnit = mpHost->getTriggerUnit();
        const std::vector<int> ids = pUnit->findItems(mModuleTriggerName, true, true);
        QCOMPARE(ids.size(), 1);
        TTrigger* pFolder = pUnit->getTrigger(ids.front())->getParent();
        QVERIFY(pFolder && pFolder->mModuleMasterFolder);
        const int folderID = pFolder->getID();
        const QString profileTriggerName = qsl("qaProfileTrigger");
        auto* pTrigger = new TTrigger(nullptr, mpHost);
        pTrigger->setName(profileTriggerName);
        QVERIFY(pUnit->registerTrigger(pTrigger));
        auto removeTrigger = qScopeGuard([pTrigger] {
            delete pTrigger;
        });
        const int itemID = pTrigger->getID();
        const QString nameTag = qsl("<name>%1</name>").arg(profileTriggerName);
        QVERIFY(!pTrigger->moduleMember());

        // An unsynced module's file is never rewritten, so an item put in and taken out again stays the profile's
        pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
        QVERIFY(pTrigger->moduleMember());
        pUnit->reParentTrigger(itemID, folderID, 0, TreeItemInsertMode::Append);
        QVERIFY(!pTrigger->moduleMember());
        QVERIFY(exportedProfileXml().contains(nameTag));

        QVERIFY(mpHost->changeModuleSync(mModuleName, QLatin1String("1")).first);
        auto restoreSync = qScopeGuard([this] {
            mpHost->changeModuleSync(mModuleName, QLatin1String("0"));
        });
        pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
        QVERIFY2(pTrigger->moduleMember(), "an item moved into a synced module is not a module member");
        QVERIFY(exportedModuleXml().contains(nameTag));
        QVERIFY(!exportedProfileXml().contains(nameTag));
        pUnit->reParentTrigger(itemID, folderID, 0, TreeItemInsertMode::Append);
        QVERIFY(!pTrigger->moduleMember());
        QVERIFY(exportedProfileXml().contains(nameTag));
        QVERIFY(!exportedModuleXml().contains(nameTag));

        // Its file was never written with the item, so turning sync off before taking it out must not strand it
        pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
        QVERIFY(pTrigger->moduleMember());
        QVERIFY(mpHost->changeModuleSync(mModuleName, QLatin1String("0")).first);
        pUnit->reParentTrigger(itemID, folderID, 0, TreeItemInsertMode::Append);
        QVERIFY2(!pTrigger->moduleMember(), "a profile item taken out of a module whose sync was turned off is still a module member");
        QVERIFY2(exportedProfileXml().contains(nameTag), "the profile save leaves out a profile item taken out of a module whose sync was turned off");
    }

    void test_anItemOfAnUnsyncedModulePassingThroughASyncedOneStaysOutOfTheProfile()
    {
        TriggerUnit* pUnit = mpHost->getTriggerUnit();
        const std::vector<int> ids = pUnit->findItems(mModuleTriggerName, true, true);
        QCOMPARE(ids.size(), 1);
        TTrigger* pTrigger = pUnit->getTrigger(ids.front());
        QVERIFY(pTrigger);
        TTrigger* pFolder = pTrigger->getParent();
        QVERIFY(pFolder && pFolder->mModuleMasterFolder);
        // An earlier test took it out of a synced module, which hands it to the profile
        pTrigger->mLoadedFromModule = mModuleName;
        const int itemID = pTrigger->getID();
        const int folderID = pFolder->getID();

        // A second, synced module, set up as installing one would leave it
        const QString otherModule = qsl("qaSyncedModule");
        auto* pOtherFolder = new TTrigger(nullptr, mpHost);
        pOtherFolder->setName(otherModule);
        pOtherFolder->setIsFolder(true);
        pOtherFolder->mPackageName = otherModule;
        pOtherFolder->mModuleMasterFolder = true;
        pOtherFolder->mLoadedFromModule = otherModule;
        QVERIFY(pUnit->registerTrigger(pOtherFolder));
        mpHost->mInstalledModules[otherModule] = QStringList{qsl("%1/%2.xml").arg(mModuleDir.path(), otherModule), qsl("1"), qsl("0")};
        mpHost->mModulesLoadedOk.insert(otherModule);
        auto cleanUp = qScopeGuard([=, this] {
            if (pTrigger->getParent() != pFolder) {
                pUnit->reParentTrigger(itemID, 0, folderID, TreeItemInsertMode::Append);
            }
            mpHost->mInstalledModules.remove(otherModule);
            mpHost->mModulesLoadedOk.remove(otherModule);
            delete pOtherFolder;
        });
        QVERIFY(mpHost->moduleSavesBack(otherModule));
        const QString nameTag = qsl("<name>%1</name>").arg(mModuleTriggerName);

        pUnit->reParentTrigger(itemID, folderID, pOtherFolder->getID(), TreeItemInsertMode::Append);
        QVERIFY(pTrigger->moduleMember());
        pUnit->reParentTrigger(itemID, pOtherFolder->getID(), 0, TreeItemInsertMode::Append);
        QVERIFY(!pTrigger->getParent());
        QVERIFY2(pTrigger->moduleMember(), "an item of an unsynced module, which that module's file still holds, stopped being a member on leaving another module");
        QVERIFY2(!exportedProfileXml().contains(nameTag), "the profile save holds an item its unsynced module's file also holds, so it would load twice");
    }

    void test_deletingTheModuleFolderCanUninstallTheModule()
    {
        QTreeWidgetItem* pFolder = moduleFolderItem();
        QVERIFY(pFolder);
        mpEditor->treeWidget_triggers->setCurrentItem(pFolder);

        QString dialogText;
        answerNextDialog(
                [](QMessageBox* pBox) -> QAbstractButton* {
                    for (QAbstractButton* pButton : pBox->buttons()) {
                        if (pBox->buttonRole(pButton) == QMessageBox::AcceptRole) {
                            return pButton;
                        }
                    }
                    return nullptr;
                },
                &dialogText);
        mpEditor->slot_deleteItemOrGroup();
        settleSaves();
        QTest::qWait(50ms);

        QVERIFY2(dialogText.contains(mModuleName), qPrintable(qsl("no dialog named the module, got: \"%1\"").arg(dialogText)));
        QVERIFY2(!mpHost->mInstalledModules.contains(mModuleName), "the module is still installed");
        QCOMPARE(mpHost->getTriggerUnit()->findItems(mModuleTriggerName, true, true).size(), 0);
        QVERIFY2(!moduleFolderItem(), "the module's folder is still in the tree");
    }
};

#include "EditorModuleFolderTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorModuleFolderTest)

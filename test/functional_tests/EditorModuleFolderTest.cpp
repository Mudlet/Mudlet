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
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <functional>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "TriggerUnit.h"
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

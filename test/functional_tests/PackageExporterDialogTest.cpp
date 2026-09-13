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

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include <chrono>

#include "ActionUnit.h"
#include "AliasUnit.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "ScriptUnit.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "TimerUnit.h"
#include "TriggerUnit.h"
#include "ctelnet.h"
#include "dlgPackageExporter.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// The package exporter dialog turns a profile's triggers, aliases, timers,
// scripts, keys and buttons into a .mpackage, and nothing but the dialog can do
// that: Lua's installPackage()/installModule() only ever read a package someone
// else built, so Package_spec.lua cannot reach a line of this file. Everything
// here is driven the way the dialog drives itself - the tree's check states, the
// metadata fields, and slot_exportPackage() - with the two QFileDialogs left
// alone; the export location comes from the "lastFileDialogLocation" setting
// that getActualPath() reads, which is what the picker writes anyway.
//
// The strongest assertion available is the round trip: export a package and
// install it back with the real package machinery, so the items, the assets and
// the config.lua metadata are checked by the code that has to read them for
// real rather than against a pinned byte layout.
class PackageExporterDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mExportRoot;
    QString mExportDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mProfileName = qsl("Test-PackageExporterDialog");
    QString mPort; // assigned the stub's actual ephemeral port in init()
    const QString mLocalhost = qsl("localhost");
    Host* mpHost = nullptr;
    dlgPackageExporter* mpExporter = nullptr;
    QTimer* mpModalAnswerTimer = nullptr;
    QStringList mStagedPackageNames;

    QString profileHome() const { return mudlet::getMudletPath(enums::profileHomePath, mProfileName); }

    QString packagePath(const QString& packageName) const { return qsl("%1/%2.mpackage").arg(mExportDir, packageName); }

    static QString stagingPath(const QString& packageName) { return qsl("%1/mudlet/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), packageName); }

    // Every export leaves a staging tree under the system temporary directory
    // that nothing else ever removes, so note the names down as they are used.
    QString notePackageName(const QString& packageName)
    {
        mStagedPackageNames << packageName;
        return packageName;
    }

    // A package name nothing else on the machine is exporting under. Every
    // export stages its work in <TempLocation>/mudlet/<package name>, which is
    // shared with every other Mudlet on the machine and wiped and rebuilt at
    // the start of each export, so two runs of this test using the same names
    // would delete each other's staging trees mid-zip.
    QString packageNamed(const QString& stem) { return notePackageName(qsl("%1-%2").arg(stem).arg(QCoreApplication::applicationPid())); }

    void openExporter()
    {
        delete mpExporter;
        mpExporter = new dlgPackageExporter(nullptr, mpHost);
    }

    QTreeWidget* tree() const { return mpExporter->findChild<QTreeWidget*>(qsl("treeWidget_exportSelection")); }
    QLabel* infoLabel() const { return mpExporter->findChild<QLabel*>(qsl("infoLabel")); }
    QLineEdit* lineEditNamed(const QString& name) const { return mpExporter->findChild<QLineEdit*>(name); }
    QComboBox* comboNamed(const QString& name) const { return mpExporter->findChild<QComboBox*>(name); }
    QPushButton* buttonNamed(const QString& name) const { return mpExporter->findChild<QPushButton*>(name); }
    QListWidget* addedFiles() const { return mpExporter->findChild<QListWidget*>(qsl("listWidget_addedFiles")); }
    QLineEdit* nameField() const { return lineEditNamed(qsl("lineEdit_packageName")); }
    QString selectionTitle() const { return mpExporter->findChild<QGroupBox*>(qsl("groupBox_exportSelection"))->title(); }

    // The Export button carries the ApplyRole so that pressing it does not close
    // the dialog, which is also how it is told apart from Close and Cancel.
    QPushButton* exportButton() const { return mpExporter->findChild<QDialogButtonBox*>(qsl("buttonBox"))->button(QDialogButtonBox::Apply); }

    QTreeWidgetItem* triggersTop() const { return tree()->topLevelItem(0); }
    QTreeWidgetItem* aliasesTop() const { return tree()->topLevelItem(1); }
    QTreeWidgetItem* timersTop() const { return tree()->topLevelItem(2); }
    QTreeWidgetItem* scriptsTop() const { return tree()->topLevelItem(3); }
    QTreeWidgetItem* keysTop() const { return tree()->topLevelItem(4); }
    QTreeWidgetItem* buttonsTop() const { return tree()->topLevelItem(5); }

    static QTreeWidgetItem* itemNamed(QTreeWidgetItem* parent, const QString& name)
    {
        for (int i = 0; i < parent->childCount(); ++i) {
            if (parent->child(i)->text(0) == name) {
                return parent->child(i);
            }
        }
        return nullptr;
    }

    bool checkItem(QTreeWidgetItem* parent, const QString& name)
    {
        auto* item = itemNamed(parent, name);
        if (!item) {
            return false;
        }
        item->setCheckState(0, Qt::Checked);
        return true;
    }

    TTrigger* makeTrigger(const QString& name, TTrigger* parent, bool folder = false)
    {
        auto* trigger = new TTrigger(parent, mpHost);
        trigger->setName(name);
        trigger->setIsFolder(folder);
        if (!folder) {
            trigger->setRegexCodeList(QStringList{name}, QList<int>{REGEX_SUBSTRING});
        }
        mpHost->getTriggerUnit()->registerTrigger(trigger);
        return trigger;
    }

    TAlias* makeAlias(const QString& name)
    {
        auto* alias = new TAlias(name, mpHost);
        alias->setRegexCode(qsl("^%1$").arg(name));
        mpHost->getAliasUnit()->registerAlias(alias);
        return alias;
    }

    TTimer* makeTimer(const QString& name)
    {
        auto* timer = new TTimer(name, QTime(0, 0, 30), mpHost, false);
        mpHost->getTimerUnit()->registerTimer(timer);
        return timer;
    }

    TScript* makeScript(const QString& name)
    {
        auto* script = new TScript(name, mpHost);
        mpHost->getScriptUnit()->registerScript(script);
        return script;
    }

    TKey* makeKey(const QString& name)
    {
        auto* key = new TKey(name, mpHost);
        key->setKeyCode(Qt::Key_F9);
        mpHost->getKeyUnit()->registerKey(key);
        return key;
    }

    TAction* makeAction(const QString& name)
    {
        auto* action = new TAction(name, mpHost);
        mpHost->getActionUnit()->registerAction(action);
        return action;
    }

    int triggerCount(const QString& name) const { return static_cast<int>(mpHost->getTriggerUnit()->findItems(name, true, true).size()); }
    int aliasCount(const QString& name) const { return static_cast<int>(mpHost->getAliasUnit()->findItems(name, true, true).size()); }
    int timerCount(const QString& name) const { return static_cast<int>(mpHost->getTimerUnit()->findItems(name, true, true).size()); }
    int scriptCount(const QString& name) const { return static_cast<int>(mpHost->getScriptUnit()->findItems(name, true, true).size()); }
    int keyCount(const QString& name) const { return static_cast<int>(mpHost->getKeyUnit()->findItems(name, true, true).size()); }
    int actionCount(const QString& name) const { return static_cast<int>(mpHost->getActionUnit()->findItems(name, true, true).size()); }

    // Every install and export arms a profile save, and installPackage() answers
    // a save in progress by postponing itself and returning true - so a step
    // taken without waiting one out is measured against a profile the previous
    // step never reached.
    void settleSaves()
    {
        for (int i = 0; i < 200 && (mpHost->hasPendingProfileSave() || mpHost->currentlySavingProfile()); ++i) {
            QTest::qWait(20);
            mpHost->waitForProfileSave();
        }
    }

    // The zip is written on a worker thread and reported back through a
    // QFutureWatcher, so the export is over when the "Exporting package..."
    // holding message has been replaced - by the result either way.
    bool waitForExportToSettle(const std::chrono::milliseconds timeout = 30s)
    {
        return QTest::qWaitFor(
                [this]() {
                    return !infoLabel()->text().contains(qsl("Exporting package..."));
                },
                timeout);
    }

    bool runExport(const QString& packageName)
    {
        notePackageName(packageName);
        nameField()->setText(packageName);
        mpExporter->slot_exportPackage();
        return waitForExportToSettle();
    }

    // QMessageBox::exec() spins its own event loop, so the answer has to come
    // from a timer armed before the call that puts the box up.
    void answerNextMessageBox(const QMessageBox::StandardButton answer)
    {
        stopAnsweringMessageBoxes();
        mpModalAnswerTimer = new QTimer(this);
        mpModalAnswerTimer->setInterval(20);
        connect(mpModalAnswerTimer, &QTimer::timeout, this, [this, answer]() {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!box) {
                return;
            }
            if (auto* button = box->button(answer)) {
                button->click();
            }
            stopAnsweringMessageBoxes();
        });
        mpModalAnswerTimer->start();
    }

    void stopAnsweringMessageBoxes()
    {
        if (!mpModalAnswerTimer) {
            return;
        }
        mpModalAnswerTimer->stop();
        mpModalAnswerTimer->deleteLater();
        mpModalAnswerTimer = nullptr;
    }

    static bool writeTextFile(const QString& path, const QString& contents)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream out(&file);
        out << contents;
        out.flush();
        file.close();
        return true;
    }

    static QByteArray readAll(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }
        return file.readAll();
    }

    void deleteProfileDirectory()
    {
        QDir dir(profileHome());
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mExportRoot.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "the test profile never finished loading");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(!connected.isEmpty() || connected.wait(2000), "the test profile never connected to the stub server");
        // a new profile is given packages of its own on connect, and each of
        // those arms a save that would otherwise land mid-export
        settleSaves();

        // A directory per test method, so one method's packages cannot be what
        // another one finds. getActualPath() falls back to this setting, which
        // is the same one the "where do you want to save it" picker writes, so
        // pointing it here is how the export is aimed without a file dialog.
        mExportDir = qsl("%1/%2").arg(mExportRoot.path(), QString::fromUtf8(QTest::currentTestFunction()));
        QVERIFY(QDir().mkpath(mExportDir));
        mudlet::getQSettings()->setValue(qsl("lastFileDialogLocation"), mExportDir);
    }

    void cleanup()
    {
        stopAnsweringMessageBoxes();
        // slot_recountItems() arms a zero timer and holds a static flag until it
        // runs, so let it run before the dialog that owns it goes away
        QApplication::processEvents();
        delete mpExporter;
        mpExporter = nullptr;
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
        mpHost = nullptr;

        for (const auto& packageName : std::as_const(mStagedPackageNames)) {
            QDir(stagingPath(packageName)).removeRecursively();
        }
        mStagedPackageNames.clear();
    }

    // The six unit lists and their recursive walks, which are what fills the
    // tree the rest of the dialog works from. Temporary items are deliberately
    // left out of it - they cease to exist when the profile closes, so a
    // package built from one would arrive at its next owner empty.
    void test_theTreeListsEveryUnitAndLeavesTemporaryItemsOut()
    {
        auto* folder = makeTrigger(qsl("exporter trigger folder"), nullptr, true);
        makeTrigger(qsl("exporter nested trigger"), folder);
        makeTrigger(qsl("exporter root trigger"), nullptr);
        makeTrigger(qsl("exporter temporary trigger"), nullptr)->setTemporary(true);
        makeAlias(qsl("exporter alias"));
        makeTimer(qsl("exporter timer"));
        makeScript(qsl("exporter script"));
        makeKey(qsl("exporter key"));
        makeAction(qsl("exporter action"));

        openExporter();

        QCOMPARE(tree()->topLevelItemCount(), 6);
        QVERIFY2(itemNamed(triggersTop(), qsl("exporter root trigger")), "a root trigger never reached the tree");
        QVERIFY2(itemNamed(aliasesTop(), qsl("exporter alias")), "an alias never reached the tree");
        QVERIFY2(itemNamed(timersTop(), qsl("exporter timer")), "a timer never reached the tree");
        QVERIFY2(itemNamed(scriptsTop(), qsl("exporter script")), "a script never reached the tree");
        QVERIFY2(itemNamed(keysTop(), qsl("exporter key")), "a key never reached the tree");
        QVERIFY2(itemNamed(buttonsTop(), qsl("exporter action")), "a button never reached the tree");
        QVERIFY2(!itemNamed(triggersTop(), qsl("exporter temporary trigger")), "a temporary trigger was offered for export");

        auto* folderItem = itemNamed(triggersTop(), qsl("exporter trigger folder"));
        QVERIFY2(folderItem, "a trigger folder never reached the tree");
        QCOMPARE(folderItem->childCount(), 1);
        QCOMPARE(folderItem->child(0)->text(0), qsl("exporter nested trigger"));
        QVERIFY2(!itemNamed(triggersTop(), qsl("exporter nested trigger")), "a nested trigger was listed at the top level as well as under its folder");

        QCOMPARE(folderItem->checkState(0), Qt::Unchecked);
        QCOMPARE(folderItem->child(0)->checkState(0), Qt::Unchecked);
    }

    // The dialog is not owned by the profile it exports from, so it says which
    // profile is going away for whoever is keeping track of it.
    void test_closingTheDialogAnnouncesTheProfileItBelongedTo()
    {
        openExporter();
        mpExporter->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpExporter));

        QSignalSpy closing(mpExporter, &dlgPackageExporter::packageExporterClosing);
        QVERIFY(mpExporter->close());
        QCOMPARE(closing.count(), 1);
        QCOMPARE(closing.at(0).at(0).toString(), mProfileName);
    }

    // Checking a folder takes everything under it, and the count in the group
    // box title is of every checked item at any depth - the folder included.
    void test_checkingAFolderChecksItsChildrenAndCountsThem()
    {
        auto* folder = makeTrigger(qsl("exporter group"), nullptr, true);
        makeTrigger(qsl("exporter group child one"), folder);
        makeTrigger(qsl("exporter group child two"), folder);
        makeTrigger(qsl("exporter loner"), nullptr);

        openExporter();
        auto* groupItem = itemNamed(triggersTop(), qsl("exporter group"));
        QVERIFY(groupItem);
        QCOMPARE(groupItem->childCount(), 2);

        const QString emptyTitle = selectionTitle();
        QVERIFY2(!emptyTitle.contains(QChar('3')), "the title already showed the count this test is about to produce");
        QCOMPARE(groupItem->child(0)->checkState(0), Qt::Unchecked);
        QCOMPARE(groupItem->child(1)->checkState(0), Qt::Unchecked);

        groupItem->setCheckState(0, Qt::Checked);
        QVERIFY2(QTest::qWaitFor(
                         [this, emptyTitle]() {
                             return selectionTitle() != emptyTitle;
                         },
                         2s),
                 "the group box title never took the count");

        QCOMPARE(groupItem->child(0)->checkState(0), Qt::Checked);
        QCOMPARE(groupItem->child(1)->checkState(0), Qt::Checked);
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter loner"))->checkState(0), Qt::Unchecked);
        // three: the folder and both of its children, so counting only the
        // leaves or only the folder would each give a different answer
        QVERIFY2(selectionTitle().contains(QChar('3')), qPrintable(qsl("The title has to count all three checked items, but said: \"%1\"").arg(selectionTitle())));
    }

    // Right-clicking an item is the way to take a folder without its contents,
    // which is what keeps a package from gaining a level of nesting nobody
    // asked for.
    void test_rightClickChecksOneItemWithoutItsChildren()
    {
        auto* folder = makeTrigger(qsl("exporter rightclick group"), nullptr, true);
        makeTrigger(qsl("exporter rightclick child"), folder);

        openExporter();
        mpExporter->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpExporter));

        auto* groupItem = itemNamed(triggersTop(), qsl("exporter rightclick group"));
        QVERIFY(groupItem);
        triggersTop()->setExpanded(true);
        groupItem->setExpanded(true);
        QApplication::processEvents();

        const QRect itemRect = tree()->visualItemRect(groupItem);
        QVERIFY2(!itemRect.isEmpty(), "the tree never laid the item out, so there is nowhere to right-click");
        emit tree() -> customContextMenuRequested(itemRect.center());

        QCOMPARE(groupItem->checkState(0), Qt::Checked);
        QCOMPARE(groupItem->child(0)->checkState(0), Qt::Unchecked);

        emit tree() -> customContextMenuRequested(itemRect.center());
        QCOMPARE(groupItem->checkState(0), Qt::Unchecked);

        // the six category rows have no parent and are not items in their own
        // right, so a right-click on one has to do nothing at all
        const QRect topRect = tree()->visualItemRect(triggersTop());
        QVERIFY(!topRect.isEmpty());
        emit tree() -> customContextMenuRequested(topRect.center());
        QCOMPARE(triggersTop()->checkState(0), Qt::Unchecked);
    }

    // Opening the exporter from the editor preselects whatever was being edited,
    // matched by the id the editor's own tree item carries.
    void test_preselectingAnItemChecksThatItemAlone()
    {
        makeTrigger(qsl("exporter preselect first"), nullptr);
        auto* second = makeTrigger(qsl("exporter preselect second"), nullptr);
        auto* alias = makeAlias(qsl("exporter preselect alias"));
        auto* timer = makeTimer(qsl("exporter preselect timer"));
        auto* script = makeScript(qsl("exporter preselect script"));
        auto* key = makeKey(qsl("exporter preselect key"));
        auto* action = makeAction(qsl("exporter preselect action"));
        QVERIFY2(second->getID() > 0, "the trigger never got an id to be preselected by");

        openExporter();

        QTreeWidgetItem editorItem;
        editorItem.setData(0, Qt::UserRole, second->getID());
        mpExporter->preselectTrigger(&editorItem);
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter preselect second"))->checkState(0), Qt::Checked);
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter preselect first"))->checkState(0), Qt::Unchecked);

        // an editor item with no id in it must not check whatever comes first
        QTreeWidgetItem itemWithoutAnId;
        mpExporter->preselectTrigger(&itemWithoutAnId);
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter preselect first"))->checkState(0), Qt::Unchecked);
        mpExporter->preselectTrigger(nullptr);

        QTreeWidgetItem aliasItem;
        aliasItem.setData(0, Qt::UserRole, alias->getID());
        mpExporter->preselectAlias(&aliasItem);
        QCOMPARE(itemNamed(aliasesTop(), qsl("exporter preselect alias"))->checkState(0), Qt::Checked);

        QTreeWidgetItem timerItem;
        timerItem.setData(0, Qt::UserRole, timer->getID());
        mpExporter->preselectTimer(&timerItem);
        QCOMPARE(itemNamed(timersTop(), qsl("exporter preselect timer"))->checkState(0), Qt::Checked);

        QTreeWidgetItem scriptItem;
        scriptItem.setData(0, Qt::UserRole, script->getID());
        mpExporter->preselectScript(&scriptItem);
        QCOMPARE(itemNamed(scriptsTop(), qsl("exporter preselect script"))->checkState(0), Qt::Checked);

        QTreeWidgetItem keyItem;
        keyItem.setData(0, Qt::UserRole, key->getID());
        mpExporter->preselectKey(&keyItem);
        QCOMPARE(itemNamed(keysTop(), qsl("exporter preselect key"))->checkState(0), Qt::Checked);

        QTreeWidgetItem actionItem;
        actionItem.setData(0, Qt::UserRole, action->getID());
        mpExporter->preselectAction(&actionItem);
        QCOMPARE(itemNamed(buttonsTop(), qsl("exporter preselect action"))->checkState(0), Qt::Checked);
    }

    // A package has to be named to be exported, and that is the one field the
    // Export button is held back for.
    void test_exportRefusesAnEmptyPackageName()
    {
        makeTrigger(qsl("exporter unnamed trigger"), nullptr);
        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter unnamed trigger")));

        QVERIFY(nameField()->text().isEmpty());
        QVERIFY2(!exportButton()->isEnabled(), "the Export button was offered with no package name to export under");
        QVERIFY2(exportButton()->toolTip().contains(qsl("package name")), qPrintable(qsl("The tooltip has to say which field is missing, but said: \"%1\"").arg(exportButton()->toolTip())));

        mpExporter->slot_exportPackage();
        QVERIFY2(infoLabel()->text().contains(qsl("Please enter the package name")), qPrintable(qsl("The refusal has to be shown, but the label held: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(QDir(mExportDir).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty(), "a package file was written for an export that has no name to be written under");

        nameField()->setText(qsl("exporter-now-named"));
        QVERIFY2(exportButton()->isEnabled(), "the Export button stayed disabled after the package was named");
    }

    // An empty package would install as nothing at all, so the export stops
    // before it writes anything rather than shipping one.
    void test_exportRefusesWhenNothingIsSelected()
    {
        const QString packageName = packageNamed(qsl("exporter-empty-selection"));
        makeTrigger(qsl("exporter unselected trigger"), nullptr);
        openExporter();

        nameField()->setText(packageName);
        QVERIFY(exportButton()->isEnabled());
        mpExporter->slot_exportPackage();

        QVERIFY2(infoLabel()->text().contains(qsl("Cannot create empty package")), qPrintable(qsl("The refusal has to be shown, but the label held: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(!QFileInfo::exists(packagePath(packageName)), "a package file was written for a selection with nothing in it");
    }

    // The round trip: what the dialog writes out is read back by the real
    // install machinery, and the items it was told to include come back while
    // the ones it was not do not.
    void test_anExportedPackageBringsItsCheckedItemsBackWhenInstalled()
    {
        const QString packageName = packageNamed(qsl("exporter-round-trip"));
        auto* folder = makeTrigger(qsl("exporter kept folder"), nullptr, true);
        makeTrigger(qsl("exporter kept trigger"), folder);
        auto* dropped = makeTrigger(qsl("exporter dropped trigger"), nullptr);
        makeAlias(qsl("exporter kept alias"));
        makeTimer(qsl("exporter kept timer"));
        makeScript(qsl("exporter kept script"));
        makeKey(qsl("exporter kept key"));
        makeAction(qsl("exporter kept action"));

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter kept folder")));
        QVERIFY(checkItem(aliasesTop(), qsl("exporter kept alias")));
        QVERIFY(checkItem(timersTop(), qsl("exporter kept timer")));
        QVERIFY(checkItem(scriptsTop(), qsl("exporter kept script")));
        QVERIFY(checkItem(keysTop(), qsl("exporter kept key")));
        QVERIFY(checkItem(buttonsTop(), qsl("exporter kept action")));
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter dropped trigger"))->checkState(0), Qt::Unchecked);

        QVERIFY2(runExport(packageName), "the export never finished");
        QVERIFY2(QFileInfo::exists(packagePath(packageName)), qPrintable(qsl("No package file was written. The dialog said: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(readAll(packagePath(packageName)).startsWith("PK"), "what was written is not a zip archive");

        // the export marks what it is leaving out, and has to put that back
        // afterwards or the profile's own next save drops those items
        QVERIFY2(dropped->exportItem, "an item left out of the package was still flagged as not-for-export afterwards");

        settleSaves();
        QCOMPARE(triggerCount(qsl("exporter kept trigger")), 1);
        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(qsl("The package the dialog produced could not be installed: \"%1\"").arg(reason)));
        QVERIFY2(mpHost->mInstalledPackages.contains(packageName), "the installed package was not listed");

        QCOMPARE(triggerCount(qsl("exporter kept trigger")), 2);
        QCOMPARE(triggerCount(qsl("exporter kept folder")), 2);
        QCOMPARE(aliasCount(qsl("exporter kept alias")), 2);
        QCOMPARE(timerCount(qsl("exporter kept timer")), 2);
        QCOMPARE(scriptCount(qsl("exporter kept script")), 2);
        QCOMPARE(keyCount(qsl("exporter kept key")), 2);
        QCOMPARE(actionCount(qsl("exporter kept action")), 2);
        QCOMPARE(triggerCount(qsl("exporter dropped trigger")), 1);
    }

    // A child taken without its folder is exported in its own right rather than
    // dragged along with a folder the user did not ask for.
    void test_aCheckedChildIsExportedWithoutItsUncheckedFolder()
    {
        const QString packageName = packageNamed(qsl("exporter-child-only"));
        auto* folder = makeTrigger(qsl("exporter unchecked folder"), nullptr, true);
        makeTrigger(qsl("exporter checked child"), folder);

        openExporter();
        auto* folderItem = itemNamed(triggersTop(), qsl("exporter unchecked folder"));
        QVERIFY(folderItem);
        QCOMPARE(folderItem->childCount(), 1);
        folderItem->child(0)->setCheckState(0, Qt::Checked);
        QCOMPARE(folderItem->checkState(0), Qt::Unchecked);

        QVERIFY2(runExport(packageName), "the export never finished");
        settleSaves();

        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));
        QCOMPARE(triggerCount(qsl("exporter checked child")), 2);
        QCOMPARE(triggerCount(qsl("exporter unchecked folder")), 1);
    }

    // Everything the metadata fields hold is written to the package's config.lua
    // and read back by the install, help URL normalisation included: a URL with
    // no scheme never opens in a browser, so one is put in front of it.
    void test_thePackageMetadataRoundTripsThroughItsConfigFile()
    {
        const QString packageName = packageNamed(qsl("exporter-metadata"));
        mpHost->mInstalledPackages << qsl("exporter-dependency");
        makeTrigger(qsl("exporter metadata trigger"), nullptr);

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter metadata trigger")));
        lineEditNamed(qsl("lineEdit_author"))->setText(qsl("A Test Author"));
        lineEditNamed(qsl("lineEdit_title"))->setText(qsl("A one line summary"));
        lineEditNamed(qsl("lineEdit_version"))->setText(qsl("2.3.4"));
        lineEditNamed(qsl("lineEdit_helpUrl"))->setText(qsl("wiki.mudlet.org/w/Manual:Packages"));
        mpExporter->mPlainDescription = qsl("What this package is for.");

        auto* available = comboNamed(qsl("DependencyList"));
        available->setCurrentIndex(available->findText(qsl("exporter-dependency")));
        buttonNamed(qsl("pushButton_addDependency"))->click();
        QCOMPARE(comboNamed(qsl("comboBox_dependencies"))->count(), 1);

        QVERIFY2(runExport(packageName), "the export never finished");
        settleSaves();
        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));

        const QMap<QString, QString> info = mpHost->mPackageInfo.value(packageName);
        QCOMPARE(info.value(qsl("mpackage")), packageName);
        QCOMPARE(info.value(qsl("author")), qsl("A Test Author"));
        QCOMPARE(info.value(qsl("title")), qsl("A one line summary"));
        QCOMPARE(info.value(qsl("version")), qsl("2.3.4"));
        QCOMPARE(info.value(qsl("description")), qsl("What this package is for."));
        QCOMPARE(info.value(qsl("dependencies")), qsl("exporter-dependency"));
        QCOMPARE(info.value(qsl("helpURL")), qsl("https://wiki.mudlet.org/w/Manual:Packages"));
        QVERIFY2(!info.value(qsl("created")).isEmpty(), "the package was not stamped with when it was made");

        // the author is the one field offered back the next time round
        QCOMPARE(mudlet::getQSettings()->value(qsl("packageAuthor")).toString(), qsl("A Test Author"));
    }

    // A help URL that already names a scheme is left exactly as it is, so the
    // normalisation above cannot be a blanket prefix.
    void test_aHelpUrlThatAlreadyHasASchemeIsLeftAlone()
    {
        const QString packageName = packageNamed(qsl("exporter-help-url"));
        makeTrigger(qsl("exporter help url trigger"), nullptr);

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter help url trigger")));
        lineEditNamed(qsl("lineEdit_helpUrl"))->setText(qsl("http://example.org/help?a=b://c"));

        QVERIFY2(runExport(packageName), "the export never finished");
        settleSaves();
        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));
        QCOMPARE(mpHost->mPackageInfo.value(packageName).value(qsl("helpURL")), qsl("http://example.org/help?a=b://c"));
    }

    // The description is only read out of the editor when focus leaves it, so a
    // package made without clicking elsewhere first would carry the template
    // rather than what was written.
    void test_theDescriptionIsTakenFromTheEditorWhenFocusLeavesIt()
    {
        const QString packageName = packageNamed(qsl("exporter-description"));
        const QString typed = qsl("Typed straight into the description box.");
        makeTrigger(qsl("exporter description trigger"), nullptr);

        openExporter();
        mpExporter->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpExporter));
        QVERIFY(checkItem(triggersTop(), qsl("exporter description trigger")));

        auto* description = mpExporter->findChild<QTextEdit*>(qsl("textEdit_description"));
        QVERIFY(description);
        description->setFocus();
        QVERIFY2(QTest::qWaitFor(
                         [description]() {
                             return description->hasFocus();
                         },
                         2s),
                 "the description editor never took focus");
        description->setPlainText(typed);
        nameField()->setFocus();
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return nameField()->hasFocus();
                         },
                         2s),
                 "focus never left the description editor");

        QVERIFY2(runExport(packageName), "the export never finished");
        settleSaves();
        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));
        QCOMPARE(mpHost->mPackageInfo.value(packageName).value(qsl("description")), typed);
    }

    // Assets ride along in the archive and are unpacked into the profile with
    // the package - a folder of them keeps its shape.
    void test_assetFilesAndFoldersAreCarriedIntoThePackage()
    {
        const QString packageName = packageNamed(qsl("exporter-assets"));
        makeTrigger(qsl("exporter asset trigger"), nullptr);

        const QString assetSource = qsl("%1/asset-source").arg(mExportDir);
        QVERIFY(QDir().mkpath(qsl("%1/soundpack/nested").arg(assetSource)));
        QVERIFY(writeTextFile(qsl("%1/readme.txt").arg(assetSource), qsl("a loose asset file")));
        QVERIFY(writeTextFile(qsl("%1/soundpack/nested/deep.txt").arg(assetSource), qsl("an asset in a subfolder")));

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter asset trigger")));
        addedFiles()->addItem(qsl("%1/readme.txt").arg(assetSource));
        addedFiles()->addItem(qsl("%1/soundpack").arg(assetSource));

        QVERIFY2(runExport(packageName), "the export never finished");
        settleSaves();
        auto [installed, reason] = mpHost->installPackage(packagePath(packageName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));

        QVERIFY2(QFileInfo::exists(qsl("%1/%2/readme.txt").arg(profileHome(), packageName)), "the loose asset file did not survive the round trip");
        QVERIFY2(QFileInfo::exists(qsl("%1/%2/soundpack/nested/deep.txt").arg(profileHome(), packageName)), "the asset folder's nested file did not survive the round trip");
        QCOMPARE(readAll(qsl("%1/%2/soundpack/nested/deep.txt").arg(profileHome(), packageName)), QByteArray("an asset in a subfolder"));
    }

    // An asset that has been moved or deleted since it was added stops the
    // export and says which one, rather than shipping a package without it.
    void test_anAssetThatIsNoLongerThereStopsTheExport()
    {
        const QString packageName = packageNamed(qsl("exporter-missing-asset"));
        makeTrigger(qsl("exporter missing asset trigger"), nullptr);

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter missing asset trigger")));
        addedFiles()->addItem(qsl("%1/never-was-here.txt").arg(mExportDir));

        nameField()->setText(packageName);
        mpExporter->slot_exportPackage();
        QVERIFY(waitForExportToSettle());

        QVERIFY2(infoLabel()->text().contains(qsl("doesn't seem to exist anymore")), qPrintable(qsl("The missing asset has to be reported, but the label held: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(infoLabel()->text().contains(qsl("never-was-here.txt")), "the message has to name the asset that is missing");
        QVERIFY2(!QFileInfo::exists(packagePath(packageName)), "a package file was written even though one of its assets was missing");
        QVERIFY2(exportButton()->isEnabled(), "the Export button was left disabled after a failed export");
    }

    // Declining the overwrite prompt has to leave the existing package file
    // exactly as it was: the zip step truncates its destination the moment it
    // opens it, so answering no after that point is no answer at all.
    void test_decliningTheOverwritePromptLeavesTheExistingFileAlone()
    {
        const QString packageName = packageNamed(qsl("exporter-overwrite"));
        makeTrigger(qsl("exporter overwrite trigger"), nullptr);
        const QByteArray sentinel("not a package at all");
        QVERIFY(writeTextFile(packagePath(packageName), QString::fromUtf8(sentinel)));

        openExporter();
        QVERIFY(checkItem(triggersTop(), qsl("exporter overwrite trigger")));
        nameField()->setText(packageName);

        answerNextMessageBox(QMessageBox::No);
        mpExporter->slot_exportPackage();
        // the zip is written on a worker thread, so the file still holding the
        // sentinel the instant slot_exportPackage() returns proves nothing on
        // its own - what proves it is that the export never started at all
        QVERIFY2(!infoLabel()->text().contains(qsl("Exporting package...")), qPrintable(qsl("Declining the overwrite has to abort the export, but the label held: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(!QFileInfo::exists(stagingPath(packageName)), "declining the overwrite still built the package in the staging directory");
        QCOMPARE(readAll(packagePath(packageName)), sentinel);

        answerNextMessageBox(QMessageBox::Yes);
        mpExporter->slot_exportPackage();
        QVERIFY(waitForExportToSettle());
        QVERIFY2(readAll(packagePath(packageName)).startsWith("PK"), qPrintable(qsl("Accepting the overwrite has to replace the file, but the dialog said: \"%1\"").arg(infoLabel()->text())));
    }

    // Choosing an installed package fills the form from what that package
    // declared and checks the items belonging to it - but not its master
    // folder, which would only add a level of nesting to the new package.
    void test_choosingAnInstalledPackageFillsTheFormAndChecksItsItems()
    {
        const QString existingPackage = qsl("exporter-existing");
        const QString iconDirectory = qsl("%1/%2/.mudlet/Icon").arg(profileHome(), existingPackage);
        QVERIFY(QDir().mkpath(iconDirectory));
        QImage icon(2, 2, QImage::Format_ARGB32);
        icon.fill(Qt::blue);
        QVERIFY(icon.save(qsl("%1/badge.png").arg(iconDirectory)));

        auto* master = makeTrigger(existingPackage, nullptr, true);
        master->mPackageName = existingPackage;
        auto* member = makeTrigger(qsl("exporter package member"), master);
        member->mPackageName = existingPackage;
        makeTrigger(qsl("exporter outsider"), nullptr);

        mpHost->mInstalledPackages << existingPackage;
        mpHost->mPackageInfo[existingPackage] = QMap<QString, QString>{{qsl("mpackage"), existingPackage},
                                                                       {qsl("author"), qsl("The Previous Author")},
                                                                       {qsl("title"), qsl("The previous summary")},
                                                                       {qsl("version"), qsl("9.9")},
                                                                       {qsl("icon"), qsl("badge.png")},
                                                                       {qsl("helpURL"), qsl("https://example.org/previous")},
                                                                       {qsl("dependencies"), qsl("")}};

        openExporter();
        auto* packageList = comboNamed(qsl("packageList"));
        const int index = packageList->findText(existingPackage);
        QVERIFY2(index > 0, "the installed package was not offered in the dropdown");
        packageList->setCurrentIndex(index);

        QCOMPARE(nameField()->text(), existingPackage);
        QCOMPARE(lineEditNamed(qsl("lineEdit_author"))->text(), qsl("The Previous Author"));
        QCOMPARE(lineEditNamed(qsl("lineEdit_title"))->text(), qsl("The previous summary"));
        QCOMPARE(lineEditNamed(qsl("lineEdit_version"))->text(), qsl("9.9"));
        QCOMPARE(lineEditNamed(qsl("lineEdit_helpUrl"))->text(), qsl("https://example.org/previous"));

        auto* masterItem = itemNamed(triggersTop(), existingPackage);
        QVERIFY(masterItem);
        QCOMPARE(masterItem->childCount(), 1);
        QCOMPARE(masterItem->child(0)->checkState(0), Qt::Checked);
        QCOMPARE(masterItem->checkState(0), Qt::Unchecked);
        QCOMPARE(itemNamed(triggersTop(), qsl("exporter outsider"))->checkState(0), Qt::Unchecked);

        // the icon the package declared is carried into whatever is exported next
        const QString rebuiltName = packageNamed(qsl("exporter-rebuilt"));
        QVERIFY2(runExport(rebuiltName), "the export never finished");
        settleSaves();
        auto [installed, reason] = mpHost->installPackage(packagePath(rebuiltName), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(reason));
        QCOMPARE(mpHost->mPackageInfo.value(rebuiltName).value(qsl("icon")), qsl("badge.png"));
        QVERIFY2(QFileInfo::exists(qsl("%1/%2/.mudlet/Icon/badge.png").arg(profileHome(), rebuiltName)), "the icon file did not survive the round trip");
    }

    // Dependencies are moved between the list of what is available and the list
    // of what this package needs, and the Delete key is the way back.
    void test_dependenciesMoveBetweenTheAvailableAndRequiredLists()
    {
        mpHost->mInstalledPackages << qsl("exporter-dep-one") << qsl("exporter-dep-two");
        openExporter();

        auto* available = comboNamed(qsl("DependencyList"));
        auto* required = comboNamed(qsl("comboBox_dependencies"));
        QCOMPARE(required->count(), 0);
        // a new profile arrives with packages of its own, so count from what is
        // there rather than from two
        const int offered = available->count();
        QVERIFY2(available->findText(qsl("exporter-dep-one")) > 0 && available->findText(qsl("exporter-dep-two")) > 0, "the installed packages were not offered as dependencies");

        // row zero is the "add dependencies" prompt, not a package
        available->setCurrentIndex(0);
        buttonNamed(qsl("pushButton_addDependency"))->click();
        QVERIFY2(required->count() == 0, "the 'add dependencies' prompt row was added as though it were a package");
        QCOMPARE(available->count(), offered);

        available->setCurrentIndex(available->findText(qsl("exporter-dep-two")));
        buttonNamed(qsl("pushButton_addDependency"))->click();
        QCOMPARE(required->count(), 1);
        QCOMPARE(required->currentText(), qsl("exporter-dep-two"));
        QCOMPARE(available->count(), offered - 1);
        QCOMPARE(available->findText(qsl("exporter-dep-two")), -1);
        QVERIFY2(available->findText(qsl("exporter-dep-one")) >= 0, "the dependency that was not chosen was taken out of the list as well");

        QTest::keyClick(required, Qt::Key_Delete);
        QCOMPARE(required->count(), 0);
        QVERIFY2(available->findText(qsl("exporter-dep-two")) >= 0, "a removed dependency was not offered again");
    }

    // Module creation is the same dialog relabelled, and it saves to the profile
    // folder and installs what it made rather than leaving a file behind.
    void test_moduleCreationModeSavesToTheProfileAndInstallsWhatItMade()
    {
        const QString moduleName = packageNamed(qsl("exporter-module"));
        makeTrigger(qsl("exporter module trigger"), nullptr);

        openExporter();
        mpExporter->setModuleCreationMode(true);
        QVERIFY2(mpExporter->windowTitle().contains(qsl("Create Module")), qPrintable(qsl("The title has to say what is being made, but said: \"%1\"").arg(mpExporter->windowTitle())));
        QCOMPARE(exportButton()->text(), qsl("Create Module"));
        QVERIFY2(comboNamed(qsl("packageList"))->isHidden(), "the 'update an installed package' dropdown has no meaning when creating a module");

        QVERIFY(checkItem(triggersTop(), qsl("exporter module trigger")));
        settleSaves();
        nameField()->setText(moduleName);
        mpExporter->slot_exportPackage();
        QVERIFY(waitForExportToSettle());

        QVERIFY2(QFileInfo::exists(qsl("%1/%2.mpackage").arg(profileHome(), moduleName)),
                 qPrintable(qsl("A module has to be saved in the profile folder rather than the last file dialog location. The dialog said: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(!QFileInfo::exists(packagePath(moduleName)), "the module was written to the package export location as well");
        QVERIFY2(mpHost->mInstalledModules.contains(moduleName), qPrintable(qsl("The module it made was not installed. The dialog said: \"%1\"").arg(infoLabel()->text())));
        QVERIFY2(infoLabel()->text().contains(qsl("created and installed successfully")), qPrintable(qsl("The label held: \"%1\"").arg(infoLabel()->text())));
        // the name is cleared so that another module can be made straight away
        QVERIFY2(nameField()->text().isEmpty(), "the module name was left in the field afterwards");
    }
};

#include "PackageExporterDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageExporterDialogTest)

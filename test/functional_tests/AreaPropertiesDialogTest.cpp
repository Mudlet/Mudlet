/***************************************************************************
 *   Copyright (C) 2026 by Jens Wegener - code@jenswegener.net             *
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
 * dlgAreaProperties edits one area's values for the fields its map defines (TAreaFields): what it shows for each, what
 * OK keeps, and that a value it cannot keep stops OK rather than being written or lost.
 */

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "TArea.h"
#include "TAreaFields.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "dlgAreaProperties.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class AreaPropertiesDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("AreaPropertiesDialog-Test");
    int mContinentId = 0;
    int mZoneId = 0;
    QPointer<dlgAreaProperties> mpDialog;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TArea* zone() const { return roomDB()->getArea(mZoneId); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // A zone inside a continent, and a field of each type
    void buildMap(const bool withFields = true)
    {
        map()->mapClear();
        mContinentId = roomDB()->addArea(qsl("Continent"));
        mZoneId = roomDB()->addArea(qsl("Zone"));
        QVERIFY(mContinentId > 0 && mZoneId > 0);
        QCOMPARE(roomDB()->setAreaParent(mZoneId, mContinentId), TRoomDB::AreaParentResult::Changed);
        QMap<QString, TAreaFields::Field> fields;
        if (withFields) {
            fields.insert(qsl("levelRange"), TAreaFields::Field{qsl("levelRange"), qsl("Level range"), TAreaFields::Type::Range, true, {}});
            fields.insert(qsl("noMagic"), TAreaFields::Field{qsl("noMagic"), qsl("No magic"), TAreaFields::Type::YesNo, true, {}});
            fields.insert(qsl("population"), TAreaFields::Field{qsl("population"), qsl("Population"), TAreaFields::Type::Number, false, {}});
            fields.insert(qsl("terrain"), TAreaFields::Field{qsl("terrain"), qsl("Terrain"), TAreaFields::Type::Choice, false, {qsl("forest"), qsl("swamp")}});
        }
        TAreaFields::setFields(*map(), fields);
    }

    dlgAreaProperties* openDialog()
    {
        mpDialog = new dlgAreaProperties(map(), mZoneId);
        return mpDialog;
    }

    QWidget* editorFor(const QString& key) const
    {
        for (QWidget* pEditor : mpDialog->findChildren<QWidget*>(qsl("areaPropertiesField"))) {
            if (pEditor->property("areaField").toString() == key) {
                return pEditor;
            }
        }
        return nullptr;
    }
    QLineEdit* lineFor(const QString& key) const { return qobject_cast<QLineEdit*>(editorFor(key)); }
    QComboBox* listFor(const QString& key) const { return qobject_cast<QComboBox*>(editorFor(key)); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());
    }

    // The dialog deletes itself on close, which may already be queued
    void cleanup()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        delete mpDialog.data();
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void aMapWithNoFieldsSaysHowToDefineThem()
    {
        buildMap(false);
        auto* pDialog = openDialog();
        QCOMPARE(pDialog->fieldCount(), 0);
        auto* pNone = pDialog->findChild<QLabel*>(qsl("areaPropertiesNoFields"));
        QVERIFY(pNone);
        QVERIFY(pNone->text().contains(qsl("setMapAreaFields")));
    }

    void eachFieldShowsTheAreasOwnValue()
    {
        buildMap();
        zone()->mUserData = {{qsl("levelRange"), qsl("10-20")}, {qsl("noMagic"), qsl("false")}, {qsl("population"), qsl("1200")}, {qsl("terrain"), qsl("swamp")}};
        openDialog();
        QCOMPARE(mpDialog->fieldCount(), 4);
        QCOMPARE(lineFor(qsl("levelRange"))->text(), qsl("10-20"));
        QCOMPARE(lineFor(qsl("population"))->text(), qsl("1200"));
        QCOMPARE(listFor(qsl("noMagic"))->currentData().toString(), qsl("false"));
        QCOMPARE(listFor(qsl("terrain"))->currentText(), qsl("swamp"));
        QCOMPARE(lineFor(qsl("levelRange"))->accessibleName(), qsl("Level range"));
    }

    // An inherited field the zone has no value for shows what it takes from the continent, and where from
    void anInheritedFieldShowsTheParentAreasValue()
    {
        buildMap();
        roomDB()->getArea(mContinentId)->mUserData = {{qsl("levelRange"), qsl("1-50")}, {qsl("noMagic"), qsl("true")}};
        openDialog();
        QCOMPARE(lineFor(qsl("levelRange"))->text(), QString());
        QCOMPARE(lineFor(qsl("levelRange"))->placeholderText(), qsl("1-50 (from Continent)"));
        QCOMPARE(listFor(qsl("noMagic"))->currentText(), qsl("(from Continent: Yes)"));
        QVERIFY(!listFor(qsl("noMagic"))->currentData().isValid());
    }

    void okKeepsWhatWasEnteredAndRemovesWhatWasEmptied()
    {
        buildMap();
        zone()->mUserData = {{qsl("population"), qsl("1200")}, {qsl("other"), qsl("left alone")}};
        openDialog();
        lineFor(qsl("levelRange"))->setText(qsl(" 10-20 "));
        lineFor(qsl("population"))->clear();
        listFor(qsl("noMagic"))->setCurrentIndex(listFor(qsl("noMagic"))->findData(qsl("true")));
        listFor(qsl("terrain"))->setCurrentText(qsl("forest"));
        mpDialog->accept();

        QCOMPARE(zone()->mUserData,
                 (QMap<QString, QString>{{qsl("levelRange"), qsl("10-20")}, {qsl("noMagic"), qsl("true")}, {qsl("terrain"), qsl("forest")}, {qsl("other"), qsl("left alone")}}));
    }

    // Nothing is written while one value does not fit: the player is told which, and the dialog stays open
    void okRefusesAValueThatDoesNotFitAndKeepsNothing()
    {
        buildMap();
        openDialog();
        mpDialog->show();
        lineFor(qsl("population"))->setText(qsl("many"));
        lineFor(qsl("levelRange"))->setText(qsl("10-20"));
        QString told;
        QTimer answer;
        answer.setInterval(20ms);
        connect(&answer, &QTimer::timeout, this, [&]() {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                told = box->text();
                answer.stop();
                box->accept();
            }
        });
        answer.start();
        mpDialog->accept();

        QVERIFY2(told.contains(qsl("Population")), qPrintable(told));
        QVERIFY(mpDialog->isVisible());
        QVERIFY(zone()->mUserData.isEmpty());
    }

    // A script's value that does not fit is kept as it is while it is left alone, so opening the dialog and pressing OK
    // changes nothing
    void aValueThatDoesNotFitIsKeptWhileUnchanged()
    {
        buildMap();
        zone()->mUserData = {{qsl("population"), qsl("lots")}, {qsl("terrain"), qsl("mountains")}};
        openDialog();
        QCOMPARE(lineFor(qsl("population"))->text(), qsl("lots"));
        QCOMPARE(listFor(qsl("terrain"))->currentText(), qsl("mountains"));
        mpDialog->accept();
        QCOMPARE(zone()->mUserData, (QMap<QString, QString>{{qsl("population"), qsl("lots")}, {qsl("terrain"), qsl("mountains")}}));
    }

    // OK on a row nobody touched leaves its text as the area had it, to the byte, though the dialog would write it
    // otherwise: without the spaces, and "true" for "yes"
    void anUntouchedValueIsKeptExactly()
    {
        buildMap();
        zone()->mUserData = {{qsl("population"), qsl(" 1200 ")}, {qsl("noMagic"), qsl("yes")}};
        openDialog();
        mpDialog->accept();
        QCOMPARE(zone()->mUserData, (QMap<QString, QString>{{qsl("population"), qsl(" 1200 ")}, {qsl("noMagic"), qsl("yes")}}));
    }

    // A map loaded or deleted while the dialog is open may hold an area of the same ID, which is another area: OK
    // writes nothing to it, and says so
    void okWritesNothingOnceTheMapIsReplaced()
    {
        buildMap();
        openDialog();
        mpDialog->show();
        lineFor(qsl("population"))->setText(qsl("1200"));
        map()->mapClear();
        QVERIFY(roomDB()->addArea(mZoneId, qsl("Another zone")));
        QString told;
        QTimer answer;
        answer.setInterval(20ms);
        connect(&answer, &QTimer::timeout, this, [&]() {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                told = box->text();
                answer.stop();
                box->accept();
            }
        });
        answer.start();
        mpDialog->accept();

        QVERIFY2(told.contains(qsl("replaced")), qPrintable(told));
        QVERIFY(zone()->mUserData.isEmpty());
        QVERIFY(!mpDialog || !mpDialog->isVisible());
    }
};

#include "AreaPropertiesDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(AreaPropertiesDialogTest)

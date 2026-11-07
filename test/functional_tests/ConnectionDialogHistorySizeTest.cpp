/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * The connection dialog's list of a profile's saves shows each save's size, so
 * a damaged save - usually far smaller than its neighbours - stands out when
 * picking one to restore. See issue #5975.
 *
 * Run with: ctest -R ConnectionDialogHistorySizeTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletApp.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QListWidget>

#include "GroupedTest.h"

class ConnectionDialogHistorySizeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;
    const QString mProfileName = qsl("ConnDialogHistorySize");

    bool writeSave(const QString& fileName, const qsizetype size) const
    {
        QFile file(qsl("%1/%2").arg(MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName), fileName));
        return file.open(QIODevice::WriteOnly) && file.write(QByteArray(size, 'x')) == size;
    }

    static QString sizeText(const qint64 size) { return mudlet::self()->getUserLocale().formattedDataSize(size, 1, QLocale::DataSizeTraditionalFormat); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path())));
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void test_everySaveShowsItsSize()
    {
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName)));
        const QMap<QString, qsizetype> saves{{qsl("2020-01-01#00-00-00.xml"), 2560}, {qsl("autosave.xml"), 12}, {qsl("before upgrade.xml"), 3 * 1024 * 1024}};
        for (auto it = saves.cbegin(); it != saves.cend(); ++it) {
            QVERIFY(writeSave(it.key(), it.value()));
        }

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->fillout_form();
        QListWidgetItem* pProfileItem = nullptr;
        for (int row = 0; row < pDialog->listWidget_profiles->count(); ++row) {
            auto* pItem = pDialog->listWidget_profiles->item(row);
            if (pItem->data(dlgConnectionProfiles::csmNameRole).toString() == mProfileName) {
                pProfileItem = pItem;
            }
        }
        QVERIFY2(pProfileItem, "the profile made for this test is not listed");
        pDialog->slot_itemClicked(pProfileItem);

        QCOMPARE(pDialog->profile_history->count(), saves.size());
        for (int index = 0; index < pDialog->profile_history->count(); ++index) {
            const QString fileName = pDialog->profile_history->itemData(index).toString();
            QVERIFY2(saves.contains(fileName), qPrintable(fileName));
            const QString text = pDialog->profile_history->itemText(index);
            QVERIFY2(text.endsWith(qsl(" (%1)").arg(sizeText(saves.value(fileName)))), qPrintable(qsl("%1 is listed as \"%2\"").arg(fileName, text)));
        }
        // the size is added to the date, not put in its place
        const QString dated = pDialog->profile_history->itemText(pDialog->profile_history->findData(qsl("2020-01-01#00-00-00.xml")));
        const QString expectedDate = mudlet::self()->getUserLocale().toString(QDate(2020, 1, 1), QLocale::LongFormat);
        QVERIFY2(dated.startsWith(expectedDate), qPrintable(dated));
        // a custom-named save is still listed by its name
        QVERIFY(pDialog->profile_history->itemText(pDialog->profile_history->findData(qsl("before upgrade.xml"))).startsWith(qsl("before upgrade.xml (")));
        delete pDialog;
    }
};

#include "ConnectionDialogHistorySizeTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogHistorySizeTest)

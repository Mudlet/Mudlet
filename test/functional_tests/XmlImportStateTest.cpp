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

// Every package, module, profile and map XML in the source tree, imported into one profile
// read from pugixml's tree and into another read by QXmlStreamReader, must leave the two
// in the same state, byte for byte as XMLexport and the map's own serialization write it.

#include "GroupedTest.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "SettingsTestHelper.h"
#include "TMap.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "XmlImportCorpus.h"
#include "mudlet.h"

#include <QDataStream>
#include <QtTest/QtTest>

#include <sstream>

class XmlImportStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mUnpackDir;
    QByteArray mSavedXdg;
    QList<XmlImportCorpus::Entry> mCorpus;
    Host* mpTreeHost = nullptr;
    Host* mpStreamHost = nullptr;
    const QString mTreeName = qsl("XmlImportStateTree-Test");
    const QString mStreamName = qsl("XmlImportStateStream-Test");

    struct Outcome
    {
        bool success = false;
        QString message;
        QString version;
        QStringList itemsWithErrors;
        QStringList itemsWithErrorNames;
        bool readAnything = false;
        bool readFromTree = false;
    };

    static Outcome import(Host* host, const QByteArray& xml, const QString& packageName, int moduleFlag, bool throughStreamReader)
    {
        XmlImportReader::setAlwaysUseStreamReaderForTesting(throughStreamReader);
        // importPackage() takes a QFile, and only ever reads it as a device
        QTemporaryFile file;
        file.open();
        file.write(xml);
        file.seek(0);
        Outcome outcome;
        XMLimport reader(host);
        std::tie(outcome.success, outcome.message) = reader.importPackage(&file, packageName, moduleFlag, &outcome.version);
        outcome.itemsWithErrors = reader.itemsWithErrors();
        outcome.itemsWithErrorNames = reader.itemsWithErrorNames();
        outcome.readAnything = reader.readAnything();
        outcome.readFromTree = reader.readingParsedTree();
        XmlImportReader::setAlwaysUseStreamReaderForTesting(false);
        return outcome;
    }

    static QByteArray state(Host* host, const QString& name)
    {
        XMLexport exporter(host);
        pugi::xml_document document;
        exporter.writeHost(host, document.append_child("MudletPackage"));
        std::ostringstream stream;
        document.save(stream);
        QByteArray result = QByteArray::fromStdString(stream.str());
        result.replace(name.toUtf8(), "<profile>");

        QByteArray map;
        QDataStream out(&map, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_12);
        host->mpMap->serialize(out);
        return result + "\n--- map ---\n" + map.toBase64();
    }

    void freshProfiles()
    {
        auto* hostManager = HostManager::self();
        for (const QString& name : {mTreeName, mStreamName}) {
            if (hostManager->getHost(name)) {
                hostManager->deleteHost(name);
            }
            hostManager->addHost(name, QString(), QString(), QString());
        }
        mpTreeHost = hostManager->getHost(mTreeName);
        mpStreamHost = hostManager->getHost(mStreamName);
    }

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
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        freshProfiles();
        QVERIFY(mpTreeHost && mpStreamHost);

        QVERIFY(mUnpackDir.isValid());
        mCorpus = XmlImportCorpus::load(qsl(MUDLET_SRC_DIR), mUnpackDir);
        QVERIFY2(mCorpus.size() > 40, qPrintable(qsl("only %1 corpus files were found").arg(mCorpus.size())));
    }

    void cleanupTestCase()
    {
        mpTreeHost = nullptr;
        mpStreamHost = nullptr;
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mTreeName);
            TestSettings::deleteProfileDirectory(mStreamName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A few at a time into the same pair of profiles, as a profile collects its packages
    void test_everyFileLeavesTheSameState()
    {
        int fromTree = 0;
        int imports = 0;
        int entries = 0;
        for (const auto& entry : std::as_const(mCorpus)) {
            if (++entries % 8 == 0) {
                freshProfiles();
                QVERIFY(mpTreeHost && mpStreamHost);
            }
            const bool map = entry.bytes.left(512).contains("<map>");
            const QString packageName = QFileInfo(entry.name).completeBaseName();
            // A map, or a profile's settings, are only read without a package name
            QList<std::pair<QString, int>> ways{{QString(), 0}};
            if (!map) {
                ways << std::pair{packageName, 0} << std::pair{packageName + qsl("-module"), 1};
            }
            for (const auto& [name, moduleFlag] : std::as_const(ways)) {
                const Outcome tree = import(mpTreeHost, entry.bytes, name, moduleFlag, false);
                const Outcome stream = import(mpStreamHost, entry.bytes, name, moduleFlag, true);
                const QString what = qsl("%1 read as \"%2\" (module %3)").arg(entry.name, name, QString::number(moduleFlag));
                QVERIFY2(!stream.readFromTree, qPrintable(what));
                QCOMPARE(tree.success, stream.success);
                QCOMPARE(tree.message, stream.message);
                QCOMPARE(tree.version, stream.version);
                QCOMPARE(tree.itemsWithErrors, stream.itemsWithErrors);
                QCOMPARE(tree.itemsWithErrorNames, stream.itemsWithErrorNames);
                QCOMPARE(tree.readAnything, stream.readAnything);
                const QByteArray treeState = state(mpTreeHost, mTreeName);
                const QByteArray streamState = state(mpStreamHost, mStreamName);
                if (treeState != streamState) {
                    qsizetype at = 0;
                    while (at < treeState.size() && at < streamState.size() && treeState.at(at) == streamState.at(at)) {
                        ++at;
                    }
                    QFAIL(qPrintable(qsl("%1 left the profiles differing from byte %2:\n    tree:   %3\n    stream: %4")
                                             .arg(what,
                                                  QString::number(at),
                                                  QString::fromUtf8(treeState.mid(std::max<qsizetype>(0, at - 80), 200)),
                                                  QString::fromUtf8(streamState.mid(std::max<qsizetype>(0, at - 80), 200)))));
                }
                fromTree += tree.readFromTree;
                ++imports;
            }
        }
        qInfo().nospace() << imports << " imports, " << fromTree << " of them read from the tree";
        QVERIFY2(fromTree * 4 > imports * 3, "most of the corpus was not read from the tree, so this compared QXmlStreamReader with itself");
    }
};

MUDLET_GROUPED_TEST_MAIN(XmlImportStateTest)

#include "XmlImportStateTest.moc"

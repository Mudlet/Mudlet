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
 * Covers the bookkeeping behind "Install" in the package manager's Explore
 * view - dlgPackageManager::slot_installPackageFromRepository(), which downloads
 * every selected package and then installs the batch once the last one has
 * landed.
 *
 * Nothing but that button starts one, so there is no Lua entry point and the
 * busted suite cannot reach any of it. What decides when the batch is finished
 * is a single counter shared between the reply handlers, and the tests here pin
 * down the ends of it that are easiest to get wrong: a download that fails, a
 * selection the repository index cannot answer for, one whose file cannot be
 * opened for writing, and a reply landing while the loop is stopped inside a
 * later selection's warning box. A counter left short of zero leaves the modal
 * "Downloading packages..." dialog up for the rest of the session, on top of a
 * profile the user can no longer click on.
 *
 * The downloads are pointed at a stub standing in as an HTTP proxy rather than
 * at github.com: an application proxy is the one place a QNetworkAccessManager
 * built with no seam for a test can still be redirected, and it means these
 * tests neither reach the network nor depend on a name resolving.
 *
 * A real profile rather than a bare Host, because the dialog connects to
 * mpHost->mainConsoleView() in its constructor and installs into a profile directory.
 *
 * Run with: ctest -R PackageRepositoryDownloadTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QPointer>
#include <QProgressDialog>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTimer>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgPackageManager.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Self-signed loopback certificate, valid until 2126, shared with
// FeedChecksumRaceTest.cpp and GMCPCharLoginTest.cpp. The client accepts it for
// any host because the test turns peer verification off in the default
// QSslConfiguration.
static const char* csmStubCertificatePem = R"PEM(-----BEGIN CERTIFICATE-----
MIIDJzCCAg+gAwIBAgIULa4vwGAVOB+r6qtcLMPqwzBlEJgwDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDgwNjE0MTM0OFoYDzIxMjYw
NzEzMTQxMzQ4WjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQDEg1HE09f69FW/OLD0jrWEQRbKkSkIkexLfV5OtzbI
ZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5LBEKGP1TTs6C9R+e5hlI6McoKown
ha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon7M4wzVvUO5VIRhFeA2AO6LRVQhyi
9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp4SbuHuXPwgiZ9hYEIX2xAFSR48OB
bjWgqVISptu/s+UkK2XckI42qdxqzwglLIIqjFYJ1HvGqhqV69DeqB0XNw6qp8W2
qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTfG1y8DnnLAgMBAAGjbzBtMB0GA1Ud
DgQWBBSyDWWzo202vFbYncaD2crvY5V2pDAfBgNVHSMEGDAWgBSyDWWzo202vFbY
ncaD2crvY5V2pDAPBgNVHRMBAf8EBTADAQH/MBoGA1UdEQQTMBGCCWxvY2FsaG9z
dIcEfwAAATANBgkqhkiG9w0BAQsFAAOCAQEAs5nw4GBPPHc9Nc08uLUYTDLkA2XM
WPugjSO7OxUe8NptVh/v4GbeKzQ4FRIF6rca8De15+OOZgIDppRUoy+fd+ncoDan
flw38rIj13XfV/3WF33Uag2xtZG0Hrpu4PFZQyIzr0MwGJJ/v2uRjMiV0CX+rc0L
BJg2JS4oCbNdQpwH81qOktoH8aHirAyLjtm732GQgAGLe0fIBBsb4Dg2ZdvN+TF5
xfKoFfri3H1rwju43zHXmUyCE/RPdIBR8flO6gzdgWAVY0jaixZi1fzQEQuReh2j
d2iZYOFSrVDea41ltrUvRC6q6gxe/REVjj1nCSYU1x44J9DQ6n6ljvJCVw==
-----END CERTIFICATE-----)PEM";

static const char* csmStubPrivateKeyPem = R"PEM(-----BEGIN PRIVATE KEY-----
MIIEwAIBADANBgkqhkiG9w0BAQEFAASCBKowggSmAgEAAoIBAQDEg1HE09f69FW/
OLD0jrWEQRbKkSkIkexLfV5OtzbIZVDWcH3Y3NKrbZ60j8WEY8DqVzO2kMnOppc5
LBEKGP1TTs6C9R+e5hlI6McoKownha4aU9nqM7dsjY71xGZNN9DCVxhRpqadlZon
7M4wzVvUO5VIRhFeA2AO6LRVQhyi9Whe/uJVlncb2tbiGgTavixWSQ5kH0ocE8Cp
4SbuHuXPwgiZ9hYEIX2xAFSR48OBbjWgqVISptu/s+UkK2XckI42qdxqzwglLIIq
jFYJ1HvGqhqV69DeqB0XNw6qp8W2qwTpv3gPGzI60vNL6aaHTivLxnsEClPbcrTf
G1y8DnnLAgMBAAECggEBALRPHebwzfrI2CilttAeZXTdWDEzsifX5K17cd3eBBkp
xVuNShuCupZq9bUNOhl4ghlDPALmpRTFDHp78YKHXWFkLN5CVeoxjL+2Po6fQ4w7
/3zOtWNMYp/q32Kn+4ocjaLT0U+SDs0G6LR7dtGWjAyXQylWiTbu9+OWJ2kXSTlH
QbdtamymoJrrjRTV1HUEq/a3qSHlqTA5/EKIcGeiETq2NR0fZ3NFbe+PLiOSpiNg
uIiVEdsItuZTdINSEzOtMFvRd2od0ITDpMtLG404aGsI4Zisiuhr5naf4DWqK2aL
n9Z/55LSuAdBqvrtJ9XVdtNsFdCRjbIj2R1qqDTFm/kCgYEA6W/+ufDXrhPi8XWS
8+7tlOoUd0jYZL9N+N1hfho21SN3eH5TtNO0b/os/PN/M+5dKeWPtnyzwg49EksF
Es9Z4+lLt/Z+71RDmYqSCwaLhXNKtUZluZmrGHcRogd4hJDYv9icgmpwMK5Hg634
PYCgVYb9C1Wug/mZhgLg7Aw3hn8CgYEA14GxAPViaSszVNRps+a9WVEJklPbPR8U
kAxWTP6n1SdT3Z9HRcHH9inIdTLyC/3ti4+4dc1pDkMrq+MUTjvF8BqN35uzJa7l
6dnsXBmWvB1cIcwQb4SLnDb7jzmiK2uIjMrO54x3+atB83GdvESLOQ/9NAJL/+NX
ILq5kAs2nrUCgYEAqq7/8pceLKNPybttMr0drEenpTx3NNsIORItydWDCD8BiPHd
ZJdzFHk5Uc780EzWg97dQNJXYWmlz+1YjVNdZ57ahW1PjNDxCKBgfn1PoMkW9ArA
MIAisSXGl9GcllmOkl/guB75Xy7fDXIz00xsb3zfIt2IV+k2Dt2l9hJMuyMCgYEA
lv45ZHCJeSJZntANF41NkazjxfCXJaYHJD5goSWztfcOHbOhnlB9qA3yc5s0WA6c
RzJ1jaRUPTf2+0HpUj8zGl2gldFjnb2DPWwA3S7YnAj+Knft9BSsNNGZQ+qfo0h+
rhbTDQ0wanABj25FlEl6OornX29UjH9e5oGtziztIhkCgYEAppTHqOgLiKmeV15d
i850uRyh7X6whywY8gm0VLO+xzCVsCR6CvgZY1MwwFuDwu2d/d5jdJXLpHueQwNU
3HipTI77OuIRv4ykXwPOIemT9VmL/N21CgrckJGA6dYywTnc/JNpOKxdTM9srOyr
Rcsgla9jttJevaHI71x2jLNBaKk=
-----END PRIVATE KEY-----)PEM";

// Stands in as the HTTP proxy every download in this file is pointed at. The
// package URLs are https, so what a real proxy would see first is a CONNECT
// establishing the tunnel - which is exactly the point at which this one either
// hangs up or says nothing at all, or else opens the tunnel and answers the
// request inside it itself.
class StubProxy : public QTcpServer
{
    Q_OBJECT

public:
    enum class Answer {
        // hang up as soon as the connection is accepted, before the tunnel
        // request is even read, so the reply fails as soon as it is made - a
        // machine that is online but cannot reach the repository
        Refuse,
        // accept and then say nothing, so the download stays in flight until
        // the request's own 30s transfer timeout, which outlasts these tests
        Stall,
        // open the tunnel and answer whatever is asked inside it with a 200
        // carrying the body given to serve()
        Serve,
    };

    explicit StubProxy(QObject* parent = nullptr)
    : QTcpServer(parent)
    {
    }

    void setAnswer(const Answer answer) { mAnswer = answer; }

    void serve(const QByteArray& body)
    {
        mAnswer = Answer::Serve;
        mBody = body;
    }

    QNetworkProxy asApplicationProxy() const { return QNetworkProxy(QNetworkProxy::HttpProxy, qsl("127.0.0.1"), serverPort()); }

protected:
    void incomingConnection(const qintptr socketDescriptor) override
    {
        auto* socket = new QSslSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
        if (mAnswer == Answer::Refuse) {
            socket->disconnectFromHost();
        } else if (mAnswer == Answer::Serve) {
            socket->setLocalCertificate(QSslCertificate(QByteArray(csmStubCertificatePem)));
            socket->setPrivateKey(QSslKey(QByteArray(csmStubPrivateKeyPem), QSsl::Rsa));
            connect(socket, &QSslSocket::readyRead, this, [this, socket]() {
                answerRequest(socket);
            });
        }
    }

private:
    void answerRequest(QSslSocket* socket)
    {
        const QByteArray request = socket->property("request").toByteArray() + socket->readAll();
        if (!request.contains("\r\n\r\n")) {
            socket->setProperty("request", request);
            return;
        }
        socket->setProperty("request", QByteArray());
        if (request.startsWith("CONNECT ")) {
            socket->write("HTTP/1.1 200 Connection established\r\n\r\n");
            socket->flush();
            socket->startServerEncryption();
            return;
        }
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + QByteArray::number(mBody.size()) + "\r\nConnection: close\r\n\r\n");
        socket->write(mBody);
        socket->flush();
        socket->disconnectFromHost();
    }

    Answer mAnswer = Answer::Refuse;
    QByteArray mBody;
};

// Closes any message box that appears, recording what it said. The download
// failure path reports itself with a static QMessageBox::warning(), which spins
// a nested event loop inside the reply handler and would otherwise stop the
// test dead - and what it said is worth keeping, since it is the proof that the
// download was really attempted and really failed.
class MessageBoxDismisser : public QObject
{
    Q_OBJECT

public:
    explicit MessageBoxDismisser(QObject* parent = nullptr)
    : QObject(parent)
    {
        mSinceStart.start();
        connect(&mTimer, &QTimer::timeout, this, &MessageBoxDismisser::dismissAny);
        mTimer.start(25ms);
    }

    // Leaves a box that is on its own up until another is stacked on top of it,
    // for at most this long. Answering a lone box is what would let the run
    // continue past the very nesting a test is waiting to observe, and how long
    // the event that stacks the second one takes is not something a loaded CI
    // runner - or a developer's machine - holds to any fixed figure. The cap is
    // there so a stack that never comes ends the test rather than hanging it.
    void setWaitForStacking(std::chrono::milliseconds maxWait) { mWaitForStackingMs = maxWait.count(); }

    // Whether two boxes were ever open at once while that wait was in force, i.e.
    // whether the nesting actually happened rather than the cap running out
    bool sawStacking() const { return mSawStacking; }

    // Whether the progress dialog was ever seen gone while a warning box was
    // still up. finishBatch() is what takes that dialog down, and every warning
    // here is raised from inside the loop that finishBatch() ends, so a true
    // means the batch was wound up while the loop was still stopped one frame
    // further in - the failure the nesting tests exist to catch.
    bool progressGoneWhileBoxUp() const { return mProgressGoneWhileBoxUp; }

    QStringList seen() const { return mSeen; }

private:
    void dismissAny()
    {
        const auto widgets = QApplication::topLevelWidgets();
        QList<QMessageBox*> visible;
        for (QWidget* widget : widgets) {
            auto* box = qobject_cast<QMessageBox*>(widget);
            if (box && box->isVisible()) {
                visible << box;
            }
        }
        if (visible.size() >= 2) {
            mSawStacking = true;
        }
        if (!visible.isEmpty() && !mProgressGoneWhileBoxUp) {
            // allWidgets() rather than the top-level list above: the progress
            // dialog is parented to the package manager, so it need not be
            // enumerated as a window of its own.
            bool progressUp = false;
            for (QWidget* widget : QApplication::allWidgets()) {
                auto* progress = qobject_cast<QProgressDialog*>(widget);
                if (progress && progress->isVisible()) {
                    progressUp = true;
                    break;
                }
            }
            mProgressGoneWhileBoxUp = !progressUp;
        }

        const qint64 now = mSinceStart.elapsed();
        for (QMessageBox* box : visible) {
            mFirstSeen.insert(box, mFirstSeen.value(box, now));
        }

        // Which box is on top, asked of the modal stack Qt keeps rather than
        // worked out from when this timer first saw each one. Two boxes routinely
        // turn up inside a single 25ms poll - a reply failing inside a warning's
        // own event loop does exactly that - and then they share a timestamp, so
        // the tie falls to the order QApplication::topLevelWidgets() returns,
        // which is not defined. That is a coin flip, and it decided the recorded
        // order of the nesting test below often enough to fail it about six runs
        // in ten. activeModalWidget() is the innermost modal by construction.
        auto* newest = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!newest || !visible.contains(newest)) {
            // Not one of ours on top (the progress dialog is modal too). With a
            // single box up there is no ambiguity left to resolve; with more,
            // wait rather than guess.
            newest = (visible.size() == 1) ? visible.constFirst() : nullptr;
        }
        if (!newest) {
            return;
        }

        // Still waiting for the interleaving under test and nothing has stacked
        // yet: leave the one box that is up alone. Answering it is what would let
        // the run continue past the very nesting it is here to observe. Once a
        // stack has been seen the wait is spent, so the boxes that unwind
        // afterwards are not held for it a second time.
        if (!mSawStacking && mWaitForStackingMs > 0 && visible.size() < 2 && now - mFirstSeen.value(newest) < mWaitForStackingMs) {
            return;
        }

        // Answering the innermost is what unwinds the stack: an outer box cannot
        // return until the one inside it has, so this records them top down.
        mSeen << newest->text();
        mFirstSeen.remove(newest);
        newest->done(QMessageBox::Ok);
    }

    QTimer mTimer;
    QElapsedTimer mSinceStart;
    QHash<QMessageBox*, qint64> mFirstSeen;
    qint64 mWaitForStackingMs = 0;
    bool mSawStacking = false;
    bool mProgressGoneWhileBoxUp = false;
    QStringList mSeen;
};

class PackageRepositoryDownloadTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    StubProxy* mpProxy = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("PackageRepositoryDownload-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    QSslConfiguration mOriginalSslConfiguration;

    QString profileHome() const { return MudletApp::getMudletPath(enums::profileHomePath, mProfileName); }

    // The repository index the package manager caches beside the profile, which
    // its constructor reads to fill the Explore view. Seeding it is what puts a
    // package in that list without a byte crossing the network.
    bool writeRepositoryIndex(const QJsonArray& packages) const
    {
        QFile file(qsl("%1/mpkg.packages.json").arg(profileHome()));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        QJsonObject root;
        root.insert(qsl("packages"), packages);
        return file.write(QJsonDocument(root).toJson()) > 0;
    }

    QByteArray readRepositoryIndex() const
    {
        QFile file(qsl("%1/mpkg.packages.json").arg(profileHome()));
        if (!file.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }
        return file.readAll();
    }

    static QJsonObject repositoryEntry(const QString& name, const QString& fileName)
    {
        QJsonObject entry;
        entry.insert(qsl("mpackage"), name);
        entry.insert(qsl("filename"), fileName);
        entry.insert(qsl("version"), qsl("1.0.0"));
        entry.insert(qsl("title"), name);
        entry.insert(qsl("description"), qsl("a package that exists only in this test"));
        entry.insert(qsl("author"), qsl("PackageRepositoryDownloadTest"));
        return entry;
    }

    // A dialog showing the Explore view - the only view whose Install button
    // downloads anything - with the given packages listed in it.
    dlgPackageManager* openManagerListing(const QJsonArray& packages)
    {
        if (!writeRepositoryIndex(packages)) {
            return nullptr;
        }
        auto* manager = new dlgPackageManager(nullptr, mpHost);
        manager->pushButton_explore->click();
        return manager;
    }

    // Selects without making anything the current item: the current item is
    // what starts an icon download, and that is a second network request with
    // nothing to do with what is being tested here.
    static bool selectPackages(dlgPackageManager* manager, const QStringList& names)
    {
        if (!manager) {
            return false;
        }
        for (const QString& name : names) {
            const auto items = manager->packageList->findItems(name, Qt::MatchExactly);
            if (items.size() != 1) {
                return false;
            }
            items.constFirst()->setSelected(true);
        }
        return manager->packageList->selectedItems().size() == names.size();
    }

    // A QProgressDialog is disposed of with deleteLater(), and a DeferredDelete
    // posted at the event loop level already being run is not delivered by a
    // plain processEvents() - so ask for it by name.
    static bool settle()
    {
        qApp->processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        return true;
    }

    static bool downloadDialogStillUp(dlgPackageManager* manager) { return manager && !manager->findChildren<QProgressDialog*>().isEmpty(); }

    // How many file descriptors this process holds on 'path'. The descriptor is
    // what matters for the half-written package, and counting it holds whether or
    // not the QFile is still reachable from the dialog.
    //
    // Matched as a prefix because the kernel renders a descriptor still held on an
    // unlinked file as "<path> (deleted)"; an exact comparison would score that
    // zero and pass the test for the one leak it is least able to afford to miss.
    static int openHandlesFor(const QString& path)
    {
        int count = 0;
        const QDir descriptors(qsl("/proc/self/fd"));
        const auto entries = descriptors.entryList(QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            if (QFile::symLinkTarget(descriptors.absoluteFilePath(entry)).startsWith(path)) {
                ++count;
            }
        }
        return count;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");

        mpProxy = new StubProxy(qApp);
        QVERIFY2(mpProxy->listen(QHostAddress::LocalHost, 0), "Could not start the stub proxy the downloads are pointed at");
        QNetworkProxy::setApplicationProxy(mpProxy->asApplicationProxy());

        // The stub answers for raw.githubusercontent.com with a certificate issued to localhost
        mOriginalSslConfiguration = QSslConfiguration::defaultConfiguration();
        QSslConfiguration configuration = mOriginalSslConfiguration;
        configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
        QSslConfiguration::setDefaultConfiguration(configuration);
    }

    void cleanupTestCase()
    {
        QSslConfiguration::setDefaultConfiguration(mOriginalSslConfiguration);
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        delete mpProxy;
        mpProxy = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            QDir(profileHome()).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A download that fails still has to end the batch: the count reaches zero,
    // and the modal progress dialog closes. One package is enough, since the
    // count then has a single step to make and zero is the only value that ends
    // the batch - a count decremented twice for one download steps over it.
    void test_aFailedDownloadEndsTheBatch()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("failing-package"), qsl("failing-package.mpackage"))});
        QVERIFY2(selectPackages(manager, {qsl("failing-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));
        QVERIFY2(downloadDialogStillUp(manager), "No progress dialog was raised for the download");

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QVERIFY2(dismisser.seen().constFirst().contains(qsl("failing-package")), "The reported failure was not the one this test arranged");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);
        QVERIFY2(manager->findChildren<QNetworkAccessManager*>().isEmpty(), "The batch's network manager outlived the batch");

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // A selected package the repository index cannot answer for never gets as far
    // as a request, so no reply will ever end the batch on its behalf: the count
    // has to be settled where the selection is refused, and the batch closed after
    // the loop. An entry with no filename is how the index produces one.
    void test_aPackageWithNothingToDownloadEndsTheBatch()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("nameless-package"), QString())});
        QVERIFY2(selectPackages(manager, {qsl("nameless-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QVERIFY2(dismisser.seen().constFirst().contains(qsl("nameless-package")), "The reported failure was not the one this test arranged");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // The file a download is written into is a child of the dialog, so closing the
    // package manager mid-download takes it, and its handle on the half-written
    // package, with it. The reply handler that would otherwise dispose of it has
    // the dialog for its context object and never runs once the dialog is gone.
    void test_closingMidDownloadReleasesTheDownloadFile()
    {
        mpProxy->setAnswer(StubProxy::Answer::Stall);
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("stalled-package"), qsl("stalled-package.mpackage"))});
        QVERIFY2(selectPackages(manager, {qsl("stalled-package")}), "The package under test was not listed in the Explore view");

        const QString downloadPath = qsl("%1/stalled-package.mpackage").arg(profileHome());
        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        // Ownership is assertable everywhere; the descriptor below only on Linux
        QCOMPARE(manager->findChildren<QFile*>().size(), 1);
#ifdef Q_OS_LINUX
        QCOMPARE(openHandlesFor(downloadPath), 1); // the download file is open, i.e. the download really did start
#endif

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);

#ifdef Q_OS_LINUX
        QCOMPARE(openHandlesFor(downloadPath), 0);
#endif
    }

    // The third way a selection can end without a download: its file cannot be
    // opened for writing. Like the refusals it has no reply to end the batch on
    // its behalf, and unlike them it used to say nothing at all, so the progress
    // dialog was the only thing the user had to go on. A directory sitting where
    // the download wants to write is a portable way to make open() fail.
    void test_aPackageThatCannotBeWrittenEndsTheBatch()
    {
        mpProxy->setAnswer(StubProxy::Answer::Stall);
        MessageBoxDismisser dismisser;

        const QString downloadPath = qsl("%1/blocked-package.mpackage").arg(profileHome());
        QVERIFY(QDir().mkpath(downloadPath));

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("blocked-package"), qsl("blocked-package.mpackage"))});
        QVERIFY2(selectPackages(manager, {qsl("blocked-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QVERIFY2(dismisser.seen().constFirst().contains(qsl("blocked-package")), "The reported failure was not the one this test arranged");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
        QVERIFY(QDir(downloadPath).removeRecursively());
    }

    // A refusal's warning box runs an event loop, so a download started by an
    // earlier selection can finish while the loop is stopped inside it. What
    // stops that from ending the batch early - tearing down the progress dialog
    // and resetting the package list while the loop is still holding pointers
    // into it - is that a refusal counts itself only after its box closes, so
    // its own selection is still outstanding for as long as the box is up, and
    // the count cannot be zero.
    //
    // Three selections, with the refusal in the middle: the last one is the one
    // that would be run against a torn-down batch, so it reporting its own
    // failure is what says the loop came through the middle one intact.
    void test_aReplyFinishingInsideARefusalDoesNotEndTheBatchEarly()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;
        // Hold the refusal's box open until the download's failure is stacked on
        // top of it, rather than for a fixed stretch and hoping the reply lands
        // inside it. The loop is stopped inside that box for exactly as long as
        // the box is up, so waiting on the stack itself is what makes the order
        // below causal instead of a race against how fast a refused connection
        // comes back - which varies enough to fail this outright, roughly six
        // runs in ten locally, never mind on a shared CI runner.
        dismisser.setWaitForStacking(20s);

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("a-first-download"), qsl("a-first-download.mpackage")),
                                                                  repositoryEntry(qsl("b-nameless-refusal"), QString()),
                                                                  repositoryEntry(qsl("c-second-download"), qsl("c-second-download.mpackage"))});
        QVERIFY2(selectPackages(manager, {qsl("a-first-download"), qsl("b-nameless-refusal"), qsl("c-second-download")}), "The packages under test were not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(settle() && dismisser.seen().size() == 3, 30s);

        // Without this the run could come out green having never stacked one box
        // inside another, which is the whole of what it is here to exercise. The
        // wait above is capped, so this is also what separates "the reply never
        // landed inside the refusal" from the ordering failures below.
        QVERIFY2(dismisser.sawStacking(), "No two warnings were ever open at once, so nothing finished inside the refusal and the case under test never happened");

        // Recorded newest first, so the first download's failure coming out ahead
        // of the refusal that was already on screen is what pins the nesting: the
        // only event loop it could have been delivered in is that refusal's
        QVERIFY2(dismisser.seen().at(0).contains(qsl("a-first-download")), "The first download did not fail inside the refusal's warning");
        QVERIFY2(dismisser.seen().at(1).contains(qsl("b-nameless-refusal")), "The refusal's warning was not the one underneath");
        QVERIFY2(dismisser.seen().at(2).contains(qsl("c-second-download")), "The selection after the refusal was never processed");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // The same nesting, with the refusal last. That ordering is what actually
    // puts the bookkeeping at risk, and the case above cannot: with the refusal
    // in the middle the selections after it are still outstanding, so the count
    // cannot reach zero inside its box however the refusal counts itself, and
    // moving that count to before the box leaves every assertion there green.
    //
    // Last, every other selection has already been counted, so a refusal that
    // counted itself before showing its box would take the count to zero the
    // moment a download failed inside that box - running finishBatch() there,
    // which closes the progress dialog, drops the network manager and resets the
    // package list while the loop is still stopped one frame further in. The
    // progress dialog still being up at the instant the two boxes are stacked is
    // what says that did not happen.
    void test_aRefusalLastKeepsTheBatchOpenUntilItsBoxCloses()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;
        dismisser.setWaitForStacking(20s);

        QPointer<dlgPackageManager> manager = openManagerListing({repositoryEntry(qsl("a-first-download"), qsl("a-first-download.mpackage")),
                                                                  repositoryEntry(qsl("b-second-download"), qsl("b-second-download.mpackage")),
                                                                  repositoryEntry(qsl("c-nameless-refusal"), QString())});
        QVERIFY2(selectPackages(manager, {qsl("a-first-download"), qsl("b-second-download"), qsl("c-nameless-refusal")}), "The packages under test were not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(settle() && dismisser.seen().size() == 3, 30s);

        QVERIFY2(dismisser.sawStacking(), "No two warnings were ever open at once, so nothing finished inside the refusal and the case under test never happened");
        QVERIFY2(!dismisser.progressGoneWhileBoxUp(), "The batch was wound up while the loop was still stopped inside the refusal's warning");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // A refusal's warning box runs an event loop, and a script installing or
    // removing a package inside it rebuilds the list the selection was taken
    // from, freeing its items. The selections after the box still have to be
    // the packages the user picked.
    void test_aListRebuiltInsideARefusalStillDownloadsTheRestOfTheSelection()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;
        // Holds the lone refusal box up long enough for the rebuild below to
        // happen inside it; nothing stacks on it, so the cap is what ends it
        dismisser.setWaitForStacking(1s);

        QPointer<dlgPackageManager> manager =
                openManagerListing({repositoryEntry(qsl("a-nameless-refusal"), QString()), repositoryEntry(qsl("b-after-the-rebuild"), qsl("b-after-the-rebuild.mpackage"))});
        QVERIFY2(selectPackages(manager, {qsl("a-nameless-refusal"), qsl("b-after-the-rebuild")}), "The packages under test were not listed in the Explore view");

        bool rebuiltInsideTheBox = false;
        QTimer rebuilder;
        connect(&rebuilder, &QTimer::timeout, this, [this, &rebuiltInsideTheBox]() {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!rebuiltInsideTheBox && box && box->text().contains(qsl("a-nameless-refusal"))) {
                rebuiltInsideTheBox = true;
                emit mpHost->signal_packageListChanged();
            }
        });
        rebuilder.start(5ms);

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(settle() && dismisser.seen().size() == 2, 30s);
        QVERIFY2(rebuiltInsideTheBox, "The package list was never rebuilt while the refusal was up, so the case under test never happened");
        QVERIFY2(dismisser.seen().at(1).contains(qsl("b-after-the-rebuild")), "The selection after the rebuild was not the package that was picked");

        QTRY_VERIFY_WITH_TIMEOUT(settle() && !downloadDialogStillUp(manager), 15s);

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // A selection the index cannot answer for sends the package manager to fetch
    // a fresh index. When that fetch fails the index already on disk is still
    // the best there is, and it is what the Explore view is filled from next time.
    void test_aFailedIndexRefreshKeepsTheCachedIndex_10217()
    {
        mpProxy->setAnswer(StubProxy::Answer::Refuse);
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager =
                openManagerListing({repositoryEntry(qsl("a-unanswerable-package"), QString()), repositoryEntry(qsl("b-cached-package"), qsl("b-cached-package.mpackage"))});
        const QByteArray cachedIndex = readRepositoryIndex();
        QVERIFY2(!cachedIndex.isEmpty(), "The repository index was not seeded");
        QVERIFY2(selectPackages(manager, {qsl("a-unanswerable-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QVERIFY2(dismisser.seen().constFirst().contains(qsl("a-unanswerable-package")), "The reported failure was not the one this test arranged");
        // the refresh's network manager is disposed of by the handler that ends it
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager->findChildren<QNetworkAccessManager*>().isEmpty(), 15s);

        QCOMPARE(readRepositoryIndex(), cachedIndex);
        const QStringList leftovers = QDir(profileHome()).entryList({qsl("mpkg.packages.json?*")}, QDir::Files | QDir::Hidden);
        QVERIFY2(leftovers.isEmpty(), qPrintable(qsl("The failed refresh left files behind: %1").arg(leftovers.join(qsl(", ")))));

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    void test_aSuccessfulIndexRefreshReplacesTheCachedIndex_10217()
    {
        if (!QSslSocket::supportsSsl()) {
            QSKIP("no TLS backend to answer the https refresh with");
        }
        QJsonObject refreshed;
        refreshed.insert(qsl("packages"), QJsonArray{repositoryEntry(qsl("a-unanswerable-package"), QString()), repositoryEntry(qsl("c-refreshed-package"), qsl("c-refreshed-package.mpackage"))});
        const QByteArray refreshedIndex = QJsonDocument(refreshed).toJson();
        mpProxy->serve(refreshedIndex);
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager =
                openManagerListing({repositoryEntry(qsl("a-unanswerable-package"), QString()), repositoryEntry(qsl("b-cached-package"), qsl("b-cached-package.mpackage"))});
        QVERIFY2(readRepositoryIndex() != refreshedIndex, "The seeded index is already the one the refresh serves");
        QVERIFY2(selectPackages(manager, {qsl("a-unanswerable-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager->findChildren<QNetworkAccessManager*>().isEmpty(), 15s);

        QCOMPARE(readRepositoryIndex(), refreshedIndex);
        const QStringList leftovers = QDir(profileHome()).entryList({qsl("mpkg.packages.json?*")}, QDir::Files | QDir::Hidden);
        QVERIFY2(leftovers.isEmpty(), qPrintable(qsl("The refresh left files behind: %1").arg(leftovers.join(qsl(", ")))));

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }

    // A captive portal or a proxy can answer the refresh with a page of its own and a success status
    void test_aRefreshAnsweredWithSomethingOtherThanAnIndexKeepsTheCachedIndex_10217()
    {
        if (!QSslSocket::supportsSsl()) {
            QSKIP("no TLS backend to answer the https refresh with");
        }
        mpProxy->serve(QByteArrayLiteral("<html><body>Sign in to the network to continue</body></html>"));
        MessageBoxDismisser dismisser;

        QPointer<dlgPackageManager> manager =
                openManagerListing({repositoryEntry(qsl("a-unanswerable-package"), QString()), repositoryEntry(qsl("b-cached-package"), qsl("b-cached-package.mpackage"))});
        const QByteArray cachedIndex = readRepositoryIndex();
        QVERIFY2(!cachedIndex.isEmpty(), "The repository index was not seeded");
        QVERIFY2(selectPackages(manager, {qsl("a-unanswerable-package")}), "The package under test was not listed in the Explore view");

        QVERIFY(QMetaObject::invokeMethod(manager, "slot_installPackageFromRepository"));

        QTRY_VERIFY_WITH_TIMEOUT(!dismisser.seen().isEmpty(), 15s);
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager->findChildren<QNetworkAccessManager*>().isEmpty(), 15s);

        QCOMPARE(readRepositoryIndex(), cachedIndex);
        const QStringList leftovers = QDir(profileHome()).entryList({qsl("mpkg.packages.json?*")}, QDir::Files | QDir::Hidden);
        QVERIFY2(leftovers.isEmpty(), qPrintable(qsl("The refused refresh left files behind: %1").arg(leftovers.join(qsl(", ")))));

        manager->close();
        QTRY_VERIFY_WITH_TIMEOUT(settle() && manager.isNull(), 15s);
    }
};

#include "PackageRepositoryDownloadTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageRepositoryDownloadTest)

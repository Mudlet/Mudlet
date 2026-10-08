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

#include <QFontDatabase>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// GlyphCacheTest proves TGlyphCache draws what drawText() would; this proves
// TTextEdit hands it, and the decorated drawText() path, the font each cell's
// attributes call for - so a bold cell really is drawn bold.
class ConsoleTextStyleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-ConsoleTextStyle";
    QString mPort;
    const QString mLocalhost = "localhost";

    struct StyledLine
    {
        QString name;
        QString luaBefore;
        QString text;
        bool bold = false;
        bool italic = false;
        bool underline = false;
    };

    static QList<StyledLine> styledLines()
    {
        return {
                {qsl("plain"), QString(), qsl("plain Wgjq_ fi"), false, false, false},
                {qsl("bold"), qsl("setBold(true)"), qsl("bold Wgjq_ fi"), true, false, false},
                {qsl("italic"), qsl("setItalics(true)"), qsl("italic Wgjq_ fi"), false, true, false},
                {qsl("bold italic"), qsl("setBold(true) setItalics(true)"), qsl("bolditalic Wgjq_ fi"), true, true, false},
                {qsl("underlined"), qsl("setUnderline(true)"), qsl("underlined Wgjq_ fi"), false, false, true},
                {qsl("underlined bold"), qsl("setUnderline(true) setBold(true)"), qsl("underbold Wgjq_ fi"), true, false, true},
        };
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
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteDirectory(MudletApp::getMudletPath(enums::profileHomePath, mHostname));
    }

    void eachCellIsDrawnInItsOwnStyle()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;

        QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont).family(), 13);
        font.setFixedPitch(true);
        QVERIFY(host->setDisplayFont(font).first);
        QApplication::processEvents();

        // A blank line between the styled ones keeps each row clear of the ink
        // the row above lets overflow into it.
        QString script = qsl("clearWindow() echo('\\n')");
        for (const StyledLine& line : styledLines()) {
            script += qsl(" resetFormat() %1 echo('%2') resetFormat() echo('\\n\\n')").arg(line.luaBefore, line.text);
        }
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "the Lua that prints the styled lines failed");
        pane->forceUpdate();
        QApplication::processEvents();

        QImage rendered(pane->size(), QImage::Format_ARGB32_Premultiplied);
        rendered.fill(host->mpConsole->getConsoleBgColor());
        pane->render(&rendered, QPoint(), QRegion(), QWidget::DrawChildren);

        const QFontMetrics metrics(pane->font());
        const int cellWidth = metrics.averageCharWidth();
        const int cellHeight = metrics.height();
        TBuffer& buffer = host->mpConsole->buffer;
        for (const StyledLine& line : styledLines()) {
            int index = -1;
            for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
                if (buffer.line(i) == line.text) {
                    index = i;
                    break;
                }
            }
            QVERIFY2(index >= 0, qPrintable(qsl("the %1 line is not in the buffer").arg(line.name)));
            const int top = (index - pane->imageTopLine()) * cellHeight;
            QVERIFY2(top >= 0 && top + cellHeight <= rendered.height(), qPrintable(qsl("the %1 line is not on screen").arg(line.name)));
            const TChar& style = buffer.buffer.at(index).at(0);
            QCOMPARE(style.isBold(), line.bold);

            QFont expectedFont = pane->font();
            expectedFont.setBold(line.bold);
            expectedFont.setItalic(line.italic);
            expectedFont.setUnderline(line.underline);
            QImage expected(line.text.size() * cellWidth, cellHeight, QImage::Format_ARGB32_Premultiplied);
            expected.fill(host->mpConsole->getConsoleBgColor());
            {
                QPainter painter(&expected);
                painter.setFont(expectedFont);
                painter.setPen(style.foreground());
                for (int column = 0; column < line.text.size(); ++column) {
                    painter.drawText(QRect(column * cellWidth, 0, cellWidth, cellHeight), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, line.text.mid(column, 1));
                }
            }
            const QImage actual = rendered.copy(0, top, expected.width(), cellHeight).convertToFormat(expected.format());
            QVERIFY2(actual == expected, qPrintable(qsl("the %1 line is not drawn in its own style").arg(line.name)));
        }
    }

    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

private:
    Host* startOfflineProfile()
    {
        if (!TestProfile::create(mHostname, mLocalhost, mPort)) {
            return nullptr;
        }
        QSignalSpy connected(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
            return nullptr;
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        mudlet::self()->resize(1400, 900);
        QApplication::processEvents();
        // Printing into a live connection would race with the stub's traffic
        host->mTelnet.disconnectIt();
        QTest::qWaitFor(
                [host]() {
                    return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                },
                5s);
        return host;
    }

    static void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "ConsoleTextStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleTextStyleTest)

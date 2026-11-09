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
 * A notice's picture is tinted through its alpha channel, so a source drawn in colour
 * over solid alpha tints to a filled disc. The middle row tells a ring, transparent
 * between its two sides, from a disc.
 *
 * Run with: ctest -R EditorNoticeGlyphTest -V
 */

#include <QImage>
#include <QLabel>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "dlgSystemMessageArea.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorNoticeGlyphTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorNoticeGlyph-Test-Profile");

    // Anything above this is ink rather than the antialiased edge of it
    static constexpr int scmInkAlpha = 128;

    static QString describeMiddleRow(const QImage& glyph)
    {
        QString marks;
        const int middle = glyph.height() / 2;
        for (int x = 0; x < glyph.width(); ++x) {
            marks.append(glyph.pixelColor(x, middle).alpha() > scmInkAlpha ? QLatin1Char('#') : QLatin1Char('.'));
        }
        return marks;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1000, 800);
        QTest::qWait(50ms);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_theInformationNoticeIsLedByARingRatherThanADisc()
    {
        mpEditor->slot_showTriggers();
        //: Placeholder notice a test puts up to read the picture beside it
        mpEditor->showInfo(qsl("A notice with a picture beside it"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QLabel* pGlyphLabel = mpEditor->mpSystemMessageArea->notificationAreaIconLabelInformation;
        QVERIFY2(pGlyphLabel->isVisible(), "the information notice is not showing its picture");
        const QImage glyph = pGlyphLabel->pixmap().toImage();
        QVERIFY2(glyph.width() > 8 && glyph.height() > 8, "the picture beside the notice is too small to read a row out of");

        const QString row = describeMiddleRow(glyph);
        qInfo().noquote() << qsl("  the middle row of the notice's picture reads %1").arg(row);

        const qsizetype firstInk = row.indexOf(QLatin1Char('#'));
        const qsizetype lastInk = row.lastIndexOf(QLatin1Char('#'));
        QVERIFY2(firstInk >= 0 && lastInk > firstInk, qPrintable(qsl("the picture beside the notice has no shape in it at all: %1").arg(row)));
        QVERIFY2(row.mid(firstInk, lastInk - firstInk + 1).contains(QLatin1Char('.')),
                 qPrintable(qsl("the picture beside the information notice is a solid disc rather than a ring - tinting kept only its alpha, and that source's alpha is a filled shape: %1").arg(row)));
    }

    void test_theWarningAndErrorNoticesAreLineGlyphsToo()
    {
        mpEditor->slot_showTriggers();
        mpEditor->showWarning(qsl("A warning with a picture beside it"), false);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QString warningRow = describeMiddleRow(mpEditor->mpSystemMessageArea->notificationAreaIconLabelWarning->pixmap().toImage());

        mpEditor->showError(qsl("An error with a picture beside it"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QString errorRow = describeMiddleRow(mpEditor->mpSystemMessageArea->notificationAreaIconLabelError->pixmap().toImage());

        qInfo().noquote() << qsl("  the warning's picture reads %1 and the error's %2").arg(warningRow, errorRow);

        for (const QString& row : {warningRow, errorRow}) {
            const qsizetype firstInk = row.indexOf(QLatin1Char('#'));
            const qsizetype lastInk = row.lastIndexOf(QLatin1Char('#'));
            QVERIFY2(firstInk >= 0 && lastInk > firstInk, qPrintable(qsl("a notice's picture has no shape in it at all: %1").arg(row)));
            QVERIFY2(row.mid(firstInk, lastInk - firstInk + 1).contains(QLatin1Char('.')), qPrintable(qsl("a notice's picture is a filled shape rather than a line glyph: %1").arg(row)));
        }
    }

    void test_theDismissCrossIsInkedFromThePaletteLikeTheNoticeBesideIt()
    {
        mpEditor->slot_showTriggers();
        mpEditor->showInfo(qsl("A notice the reader can dismiss"));
        QCoreApplication::processEvents();
        QTest::qWait(50ms);

        QToolButton* pClose = mpEditor->mpSystemMessageArea->messageAreaCloseButton;
        QVERIFY2(pClose, "the banner has no close button");
        const QImage cross = pClose->icon().pixmap(pClose->iconSize()).toImage();
        QVERIFY2(!cross.isNull(), "the banner's close button carries no picture at all");

        // Tinting keeps only the alpha, so every inked pixel is the one colour
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSet<QRgb> inks;
        for (int y = 0; y < cross.height(); ++y) {
            for (int x = 0; x < cross.width(); ++x) {
                const QColor pixel = cross.pixelColor(x, y);
                if (pixel.alpha() >= scmInkAlpha) {
                    inks.insert(qRgb(pixel.red(), pixel.green(), pixel.blue()));
                }
            }
        }
        QVERIFY2(!inks.isEmpty(), "the banner's close button has no shape in it");
        QCOMPARE(inks.size(), 1);
        QCOMPARE(QColor(*inks.cbegin()).name(), tokens.mutedText.name());
    }
};

#include "EditorNoticeGlyphTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorNoticeGlyphTest)

/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * The settings pages that held the window wide - Mapper, General and Privacy -
 * were grids whose columns took the width of their longest member, so one long
 * German label widened every row and held the page past the reading column.
 * Each row now flows: its pieces keep their own widths and the trailing one
 * drops to the next line only when that row runs out of room. Check boxes that
 * sat side by side are columns that stay whole, side by side while both fit and
 * one under the other when they do not. The two colour cards, the Main
 * display's and the map's, are one grid of label-and-well pairs, two to a row
 * with every well one width, and one to a row where two do not fit.
 *
 * Run with: ctest -R SettingsPageFlowTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QFontInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QTranslator>

#include <tuple>

#include "FlowLayout.h"
#include "MudletApp.h"
#include "PairGridLayout.h"
#include "PixelTestHelper.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "uiDesign.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsPageFlowTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsPageFlow-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // Reports rather than asserting: a QVERIFY here would only return from this
    // helper, leaving the caller reading widgets off a dialog that never opened.
    bool openPreferences(const QString& page = qsl("mapper"))
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        if (!QTest::qWaitForWindowExposed(mpPreferences)) {
            return false;
        }
        mpPreferences->setTab(page);
        qApp->processEvents();
        return true;
    }

    // What a card leaves round a row that spans it, so a test can hand the row a
    // width of its choosing by capping the card
    static int cardChrome(const QWidget* pCard, const QWidget* pRow) { return pCard->width() - pRow->width(); }

    // Centred on one line: an odd difference in height leaves a pixel to one side
    static bool levelWith(const QWidget* pOne, const QWidget* pOther) { return qAbs(pOne->geometry().center().y() - pOther->geometry().center().y()) <= 1; }

    // Puts back what a case's own setGeometry() on the layout moved, before the page is read again
    static void relayout(QLayout* pLayout)
    {
        pLayout->invalidate();
        pLayout->activate();
    }

    // The size and family macOS sets its interface in
    static QFont interfaceFontAt13pt()
    {
        QFont font(qsl(".AppleSystemUIFont"));
        font.setPointSize(13);
        return font;
    }

    // A row on its card: every piece on one line at the width the page gives
    // it, and one pixel short of that the last piece under the first, each
    // piece as wide as its contents. The one piece a row may stretch takes the
    // room left at the end of its line instead, so that line reaches the row's
    // edge. The narrow case is the row's own layout given that width, since a
    // card can be held no narrower than its widest row and this one need not
    // be it. A void helper, so the caller checks QTest::currentTestFailed()
    // after it.
    void verifyRowBreaksAfterItsLastFit(QGroupBox* pCard, const QString& rowName, const QList<QWidget*>& pieces, QWidget* pStretching = nullptr)
    {
        auto* pRow = mpPreferences->findChild<QWidget*>(rowName);
        QVERIFY2(pRow, qPrintable(qsl("there is no row '%1'").arg(rowName)));
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pRow->layout());
        QVERIFY2(pFlow, qPrintable(qsl("the row '%1' is not laid out by a flow").arg(rowName)));
        QCOMPARE(pRow->width(), pCard->contentsRect().width());
        // What the flow wraps by: every piece at its own width
        int oneLine = -pFlow->horizontalSpacing();
        for (QWidget* pPiece : pieces) {
            QVERIFY2(pPiece->parentWidget() == pRow, qPrintable(qsl("'%1' is not on the row '%2'").arg(pPiece->objectName(), rowName)));
            oneLine += pFlow->horizontalSpacing() + pPiece->sizeHint().width();
        }
        // Each piece at its own width, but for the stretching one, which is never narrower than that
        const auto verifyWidths = [&pieces, pStretching]() {
            for (QWidget* pPiece : pieces) {
                if (pPiece == pStretching) {
                    QVERIFY2(pPiece->width() >= pPiece->sizeHint().width(),
                             qPrintable(qsl("'%1' is %2px wide, narrower than the %3px its contents need").arg(pPiece->objectName()).arg(pPiece->width()).arg(pPiece->sizeHint().width())));
                } else {
                    QVERIFY2(pPiece->width() == pPiece->sizeHint().width(),
                             qPrintable(qsl("'%1' is %2px wide rather than the %3px its contents need").arg(pPiece->objectName()).arg(pPiece->width()).arg(pPiece->sizeHint().width())));
                }
            }
        };
        verifyWidths();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(pRow->width() >= oneLine,
                 qPrintable(qsl("the card gives the row '%1' %2px, less than the %3px it needs on one line, so the wide case cannot be shown").arg(rowName).arg(pRow->width()).arg(oneLine)));
        for (int index = 1; index < pieces.size(); ++index) {
            QVERIFY2(levelWith(pieces.at(index), pieces.first()) && pieces.at(index)->geometry().left() > pieces.at(index - 1)->geometry().right(),
                     qPrintable(qsl("'%1' is not on the line of the row '%2' after the piece before it").arg(pieces.at(index)->objectName(), rowName)));
        }
        if (pStretching) {
            QVERIFY2(pieces.last()->geometry().right() == pRow->width() - 1,
                     qPrintable(qsl("'%1' left %2px of its line empty").arg(pStretching->objectName()).arg(pRow->width() - 1 - pieces.last()->geometry().right())));
        }

        const int narrow = oneLine - 1;
        pFlow->setGeometry(QRect(0, 0, narrow, pFlow->heightForWidth(narrow)));
        QWidget* pLast = pieces.last();
        for (int index = 0; index + 1 < pieces.size(); ++index) {
            QVERIFY2(pLast->geometry().top() > pieces.at(index)->geometry().bottom(),
                     qPrintable(qsl("one pixel short of one line, '%1' is still beside '%2'").arg(pLast->objectName(), pieces.at(index)->objectName())));
            if (index > 0) {
                QVERIFY2(levelWith(pieces.at(index), pieces.first()), qPrintable(qsl("'%1' left the first line though it still fits on it").arg(pieces.at(index)->objectName())));
            }
        }
        QCOMPARE(pLast->geometry().left(), pieces.first()->geometry().left());
        verifyWidths();
        if (pStretching && pieces.size() > 2) {
            QCOMPARE(pieces.at(pieces.size() - 2)->geometry().right(), narrow - 1);
        }
        relayout(pFlow);
    }

    // Two columns of check boxes, or two check boxes: side by side at exactly
    // the room both need, the second under the first one pixel short of it,
    // and the same on the page once the card is that narrow
    void verifyColumnsStackWhole(QGroupBox* pCard, const QString& rowName, QWidget* pLeading, QWidget* pTrailing)
    {
        auto* pRow = mpPreferences->findChild<QWidget*>(rowName);
        QVERIFY2(pRow, qPrintable(qsl("there is no row '%1'").arg(rowName)));
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pRow->layout());
        QVERIFY2(pFlow, qPrintable(qsl("the row '%1' is not laid out by a flow, so its second column cannot move under the first").arg(rowName)));
        QVERIFY2(pLeading->parentWidget() == pRow && pTrailing->parentWidget() == pRow, qPrintable(qsl("the row '%1' does not hold the two columns as two pieces").arg(rowName)));

        const int both = pLeading->sizeHint().width() + pFlow->horizontalSpacing() + pTrailing->sizeHint().width();
        pFlow->setGeometry(QRect(0, 0, both, pFlow->heightForWidth(both)));
        QVERIFY2(pTrailing->geometry().left() > pLeading->geometry().right(), qPrintable(qsl("given the %1px both columns of '%2' need, the second starts inside the first").arg(both).arg(rowName)));
        QCOMPARE(pTrailing->geometry().top(), pLeading->geometry().top());
        pFlow->setGeometry(QRect(0, 0, both - 1, pFlow->heightForWidth(both - 1)));
        QVERIFY2(pTrailing->geometry().top() > pLeading->geometry().bottom(),
                 qPrintable(qsl("one pixel short of the room both columns of '%1' need, the second is still beside the first").arg(rowName)));
        QCOMPARE(pTrailing->geometry().left(), pLeading->geometry().left());
        relayout(pFlow);
        qApp->processEvents();

        QCOMPARE(pRow->width(), pCard->contentsRect().width());
        const int narrow = cardChrome(pCard, pRow) + both - 1;
        QVERIFY2(
                pCard->minimumSizeHint().width() <= narrow,
                qPrintable(
                        qsl("the card holding '%1' cannot be narrowed below %2px, so the columns cannot be shown one pixel short of side by side").arg(rowName).arg(pCard->minimumSizeHint().width())));
        pCard->setMaximumWidth(narrow);
        qApp->processEvents();
        QCOMPARE(pRow->width(), both - 1);
        QVERIFY2(pTrailing->geometry().top() > pLeading->geometry().bottom(), qPrintable(qsl("a card one pixel too narrow for both columns of '%1' keeps them side by side").arg(rowName)));
        QCOMPARE(pTrailing->geometry().left(), pLeading->geometry().left());
        pCard->setMaximumWidth(QWIDGETSIZE_MAX);
        qApp->processEvents();
    }

    // However far the window is narrowed, the page fits beside the sidebar or
    // its rail; walked from the widest the window goes to the narrowest
    void verifyPageNeverScrollsSideways(const QString& page)
    {
        QVERIFY2(openPreferences(page), "the settings dialog never appeared");
        QScrollArea* pPage = TestSettings::pageOf(mpPreferences, page);
        QVERIFY2(pPage && pPage->widget(), qPrintable(qsl("there is no '%1' page").arg(page)));
        mpPreferences->resize(mpPreferences->sidebarWidths().fullyExpanded + 400, 760);
        qApp->processEvents();
        const int widest = mpPreferences->width();
        const int narrowest = mpPreferences->minimumWidth();
        for (int width = widest; width > narrowest - 8; width -= 8) {
            mpPreferences->resize(std::max(width, narrowest), 760);
            qApp->processEvents();
            // Waited on rather than read once: in the one pass in which the sidebar folds, the page has been
            // squeezed and not yet given the room back, and the bar is on show until the next
            QTRY_VERIFY2_WITH_TIMEOUT(!pPage->horizontalScrollBar()->isVisible(), qPrintable(qsl("the '%1' page scrolls sideways in a %2px window").arg(page).arg(mpPreferences->width())), 1000);
            QVERIFY2(pPage->widget()->geometry().right() < pPage->viewport()->width(),
                     qPrintable(qsl("the '%1' page's cards end at %2px in a %3px viewport, in a %4px window")
                                        .arg(page)
                                        .arg(pPage->widget()->geometry().right() + 1)
                                        .arg(pPage->viewport()->width())
                                        .arg(mpPreferences->width())));
        }
    }

    // German at the 13pt macOS reads the interface at: each card, so a failure
    // names the one that is too wide, then the page
    void verifyPageFitsTheReadingColumnInGermanAt13pt(const QString& page)
    {
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so nothing here is long enough to measure");
        QVERIFY(qApp->installTranslator(pGerman));
        const QFont priorFont = QApplication::font();
        QApplication::setFont(interfaceFontAt13pt());
        auto restore = qScopeGuard([pGerman, priorFont]() {
            QApplication::setFont(priorFont);
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        QVERIFY2(openPreferences(page), "the settings dialog never appeared");

        QWidget* pColumn = TestSettings::pageOf(mpPreferences, page)->widget();
        for (auto* pCard : pColumn->findChildren<QGroupBox*>(QString(), Qt::FindDirectChildrenOnly)) {
            QVERIFY2(pCard->minimumSizeHint().width() <= 640,
                     qPrintable(qsl("the German '%1' card on the '%2' page needs %3px at 13pt, past the 640px reading column").arg(pCard->title(), page).arg(pCard->minimumSizeHint().width())));
        }
        QVERIFY2(pColumn->minimumSizeHint().width() <= 640,
                 qPrintable(qsl("the German '%1' page needs %2px at 13pt, past the 640px reading column").arg(page).arg(pColumn->minimumSizeHint().width())));
    }

    // Both states of a colour grid: every well one width, the leading wells
    // down one edge and the trailing down another, two pairs to a row; or one
    // pair to a row, in order, every well down the one edge
    void verifyColourGrid(const QList<std::pair<QLabel*, QPushButton*>>& pairs, const bool twoPairs)
    {
        const int width = pairs.first().second->width();
        for (int index = 0; index < pairs.size(); ++index) {
            const auto& [pLabel, pWell] = pairs.at(index);
            QVERIFY2(pWell->width() == width, qPrintable(qsl("the well '%1' is %2px wide where the first is %3px").arg(pWell->objectName()).arg(pWell->width()).arg(width)));
            QVERIFY2(levelWith(pWell, pLabel) && pWell->geometry().left() > pLabel->geometry().right(), qPrintable(qsl("'%1' is not level with the well after it").arg(pLabel->objectName())));
            QPushButton* pAlignedWell = pairs.at(twoPairs ? index % 2 : 0).second;
            QVERIFY2(pWell->geometry().left() == pAlignedWell->geometry().left(),
                     qPrintable(qsl("the well '%1' starts at %2px, out of line with '%3' at %4px")
                                        .arg(pWell->objectName())
                                        .arg(pWell->geometry().left())
                                        .arg(pAlignedWell->objectName())
                                        .arg(pAlignedWell->geometry().left())));
            if (index == 0) {
                continue;
            }
            QPushButton* pBefore = pairs.at(index - 1).second;
            if (twoPairs && index % 2) {
                QVERIFY2(levelWith(pWell, pBefore) && pLabel->geometry().left() > pBefore->geometry().right(),
                         qPrintable(qsl("two pairs to a row, '%1' is not beside '%2'").arg(pWell->objectName(), pBefore->objectName())));
            } else {
                QVERIFY2(pWell->geometry().top() > pBefore->geometry().bottom(), qPrintable(qsl("'%1' does not start a row under '%2'").arg(pWell->objectName(), pBefore->objectName())));
            }
        }
    }

    // Each well that may hold a see-through colour as tall and as wide as the
    // plain one beside it; inside it a square swatch with the field showing
    // round it by the well's 3px padding, the colour over a checkerboard - both
    // the board's black and its white showing through a translucent one - under
    // a hairline in the border token, and then the field again, carrying the
    // words saying how opaque the colour is
    void verifyAlphaWells(const QList<std::tuple<QPushButton*, QPushButton*, QColor>>& wells)
    {
        QGroupBox* pCard = mpPreferences->groupBox_mapperColors;
        const QImage card = pCard->grab().toImage();
        const qreal ratio = card.devicePixelRatio();
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const auto pixelAt = [&card, ratio](const QPoint& point) {
            return card.pixelColor((QPointF(point) * ratio).toPoint());
        };
        constexpr int inset = 3;
        constexpr int trailingPadding = 6;
        QList<int> wordsEnd;
        QList<int> wordsStart;
        for (const auto& [pAlpha, pPlain, colour] : wells) {
            QVERIFY2(pAlpha->height() == pPlain->height() && pAlpha->width() == pPlain->width(),
                     qPrintable(qsl("the see-through well '%1' is %2x%3 beside the plain '%4' at %5x%6")
                                        .arg(pAlpha->objectName())
                                        .arg(pAlpha->width())
                                        .arg(pAlpha->height())
                                        .arg(pPlain->objectName())
                                        .arg(pPlain->width())
                                        .arg(pPlain->height())));
            // ...in a fixed-pitch face, so the digits are one width, a step smaller than the words beside the well
            QVERIFY2(QFontInfo(pAlpha->font()).fixedPitch(),
                     qPrintable(qsl("the words in '%1' are set in %2, which is not a fixed-pitch face").arg(pAlpha->objectName(), QFontInfo(pAlpha->font()).family())));
            QCOMPARE(pAlpha->font().pointSize(), uiDesign::typeSize(uiDesign::TypeStep::Caption));
            QVERIFY2(pAlpha->font().pointSizeF() < pPlain->font().pointSizeF(),
                     qPrintable(qsl("the words in '%1' are %2pt, no smaller than the %3pt beside them").arg(pAlpha->objectName()).arg(pAlpha->font().pointSizeF()).arg(pPlain->font().pointSizeF())));
            // Never narrower than the widest words need, whichever these are
            QCOMPARE(pAlpha->sizeHint().width(), std::get<0>(wells.first())->sizeHint().width());

            const auto over = [&colour](const int ground) {
                const qreal alpha = colour.alphaF();
                return QColor(qRound(colour.red() * alpha + ground * (1 - alpha)), qRound(colour.green() * alpha + ground * (1 - alpha)), qRound(colour.blue() * alpha + ground * (1 - alpha)));
            };
            const QColor overBlack = over(0);
            const QColor overWhite = over(255);
            const auto onTheBoard = [&](const QPoint& point) {
                return TestPixels::readsAs(pixelAt(point), overBlack, 3) || TestPixels::readsAs(pixelAt(point), overWhite, 3);
            };
            const auto verifyIs = [&](const QPoint& point, const QColor& wanted, const QString& where) {
                QVERIFY2(TestPixels::readsAs(pixelAt(point), wanted, 3), qPrintable(qsl("%1 in '%2' is %3 rather than %4").arg(where, pAlpha->objectName(), pixelAt(point).name(), wanted.name())));
            };
            const QPoint origin = pAlpha->mapTo(pCard, QPoint(0, 0));
            // Inside the well's hairline, and the swatch inset in that
            const QRect inside(origin.x() + 1, origin.y() + 1, pAlpha->width() - 2, pAlpha->height() - 2);
            const QRect swatch(inside.left() + inset, inside.top() + inset, inside.height() - 2 * inset, inside.height() - 2 * inset);
            const QPoint centre = swatch.center();
            QVERIFY2(onTheBoard(centre), qPrintable(qsl("the middle of the swatch in '%1' is %2, not the colour").arg(pAlpha->objectName(), pixelAt(centre).name())));
            // Square: the colour runs to just inside the swatch's far edge, which carries its hairline
            QVERIFY2(onTheBoard(QPoint(swatch.right() - 1, centre.y())), qPrintable(qsl("the swatch in '%1' ends before it is as wide as it is tall").arg(pAlpha->objectName())));
            verifyIs(QPoint(swatch.left(), centre.y()), tokens.border, qsl("the swatch's leading edge"));
            verifyIs(QPoint(swatch.right(), centre.y()), tokens.border, qsl("the swatch's trailing edge"));
            verifyIs(QPoint(centre.x(), swatch.top()), tokens.border, qsl("the swatch's top edge"));
            // The words end at the trailing padding: the last ink across the well's middle stands there
            int inkEnd = -1;
            for (int x = inside.right() - 1; x > swatch.right() && inkEnd < 0; --x) {
                for (int y = inside.top() + inset; y <= inside.bottom() - inset; ++y) {
                    if (!TestPixels::readsAs(pixelAt(QPoint(x, y)), tokens.field, 3)) {
                        inkEnd = x;
                        break;
                    }
                }
            }
            const int contentEnd = inside.right() - trailingPadding;
            // A last glyph's antialiased edge can stand a pixel past its advance, as "%" does in Windows' fixed-pitch faces
            QVERIFY2(inkEnd <= contentEnd + 1 && inkEnd >= contentEnd - 3,
                     qPrintable(qsl("the words in '%1' end at %2px, not at the trailing padding at %3px").arg(pAlpha->objectName()).arg(inkEnd - inside.left()).arg(contentEnd - inside.left())));
            wordsEnd.append(inkEnd);
            // ...and where they start, the first ink past the field after the swatch
            int inkStart = -1;
            for (int x = swatch.right() + 2; x < inside.right() && inkStart < 0; ++x) {
                for (int y = inside.top() + inset; y <= inside.bottom() - inset; ++y) {
                    if (!TestPixels::readsAs(pixelAt(QPoint(x, y)), tokens.field, 3)) {
                        inkStart = x;
                        break;
                    }
                }
            }
            wordsStart.append(inkStart);
            // ...and the field all round it
            verifyIs(QPoint(swatch.right() + 2, centre.y()), tokens.field, qsl("just past the swatch"));
            verifyIs(QPoint(centre.x(), inside.top() + 1), tokens.field, qsl("the margin above the swatch"));
            verifyIs(QPoint(centre.x(), inside.bottom() - 1), tokens.field, qsl("the margin below the swatch"));
            verifyIs(QPoint(inside.left() + 1, centre.y()), tokens.field, qsl("the margin before the swatch"));
            if (QTest::currentTestFailed() || colour.alpha() == 255) {
                continue;
            }
            // The board shows through: both of its inks inside the hairline
            bool sawBlack = false;
            bool sawWhite = false;
            for (int y = swatch.top() + 1; y < swatch.bottom(); ++y) {
                for (int x = swatch.left() + 1; x < swatch.right(); ++x) {
                    sawBlack |= TestPixels::readsAs(pixelAt(QPoint(x, y)), overBlack, 3);
                    sawWhite |= TestPixels::readsAs(pixelAt(QPoint(x, y)), overWhite, 3);
                }
            }
            QVERIFY2(sawBlack && sawWhite,
                     qPrintable(qsl("the swatch in '%1' does not show the checkerboard through its translucent colour: over black %2, over white %3")
                                        .arg(pAlpha->objectName())
                                        .arg(sawBlack)
                                        .arg(sawWhite)));
        }
        // ...so down a column of wells both the word and the percent sign stand under the one above
        for (int index = 1; index < wordsEnd.size(); ++index) {
            const QPushButton* pWell = std::get<0>(wells.at(index));
            const QPushButton* pFirst = std::get<0>(wells.first());
            if (pWell->mapTo(pCard, QPoint(0, 0)).x() != pFirst->mapTo(pCard, QPoint(0, 0)).x()) {
                continue;
            }
            QVERIFY2(qAbs(wordsEnd.at(index) - wordsEnd.first()) <= 1,
                     qPrintable(qsl("the words in '%1' end at %2px and in '%3' at %4px").arg(pWell->objectName()).arg(wordsEnd.at(index)).arg(pFirst->objectName()).arg(wordsEnd.first())));
            QVERIFY2(qAbs(wordsStart.at(index) - wordsStart.first()) <= 1,
                     qPrintable(qsl("the words in '%1' start at %2px and in '%3' at %4px").arg(pWell->objectName()).arg(wordsStart.at(index)).arg(pFirst->objectName()).arg(wordsStart.first())));
        }
        // ...which they do because the number takes three places, so every well's words are one length
        for (const auto& [pAlpha, pPlain, colour] : wells) {
            QCOMPARE(pAlpha->text(), qsl("Opacity %1%").arg(qRound(colour.alphaF() * 100), 3));
        }
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see SettingsMapControlsTest
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // Each column is as wide as its own longest words, not as the longest of
    // all six, and the second moves under the first whole the moment one pixel
    // is missing - never a check box at a time
    void test_theMapViewCheckBoxesAreTwoColumnsThatStackWhole()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pChoices = mpPreferences->findChild<QWidget*>(qsl("settingsMapViewChoices"));
        auto* pLeading = mpPreferences->findChild<QWidget*>(qsl("settingsMapViewLeadingChoices"));
        auto* pTrailing = mpPreferences->findChild<QWidget*>(qsl("settingsMapViewTrailingChoices"));
        QVERIFY2(pChoices && pLeading && pTrailing, "the Map view card has no row holding two columns of check boxes");
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pChoices->layout());
        QVERIFY2(pFlow, "the two columns are not laid out by a flow, so the second cannot move under the first");

        const QList<QCheckBox*> leadingBoxes{mpPreferences->mMapperUseAntiAlias, mpPreferences->checkBox_drawUpperLowerLevels, mpPreferences->checkBox_invertMapZoom};
        const QList<QCheckBox*> trailingBoxes{mpPreferences->checkbox_mMapperShowRoomBorders, mpPreferences->checkBox_largeAreaExitArrows, mpPreferences->checkBox_showDefaultArea};
        for (const auto& [pGroup, boxes] : {std::pair{pLeading, leadingBoxes}, std::pair{pTrailing, trailingBoxes}}) {
            int widest = 0;
            for (auto* pBox : boxes) {
                QVERIFY2(pBox->parentWidget() == pGroup, qPrintable(qsl("'%1' is not in the '%2' column").arg(pBox->objectName(), pGroup->objectName())));
                if (pBox->isVisibleTo(pGroup)) {
                    widest = std::max(widest, pBox->sizeHint().width());
                }
            }
            QVERIFY2(qAbs(pGroup->width() - widest) <= 2,
                     qPrintable(qsl("the '%1' column is %2px wide for check boxes whose widest is %3px").arg(pGroup->objectName()).arg(pGroup->width()).arg(widest)));
        }

        // Exactly the room both need, and then one pixel less
        const int both = pLeading->sizeHint().width() + pFlow->horizontalSpacing() + pTrailing->sizeHint().width();
        pFlow->setGeometry(QRect(0, 0, both, pFlow->heightForWidth(both)));
        QVERIFY2(pTrailing->geometry().left() > pLeading->geometry().right(),
                 qPrintable(qsl("given the %1px both columns need, the second starts at %2px, inside the first, which ends at %3px")
                                    .arg(both)
                                    .arg(pTrailing->geometry().left())
                                    .arg(pLeading->geometry().right())));
        QCOMPARE(pTrailing->geometry().top(), pLeading->geometry().top());
        pFlow->setGeometry(QRect(0, 0, both - 1, pFlow->heightForWidth(both - 1)));
        QVERIFY2(pTrailing->geometry().top() > pLeading->geometry().bottom(),
                 qPrintable(qsl("one pixel short of the room both columns need, the second starts at %1px, beside the first rather than under it").arg(pTrailing->geometry().top())));
        QCOMPARE(pTrailing->geometry().left(), pLeading->geometry().left());

        // ...and the card gives the row its whole width, so the page decides by
        // the same rule
        relayout(pFlow);
        qApp->processEvents();
        QGroupBox* pCard = mpPreferences->groupBox_mapViewOptions;
        QCOMPARE(pChoices->width(), pCard->contentsRect().width());
        pCard->setMaximumWidth(cardChrome(pCard, pChoices) + both - 1);
        qApp->processEvents();
        QCOMPARE(pChoices->width(), both - 1);
        QVERIFY2(pTrailing->geometry().top() > pLeading->geometry().bottom(), "a card one pixel too narrow for both columns keeps them side by side");
        QCOMPARE(pTrailing->geometry().left(), pLeading->geometry().left());
    }

    // A row keeps every piece at the width its contents need and breaks after
    // the last piece that still fits, rather than squeezing or stretching any
    void test_aMapFileRowBreaksAfterTheLastControlThatFits()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pRow = mpPreferences->findChild<QWidget*>(qsl("settingsMapperRow_loadHistoricMap"));
        QVERIFY2(pRow, "the Map files card has no row for loading an older map");
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pRow->layout());
        QVERIFY2(pFlow, "the older-map row is not laid out by a flow");
        QLabel* pLabel = mpPreferences->label_loadHistoricMap;
        QComboBox* pCombo = mpPreferences->comboBox_mapHistory;
        QPushButton* pButton = mpPreferences->pushButton_loadHistoricMap;
        for (QWidget* pPiece : std::initializer_list<QWidget*>{pLabel, pCombo, pButton}) {
            QVERIFY2(pPiece->parentWidget() == pRow, qPrintable(qsl("'%1' is not on the older-map row").arg(pPiece->objectName())));
        }

        const int gap = pFlow->horizontalSpacing();
        const int oneLine = pLabel->width() + gap + pCombo->width() + gap + pButton->width();
        QVERIFY2(pRow->width() >= oneLine, qPrintable(qsl("the card gives the row %1px, less than the %2px it needs on one line, so the wide case cannot be shown").arg(pRow->width()).arg(oneLine)));
        QVERIFY2(levelWith(pCombo, pLabel) && levelWith(pButton, pLabel), "the older-map row's label, list and button are not level on one line");
        QVERIFY(pCombo->geometry().left() > pLabel->geometry().right());
        QVERIFY(pButton->geometry().left() > pCombo->geometry().right());
        const int comboWidth = pCombo->width();
        QCOMPARE(comboWidth, pCombo->sizeHint().width());
        QCOMPARE(mpPreferences->comboBox_mapFileSaveFormatVersion->width(), mpPreferences->comboBox_mapFileSaveFormatVersion->sizeHint().width());

        QGroupBox* pCard = mpPreferences->groupBox_mapFiles;
        const int narrow = cardChrome(pCard, pRow) + oneLine - 1;
        QVERIFY2(pCard->minimumSizeHint().width() <= narrow,
                 qPrintable(qsl("the Map files card cannot be narrowed below %1px, so the row cannot be shown one pixel short of one line").arg(pCard->minimumSizeHint().width())));
        pCard->setMaximumWidth(narrow);
        qApp->processEvents();
        QCOMPARE(pRow->width(), oneLine - 1);
        QVERIFY2(levelWith(pCombo, pLabel), "the list left the line of the label it belongs to");
        QVERIFY2(pButton->geometry().top() > pCombo->geometry().bottom(),
                 qPrintable(qsl("one pixel short of one line, the load button is at %1px, beside the list rather than under it").arg(pButton->geometry().top())));
        QCOMPARE(pButton->geometry().left(), pLabel->geometry().left());
        QCOMPARE(pCombo->width(), comboWidth);
    }

    // The player room marker's style is a row like the Map files rows: its
    // list beside its label while both fit, and under it once they do not
    void test_thePlayerRoomStyleListDropsUnderItsLabel()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pRow = mpPreferences->findChild<QWidget*>(qsl("settingsMapperRow_playerRoomStyle"));
        QVERIFY2(pRow, "the Player room marker card has no row for the marker style");
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pRow->layout());
        QVERIFY2(pFlow, "the marker style row is not laid out by a flow");
        QLabel* pLabel = mpPreferences->label_playerRoomStyle;
        QComboBox* pCombo = mpPreferences->comboBox_playerRoomStyle;
        QVERIFY2(pLabel->parentWidget() == pRow && pCombo->parentWidget() == pRow, "the marker style's label and list are not two pieces of the one row");

        const int oneLine = pLabel->width() + pFlow->horizontalSpacing() + pCombo->width();
        QVERIFY2(pRow->width() >= oneLine, qPrintable(qsl("the card gives the row %1px, less than the %2px it needs on one line, so the wide case cannot be shown").arg(pRow->width()).arg(oneLine)));
        QVERIFY2(levelWith(pCombo, pLabel), "the marker style's list is not level with its label");
        QVERIFY(pCombo->geometry().left() > pLabel->geometry().right());
        const int comboWidth = pCombo->width();

        QGroupBox* pCard = mpPreferences->groupBox_playerRoomStyle;
        const int narrow = cardChrome(pCard, pRow) + oneLine - 1;
        QVERIFY2(pCard->minimumSizeHint().width() <= narrow,
                 qPrintable(qsl("the Player room marker card cannot be narrowed below %1px, so the row cannot be shown one pixel short of one line").arg(pCard->minimumSizeHint().width())));
        pCard->setMaximumWidth(narrow);
        qApp->processEvents();
        QCOMPARE(pRow->width(), oneLine - 1);
        QVERIFY2(pCombo->geometry().top() > pLabel->geometry().bottom(),
                 qPrintable(qsl("one pixel short of one line, the marker style list is at %1px, beside its label rather than under it").arg(pCombo->geometry().top())));
        QCOMPARE(pCombo->geometry().left(), pLabel->geometry().left());
        QCOMPARE(pCombo->width(), comboWidth);
    }

    // Each size is read with its own stepper, so the two stay on one line and a
    // pair moves on whole; as many pairs as fit share a line
    void test_eachFeatureSizeStaysWithItsStepper()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pSizes = mpPreferences->findChild<QWidget*>(qsl("settingsMapperFeatureSizes"));
        QVERIFY2(pSizes, "the Feature sizes group has no row of sizes");
        auto* pFlow = dynamic_cast<uiDesign::FlowLayout*>(pSizes->layout());
        QVERIFY2(pFlow && pFlow->count() == 4, "the four feature sizes are not four pieces of one flow");

        const QList<std::pair<QWidget*, QWidget*>> pairs{{mpPreferences->label_roomSize, mpPreferences->spinBox_roomSize},
                                                         {mpPreferences->label_exitSize, mpPreferences->spinBox_exitSize},
                                                         {mpPreferences->label_borderSize, mpPreferences->spinBox_borderSize},
                                                         {mpPreferences->label_gridSize, mpPreferences->doubleSpinBox_gridSize}};
        QList<QWidget*> pieces;
        for (const auto& [pLabel, pStepper] : pairs) {
            QWidget* pPiece = pLabel->parentWidget();
            QVERIFY2(pStepper->parentWidget() == pPiece && pPiece->parentWidget() == pSizes,
                     qPrintable(qsl("'%1' and '%2' are not one piece of the sizes row").arg(pLabel->objectName(), pStepper->objectName())));
            pieces.append(pPiece);
        }

        // Room for the first two pairs and not the third
        const int two = pieces.at(0)->sizeHint().width() + pFlow->horizontalSpacing() + pieces.at(1)->sizeHint().width();
        pFlow->setGeometry(QRect(0, 0, two, pFlow->heightForWidth(two)));
        QCOMPARE(pieces.at(1)->geometry().top(), pieces.at(0)->geometry().top());
        QVERIFY2(pieces.at(2)->geometry().top() > pieces.at(0)->geometry().bottom(), "a third pair shares a line that only has room for two");
        QCOMPARE(pieces.at(2)->geometry().left(), pieces.at(0)->geometry().left());
        for (const auto& [pLabel, pStepper] : pairs) {
            QVERIFY2(levelWith(pStepper, pLabel), qPrintable(qsl("'%1' is not level with its stepper").arg(pLabel->objectName())));
            QVERIFY(pStepper->geometry().left() > pLabel->geometry().right());
        }
    }

    // The Main display's colours and the map's are one recipe. The map's
    // twelve and sixteen colours, the two kinds of well among them: two pairs
    // to a row with every well the one width where two fit, and one pair to a
    // row one pixel short of that
    void test_theColourCardsAreOneGridThatDropsToOnePairARow()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pMapColours = mpPreferences->findChild<QWidget*>(qsl("settingsMapColours"));
        auto* pDisplayColours = mpPreferences->findChild<QWidget*>(qsl("settingsDisplayColours"));
        QVERIFY2(pMapColours && pDisplayColours, "a colour card has no grid of colours");
        auto* pGrid = dynamic_cast<uiDesign::PairGridLayout*>(pMapColours->layout());
        QVERIFY2(pGrid && dynamic_cast<uiDesign::PairGridLayout*>(pDisplayColours->layout()), "the two colour cards are not laid out by the one grid");

        auto* p = mpPreferences;
        const QList<std::pair<QLabel*, QPushButton*>> colours{{p->label_39, p->pushButton_foreground_color_2},
                                                              {p->label_40, p->pushButton_background_color_2},
                                                              {p->label_4, p->pushButton_roomBorderColor},
                                                              {p->label_mapInfoBg, p->pushButton_mapInfoBg},
                                                              {p->label_lowerLevelColor, p->pushButton_lowerLevelColor},
                                                              {p->label_upperLevelColor, p->pushButton_upperLevelColor},
                                                              {p->label_pushButton_roomCollisionBorderColor, p->pushButton_roomCollisionBorderColor},
                                                              {p->label_mapGridColor, p->pushButton_mapGridColor}};
        const QList<std::pair<QLabel*, QPushButton*>> ansiColours{{p->label_17, p->pushButton_black_2},
                                                                  {p->label_49, p->pushButton_Lblack_2},
                                                                  {p->label_42, p->pushButton_red_2},
                                                                  {p->label_50, p->pushButton_Lred_2},
                                                                  {p->label_43, p->pushButton_green_2},
                                                                  {p->label_51, p->pushButton_Lgreen_2},
                                                                  {p->label_48, p->pushButton_yellow_2},
                                                                  {p->label_52, p->pushButton_Lyellow_2},
                                                                  {p->label_44, p->pushButton_blue_2},
                                                                  {p->label_53, p->pushButton_Lblue_2},
                                                                  {p->label_45, p->pushButton_magenta_2},
                                                                  {p->label_54, p->pushButton_Lmagenta_2},
                                                                  {p->label_46, p->pushButton_cyan_2},
                                                                  {p->label_55, p->pushButton_Lcyan_2},
                                                                  {p->label_47, p->pushButton_white_2},
                                                                  {p->label_56, p->pushButton_Lwhite_2}};
        // Three see-through colours, so the board shows through them, and one opaque one
        const QList<std::tuple<QPushButton*, QPushButton*, QColor>> alphaWells{{p->pushButton_mapInfoBg, p->pushButton_roomBorderColor, QColor(200, 40, 80, 160)},
                                                                               {p->pushButton_lowerLevelColor, p->pushButton_foreground_color_2, QColor(40, 90, 200, 204)},
                                                                               {p->pushButton_upperLevelColor, p->pushButton_background_color_2, QColor(30, 160, 60)},
                                                                               {p->pushButton_mapGridColor, p->pushButton_roomCollisionBorderColor, QColor(220, 180, 20, 77)}};
        for (const auto& [pAlpha, pPlain, colour] : alphaWells) {
            p->setButtonColor(pAlpha, colour, true);
        }
        qApp->processEvents();

        const int twoPairs = pGrid->sizeHint().width();
        QVERIFY2(pMapColours->width() >= twoPairs,
                 qPrintable(qsl("the card gives the colours %1px, less than the %2px two pairs to a row need, so the wide case cannot be shown").arg(pMapColours->width()).arg(twoPairs)));
        for (const bool wide : {true, false}) {
            verifyColourGrid(colours, wide);
            verifyColourGrid(ansiColours, wide);
            verifyAlphaWells(alphaWells);
            if (QTest::currentTestFailed()) {
                return;
            }
            QCOMPARE(p->pushButton_black_2->geometry().left(), p->pushButton_foreground_color_2->geometry().left());
            // Reset ends the card on its trailing edge: across the trailing half
            // two pairs to a row, and at its own width one pair to a row
            QPushButton* pReset = p->reset_colors_button_2;
            QCOMPARE(pReset->geometry().right(), pMapColours->width() - 1);
            if (wide) {
                QCOMPARE(pReset->geometry().left(), p->label_40->geometry().left());
                // A pixel narrower, so both parities of what the two controls share are seen
                QGroupBox* pCard = p->groupBox_mapperColors;
                QVERIFY2(pMapColours->width() > twoPairs, "SETUP: the card has no pixel to spare above two pairs to a row");
                pCard->setMaximumWidth(cardChrome(pCard, pMapColours) + pMapColours->width() - 1);
                qApp->processEvents();
                QCOMPARE(pReset->geometry().right(), pMapColours->width() - 1);
                QCOMPARE(pReset->geometry().left(), p->label_40->geometry().left());
            } else {
                QCOMPARE(pReset->width(), pReset->sizeHint().width());
                break;
            }
            QGroupBox* pCard = p->groupBox_mapperColors;
            const int narrow = cardChrome(pCard, pMapColours) + twoPairs - 1;
            QVERIFY2(pCard->minimumSizeHint().width() <= narrow, "the map colours card cannot be narrowed below two pairs to a row");
            pCard->setMaximumWidth(narrow);
            qApp->processEvents();
            QCOMPARE(pMapColours->width(), twoPairs - 1);
        }
    }

    // However far the window is narrowed, the page fits beside the sidebar or
    // its rail; walked from the widest the window goes to the narrowest
    void test_theMapperPageNeverScrollsSideways() { verifyPageNeverScrollsSideways(qsl("mapper")); }

    // The case the flow was made for: German at the 13pt macOS reads the
    // interface at, where the page was 1001px wide
    void test_theMapperPageFitsTheReadingColumnInGermanAt13pt()
    {
#if !defined(Q_OS_MACOS)
        QSKIP("the 13pt interface font this case measures is macOS's own; elsewhere the family falls back to one whose widths say nothing about it");
#endif
        verifyPageFitsTheReadingColumnInGermanAt13pt(qsl("mapper"));
    }

    // The log folder's row: its label, the path field and the two buttons on
    // one line while they fit, the last button under the label when not
    void test_aGeneralRowBreaksAfterTheLastControlThatFits()
    {
        QVERIFY2(openPreferences(qsl("general")), "the settings dialog never appeared");
        verifyRowBreaksAfterItsLastFit(mpPreferences->groupBox_logOptions,
                                       qsl("settingsGeneralRow_logFolder"),
                                       {mpPreferences->label_whereToLog, mpPreferences->lineEdit_logFileFolder, mpPreferences->pushButton_whereToLog, mpPreferences->pushButton_resetLogDir},
                                       mpPreferences->lineEdit_logFileFolder);
    }

    void test_theSavingCheckBoxesStackWhole()
    {
        QVERIFY2(openPreferences(qsl("general")), "the settings dialog never appeared");
        verifyColumnsStackWhole(mpPreferences->groupBox_miscellaneous, qsl("settingsSavingChoices"), mpPreferences->mFORCE_SAVE_ON_EXIT, mpPreferences->mAlertOnNewData);
    }

    void test_theGeneralPageNeverScrollsSideways() { verifyPageNeverScrollsSideways(qsl("general")); }

    // 860px before its rows flowed
    void test_theGeneralPageFitsTheReadingColumnInGermanAt13pt()
    {
#if !defined(Q_OS_MACOS)
        QSKIP("the 13pt interface font this case measures is macOS's own; elsewhere the family falls back to one whose widths say nothing about it");
#endif
        verifyPageFitsTheReadingColumnInGermanAt13pt(qsl("general"));
    }

    // Where passwords are kept: the list beside its label while both fit, and
    // under it once they do not
    void test_aPrivacyRowBreaksAfterTheLastControlThatFits()
    {
        QVERIFY2(openPreferences(qsl("privacy")), "the settings dialog never appeared");
        auto* pCard = mpPreferences->findChild<QGroupBox*>(qsl("card_passwords"));
        QVERIFY2(pCard, "the Privacy page has no Passwords card");
        verifyRowBreaksAfterItsLastFit(pCard, qsl("settingsPrivacyRow_storePasswordsIn"), {mpPreferences->label_store_passwords_in, mpPreferences->comboBox_store_passwords_in});
    }

    // The certificate check boxes: a column of two and a column of one
    void test_theCertificateCheckBoxesAreColumnsThatStackWhole()
    {
        QVERIFY2(openPreferences(qsl("privacy")), "the settings dialog never appeared");
        auto* pLeading = mpPreferences->findChild<QWidget*>(qsl("settingsSslLeadingChoices"));
        auto* pTrailing = mpPreferences->findChild<QWidget*>(qsl("settingsSslTrailingChoices"));
        QVERIFY2(pLeading && pTrailing, "the secure connection card has no two columns of check boxes");
        QVERIFY2(mpPreferences->checkBox_self_signed->parentWidget() == pLeading && mpPreferences->checkBox_ignore_all->parentWidget() == pLeading
                         && mpPreferences->checkBox_expired->parentWidget() == pTrailing,
                 "the certificate check boxes are not in the columns they were in");
        verifyColumnsStackWhole(mpPreferences->groupBox_ssl, qsl("settingsSslChoices"), pLeading, pTrailing);
    }

    void test_thePrivacyPageNeverScrollsSideways() { verifyPageNeverScrollsSideways(qsl("privacy")); }

    // 798px before its rows flowed
    void test_thePrivacyPageFitsTheReadingColumnInGermanAt13pt()
    {
#if !defined(Q_OS_MACOS)
        QSKIP("the 13pt interface font this case measures is macOS's own; elsewhere the family falls back to one whose widths say nothing about it");
#endif
        verifyPageFitsTheReadingColumnInGermanAt13pt(qsl("privacy"));
    }
};

#include "SettingsPageFlowTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsPageFlowTest)

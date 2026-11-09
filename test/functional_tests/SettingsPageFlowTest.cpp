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
 * display's and the map's, are one grid of label-and-row pairs, two to a row
 * with every row one width, and one to a row where two do not fit; under them
 * the console colours are a table with a preview beside it.
 *
 * Run with: ctest -R SettingsPageFlowTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QTranslator>

#include <functional>
#include <tuple>

#include "ColourValue.h"
#include "FlowLayout.h"
#include "MudletApp.h"
#include "PairGridLayout.h"
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

    // The field showing a colour of the two colour cards, by the button that chooses it
    uiDesign::ColourValue* fieldOf(const QPushButton* pChoose) const
    {
        return mpPreferences->findChild<uiDesign::ColourValue*>(qsl("settingsColour_%1").arg(pChoose->objectName().remove(qsl("pushButton_"))));
    }

    // Both states of a colour grid: every row one width, the leading rows
    // down one edge and the trailing down another, two pairs to a row; or one
    // pair to a row, in order, every row down the one edge
    void verifyColourGrid(const QList<std::pair<QLabel*, QWidget*>>& pairs, const bool twoPairs)
    {
        const int width = pairs.first().second->width();
        for (int index = 0; index < pairs.size(); ++index) {
            const auto& [pLabel, pRow] = pairs.at(index);
            QVERIFY2(pRow->width() == width, qPrintable(qsl("the row '%1' is %2px wide where the first is %3px").arg(pRow->objectName()).arg(pRow->width()).arg(width)));
            QVERIFY2(levelWith(pRow, pLabel) && pRow->geometry().left() > pLabel->geometry().right(), qPrintable(qsl("'%1' is not level with the row after it").arg(pLabel->objectName())));
            QWidget* pAlignedRow = pairs.at(twoPairs ? index % 2 : 0).second;
            QVERIFY2(pRow->geometry().left() == pAlignedRow->geometry().left(),
                     qPrintable(qsl("the row '%1' starts at %2px, out of line with '%3' at %4px")
                                        .arg(pRow->objectName())
                                        .arg(pRow->geometry().left())
                                        .arg(pAlignedRow->objectName())
                                        .arg(pAlignedRow->geometry().left())));
            if (index == 0) {
                continue;
            }
            QWidget* pBefore = pairs.at(index - 1).second;
            if (twoPairs && index % 2) {
                QVERIFY2(levelWith(pRow, pBefore) && pLabel->geometry().left() > pBefore->geometry().right(),
                         qPrintable(qsl("two pairs to a row, '%1' is not beside '%2'").arg(pRow->objectName(), pBefore->objectName())));
            } else {
                QVERIFY2(pRow->geometry().top() > pBefore->geometry().bottom(), qPrintable(qsl("'%1' does not start a row under '%2'").arg(pRow->objectName(), pBefore->objectName())));
            }
        }
    }

    // A column of the map's own colours, each label with the row it names, read down
    QList<std::pair<QLabel*, QWidget*>> mapColumn(const bool trailing)
    {
        auto* p = mpPreferences;
        const QList<std::tuple<QLabel*, QPushButton*, bool>> column =
                trailing ? QList<std::tuple<QLabel*, QPushButton*, bool>>{{p->label_4, p->pushButton_roomBorderColor, false},
                                                                          {p->label_pushButton_roomCollisionBorderColor, p->pushButton_roomCollisionBorderColor, false},
                                                                          {p->label_39, p->pushButton_foreground_color_2, false}}
                         : QList<std::tuple<QLabel*, QPushButton*, bool>>{{p->label_40, p->pushButton_background_color_2, true},
                                                                          {p->label_mapInfoBg, p->pushButton_mapInfoBg, true},
                                                                          {p->label_upperLevelColor, p->pushButton_upperLevelColor, true},
                                                                          {p->label_lowerLevelColor, p->pushButton_lowerLevelColor, true},
                                                                          {p->label_mapGridColor, p->pushButton_mapGridColor, true}};
        QList<std::pair<QLabel*, QWidget*>> rows;
        for (const auto& [pLabel, pChoose, seeThrough] : column) {
            verifyColourRow(pChoose, seeThrough);
            if (QTest::currentTestFailed()) {
                return {};
            }
            [&]() {
                QCOMPARE(pLabel->buddy(), fieldOf(pChoose));
            }();
            rows.append({pLabel, fieldOf(pChoose)->parentWidget()});
        }
        return rows;
    }

    // By the rows, which share a parent where the fields in them do not
    bool mapSideBySide() const { return levelWith(fieldOf(mpPreferences->pushButton_roomBorderColor)->parentWidget(), fieldOf(mpPreferences->pushButton_background_color_2)->parentWidget()); }

    bool displayTwoPairsToARow() const { return levelWith(fieldOf(mpPreferences->pushButton_background_color)->parentWidget(), fieldOf(mpPreferences->pushButton_foreground_color)->parentWidget()); }

    // Resizes the window until the widget is that wide, the sidebar folding or not as it will on the way
    bool widthTo(QWidget* pWidget, const int width)
    {
        for (int attempt = 0; attempt < 4 && pWidget->width() != width; ++attempt) {
            mpPreferences->resize(mpPreferences->width() + width - pWidget->width(), 760);
            qApp->processEvents();
            qApp->processEvents();
        }
        return pWidget->width() == width;
    }

    // The map's two columns: side by side, each read down and the trailing
    // one's rows level with the leading one's, the leading one the longer; or
    // one under the other, the leading column whole and then the trailing one,
    // never the two taking turns
    void verifyColourColumns(const QList<std::pair<QLabel*, QWidget*>>& leading, const QList<std::pair<QLabel*, QWidget*>>& trailing, const bool sideBySide)
    {
        QVERIFY2(!trailing.isEmpty() && leading.size() >= trailing.size(), "SETUP: the two columns are not there to compare");
        const QList<std::pair<QLabel*, QWidget*>> all = leading + trailing;
        for (const auto& [pLabel, pRow] : all) {
            QVERIFY2(levelWith(pRow, pLabel) && pRow->geometry().left() > pLabel->geometry().right(), qPrintable(qsl("'%1' is not level with the row after it").arg(pLabel->objectName())));
        }
        if (sideBySide) {
            for (int index = 0; index < leading.size(); ++index) {
                const auto& [pLabel, pRow] = leading.at(index);
                QCOMPARE(pRow->geometry().left(), leading.first().second->geometry().left());
                if (index > 0) {
                    QVERIFY2(pRow->geometry().top() > leading.at(index - 1).second->geometry().bottom(),
                             qPrintable(qsl("'%1' is not under '%2'").arg(pRow->objectName(), leading.at(index - 1).second->objectName())));
                }
                if (index >= trailing.size()) {
                    continue;
                }
                const auto& [pTrailingLabel, pTrailingRow] = trailing.at(index);
                QVERIFY2(levelWith(pTrailingRow, pRow) && pTrailingLabel->geometry().left() > pRow->geometry().right(),
                         qPrintable(qsl("side by side, '%1' is not beside '%2'").arg(pTrailingRow->objectName(), pRow->objectName())));
                // The console colours' Light column stands a pair's gap from Normal's, 24px, and the columns above as far
                const int between = pTrailingLabel->geometry().left() - pRow->geometry().right() - 1;
                QVERIFY2(between >= 24, qPrintable(qsl("side by side, '%1' stands %2px from '%3'").arg(pTrailingLabel->objectName()).arg(between).arg(pRow->objectName())));
                QCOMPARE(pTrailingRow->geometry().left(), trailing.first().second->geometry().left());
            }
            return;
        }
        for (int index = 1; index < all.size(); ++index) {
            const QWidget* pRow = all.at(index).second;
            const QWidget* pAbove = all.at(index - 1).second;
            QVERIFY2(pRow->geometry().left() == all.first().second->geometry().left() && pRow->geometry().top() > pAbove->geometry().bottom(),
                     qPrintable(qsl("one under the other, '%1' does not come straight under '%2'").arg(pRow->objectName(), pAbove->objectName())));
        }
    }

    // A colour's row: its field, then its opacity box where it may be
    // see-through, then the button choosing it, level with one another and at a
    // colour well's height, so the card is no taller than its wells made it
    void verifyColourRow(QPushButton* pChoose, const bool seeThrough)
    {
        uiDesign::ColourValue* pField = fieldOf(pChoose);
        QVERIFY2(pField, qPrintable(qsl("'%1' has no field beside it").arg(pChoose->objectName())));
        QWidget* pRow = pField->parentWidget();
        QCOMPARE(pChoose->parentWidget(), pRow);
        auto* pOpacity = mpPreferences->findChild<QSpinBox*>(qsl("settingsOpacity_%1").arg(pChoose->objectName().remove(qsl("pushButton_"))));
        QVERIFY2(bool(pOpacity) == seeThrough, qPrintable(qsl("'%1' %2 an opacity box").arg(pField->objectName(), seeThrough ? qsl("has no") : qsl("has"))));
        QList<QWidget*> inOrder{pField};
        if (pOpacity) {
            QCOMPARE(pOpacity->parentWidget(), pRow);
            inOrder.append(pOpacity);
        }
        inOrder.append(pChoose);
        for (int index = 0; index < inOrder.size(); ++index) {
            QWidget* pPart = inOrder.at(index);
            QVERIFY2(pPart->height() == uiDesign::scmColourWellHeight,
                     qPrintable(qsl("'%1' is %2px tall, not a colour well's %3px").arg(pPart->objectName()).arg(pPart->height()).arg(uiDesign::scmColourWellHeight)));
            QVERIFY2(levelWith(pPart, pField), qPrintable(qsl("'%1' is not level with '%2'").arg(pPart->objectName(), pField->objectName())));
            if (index > 0) {
                QVERIFY2(pPart->geometry().left() > inOrder.at(index - 1)->geometry().right(),
                         qPrintable(qsl("'%1' does not follow '%2'").arg(pPart->objectName(), inOrder.at(index - 1)->objectName())));
            }
        }
        QCOMPARE(pChoose->width(), pChoose->height());
        QVERIFY2(pChoose->text().isEmpty() && !pChoose->icon().isNull(), qPrintable(qsl("'%1' is not a glyph button").arg(pChoose->objectName())));
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

    // The Main display's colours and the map's are one recipe: each of a
    // card's own colours a label and a row. The Main display's are pairs that
    // belong on a row together, two to a row where they fit and one to a row
    // in the same order where they do not. The map's are two columns read down,
    // the five that may be see-through all in the leading one, so their
    // opacity boxes stand one over the other: side by side where they fit, and
    // where they do not, the trailing column under the leading one whole.
    void test_theColourCardsAreOneGridThatDropsToOnePairARow()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pMapColours = mpPreferences->findChild<QWidget*>(qsl("settingsMapColours"));
        auto* pDisplayColours = mpPreferences->findChild<QWidget*>(qsl("settingsDisplayColours"));
        QVERIFY2(pMapColours && pDisplayColours, "a colour card has no grid of colours");
        auto* pGrid = dynamic_cast<uiDesign::PairGridLayout*>(pMapColours->layout());
        QVERIFY2(pGrid && dynamic_cast<uiDesign::PairGridLayout*>(pDisplayColours->layout()), "the two colour cards are not laid out by the one grid");

        auto* p = mpPreferences;
        const QList<std::pair<QLabel*, QWidget*>> leading = mapColumn(false);
        const QList<std::pair<QLabel*, QWidget*>> trailing = mapColumn(true);
        if (QTest::currentTestFailed()) {
            return;
        }

        const int twoPairs = pGrid->sizeHint().width();
        QVERIFY2(pMapColours->width() >= twoPairs,
                 qPrintable(qsl("the card gives the colours %1px, less than the %2px the two columns side by side need, so the wide case cannot be shown").arg(pMapColours->width()).arg(twoPairs)));
        for (const bool wide : {true, false}) {
            verifyColourColumns(leading, trailing, wide);
            if (QTest::currentTestFailed()) {
                return;
            }
            // ...the see-through ones in the leading column, their opacity boxes one above the other
            const int opacityX = p->findChild<QSpinBox*>(qsl("settingsOpacity_background_color_2"))->mapTo(pMapColours, QPoint(0, 0)).x();
            for (const QString& stem : {qsl("mapInfoBg"), qsl("upperLevelColor"), qsl("lowerLevelColor"), qsl("mapGridColor")}) {
                QCOMPARE(p->findChild<QSpinBox*>(qsl("settingsOpacity_%1").arg(stem))->mapTo(pMapColours, QPoint(0, 0)).x(), opacityX);
            }
            // Reset ends the card on its trailing edge: across the trailing
            // column side by side, and at its own width one under the other
            QPushButton* pReset = p->reset_colors_button_2;
            QCOMPARE(pReset->geometry().right(), pMapColours->width() - 1);
            if (wide) {
                QCOMPARE(pReset->geometry().left(), p->label_4->geometry().left());
                // A pixel narrower, so both parities of what the two columns share are seen
                QGroupBox* pCard = p->groupBox_mapperColors;
                QVERIFY2(pMapColours->width() > twoPairs, "SETUP: the card has no pixel to spare above the two columns side by side");
                pCard->setMaximumWidth(cardChrome(pCard, pMapColours) + pMapColours->width() - 1);
                qApp->processEvents();
                QCOMPARE(pReset->geometry().right(), pMapColours->width() - 1);
                QCOMPARE(pReset->geometry().left(), p->label_4->geometry().left());
            } else {
                QCOMPARE(pReset->width(), pReset->sizeHint().width());
                break;
            }
            QGroupBox* pCard = p->groupBox_mapperColors;
            const int narrow = cardChrome(pCard, pMapColours) + twoPairs - 1;
            QVERIFY2(pCard->minimumSizeHint().width() <= narrow, "the map colours card cannot be narrowed below the two columns side by side");
            pCard->setMaximumWidth(narrow);
            qApp->processEvents();
            QCOMPARE(pMapColours->width(), twoPairs - 1);
        }
        p->groupBox_mapperColors->setMaximumWidth(QWIDGETSIZE_MAX);

        p->setTab(qsl("mainDisplay"));
        qApp->processEvents();
        const QList<std::tuple<QLabel*, QPushButton*, bool>> displayColours{{p->label_foreground_color, p->pushButton_foreground_color, false},
                                                                            {p->label_background_color, p->pushButton_background_color, true},
                                                                            {p->label_command_line_foreground_color, p->pushButton_command_line_foreground_color, false},
                                                                            {p->label_command_line_background_color, p->pushButton_command_line_background_color, false},
                                                                            {p->label_command_foreground_color, p->pushButton_command_foreground_color, false},
                                                                            {p->label_command_background_color, p->pushButton_command_background_color, false}};
        QList<std::pair<QLabel*, QWidget*>> displayRows;
        for (const auto& [pLabel, pChoose, seeThrough] : displayColours) {
            verifyColourRow(pChoose, seeThrough);
            if (QTest::currentTestFailed()) {
                return;
            }
            displayRows.append({pLabel, fieldOf(pChoose)->parentWidget()});
        }
        auto* pDisplayGrid = dynamic_cast<uiDesign::PairGridLayout*>(pDisplayColours->layout());
        const int displayTwoPairs = pDisplayGrid->sizeHint().width();
        verifyColourGrid(displayRows, true);
        QGroupBox* pDisplayCard = p->groupBox_displayColors;
        pDisplayCard->setMaximumWidth(cardChrome(pDisplayCard, pDisplayColours) + displayTwoPairs - 1);
        qApp->processEvents();
        verifyColourGrid(displayRows, false);
        pDisplayCard->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    // Tab goes down the map's leading column and then the trailing one, as they are read
    void test_tabGoesDownTheMapsLeadingColumnFirst()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        QList<QWidget*> fields;
        QStringList expected;
        for (const bool trailing : {false, true}) {
            for (const auto& [pLabel, pRow] : mapColumn(trailing)) {
                fields.append(pLabel->buddy());
                expected.append(pLabel->buddy()->objectName());
            }
        }
        if (QTest::currentTestFailed()) {
            return;
        }
        QStringList reached;
        QWidget* pStop = fields.first();
        for (int step = 0; step < 1000 && reached.size() < fields.size(); ++step, pStop = pStop->nextInFocusChain()) {
            if (fields.contains(pStop)) {
                reached.append(pStop->objectName());
            }
        }
        QCOMPARE(reached, expected);
    }

    // However narrow the window is taken, the map's colours never need more
    // than the card has: walked from the widest the window goes to the
    // narrowest, the narrowest the card's colours are given is too little for
    // the two columns side by side, and there they stand one under the other
    // whole. The numbers are in the message for when the words change.
    void test_atTheNarrowestTheMapColoursAreOneColumnUnderTheOther()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pMapColours = mpPreferences->findChild<QWidget*>(qsl("settingsMapColours"));
        auto* pGrid = dynamic_cast<uiDesign::PairGridLayout*>(pMapColours->layout());
        mpPreferences->resize(mpPreferences->sidebarWidths().fullyExpanded + 400, 760);
        qApp->processEvents();
        int narrowestColours = pMapColours->width();
        int narrowestWindow = mpPreferences->width();
        for (int width = mpPreferences->width(); width > mpPreferences->minimumWidth() - 8; width -= 8) {
            mpPreferences->resize(std::max(width, mpPreferences->minimumWidth()), 760);
            qApp->processEvents();
            qApp->processEvents();
            QVERIFY2(pMapColours->width() >= pGrid->minimumSize().width(),
                     qPrintable(qsl("in a %1px window the map's colours have %2px, less than the %3px one column needs")
                                        .arg(mpPreferences->width())
                                        .arg(pMapColours->width())
                                        .arg(pGrid->minimumSize().width())));
            if (pMapColours->width() < narrowestColours) {
                narrowestColours = pMapColours->width();
                narrowestWindow = mpPreferences->width();
            }
        }
        const int sideBySide = pGrid->sizeHint().width();
        QVERIFY2(narrowestColours < sideBySide,
                 qPrintable(qsl("the two columns side by side need %1px and the narrowest the card gives them is %2px, so they need never wrap").arg(sideBySide).arg(narrowestColours)));
        mpPreferences->resize(narrowestWindow, 760);
        qApp->processEvents();
        qApp->processEvents();
        QCOMPARE(pMapColours->width(), narrowestColours);
        verifyColourColumns(mapColumn(false), mapColumn(true), false);
    }

    // What the colour cards look like follows the width the window gives the
    // page and nothing else: not the page's scroll bar, which a card that wraps
    // can bring up and which then takes width that would keep it wrapped, nor
    // what the window was before. Each case sits a few pixels from a
    // breakpoint, on the side the scroll bar showing would put it.
    void test_theColourCardsLookTheSameWhateverTheScrollBarDoes()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* p = mpPreferences;
        struct Breakpoint
        {
            QString page;
            QWidget* pWidget;
            std::function<int()> needed;
            std::function<bool()> arrangement;
            // In English every one of these fits at the narrowest the window goes, the map's columns because the names
            // fold first, so a label of theirs is given more words, as a translation might, to bring the breakpoint
            // within reach, though not so many that the window is held wider than the breakpoint
            QLabel* pLengthened;
            QString moreWords;
        };
        auto* pMapColours = p->findChild<QWidget*>(qsl("settingsMapColours"));
        auto* pDisplayColours = p->findChild<QWidget*>(qsl("settingsDisplayColours"));
        auto* pMapTable = p->findChild<QWidget*>(qsl("settingsMapConsoleColours"));
        const QList<Breakpoint> breakpoints{{qsl("mapper"),
                                             pMapColours,
                                             [pMapColours]() {
                                                 return pMapColours->layout()->sizeHint().width();
                                             },
                                             [this]() {
                                                 return mapSideBySide();
                                             },
                                             p->label_mapInfoBg,
                                             qsl(" and more")},
                                            {qsl("mainDisplay"),
                                             pDisplayColours,
                                             [pDisplayColours]() {
                                                 return pDisplayColours->layout()->sizeHint().width();
                                             },
                                             [this]() {
                                                 return displayTwoPairsToARow();
                                             },
                                             p->label_command_line_background_color,
                                             qsl(" and more")},
                                            {qsl("mapper"),
                                             pMapTable,
                                             [p, pMapTable]() {
                                                 return p->findChild<QWidget*>(qsl("settingsMapConsoleColoursTable"))->sizeHint().width() + pMapTable->layout()->spacing() + 200;
                                             },
                                             [p]() {
                                                 return !p->findChild<QWidget*>(qsl("settingsMapColoursPreview"))->isHidden();
                                             },
                                             p->label_17,
                                             qsl(" and more and more")}};
        const int hysteresis = dlgProfilePreferences::scmColourArrangementHysteresis;
        for (const Breakpoint& breakpoint : breakpoints) {
            p->setTab(breakpoint.page);
            qApp->processEvents();
            const QString priorWords = breakpoint.pLengthened ? breakpoint.pLengthened->text() : QString();
            if (breakpoint.pLengthened) {
                breakpoint.pLengthened->setText(priorWords + breakpoint.moreWords);
                qApp->processEvents();
            }
            const auto restoreWords = qScopeGuard([&breakpoint, priorWords]() {
                if (breakpoint.pLengthened) {
                    breakpoint.pLengthened->setText(priorWords);
                }
            });
            QScrollArea* pPage = TestSettings::pageOf(p, breakpoint.page);
            // With the bar showing, a few pixels short of the breakpoint and a few past it with the hysteresis
            for (const int fromBreakpoint : {-6, hysteresis + 2}) {
                const int width = breakpoint.needed() + fromBreakpoint;
                // From the other side of the breakpoint, so what the card had before is not what it is to show
                QVERIFY2(widthTo(breakpoint.pWidget, fromBreakpoint < 0 ? width + 20 : width - 20),
                         qPrintable(qsl("SETUP: '%1' could not be taken to %2px on the other side of the breakpoint; it is %3px in a %4px window")
                                            .arg(breakpoint.pWidget->objectName())
                                            .arg(fromBreakpoint < 0 ? width + 20 : width - 20)
                                            .arg(breakpoint.pWidget->width())
                                            .arg(p->width())));
                QVERIFY2(widthTo(breakpoint.pWidget, width), qPrintable(qsl("SETUP: '%1' could not be given %2px").arg(breakpoint.pWidget->objectName()).arg(width)));
                QVERIFY2(pPage->verticalScrollBar()->isVisible(), qPrintable(qsl("SETUP: the '%1' page is not scrolling").arg(breakpoint.page)));
                const bool expected = fromBreakpoint > 0;
                const QString where = qsl("'%1' at %2px from its breakpoint").arg(breakpoint.pWidget->objectName()).arg(fromBreakpoint);
                QVERIFY2(breakpoint.arrangement() == expected, qPrintable(qsl("%1 with the scroll bar showing").arg(where)));

                // The same window, the page taller than its contents or not
                pPage->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                qApp->processEvents();
                qApp->processEvents();
                QVERIFY2(breakpoint.pWidget->width() > width, "SETUP: hiding the scroll bar gave nothing more room");
                QVERIFY2(breakpoint.arrangement() == expected, qPrintable(qsl("%1 changed when the scroll bar went").arg(where)));
                pPage->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                qApp->processEvents();
                qApp->processEvents();
                QVERIFY2(breakpoint.arrangement() == expected, qPrintable(qsl("%1 changed when the scroll bar came back").arg(where)));
                pPage->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                qApp->processEvents();
                breakpoint.pWidget->parentWidget()->layout()->invalidate();
                qApp->processEvents();
                QVERIFY2(breakpoint.arrangement() == expected, qPrintable(qsl("%1 changed when it was laid out again").arg(where)));

                // ...and narrowed as far as the window goes and widened back to it
                const int window = p->width();
                p->resize(p->minimumWidth(), 760);
                qApp->processEvents();
                p->resize(window, 760);
                qApp->processEvents();
                qApp->processEvents();
                QCOMPARE(breakpoint.pWidget->width(), width);
                QVERIFY2(breakpoint.arrangement() == expected, qPrintable(qsl("%1 changed after the window was narrowed and widened back").arg(where)));
            }
        }
    }

    // A window with no size remembered opens wide enough for the map's colours
    // to stand in their two columns beside the sidebar's names, and no wider
    // than the 1060px it ever opened at; one with a size remembered opens at
    // that size, and the columns follow it
    void test_theWindowOpensWithTheMapColoursSideBySide()
    {
        QSettings* pSettings = MudletApp::getQSettings();
        const QVariant priorGeometry = pSettings->value(qsl("profilePreferencesGeometry"));
        const auto restoreGeometry = qScopeGuard([pSettings, priorGeometry]() {
            if (priorGeometry.isValid()) {
                pSettings->setValue(qsl("profilePreferencesGeometry"), priorGeometry);
            } else {
                pSettings->remove(qsl("profilePreferencesGeometry"));
            }
        });
        const auto open = [this]() {
            mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
            mpPreferences->show();
            QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
            mpPreferences->setTab(qsl("mapper"));
            qApp->processEvents();
            qApp->processEvents();
        };

        pSettings->remove(qsl("profilePreferencesGeometry"));
        open();
        const int opened = mpPreferences->width();
        QVERIFY2(opened <= 1060, qPrintable(qsl("the window opened %1px wide, past the 1060px it opens at at most").arg(opened)));
        QVERIFY2(!TestSettings::sidebar(mpPreferences)->property(uiDesign::scmProp_rail).toBool(), qPrintable(qsl("the window opened %1px wide with the sidebar's names folded").arg(opened)));
        QVERIFY2(mapSideBySide(), qPrintable(qsl("the window opened %1px wide with the map's colours one column under the other").arg(opened)));
        verifyColourColumns(mapColumn(false), mapColumn(true), true);
        if (QTest::currentTestFailed()) {
            return;
        }

        // Remembered at the narrowest the window goes, which fits on the small screen an offscreen test has: the
        // window keeps it, and the columns are what that width gives them
        const int remembered = mpPreferences->minimumWidth();
        mpPreferences->resize(remembered, 760);
        qApp->processEvents();
        pSettings->setValue(qsl("profilePreferencesGeometry"), mpPreferences->saveGeometry());
        delete mpPreferences;
        mpPreferences = nullptr;
        open();
        QCOMPARE(mpPreferences->width(), remembered);
        auto* pMapColours = mpPreferences->findChild<QWidget*>(qsl("settingsMapColours"));
        const bool sideBySide = pMapColours->width() >= pMapColours->layout()->sizeHint().width() + dlgProfilePreferences::scmColourArrangementHysteresis;
        verifyColourColumns(mapColumn(false), mapColumn(true), sideBySide);
    }

    // Narrowed a few pixels at a time and widened again, a page with a colour card folds the sidebar's names before
    // the card drops to one pair a row, and each changes once: the other way round, the card stacked and then spread
    // out again as the names went
    void test_theNamesFoldBeforeAColourCardStacks()
    {
        MudletApp::getQSettings()->setValue(qsl("settingsSidebarLabelsShown"), true);
        for (const QString& page : {qsl("mapper"), qsl("mainDisplay")}) {
            QVERIFY2(openPreferences(page), "the settings dialog never appeared");
            const bool mapper = page == qsl("mapper");
            const auto sideBySide = [this, mapper]() {
                return mapper ? mapSideBySide() : displayTwoPairsToARow();
            };
            const auto namesShown = [this]() {
                return !TestSettings::sidebar(mpPreferences)->property(uiDesign::scmProp_rail).toBool();
            };
            QVERIFY2(namesShown() && sideBySide(), qPrintable(qsl("SETUP: at 1060px the %1 page does not start with the names and two pairs a row").arg(page)));

            QList<int> widths;
            for (int width = 1060; width >= mpPreferences->minimumWidth(); width -= 3) {
                widths.append(width);
            }
            for (qsizetype index = widths.size() - 2; index >= 0; --index) {
                widths.append(widths.at(index));
            }
            int namesChanges = 0;
            int arrangementChanges = 0;
            bool hadNames = true;
            bool hadTwo = true;
            for (const int width : widths) {
                mpPreferences->resize(width, 760);
                qApp->processEvents();
                qApp->processEvents();
                const bool names = namesShown();
                const bool two = sideBySide();
                QVERIFY2(!names || two, qPrintable(qsl("at %1px the %2 page shows the names with its colours one pair a row").arg(width).arg(page)));
                namesChanges += names != hadNames;
                arrangementChanges += two != hadTwo;
                hadNames = names;
                hadTwo = two;
            }
            // Down and back up: at most one change each way
            QVERIFY2(namesChanges <= 2 && arrangementChanges <= 2,
                     qPrintable(qsl("on the %1 page the names changed %2 times and the colours' arrangement %3 times").arg(page).arg(namesChanges).arg(arrangementChanges)));
            delete mpPreferences;
            mpPreferences = nullptr;
        }
    }

    // A width at the breakpoint keeps what it had: two columns side by side
    // are taken up a few pixels past the width they need and given up only
    // below it, so a window held right there cannot flip between them
    void test_aWidthAtTheBreakpointKeepsTheArrangementItHad()
    {
        QVERIFY2(openPreferences(), "the settings dialog never appeared");
        auto* pMapColours = mpPreferences->findChild<QWidget*>(qsl("settingsMapColours"));
        QGroupBox* pCard = mpPreferences->groupBox_mapperColors;
        const int needed = pMapColours->layout()->sizeHint().width();
        const int hysteresis = dlgProfilePreferences::scmColourArrangementHysteresis;
        const int chrome = cardChrome(pCard, pMapColours);
        for (const auto& [width, sideBySide] : {std::pair{needed - 1, false},
                                                std::pair{needed, false},
                                                std::pair{needed + hysteresis - 1, false},
                                                std::pair{needed + hysteresis, true},
                                                std::pair{needed, true},
                                                std::pair{needed - 1, false}}) {
            pCard->setMaximumWidth(chrome + width);
            qApp->processEvents();
            qApp->processEvents();
            QCOMPARE(pMapColours->width(), width);
            QVERIFY2(mapSideBySide() == sideBySide,
                     qPrintable(qsl("at %1px against the %2px needed the map's columns are %3").arg(width).arg(needed).arg(mapSideBySide() ? qsl("side by side") : qsl("one under the other"))));
        }
        pCard->setMaximumWidth(QWIDGETSIZE_MAX);
    }

    // The sixteen console colours under the divider of both cards: a heading
    // over each column, then a row for each of the eight names, its normal
    // colour under Normal and its light twin under Light. The light twin's own
    // label is gone from the card, and its field and button carry its name.
    void test_theConsoleColoursAreATableOfNormalAndLight()
    {
        QVERIFY2(openPreferences(qsl("mainDisplay")), "the settings dialog never appeared");
        auto* p = mpPreferences;
        struct Table
        {
            QString page;
            QString prefix;
            QList<std::tuple<QLabel*, QPushButton*, QLabel*, QPushButton*>> rows;
        };
        const QList<Table> tables{{qsl("mainDisplay"),
                                   qsl("settingsDisplay"),
                                   {{p->label_black, p->pushButton_black, p->label_lBlack, p->pushButton_lBlack},
                                    {p->label_red, p->pushButton_red, p->label_lRed, p->pushButton_lRed},
                                    {p->label_green, p->pushButton_green, p->label_lGreen, p->pushButton_lGreen},
                                    {p->label_yellow, p->pushButton_yellow, p->label_lYellow, p->pushButton_lYellow},
                                    {p->label_blue, p->pushButton_blue, p->label_lBlue, p->pushButton_lBlue},
                                    {p->label_magenta, p->pushButton_magenta, p->label_lMagenta, p->pushButton_lMagenta},
                                    {p->label_cyan, p->pushButton_cyan, p->label_lCyan, p->pushButton_lCyan},
                                    {p->label_white, p->pushButton_white, p->label_lWhite, p->pushButton_lWhite}}},
                                  {qsl("mapper"),
                                   qsl("settingsMap"),
                                   {{p->label_17, p->pushButton_black_2, p->label_49, p->pushButton_Lblack_2},
                                    {p->label_42, p->pushButton_red_2, p->label_50, p->pushButton_Lred_2},
                                    {p->label_43, p->pushButton_green_2, p->label_51, p->pushButton_Lgreen_2},
                                    {p->label_48, p->pushButton_yellow_2, p->label_52, p->pushButton_Lyellow_2},
                                    {p->label_44, p->pushButton_blue_2, p->label_53, p->pushButton_Lblue_2},
                                    {p->label_45, p->pushButton_magenta_2, p->label_54, p->pushButton_Lmagenta_2},
                                    {p->label_46, p->pushButton_cyan_2, p->label_55, p->pushButton_Lcyan_2},
                                    {p->label_47, p->pushButton_white_2, p->label_56, p->pushButton_Lwhite_2}}}};
        const QStringList names{qsl("Black"), qsl("Red"), qsl("Green"), qsl("Yellow"), qsl("Blue"), qsl("Magenta"), qsl("Cyan"), qsl("White")};
        for (const Table& table : tables) {
            p->setTab(table.page);
            qApp->processEvents();
            auto* pTable = p->findChild<QWidget*>(qsl("%1ConsoleColoursTable").arg(table.prefix));
            auto* pNormal = p->findChild<QLabel*>(qsl("%1ColoursNormal").arg(table.prefix));
            auto* pLight = p->findChild<QLabel*>(qsl("%1ColoursLight").arg(table.prefix));
            QVERIFY2(pTable && pNormal && pLight, qPrintable(qsl("the '%1' page has no table of console colours with its two headings").arg(table.page)));
            QCOMPARE(pNormal->text(), qsl("Normal"));
            QCOMPARE(pLight->text(), qsl("Light"));
            QVERIFY(pNormal->isVisible() && pLight->isVisible());
            const auto at = [pTable](const QWidget* pWidget) {
                return QRect(pWidget->mapTo(pTable, QPoint(0, 0)), pWidget->size());
            };
            QWidget* pRowAbove = pNormal;
            for (int row = 0; row < table.rows.size(); ++row) {
                const auto& [pName, pChoose, pLightName, pLightChoose] = table.rows.at(row);
                uiDesign::ColourValue* pField = fieldOf(pChoose);
                uiDesign::ColourValue* pLightField = fieldOf(pLightChoose);
                QVERIFY2(pField && pLightField, qPrintable(qsl("'%1' or '%2' has no field").arg(pChoose->objectName(), pLightChoose->objectName())));
                const QString where = qsl("%1, row %2").arg(table.page).arg(row);
                QVERIFY2(pName->isVisible() && pName->text().startsWith(names.at(row)), qPrintable(qsl("%1 is not named '%2' once: '%3'").arg(where, names.at(row), pName->text())));
                QVERIFY2(pLightName->isHidden(), qPrintable(qsl("%1: the light twin's own label '%2' is still on the card").arg(where, pLightName->text())));
                QCOMPARE(pName->buddy(), pField);
                for (QWidget* pPart : {static_cast<QWidget*>(pField), static_cast<QWidget*>(pChoose), static_cast<QWidget*>(pLightField), static_cast<QWidget*>(pLightChoose)}) {
                    QVERIFY2(pPart->isVisible() && qAbs(at(pPart).center().y() - at(pName).center().y()) <= 1,
                             qPrintable(qsl("%1: '%2' is not on the row of its name").arg(where, pPart->objectName())));
                    QCOMPARE(pPart->height(), uiDesign::scmColourWellHeight);
                }
                QVERIFY2(at(pName).top() > at(pRowAbove).bottom(), qPrintable(qsl("%1 is not under the row before it").arg(where)));
                pRowAbove = pName;
                QVERIFY2(at(pField).left() > at(pName).right() && at(pChoose).left() > at(pField).right(), qPrintable(qsl("%1: the name, the field and its button are out of order").arg(where)));
                QCOMPARE(at(pField).left(), at(pNormal).left());
                QCOMPARE(at(pLightField).left(), at(pLight).left());
                QVERIFY2(at(pLightField).left() > at(pChoose).right(), qPrintable(qsl("%1: the light colour does not stand after the normal one").arg(where)));
                // Named in full where the column heading cannot be heard
                const QString lightName = qsl("Light %1").arg(names.at(row).toLower());
                QCOMPARE(pLightField->accessibleName(), lightName);
                QCOMPARE(pLightChoose->accessibleName(), lightName);
                QCOMPARE(pLightChoose->toolTip(), qsl("Choose %1…").arg(lightName));
                QCOMPARE(pField->accessibleName(), names.at(row));
            }
        }
    }

    // The preview takes the room the table leaves, as tall as the table and
    // never taller, so the card is no taller for it; where too little is left
    // to show anything by, it goes rather than dropping under the table
    void test_theColourPreviewStandsBesideTheTableOrNotAtAll()
    {
        QVERIFY2(openPreferences(qsl("mainDisplay")), "the settings dialog never appeared");
        const QList<std::tuple<QString, QString, QGroupBox*>> cards{{qsl("mainDisplay"), qsl("settingsDisplay"), mpPreferences->groupBox_displayColors},
                                                                    {qsl("mapper"), qsl("settingsMap"), mpPreferences->groupBox_mapperColors}};
        for (const auto& [page, prefix, pCard] : cards) {
            mpPreferences->setTab(page);
            qApp->processEvents();
            auto* pBeside = mpPreferences->findChild<QWidget*>(qsl("%1ConsoleColours").arg(prefix));
            auto* pTable = mpPreferences->findChild<QWidget*>(qsl("%1ConsoleColoursTable").arg(prefix));
            auto* pPreview = mpPreferences->findChild<QWidget*>(qsl("%1ColoursPreview").arg(prefix));
            QVERIFY2(pBeside && pTable && pPreview, qPrintable(qsl("the '%1' page has no preview beside its console colours").arg(page)));
            QVERIFY2(pPreview->isVisible(), qPrintable(qsl("the '%1' preview is hidden in a %2px card").arg(page).arg(pCard->width())));
            QVERIFY2(!pPreview->accessibleName().isEmpty(), "the preview has no name for a screen reader");
            QCOMPARE(pPreview->geometry().top(), pTable->geometry().top());
            QVERIFY2(pPreview->height() <= pTable->height(), qPrintable(qsl("the '%1' preview is %2px tall beside a %3px table").arg(page).arg(pPreview->height()).arg(pTable->height())));
            QCOMPARE(pPreview->height(), pTable->height());
            QVERIFY(pPreview->geometry().left() > pTable->geometry().right());
            QCOMPARE(pPreview->geometry().right(), pBeside->width() - 1);
            QCOMPARE(pBeside->height(), pTable->height());

            // Narrowed to exactly the room a useful preview needs and then a pixel under it, and widened back: it
            // comes back a few pixels past that room, so a width right at it keeps what it had
            const int gap = pPreview->geometry().left() - pTable->geometry().right() - 1;
            const int room = pTable->sizeHint().width() + gap + 200;
            const int chrome = pCard->width() - pBeside->width();
            const int hysteresis = dlgProfilePreferences::scmColourArrangementHysteresis;
            for (const auto& [width, shown] :
                 {std::pair{room, true}, std::pair{room - 1, false}, std::pair{room, false}, std::pair{room + hysteresis - 1, false}, std::pair{room + hysteresis, true}}) {
                pCard->setMaximumWidth(chrome + width);
                qApp->processEvents();
                qApp->processEvents();
                QCOMPARE(pBeside->width(), width);
                QVERIFY2(pPreview->isVisible() == shown,
                         qPrintable(qsl("with %1px beside a %2px table the '%3' preview is %4")
                                            .arg(width - pTable->width() - gap)
                                            .arg(pTable->width())
                                            .arg(page, pPreview->isVisible() ? qsl("shown") : qsl("hidden"))));
                QCOMPARE(pBeside->height(), pTable->height());
            }
            pCard->setMaximumWidth(QWIDGETSIZE_MAX);
            qApp->processEvents();
        }
    }

    // The colour cards' rows were wells a line of words and 8px tall; they are
    // now fields at the 22px such a well comes to under macOS's 13pt face, and
    // the table adds its line of headings. The heights compared against were
    // measured with the wells, in this 1060px dialog: offscreen the Main
    // display's card was 434px and the map's 442px, under cocoa 448px and
    // 456px. Each card is to stay within 12px of that: the headings add a line
    // of words and a row gap (21px offscreen), and each of the 11 or 12 rows
    // gives back what its well was taller than 22px (1px offscreen, 2px under
    // cocoa) - about 10px taller offscreen and level under cocoa. The wells
    // stood the map's eight four to a column, where its leading column holds
    // five, so that card is allowed one row more on top of that.
    void test_theColourCardsStayAsTallAsTheirWellsMadeThem()
    {
#if !defined(Q_OS_MACOS)
        QSKIP("the heights compared against were measured with macOS's interface face");
#endif
        const bool offscreen = QGuiApplication::platformName() == qsl("offscreen");
        if (!offscreen && QGuiApplication::platformName() != qsl("cocoa")) {
            QSKIP("the heights compared against were measured offscreen and under cocoa");
        }
        QVERIFY2(openPreferences(qsl("mainDisplay")), "the settings dialog never appeared");
        const QList<std::tuple<QString, QGroupBox*, int, int>> cards{{qsl("mainDisplay"), mpPreferences->groupBox_displayColors, offscreen ? 434 : 448, 0},
                                                                     {qsl("mapper"), mpPreferences->groupBox_mapperColors, offscreen ? 442 : 456, 1}};
        for (const auto& [page, pCard, before, rowsAdded] : cards) {
            mpPreferences->setTab(page);
            qApp->processEvents();
            const int row = fieldOf(mpPreferences->pushButton_mapInfoBg)->parentWidget()->geometry().top() - fieldOf(mpPreferences->pushButton_background_color_2)->parentWidget()->geometry().top();
            const int allowed = before + rowsAdded * row;
            QVERIFY2(qAbs(pCard->height() - allowed) <= 12,
                     qPrintable(qsl("the '%1' card is %2px tall, where its wells made it %3px and its rows add %4px").arg(pCard->title()).arg(pCard->height()).arg(before).arg(rowsAdded * row)));
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

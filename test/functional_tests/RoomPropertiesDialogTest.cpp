/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * dlgRoomProperties is what the 2D mapper's "Configure room..." menu entry opens
 * on a room selection. It has to fold a whole selection into one form - a single
 * shared value gets a plain entry field, a mixed one gets a dropdown of what is
 * in use, a placeholder or a partially-checked box - and then report on OK only
 * the fields the player actually decided about, so that the ones left showing
 * "(Multiple values...)" leave every room alone.
 *
 * None of that is reachable from Lua: the dialog is a QDialog with no scripting
 * entry point, and its decisions live in widget state and in the arguments of
 * signal_save_symbol rather than in the map.
 *
 * Run with: ctest -R RoomPropertiesDialogTest -V
 */

#include <QAccessible>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFontInfo>
#include <QFontMetrics>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLine>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSpinBox>
#include <QTabBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTimer>
#include <QtTest/QtTest>

#include <limits>

#include "ColourValue.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "dlgRoomProperties.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

#include <algorithm>
#include <cstdlib>

// What the dialog shows in place of a value the selection does not agree on.
// dlgRoomProperties keeps it as a private tr() string, and no translator is
// installed in a test run, so the source string is what appears.
static const QString scmMultipleValues{qsl("(Multiple values...)")};

// Everything signal_save_symbol carries, so a case can assert on the decision
// the dialog reached rather than on a room the mapper has yet to write to.
struct EmittedProperties
{
    int emitCount = 0;
    bool changeName = false;
    QString newName;
    bool changeRoomColor = false;
    int roomColorNumber = 0;
    bool changeSymbol = false;
    QString newSymbol;
    bool changeSymbolColor = false;
    QColor newSymbolColor;
    bool changeWeight = false;
    int newWeight = 0;
    bool changeLockStatus = false;
    std::optional<bool> newLockStatus;
    bool changeHiddenStatus = false;
    std::optional<bool> newHiddenStatus;
    bool changeBorderColor = false;
    QColor newBorderColor;
    bool changeBorderThickness = false;
    int newBorderThickness = 0;
    QList<int> roomIds;
    // signal_save_userData, which is only emitted when a key changed
    int userDataEmitCount = 0;
    QMap<QString, std::optional<QString>> userDataChanges;
    QList<int> userDataRoomIds;
    // signal_save_hash, only emitted for one room whose game ID changed
    int hashEmitCount = 0;
    int hashRoomId = 0;
    QString newHash;
    // signal_save_area, only emitted when another area was picked
    int areaEmitCount = 0;
    int newAreaId = 0;
    QList<int> areaRoomIds;
};

class RoomPropertiesDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("RoomPropertiesDialog-Test");
    int mAreaId = 0;
    QPointer<dlgRoomProperties> mpDialog;
    EmittedProperties mEmitted;
    static constexpr int scmFirstRoom = 11;
    static constexpr int scmSecondRoom = 12;
    static constexpr int scmThirdRoom = 13;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }
    TRoom* room(const int id) const { return roomDB()->getRoom(id); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildMap()
    {
        map()->mapClear();
        mAreaId = roomDB()->addArea(qsl("Properties area"));
        QVERIFY(mAreaId > 0);
        int offset = 0;
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, offset++, 0, 0));
        }
        mEmitted = EmittedProperties();
    }

    // The scan T2DMap::slot_setRoomProperties()'s caller does before it opens
    // the dialog: the dialog is handed counts, not rooms, so how a selection is
    // summarised is part of what it is being asked to display.
    dlgRoomProperties* openDialogOn(const QList<int>& roomIds)
    {
        QHash<QString, int> usedNames;
        QHash<int, int> usedColors;
        QHash<QString, int> usedSymbols;
        QHash<int, int> usedWeights;
        QHash<bool, int> usedLockStatus;
        int hiddenRoomCount = 0;
        QSet<int> rooms;

        for (const int roomId : roomIds) {
            TRoom* pRoom = room(roomId);
            if (!pRoom) {
                continue;
            }
            rooms.insert(roomId);
            if (!pRoom->name.isEmpty()) {
                ++usedNames[pRoom->name];
            }
            ++usedColors[pRoom->environment];
            ++usedSymbols[pRoom->mSymbol];
            if (pRoom->getWeight() > 0) {
                ++usedWeights[pRoom->getWeight()];
            }
            ++usedLockStatus[pRoom->isLocked];
            if (pRoom->isHidden()) {
                ++hiddenRoomCount;
            }
        }

        auto* pDlg = new dlgRoomProperties(mpHost);
        connect(pDlg,
                &dlgRoomProperties::signal_save_symbol,
                this,
                [this](bool changeName,
                       QString newName,
                       bool changeRoomColor,
                       int roomColorNumber,
                       bool changeSymbol,
                       QString newSymbol,
                       bool changeSymbolColor,
                       QColor newSymbolColor,
                       bool changeWeight,
                       int newWeight,
                       bool changeLockStatus,
                       std::optional<bool> newLockStatus,
                       bool changeHiddenStatus,
                       std::optional<bool> newHiddenStatus,
                       bool changeBorderColor,
                       QColor newBorderColor,
                       bool changeBorderThickness,
                       int newBorderThickness,
                       QSet<int> roomIds) {
                    ++mEmitted.emitCount;
                    mEmitted.roomIds = QList<int>(roomIds.cbegin(), roomIds.cend());
                    std::sort(mEmitted.roomIds.begin(), mEmitted.roomIds.end());
                    mEmitted.changeName = changeName;
                    mEmitted.newName = newName;
                    mEmitted.changeRoomColor = changeRoomColor;
                    mEmitted.roomColorNumber = roomColorNumber;
                    mEmitted.changeSymbol = changeSymbol;
                    mEmitted.newSymbol = newSymbol;
                    mEmitted.changeSymbolColor = changeSymbolColor;
                    mEmitted.newSymbolColor = newSymbolColor;
                    mEmitted.changeWeight = changeWeight;
                    mEmitted.newWeight = newWeight;
                    mEmitted.changeLockStatus = changeLockStatus;
                    mEmitted.newLockStatus = newLockStatus;
                    mEmitted.changeHiddenStatus = changeHiddenStatus;
                    mEmitted.newHiddenStatus = newHiddenStatus;
                    mEmitted.changeBorderColor = changeBorderColor;
                    mEmitted.newBorderColor = newBorderColor;
                    mEmitted.changeBorderThickness = changeBorderThickness;
                    mEmitted.newBorderThickness = newBorderThickness;
                });
        connect(pDlg, &dlgRoomProperties::signal_save_area, this, [this](int areaId, QSet<int> roomIds) {
            ++mEmitted.areaEmitCount;
            mEmitted.newAreaId = areaId;
            mEmitted.areaRoomIds = QList<int>(roomIds.cbegin(), roomIds.cend());
            std::sort(mEmitted.areaRoomIds.begin(), mEmitted.areaRoomIds.end());
        });
        connect(pDlg, &dlgRoomProperties::signal_save_hash, this, [this](int roomId, QString hash) {
            ++mEmitted.hashEmitCount;
            mEmitted.hashRoomId = roomId;
            mEmitted.newHash = hash;
        });
        connect(pDlg, &dlgRoomProperties::signal_save_userData, this, [this](QMap<QString, std::optional<QString>> changes, QSet<int> roomIds) {
            ++mEmitted.userDataEmitCount;
            mEmitted.userDataChanges = changes;
            mEmitted.userDataRoomIds = QList<int>(roomIds.cbegin(), roomIds.cend());
            std::sort(mEmitted.userDataRoomIds.begin(), mEmitted.userDataRoomIds.end());
        });
        pDlg->init(usedNames, usedColors, usedSymbols, usedWeights, usedLockStatus, hiddenRoomCount, rooms);
        mpDialog = pDlg;
        return pDlg;
    }

    // The keys the user data table lists, top to bottom
    static QStringList userDataKeys(dlgRoomProperties* pDlg)
    {
        QStringList keys;
        for (int row = 0; row < pDlg->tableWidget_userData->rowCount(); ++row) {
            keys << pDlg->tableWidget_userData->item(row, 0)->text();
        }
        return keys;
    }

    static QTableWidgetItem* userDataValue(dlgRoomProperties* pDlg, const QString& key)
    {
        for (int row = 0; row < pDlg->tableWidget_userData->rowCount(); ++row) {
            if (pDlg->tableWidget_userData->item(row, 0)->text() == key) {
                return pDlg->tableWidget_userData->item(row, 1);
            }
        }
        return nullptr;
    }

    // Where a value is shown whole and edited: the cells show one line of it and are not edited in place
    static QPlainTextEdit* valueBox(const dlgRoomProperties* pDlg) { return pDlg->findChild<QPlainTextEdit*>(qsl("roomPropertiesValueBox")); }
    static QLabel* valueLabel(const dlgRoomProperties* pDlg) { return pDlg->findChild<QLabel*>(qsl("roomPropertiesValueLabel")); }
    static QLabel* jsonWarning(const dlgRoomProperties* pDlg) { return pDlg->findChild<QLabel*>(qsl("roomPropertiesJsonWarning")); }

    // As a click on the row does; false where the table has no row of that key
    static bool chooseRow(dlgRoomProperties* pDlg, const QString& key)
    {
        for (int row = 0; row < pDlg->tableWidget_userData->rowCount(); ++row) {
            if (pDlg->tableWidget_userData->item(row, 0)->text() == key) {
                pDlg->tableWidget_userData->setCurrentCell(row, 1);
                return true;
            }
        }
        return false;
    }

    // Typed after the last word of what the value box shows
    static void typeAtTheEndOfTheValue(dlgRoomProperties* pDlg, const QString& typed)
    {
        QPlainTextEdit* pBox = valueBox(pDlg);
        QTextCursor cursor = pBox->textCursor();
        cursor.movePosition(QTextCursor::End);
        pBox->setTextCursor(cursor);
        QTest::keyClicks(pBox, typed);
    }

    // In place of everything the value box shows, as select all and paste does
    static void replaceTheValue(dlgRoomProperties* pDlg, const QString& text)
    {
        QPlainTextEdit* pBox = valueBox(pDlg);
        pBox->selectAll();
        pBox->insertPlainText(text);
    }

    // The tab lays its table and value box out only once it is on show
    static bool showTheScriptData(dlgRoomProperties* pDlg)
    {
        pDlg->show();
        if (!QTest::qWaitForWindowExposed(pDlg)) {
            return false;
        }
        pDlg->tabWidget_room->setCurrentWidget(pDlg->tab_userData);
        QCoreApplication::processEvents();
        return pDlg->tableWidget_userData->isVisible();
    }

    struct ColorSelectorOutcome
    {
        bool sawSelector = false;
        bool clickLandedOnAColor = false;
        bool sawMenu = false;
        int colorsLeftInList = -1;
    };
    ColorSelectorOutcome mColorSelectorOutcome;

    // The colour selector and its context menu each run their own event loop,
    // so the right-click is made, and "Delete room color" picked, from timers
    // inside them. A clickedRow of -1 right-clicks the empty space past the colours.
    void rightClickInTheRoomColorSelector(dlgRoomProperties* pDlg, const int currentRow, const int clickedRow)
    {
        using namespace std::chrono_literals;
        mColorSelectorOutcome = ColorSelectorOutcome();
        auto* menuAnswerer = new QTimer(this);
        menuAnswerer->setInterval(20ms);
        connect(menuAnswerer, &QTimer::timeout, this, [this, menuAnswerer]() {
            auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if (!menu) {
                return;
            }
            menuAnswerer->stop();
            mColorSelectorOutcome.sawMenu = true;
            // Its only action, so a translation cannot hide it
            const auto actions = menu->actions();
            if (actions.size() == 1) {
                actions.constFirst()->trigger();
            }
            menu->close();
        });
        auto* selectorDriver = new QTimer(this);
        selectorDriver->setInterval(20ms);
        QElapsedTimer waited;
        waited.start();
        connect(selectorDriver, &QTimer::timeout, this, [this, selectorDriver, menuAnswerer, currentRow, clickedRow, &waited]() {
            auto* selector = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            auto* list = selector ? selector->findChild<QListWidget*>() : nullptr;
            if (!list) {
                // Something else is modal, and the button click would wait on it for ever
                if (waited.hasExpired(10000)) {
                    selectorDriver->stop();
                    if (selector) {
                        selector->reject();
                    }
                }
                return;
            }
            selectorDriver->stop();
            mColorSelectorOutcome.sawSelector = true;
            list->setCurrentRow(currentRow);
            QWidget* viewport = list->viewport();
            const QPoint pos = clickedRow >= 0 ? list->visualItemRect(list->item(clickedRow)).center() : QPoint(viewport->width() - 2, viewport->height() - 2);
            mColorSelectorOutcome.clickLandedOnAColor = list->itemAt(pos);
            menuAnswerer->start();
            QTest::mouseClick(viewport, Qt::RightButton, Qt::NoModifier, pos);
            QContextMenuEvent event(QContextMenuEvent::Mouse, pos, viewport->mapToGlobal(pos));
            QApplication::sendEvent(viewport, &event);
            menuAnswerer->stop();
            mColorSelectorOutcome.colorsLeftInList = list->count();
            selector->reject();
        });
        selectorDriver->start();
        pDlg->pushButton_setRoomColor->click();
        selectorDriver->stop();
        delete selectorDriver;
        delete menuAnswerer;
    }

    // Left-clicks a colour in the selector, from a timer inside its event
    // loop, then closes the selector with OK or Cancel; false if the click missed that row
    bool clickAColorInTheRoomColorSelector(dlgRoomProperties* pDlg, const int row, const bool pressOk)
    {
        using namespace std::chrono_literals;
        bool clickedTheRow = false;
        auto* selectorDriver = new QTimer(this);
        selectorDriver->setInterval(20ms);
        QElapsedTimer waited;
        waited.start();
        connect(selectorDriver, &QTimer::timeout, this, [selectorDriver, row, pressOk, &waited, &clickedTheRow]() {
            auto* selector = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            auto* list = selector ? selector->findChild<QListWidget*>() : nullptr;
            if (!list) {
                if (waited.hasExpired(10000)) {
                    selectorDriver->stop();
                    if (selector) {
                        selector->reject();
                    }
                }
                return;
            }
            selectorDriver->stop();
            QListWidgetItem* pItem = list->item(row);
            const QPoint pos = pItem ? list->visualItemRect(pItem).center() : QPoint();
            clickedTheRow = pItem && list->itemAt(pos) == pItem;
            QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, pos);
            if (pressOk) {
                selector->accept();
            } else {
                selector->reject();
            }
        });
        selectorDriver->start();
        pDlg->pushButton_setRoomColor->click();
        selectorDriver->stop();
        delete selectorDriver;
        return clickedTheRow;
    }

    // In the grabbed image's pixels, which outnumber the label's on a high-DPI screen
    struct PreviewInk
    {
        QRect contents;
        // Where the symbol was drawn, invalid when nothing of it was
        QRect ink;
    };

    // insideCircle counts only what is inside the circle a round room fills,
    // as outside it a round preview shows the dialog's background
    static PreviewInk symbolInk(QLabel* preview, const bool insideCircle = false)
    {
        const QImage shown = preview->grab().toImage();
        const qreal ratio = shown.devicePixelRatio();
        const QRect logical = preview->contentsRect();
        PreviewInk result;
        result.contents = QRect(qRound(logical.x() * ratio), qRound(logical.y() * ratio), qRound(logical.width() * ratio), qRound(logical.height() * ratio));
        const QPointF center = QRectF(result.contents).center();
        const qreal radius = result.contents.width() / 2.0 - 2 * ratio;
        const QColor background = shown.pixelColor(qRound(center.x()), result.contents.top() + qRound(2 * ratio));
        for (int y = result.contents.top(); y <= result.contents.bottom(); ++y) {
            for (int x = result.contents.left(); x <= result.contents.right(); ++x) {
                if (insideCircle && QLineF(center, QPointF(x + 0.5, y + 0.5)).length() > radius) {
                    continue;
                }
                const QColor pixel = shown.pixelColor(x, y);
                if (std::abs(pixel.red() - background.red()) + std::abs(pixel.green() - background.green()) + std::abs(pixel.blue() - background.blue()) > 48) {
                    result.ink |= QRect(x, y, 1, 1);
                }
            }
        }
        return result;
    }

    static uiDesign::ColourValue* roomColour(const dlgRoomProperties* pDlg) { return pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesRoomColour")); }
    static uiDesign::ColourValue* symbolColour(const dlgRoomProperties* pDlg) { return pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesSymbolColour")); }
    static uiDesign::ColourValue* borderColour(const dlgRoomProperties* pDlg) { return pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesBorderColour")); }
    static QSpinBox* borderOpacity(const dlgRoomProperties* pDlg) { return pDlg->findChild<QSpinBox*>(qsl("roomPropertiesBorderOpacity")); }

    // The swatch is the square at the field's leading edge, inset alike from the top and the leading edge, so at the
    // field's own height its middle is half that height in from both
    static QList<QColor> swatchRow(QWidget* pField)
    {
        const QImage image = pField->grab().toImage();
        const qreal ratio = image.devicePixelRatio();
        const int middle = pField->height() / 2;
        // Inside the swatch's hairline, which spans the field's height less 2 * (hairline + inset) pixels
        const int reach = middle - 6;
        QList<QColor> row;
        for (int x = middle - reach; x <= middle + reach; ++x) {
            row.append(image.pixelColor(QPointF(x * ratio, (pField->height() / 2) * ratio).toPoint()));
        }
        return row;
    }
    static QColor swatchOf(QWidget* pField)
    {
        const QList<QColor> row = swatchRow(pField);
        return row.at(row.size() / 2);
    }
    // A see-through colour shows the checkerboard's squares through it, which no opaque one does
    static bool showsTheBoardThrough(QWidget* pField)
    {
        const QList<QColor> row = swatchRow(pField);
        return std::any_of(row.cbegin(), row.cend(), [&row](const QColor& pixel) {
            return std::abs(pixel.lightness() - row.constFirst().lightness()) > 24;
        });
    }

    // Types into the field, ending with key: the field takes the code at Return and gives it up at Escape
    static void typeInto(QWidget* pField, const QString& typed, const Qt::Key key)
    {
        pField->setFocus();
        if (auto* pLineEdit = qobject_cast<QLineEdit*>(pField)) {
            pLineEdit->selectAll();
        } else if (auto* pSpinBox = qobject_cast<QSpinBox*>(pField)) {
            pSpinBox->selectAll();
        }
        if (typed.isEmpty()) {
            QTest::keyClick(pField, Qt::Key_Backspace);
        } else {
            QTest::keyClicks(pField, typed);
        }
        QTest::keyClick(pField, key);
    }

    // A field only loses the focus, and takes the code typed in it, if it had the focus in the active window first
    static bool focusInTheActiveDialog(dlgRoomProperties* pDlg, QWidget* pField)
    {
        pDlg->show();
        pDlg->activateWindow();
        if (!QTest::qWaitForWindowActive(pDlg)) {
            return false;
        }
        pField->setFocus();
        return QTest::qWaitFor([pField]() {
            return pField->hasFocus();
        });
    }

    static QColor atAlpha(QColor colour, const int alpha)
    {
        colour.setAlpha(alpha);
        return colour;
    }
    static QColor firstOwnBorder() { return {200, 40, 80}; }
    static QColor secondOwnBorder() { return {10, 20, 200}; }

    // Two rooms with border colours of their own that differ, and a third with none, drawn in the map's; OK lands on
    // the map
    dlgRoomProperties* openDialogOnRoomsThatDifferInBorderColour()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        room(scmFirstRoom)->mBorderColor = firstOwnBorder();
        room(scmSecondRoom)->mBorderColor = secondOwnBorder();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->show();
        return pDlg;
    }

    // The colours window is opened rather than run, so it is among the dialog's children as soon as Choose is clicked
    static bool chooseInTheColoursWindow(dlgRoomProperties* pDlg, QPushButton* pChoose, const QColor& colour)
    {
        QTest::mouseClick(pChoose, Qt::LeftButton);
        auto* pWindow = pDlg->findChild<QColorDialog*>();
        if (!pWindow || !pWindow->isVisible()) {
            return false;
        }
        pWindow->setCurrentColor(colour);
        pWindow->accept();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        return true;
    }

    // Everything a hand-made indenter could trip on: braces, brackets, colons and commas in strings, escaped quotes
    // and backslashes, escapes, letters past ASCII, numbers as they were written and empty tables
    static QString trickyJson()
    {
        return qsl("{\"name\":\"a {b} [c], d: e\",\"quote\":\"say \\\"hi\\\", {ok}\",\"escapes\":\"back\\\\slash\\n\\t\\u00e9\\\\\",\"unicode\":\"Ünïcødé ✓ "
                   "東京\",\"list\":[1,2.50,-3e2,true,null,{\"x\":[]}],\"empty\":{}}");
    }

    // QTableView hides the size its delegate asks for; its base class answers for it
    static int keysNeed(QTableWidget* pTable) { return static_cast<QAbstractItemView*>(pTable)->sizeHintForColumn(0); }

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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        // The other end of signal_save_symbol, where the OK lands; these are the
        // two things dlgMapper's constructor sets that the slot needs
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    // Runs after every case, a case QTest cut short at a failed QVERIFY
    // included, so nothing a failure walked away from is left for the leak
    // check to find. The dialog carries WA_DeleteOnClose, so accept()/reject()
    // may already have queued its deletion; run that before deciding there is
    // one left to do.
    void cleanup()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        delete mpDialog.data();
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void oneRoomPrefillsItsOwnValues()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->name = qsl("The Armoury");
        pRoom->mSymbol = qsl("!");
        pRoom->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->lineEdit_name->text(), qsl("The Armoury"));
        QCOMPARE(pDlg->lineEdit_roomSymbol->text(), qsl("!"));
        QVERIFY2(pDlg->comboBox_roomSymbol->isHidden(), "the symbol dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->spinBox_weight->value(), 5);
        QVERIFY2(pDlg->comboBox_weight->isHidden(), "the weight dropdown is only for a selection that disagrees");
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::Unchecked);
        QVERIFY(!pDlg->checkBox_hidden->isTristate());
    }

    void aSelectionThatDisagreesShowsPlaceholdersAndTristates()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        // A placeholder, not text typed into the field
        QVERIFY2(pDlg->lineEdit_name->text().isEmpty(), qPrintable(pDlg->lineEdit_name->text()));
        QCOMPARE(pDlg->lineEdit_name->placeholderText(), scmMultipleValues);
        QVERIFY2(pDlg->lineEdit_roomSymbol->isHidden(), "the plain symbol field cannot show two different symbols");
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(0), scmMultipleValues);
        QVERIFY2(pDlg->spinBox_weight->isHidden(), "the plain weight field cannot show two different weights");
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(0), scmMultipleValues);
        QVERIFY(pDlg->checkBox_locked->isTristate());
        QCOMPARE(pDlg->checkBox_locked->checkState(), Qt::PartiallyChecked);
        QVERIFY(pDlg->checkBox_hidden->isTristate());
        QCOMPARE(pDlg->checkBox_hidden->checkState(), Qt::PartiallyChecked);
    }

    // Without a buddy label or an accessible name a screen reader announces a field by its type alone
    void everyRoomFieldHasAScreenReaderName()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        const QList<QWidget*> fields{pDlg->lineEdit_name,
                                     pDlg->lineEdit_hash,
                                     roomColour(pDlg),
                                     pDlg->lineEdit_roomSymbol,
                                     pDlg->spinBox_weight,
                                     pDlg->spinBox_borderThickness,
                                     pDlg->tableWidget_userData,
                                     valueBox(pDlg),
                                     pDlg->pushButton_setRoomColor};
        for (QWidget* pField : fields) {
            QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pField);
            QVERIFY(pInterface);
            QVERIFY2(!pInterface->text(QAccessible::Name).trimmed().isEmpty(), qPrintable(qsl("%1 has no name for a screen reader to announce").arg(pField->objectName())));
        }
        // Everywhere but Windows, Qt reports a combo box's current text as its
        // name, so only the property itself can be checked here
        for (QWidget* pCombo : {static_cast<QWidget*>(pDlg->comboBox_area), static_cast<QWidget*>(pDlg->comboBox_roomSymbol), static_cast<QWidget*>(pDlg->comboBox_weight)}) {
            QVERIFY2(!pCombo->accessibleName().trimmed().isEmpty(), qPrintable(qsl("%1 has no accessible name").arg(pCombo->objectName())));
        }
    }

    // Tab goes down each tab of the dialog as its controls stand, along each row from the colour field to its Reset,
    // reaches OK and Cancel after them and the tab bar last, from where it starts over; the colour fields and the
    // opacity box are made after the form's own tab order was set. The room colour box only shows the colour, so Tab
    // passes it for its Choose
    void tabGoesThroughTheControlsInTheOrderTheyStand()
    {
        // As nextInFocusChain() reaches them with that tab in front; the line edit inside a spin or combo box hands
        // its focus to the box
        const auto tabStops = [](dlgRoomProperties* pDlg, QWidget* pTab) {
            pDlg->tabWidget_room->setCurrentWidget(pTab);
            QCoreApplication::processEvents();
            QPushButton* pOk = pDlg->dialogButtonBox->button(QDialogButtonBox::Ok);
            QPushButton* pCancel = pDlg->dialogButtonBox->button(QDialogButtonBox::Cancel);
            QStringList stops;
            QWidget* pWidget = pDlg->lineEdit_name;
            do {
                if ((pWidget->focusPolicy() & Qt::TabFocus) == Qt::TabFocus && !pWidget->focusProxy() && pWidget->isVisibleTo(pDlg) && pWidget->isEnabled()) {
                    stops.append(pWidget == pOk ? qsl("OK") : pWidget == pCancel ? qsl("Cancel") : pWidget == pDlg->tabWidget_room->tabBar() ? qsl("tabs") : pWidget->objectName());
                }
                pWidget = pWidget->nextInFocusChain();
            } while (pWidget && pWidget != pDlg->lineEdit_name);
            return stops;
        };
        // OK and Cancel where the platform puts them, which is where Tab has to find them, then the tab bar
        const auto buttonsAsTheyStandAndTabs = [](dlgRoomProperties* pDlg) {
            const bool okFirst = pDlg->dialogButtonBox->button(QDialogButtonBox::Ok)->x() < pDlg->dialogButtonBox->button(QDialogButtonBox::Cancel)->x();
            return okFirst ? QStringList{qsl("OK"), qsl("Cancel"), qsl("tabs")} : QStringList{qsl("Cancel"), qsl("OK"), qsl("tabs")};
        };

        // Rooms that agree on symbol and weight show the plain fields; two of them offer both Resets and no game ID
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QVERIFY(pDlg->pushButton_resetSymbolColor->isEnabled() && pDlg->pushButton_resetBorderColor->isEnabled());
        QVERIFY(borderOpacity(pDlg)->isEnabled());
        QCOMPARE(tabStops(pDlg, pDlg->tab_general),
                 QStringList({qsl("lineEdit_name"),
                              qsl("comboBox_area"),
                              qsl("pushButton_setRoomColor"),
                              qsl("lineEdit_roomSymbol"),
                              qsl("roomPropertiesSymbolColour"),
                              qsl("pushButton_setSymbolColor"),
                              qsl("pushButton_resetSymbolColor"),
                              qsl("roomPropertiesBorderColour"),
                              qsl("roomPropertiesBorderOpacity"),
                              qsl("pushButton_setBorderColor"),
                              qsl("pushButton_resetBorderColor"),
                              qsl("spinBox_borderThickness"),
                              qsl("checkBox_hidden"),
                              qsl("spinBox_weight"),
                              qsl("checkBox_locked")})
                         + buttonsAsTheyStandAndTabs(pDlg));
        // Remove is unavailable with no row chosen
        QCOMPARE(tabStops(pDlg, pDlg->tab_userData), QStringList({qsl("tableWidget_userData"), qsl("roomPropertiesValueBox"), qsl("pushButton_addUserData")}) + buttonsAsTheyStandAndTabs(pDlg));
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        // ...and rooms that differ show the dropdowns in their place
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QCOMPARE(tabStops(pDlg, pDlg->tab_general),
                 QStringList({qsl("lineEdit_name"),
                              qsl("comboBox_area"),
                              qsl("pushButton_setRoomColor"),
                              qsl("comboBox_roomSymbol"),
                              qsl("roomPropertiesSymbolColour"),
                              qsl("pushButton_setSymbolColor"),
                              qsl("pushButton_resetSymbolColor"),
                              qsl("roomPropertiesBorderColour"),
                              qsl("roomPropertiesBorderOpacity"),
                              qsl("pushButton_setBorderColor"),
                              qsl("pushButton_resetBorderColor"),
                              qsl("spinBox_borderThickness"),
                              qsl("checkBox_hidden"),
                              qsl("comboBox_weight"),
                              qsl("checkBox_locked")})
                         + buttonsAsTheyStandAndTabs(pDlg));
    }

    // Two tabs, the one with the room's own properties first
    void theWindowHasARoomTabAndAScriptDataTab()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->windowTitle(), qsl("Configure room"));
        QCOMPARE(pDlg->tabWidget_room->count(), 2);
        QCOMPARE(pDlg->tabWidget_room->tabText(0), qsl("Room"));
        QCOMPARE(pDlg->tabWidget_room->tabText(1), qsl("Script data"));
        QCOMPARE(pDlg->tabWidget_room->widget(0), pDlg->tab_general);
        QCOMPARE(pDlg->tabWidget_room->widget(1), pDlg->tab_userData);
    }

    // The tabs every window with tabs has: words on a hairline across the pane, the chosen one in the accent and
    // semibold on an accent underline with nothing behind it, and the first word starting on the line the words of
    // both tabs start on
    void theTabsAreWordsOnAHairlineStartingOnTheContentsEdge()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        const auto restoreAppearance = qScopeGuard([]() {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
            TestPixels::settle();
        });
        // A flat fill or a hairline reads back to a level or two
        constexpr int inkTolerance = 2;
        QTabBar* pBar = pDlg->tabWidget_room->tabBar();
        // The tab less its trailing gap, the air over the word and the room under it for the underline
        const auto wordLine = [pDlg, pBar](const int index) {
            const QRect tab = pBar->tabRect(index);
            return QRect(pBar->mapTo(pDlg, QPoint(tab.left(), tab.top() + uiDesign::scmTabPaddingTop)),
                         QSize(tab.width() - uiDesign::scmTabGap, tab.height() - uiDesign::scmTabPaddingTop - uiDesign::scmTabPaddingBottom));
        };
        const int hairlineRow = pBar->mapTo(pDlg, QPoint(0, pBar->height() - 1)).y();
        const int paneRight = pDlg->tabWidget_room->mapTo(pDlg, QPoint(pDlg->tabWidget_room->width() - 1, 0)).x();
        const QList<QPair<QWidget*, QList<const QWidget*>>> contentOf{{pDlg->tab_general, {pDlg->label_roomIdValue, pDlg->label_name, pDlg->roomPropertiesMapHeadingText}},
                                                                      {pDlg->tab_userData, {pDlg->label_userDataInstructions, pDlg->tableWidget_userData, pDlg->pushButton_addUserData}}};

        for (const auto appearance : {enums::Appearance::light, enums::Appearance::dark}) {
            TestPixels::setAppearanceAndWait(appearance, pDlg);
            QList<int> firstWordCoverage;
            for (const auto& [pPage, content] : contentOf) {
                pDlg->tabWidget_room->setCurrentWidget(pPage);
                TestPixels::settle();
                const int chosen = pDlg->tabWidget_room->currentIndex();
                const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
                const QImage shot = TestPixels::shot(pDlg);
                const QString where = qsl("%1 appearance, %2 chosen").arg(appearance == enums::Appearance::light ? qsl("light") : qsl("dark"), pBar->tabText(chosen));
                for (int index = 0; index < pBar->count(); ++index) {
                    const QRect line = wordLine(index);
                    const QColor ink = index == chosen ? tokens.accentText : tokens.mutedText;
                    // Columns past halfway from the page to the word's ink, so antialiased fringes count where they are
                    // more word than page
                    int first = -1;
                    int last = -1;
                    int covered = 0;
                    int onThePage = 0;
                    bool inkRead = false;
                    for (int x = line.left(); x <= line.right(); ++x) {
                        for (int y = line.top(); y <= line.bottom(); ++y) {
                            const QColor read = TestPixels::sampled(shot, QPoint(x, y));
                            onThePage += TestPixels::readsAs(read, tokens.page, inkTolerance) ? 1 : 0;
                            inkRead = inkRead || TestPixels::readsAs(read, ink, inkTolerance);
                            if (2 * TestPixels::distance(read, tokens.page) >= TestPixels::distance(ink, tokens.page)) {
                                first = first < 0 ? x : first;
                                last = x;
                                ++covered;
                            }
                        }
                    }
                    QVERIFY2(first >= 0 && inkRead, qPrintable(qsl("%1: tab %2's word is not drawn in %3").arg(where, pBar->tabText(index), ink.name())));
                    QVERIFY2(2 * onThePage > line.width() * line.height(),
                             qPrintable(qsl("%1: behind tab %2's word only %3 of %4 pixels are the page, so the tab has a face of its own")
                                                .arg(where, pBar->tabText(index))
                                                .arg(onThePage)
                                                .arg(line.width() * line.height())));
                    for (int row = hairlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= hairlineRow; ++row) {
                        for (int x = first; x <= last; ++x) {
                            const QColor read = TestPixels::sampled(shot, QPoint(x, row));
                            QVERIFY2(TestPixels::readsAs(read, tokens.accent, inkTolerance) == (index == chosen),
                                     qPrintable(qsl("%1: under tab %2's word, at %3,%4, the strip reads %5 where the accent is %6")
                                                        .arg(where, pBar->tabText(index))
                                                        .arg(x)
                                                        .arg(row)
                                                        .arg(read.name(), tokens.accent.name())));
                        }
                    }
                    if (index != 0) {
                        continue;
                    }
                    firstWordCoverage.append(covered);
                    QFont drawn = pBar->font();
                    if (index == chosen) {
                        drawn.setWeight(uiDesign::scmTabChosenWeight);
                    }
                    const int wordStart = first - std::max(0, qRound(QFontMetricsF(drawn).leftBearing(pBar->tabText(0).at(0))));
                    for (const QWidget* pContent : content) {
                        const int contentLeft = pContent->mapTo(pDlg, QPoint()).x();
                        QVERIFY2(std::abs(wordStart - contentLeft) <= 1,
                                 qPrintable(qsl("%1: the first tab's word starts at x %2, %3 at %4").arg(where).arg(wordStart).arg(pContent->objectName()).arg(contentLeft)));
                    }
                }
                // The hairline runs on past the last tab to the end of the pane
                const QColor lineEnd = TestPixels::sampled(shot, QPoint(paneRight, hairlineRow));
                QVERIFY2(TestPixels::readsAs(lineEnd, tokens.border, inkTolerance),
                         qPrintable(qsl("%1: at the pane's end the strip's bottom row reads %2, not the hairline %3").arg(where, lineEnd.name(), tokens.border.name())));
                QVERIFY2(!TestPixels::readsAs(TestPixels::sampled(shot, QPoint(paneRight, hairlineRow - 1)), tokens.border, inkTolerance),
                         qPrintable(qsl("%1: the row over the hairline is the hairline's colour too").arg(where)));
            }
            QVERIFY2(firstWordCoverage.size() == 2 && firstWordCoverage.at(0) > firstWordCoverage.at(1),
                     qPrintable(qsl("%1 appearance: \"Room\" covers %2 pixels chosen and %3 not, so it is not drawn heavier when chosen")
                                        .arg(appearance == enums::Appearance::light ? qsl("light") : qsl("dark"))
                                        .arg(firstWordCoverage.value(0))
                                        .arg(firstWordCoverage.value(1))));
        }
    }

    // As tall as the Room tab needs, whichever tab is on show and whatever the Script data tab holds
    void theWindowKeepsItsSizeWhenTheTabOrTheChosenValueChanges()
    {
        buildMap();
        QMap<QString, QString> data;
        for (int key = 0; key < 30; ++key) {
            data.insert(qsl("key %1").arg(key, 2, 10, QLatin1Char('0')), qsl("value %1").arg(key));
        }
        QStringList lines;
        for (int line = 0; line < 60; ++line) {
            lines << qsl("Line %1 of a long value.").arg(line);
        }
        data.insert(qsl("key 15"), lines.join(QLatin1Char('\n')));
        room(scmFirstRoom)->userData = data;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        const QSize opened = pDlg->size();
        // No band under the Room tab's last row, and no row cut short
        QCOMPARE(pDlg->tab_general->height(), pDlg->tab_general->sizeHint().height());
        QVERIFY2(showTheScriptData(pDlg), "the Script data tab did not come on show");
        QVERIFY(chooseRow(pDlg, qsl("key 15")));
        QCoreApplication::processEvents();
        QCOMPARE(pDlg->size(), opened);
        // The table has given way to four rows, the chosen one still in sight, and the box scrolls the rest
        const int rowHeight = pDlg->tableWidget_userData->verticalHeader()->defaultSectionSize();
        QCOMPARE(pDlg->tableWidget_userData->viewport()->height(), 4 * rowHeight);
        const QRect chosen = pDlg->tableWidget_userData->visualRect(pDlg->tableWidget_userData->currentIndex());
        QVERIFY2(pDlg->tableWidget_userData->viewport()->rect().contains(chosen), "the chosen row was scrolled out of the table");
        QVERIFY2(valueBox(pDlg)->verticalScrollBar()->maximum() > 0, "a value much taller than the room left does not scroll");
        // ...and everything else still fits in the tab
        const QWidget* pButtons = pDlg->pushButton_addUserData;
        QVERIFY2(pButtons->mapTo(pDlg->tab_userData, QPoint(0, pButtons->height())).y() <= pDlg->tab_userData->height(),
                 qPrintable(qsl("Add reaches %1 down a tab %2 tall").arg(pButtons->mapTo(pDlg->tab_userData, QPoint(0, pButtons->height())).y()).arg(pDlg->tab_userData->height())));

        pDlg->tabWidget_room->setCurrentWidget(pDlg->tab_general);
        QCoreApplication::processEvents();
        QCOMPARE(pDlg->size(), opened);
    }

    // Bold capitals in a hue of their own, drawn by the font so the words stay as translated; the space above a
    // heading sets its section apart, as a line under it would double the one under the tab strip
    void theSectionHeadingsAreBoldGreenCapitalsWithNoLineUnderThem()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        const auto restoreAppearance = qScopeGuard([]() {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
            QCoreApplication::processEvents();
        });
        const QList<QPair<QLabel*, QWidget*>> headings{{pDlg->label_roomIdValue, pDlg->roomPropertiesRoomHeading},
                                                       {pDlg->roomPropertiesMapHeadingText, pDlg->roomPropertiesMapHeading},
                                                       {pDlg->roomPropertiesPathHeadingText, pDlg->roomPropertiesPathHeading}};
        for (const auto appearance : {enums::Appearance::light, enums::Appearance::dark}) {
            mudlet::self()->setAppearance(appearance);
            QCoreApplication::processEvents();
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QColor ink = uiDesign::sectionHeadingInk(tokens, tokens.page);
            // Painting settles a widget's palette
            const QImage shot = pDlg->grab().toImage();
            const qreal ratio = shot.devicePixelRatio();
            for (const auto& [pHeading, pRow] : headings) {
                const QFont font = pHeading->font();
                QCOMPARE(font.weight(), QFont::Bold);
                QCOMPARE(font.capitalization(), QFont::AllUppercase);
                QCOMPARE(font.pointSize(), uiDesign::typeSize(uiDesign::TypeStep::Caption));
                // By value: the ink is mixed as an HSL colour, and the palette holds it as RGB
                QVERIFY2(pHeading->palette().color(pHeading->foregroundRole()).rgba() == ink.rgba(),
                         qPrintable(qsl("%1 is inked %2, not the heading ink %3").arg(pHeading->objectName(), pHeading->palette().color(pHeading->foregroundRole()).name(), ink.name())));
                // The row's last line of pixels, where a rule under it would be drawn
                const QRect row(pRow->mapTo(pDlg, QPoint()), pRow->size());
                const int y = qRound((row.bottom() + 1) * ratio) - 1;
                int drawn = 0;
                QColor seen;
                for (int x = qRound(row.left() * ratio); x < qRound((row.right() + 1) * ratio); ++x) {
                    if (shot.pixelColor(x, y).rgb() != tokens.page.rgb()) {
                        ++drawn;
                        seen = shot.pixelColor(x, y);
                    }
                }
                QVERIFY2(!drawn, qPrintable(qsl("%1 draws %2 pixels of a line along its foot, %3 on the page's %4").arg(pRow->objectName()).arg(drawn).arg(seen.name(), tokens.page.name())));
            }
        }
    }

    // The tile stands in a column of its own beside the four rows that decide how the map draws the room, behind a
    // hairline as tall as those rows, under the word "Preview" on the Room color row's line and midway between the
    // hairline and the window's edge
    void thePreviewStandsInAColumnOfItsOwnBesideTheMapRows()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("A");
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QCoreApplication::processEvents();
        const auto inDialog = [pDlg](const QWidget* pWidget) {
            return QRect(pWidget->mapTo(pDlg, QPoint()), pWidget->size());
        };
        const QWidget* pRule = pDlg->findChild<QWidget*>(qsl("roomPropertiesPreviewRule"));
        const QLabel* pWord = pDlg->findChild<QLabel*>(qsl("roomPropertiesPreviewLabel"));
        QVERIFY(pRule && pWord);
        const QRect tile = inDialog(pDlg->label_preview);
        const QRect rule = inDialog(pRule);
        const QRect word = inDialog(pWord);
        QCOMPARE(tile.size(), QSize(48, 48));

        for (const QWidget* pControl : {static_cast<const QWidget*>(roomColour(pDlg)),
                                        static_cast<const QWidget*>(pDlg->pushButton_setRoomColor),
                                        static_cast<const QWidget*>(pDlg->pushButton_resetSymbolColor),
                                        static_cast<const QWidget*>(pDlg->pushButton_resetBorderColor),
                                        static_cast<const QWidget*>(pDlg->spinBox_borderThickness),
                                        static_cast<const QWidget*>(pDlg->checkBox_hidden)}) {
            QVERIFY2(rule.left() > inDialog(pControl).right(),
                     qPrintable(qsl("%1 reaches x %2, past the hairline at %3").arg(pControl->objectName()).arg(inDialog(pControl).right()).arg(rule.left())));
        }
        QVERIFY(tile.left() > rule.right());
        QCOMPARE(rule.width(), 1);
        QCOMPARE(rule.top(), inDialog(pDlg->roomPropertiesRoomColorLabel).top());
        QCOMPARE(rule.bottom(), inDialog(pDlg->label_borderThickness).bottom());
        const QImage shot = pDlg->grab().toImage();
        const qreal ratio = shot.devicePixelRatio();
        QCOMPARE(shot.pixelColor(qRound(rule.left() * ratio), qRound(rule.center().y() * ratio)).name(), uiDesign::themeTokens().border.name());

        const qreal tileMiddle = tile.left() + tile.width() / 2.0;
        const qreal fromRule = tileMiddle - (rule.left() + rule.width() / 2.0);
        const qreal toEdge = pDlg->width() - tileMiddle;
        QVERIFY2(std::abs(fromRule - toEdge) <= 1.0, qPrintable(qsl("the tile's middle is %1 from the hairline and %2 from the window's edge").arg(fromRule).arg(toEdge)));

        QCOMPARE(pWord->text(), qsl("Preview"));
        QCOMPARE(pWord->font().pointSizeF(), pDlg->roomPropertiesRoomColorLabel->font().pointSizeF());
        QCOMPARE(word.center().y(), inDialog(pDlg->roomPropertiesRoomColorLabel).center().y());
        QVERIFY2(std::abs((word.left() + word.width() / 2.0) - tileMiddle) <= 1.0, "the word is not centred over the tile");
        // Centred in the room under the word, down to the foot of the hairline
        QVERIFY2(std::abs((tile.top() - word.bottom() - 1) - (rule.bottom() - tile.bottom())) <= 1,
                 qPrintable(qsl("the tile stands %1 under the word and %2 over the foot of the hairline").arg(tile.top() - word.bottom() - 1).arg(rule.bottom() - tile.bottom())));
    }

    // The whole point of the placeholders: a field left showing one has to come
    // back as "no decision", or OK would overwrite every room in the selection
    // with the placeholder text.
    void okOnAnUntouchedMixedSelectionChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(9);
        room(scmSecondRoom)->isLocked = true;
        room(scmSecondRoom)->setHidden(true);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        // the set decides which rooms the save applies to, so an emit that
        // changes nothing still has to name the selection it was opened on
        QCOMPARE(mEmitted.roomIds, QList<int>({scmFirstRoom, scmSecondRoom}));
        QVERIFY2(!mEmitted.changeName, "the name placeholder was taken for a new name");
        QVERIFY2(!mEmitted.changeSymbol, "the symbol placeholder was taken for a new symbol");
        QVERIFY2(!mEmitted.changeWeight, "the weight placeholder was taken for a new weight");
        QVERIFY2(!mEmitted.changeLockStatus, "a partially checked lock box was taken for a decision");
        QVERIFY2(!mEmitted.changeHiddenStatus, "a partially checked hidden box was taken for a decision");
        QVERIFY2(!mEmitted.changeRoomColor, "the room colour was reported as changed without the colour selector being used");
        QVERIFY2(!mEmitted.changeSymbolColor, "the symbol colour was reported as changed without the colour selector being used");
    }

    void okReportsTheEditedNameSymbolWeightAndCheckboxes()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmFirstRoom)->setWeight(5);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Old Armoury"));
        pDlg->lineEdit_roomSymbol->setText(qsl("A"));
        pDlg->spinBox_weight->setValue(12);
        pDlg->checkBox_locked->setChecked(true);
        pDlg->checkBox_hidden->setChecked(true);
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QCOMPARE(mEmitted.roomIds, QList<int>({scmFirstRoom}));
        QVERIFY(mEmitted.changeName);
        QCOMPARE(mEmitted.newName, qsl("Old Armoury"));
        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("A"));
        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 12);
        QVERIFY(mEmitted.changeLockStatus);
        QCOMPARE(mEmitted.newLockStatus, std::optional<bool>(true));
        QVERIFY(mEmitted.changeHiddenStatus);
        QCOMPARE(mEmitted.newHiddenStatus, std::optional<bool>(true));
    }

    // Cancel has to emit nothing at all, whatever was typed into the form
    void cancelReportsNothing()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_name->setText(qsl("Never Applied"));
        pDlg->reject();

        QCOMPARE(mEmitted.emitCount, 0);
    }

    // The dropdown entries carry a usage count for the player's benefit; the
    // count has to be stripped back off before the value is handed on.
    void theWeightDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);
        room(scmThirdRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_weight->count(), 3);
        QCOMPARE(pDlg->comboBox_weight->itemText(1), qsl("7 (count: 2)"));
        QCOMPARE(pDlg->comboBox_weight->itemText(2), qsl("3 (count: 1)"));

        pDlg->comboBox_weight->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 7);
    }

    void theSymbolDropdownIsOrderedByUseAndItsCountIsStripped()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmThirdRoom)->mSymbol = qsl("?");

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        QCOMPARE(pDlg->comboBox_roomSymbol->count(), 3);
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(1), qsl("? (count: 2)"));
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(2), qsl("! (count: 1)"));

        pDlg->comboBox_roomSymbol->setCurrentIndex(1);
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QCOMPARE(mEmitted.newSymbol, qsl("?"));
    }

    // The weight dropdown is editable, so a player can type anything into it
    void anUnreadableTypedWeightIsNotAppliedToTheSelection()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(pDlg->comboBox_weight->lineEdit());
        pDlg->comboBox_weight->setCurrentText(qsl("heavy"));
        pDlg->accept();

        QVERIFY2(!mEmitted.changeWeight, "text that names no number was still applied as a room weight");
    }

    // Each colour is a field showing the room's own colour as a code over a swatch of it, beside glyph buttons that
    // choose it in the colours window and reset it
    void theColourFieldsShowTheRoomsOwnColours()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY2(pSymbol && pBorder && borderOpacity(pDlg), "the dialog has no colour fields, or no opacity for the border");
        QCOMPARE(pSymbol->text(), qsl("#FF00FF"));
        QCOMPARE(pBorder->text(), qsl("#008000"));
        QCOMPARE(swatchOf(pSymbol), QColor(Qt::magenta));
        QCOMPARE(swatchOf(pBorder), QColor(Qt::darkGreen));
        QCOMPARE(borderOpacity(pDlg)->value(), 100);
        QVERIFY2(!pDlg->findChild<QSpinBox*>(qsl("roomPropertiesSymbolOpacity")), "the symbol, which is never see-through, was given an opacity");
        for (QPushButton* pButton :
             {pDlg->pushButton_setRoomColor, pDlg->pushButton_setSymbolColor, pDlg->pushButton_setBorderColor, pDlg->pushButton_resetSymbolColor, pDlg->pushButton_resetBorderColor}) {
            QVERIFY2(pButton->text().isEmpty() && !pButton->icon().isNull(), qPrintable(qsl("%1 is not a glyph button: '%2'").arg(pButton->objectName(), pButton->text())));
            QCOMPARE(pButton->size(), QSize(uiDesign::scmInputHeight, uiDesign::scmInputHeight));
        }
        // Every field, colour box and number box of the tab stands as tall as the name field
        for (const QWidget* pControl : {static_cast<const QWidget*>(pDlg->lineEdit_name),
                                        static_cast<const QWidget*>(pDlg->comboBox_area),
                                        static_cast<const QWidget*>(pDlg->lineEdit_hash),
                                        static_cast<const QWidget*>(pDlg->findChild<uiDesign::ColourValue*>(qsl("roomPropertiesRoomColour"))),
                                        static_cast<const QWidget*>(pDlg->lineEdit_roomSymbol),
                                        static_cast<const QWidget*>(pSymbol),
                                        static_cast<const QWidget*>(pBorder),
                                        static_cast<const QWidget*>(borderOpacity(pDlg)),
                                        static_cast<const QWidget*>(pDlg->spinBox_borderThickness),
                                        static_cast<const QWidget*>(pDlg->spinBox_weight)}) {
            QVERIFY(pControl);
            QVERIFY2(pControl->height() == uiDesign::scmInputHeight,
                     qPrintable(qsl("%1 is %2 tall, not the field's %3").arg(pControl->objectName()).arg(pControl->height()).arg(uiDesign::scmInputHeight)));
        }
        QPushButton reference;
        reference.setStyleSheet(uiDesign::buttonStyleSheet(uiDesign::themeTokens()));
        reference.setFixedSize(pDlg->pushButton_setSymbolColor->size());
        const QImage referenceImage = reference.grab().toImage();
        const QPoint onTheFace(2, uiDesign::scmInputHeight / 2);
        for (QPushButton* pButton : {pDlg->pushButton_setSymbolColor, pDlg->pushButton_setBorderColor}) {
            const QImage image = pButton->grab().toImage();
            const QColor face = image.pixelColor(onTheFace * image.devicePixelRatio());
            const QColor designFace = referenceImage.pixelColor(onTheFace * referenceImage.devicePixelRatio());
            QVERIFY2(face == designFace, qPrintable(qsl("%1 is drawn in %2, not the design's button face %3").arg(pButton->objectName(), face.name(), designFace.name())));
        }
        QVERIFY(pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(pDlg->pushButton_resetBorderColor->isEnabled());
    }

    // The lists standing in for the symbol and the weight where the rooms disagree are as tall as the fields
    void theListsForRoomsThatDisagreeAreAFieldsHeight()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("A");
        room(scmSecondRoom)->mSymbol = qsl("B");
        room(scmFirstRoom)->setWeight(3);
        room(scmSecondRoom)->setWeight(7);
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        for (const QComboBox* pList : {pDlg->comboBox_roomSymbol, pDlg->comboBox_weight}) {
            QVERIFY2(pList->isVisible(), qPrintable(qsl("%1 is not on show for rooms that disagree").arg(pList->objectName())));
            QVERIFY2(pList->height() == uiDesign::scmInputHeight, qPrintable(qsl("%1 is %2 tall, not the field's %3").arg(pList->objectName()).arg(pList->height()).arg(uiDesign::scmInputHeight)));
        }
    }

    // A number box's value starts where a field's text does, so the values down the column line up
    void theNumberBoxesStartTheirValuesWhereTheFieldsStartTheirText()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        const auto textStart = [pDlg](QLineEdit* pEdit) {
            pEdit->setCursorPosition(0);
            return pEdit->mapTo(pDlg, pEdit->inputMethodQuery(Qt::ImCursorRectangle).toRect().topLeft()).x();
        };
        const int fieldText = textStart(pDlg->lineEdit_name);
        for (QSpinBox* pBox : {pDlg->spinBox_borderThickness, pDlg->spinBox_weight}) {
            QLineEdit* pEdit = pBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly);
            QVERIFY(pEdit);
            QVERIFY2(textStart(pEdit) == fieldText, qPrintable(qsl("%1's value starts at x %2, the name's text at %3").arg(pBox->objectName()).arg(textStart(pEdit)).arg(fieldText)));
        }
    }

    // With no colour of its own the field is empty, says why, and its swatch shows what the room is drawn with
    void withNoColourOfItsOwnTheFieldShowsWhatTheRoomIsDrawnWith()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("=");
        // A light room, so its symbol is drawn in black rather than in the white of the field round the swatch
        const int lightEnvironment = 300;
        map()->mCustomEnvColors[lightEnvironment] = QColor(240, 240, 200);
        const auto forgetEnvironment = qScopeGuard([this, lightEnvironment]() {
            map()->mCustomEnvColors.remove(lightEnvironment);
        });
        room(scmFirstRoom)->environment = lightEnvironment;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->tabWidget_room->setCurrentWidget(pDlg->tab_general);
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY(pSymbol && pBorder);
        QVERIFY2(pSymbol->text().isEmpty() && pBorder->text().isEmpty(), qPrintable(qsl("'%1' and '%2'").arg(pSymbol->text(), pBorder->text())));
        QCOMPARE(pSymbol->placeholderText(), qsl("Automatic"));
        QCOMPARE(pBorder->placeholderText(), mpHost->mRoomBorderColor.name(QColor::HexRgb).toUpper());
        // ...so it needs no more room than a code; the boxes standing one over another share the width the widest of
        // them needs, the border width's "Map default" among them
        const uiDesign::ColourValue codeOnly(pDlg->widget_borderColor);
        QCOMPARE(pBorder->sizeHint().width(), codeOnly.sizeHint().width());
        QCOMPARE(pBorder->width(), std::max({roomColour(pDlg)->sizeHint().width(), pSymbol->sizeHint().width(), pBorder->sizeHint().width(), pDlg->spinBox_borderThickness->sizeHint().width()}));
        for (QWidget* pBox :
             {static_cast<QWidget*>(roomColour(pDlg)), static_cast<QWidget*>(pDlg->lineEdit_roomSymbol), static_cast<QWidget*>(pSymbol), static_cast<QWidget*>(pDlg->spinBox_borderThickness)}) {
            QVERIFY2(pBox->width() == pBorder->width(), qPrintable(qsl("%1 is %2 wide, the border color %3").arg(pBox->objectName()).arg(pBox->width()).arg(pBorder->width())));
        }
        QCOMPARE(roomColour(pDlg)->mapTo(pDlg, QPoint()).x(), pBorder->mapTo(pDlg, QPoint()).x());
        QCOMPARE(pDlg->spinBox_borderThickness->mapTo(pDlg, QPoint()).x(), pBorder->mapTo(pDlg, QPoint()).x());
        // The symbol is black or white, whichever stands out against the room
        QCOMPARE(swatchOf(pSymbol), QColor(Qt::black));
        QCOMPARE(pBorder->colour(), mpHost->mRoomBorderColor);
        QCOMPARE(swatchOf(pBorder), QColor(mpHost->mRoomBorderColor.rgb()));
        QCOMPARE(borderOpacity(pDlg)->value(), qRound(mpHost->mRoomBorderColor.alphaF() * 100));
        QVERIFY2(!pDlg->pushButton_resetSymbolColor->isEnabled(), "Reset is offered for a symbol colour the room does not have");
        QVERIFY2(!pDlg->pushButton_resetBorderColor->isEnabled(), "Reset is offered for a border colour the room does not have");
    }

    // Reset is there while the room has a colour to reset, and always for several rooms, as the others may have one
    void resetIsOfferedOnlyWhileThereIsAColourToReset()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QVERIFY(!pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());

        QVERIFY2(chooseInTheColoursWindow(pDlg, pDlg->pushButton_setSymbolColor, QColor(Qt::cyan)), "Choose opened no colours window for the symbol");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#00FFFF"));
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled(), "a chosen symbol colour cannot be reset");
        // The opacity follows a colour the colours window chose
        QVERIFY2(chooseInTheColoursWindow(pDlg, pDlg->pushButton_setBorderColor, QColor(10, 20, 30, 153)), "Choose opened no colours window for the border");
        QCOMPARE(borderColour(pDlg)->text(), qsl("#0A141E"));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
        QVERIFY2(pDlg->pushButton_resetBorderColor->isEnabled(), "a chosen border colour cannot be reset");

        pDlg->pushButton_resetSymbolColor->click();
        pDlg->pushButton_resetBorderColor->click();
        QVERIFY(!pDlg->pushButton_resetSymbolColor->isEnabled());
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled() && pDlg->pushButton_resetBorderColor->isEnabled(), "Reset is held back for rooms other than the first, which may carry colours");
    }

    // A click on the code edits it; the colours window is Choose's, and its swatch's (below)
    void onlyChooseOpensTheColoursWindow()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QTest::mouseClick(symbolColour(pDlg), Qt::LeftButton);
        QTest::mouseClick(borderColour(pDlg), Qt::LeftButton);
        QCoreApplication::processEvents();
        QVERIFY2(!pDlg->findChild<QColorDialog*>(), "clicking a colour field opened the colours window");

        QTest::mouseClick(pDlg->pushButton_setSymbolColor, Qt::LeftButton);
        auto* pWindow = pDlg->findChild<QColorDialog*>();
        QVERIFY2(pWindow && pWindow->isVisible(), "Choose opened no colours window");
        pWindow->reject();
    }

    // The swatch stands for Choose: the pointer over it is a hand and rings it in the accent, and a click on it opens
    // the colours window for that row's colour
    void aClickOnTheSwatchOpensTheColoursWindow()
    {
        buildMap();
        room(scmFirstRoom)->mBorderColor = QColor(Qt::darkGreen);
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->tabWidget_room->setCurrentWidget(pDlg->tab_general);
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY(pBorder);
        QCOMPARE(pBorder->chooseButton(), pDlg->pushButton_setBorderColor);
        QCOMPARE(symbolColour(pDlg)->chooseButton(), pDlg->pushButton_setSymbolColor);
        const int middle = pBorder->height() / 2;
        const QPoint onTheSwatch(middle, middle);
        const QPoint onTheCode(pBorder->width() - middle, middle);
        // In the inset between the swatch and the field's frame
        const QPoint onTheRing(uiDesign::scmInputBorderWidth + 2, middle);
        const auto ringInk = [pBorder, onTheRing]() {
            const QImage image = pBorder->grab().toImage();
            return image.pixelColor(onTheRing * image.devicePixelRatio());
        };
        const QColor accent = uiDesign::themeTokens().accent;

        QTest::mouseMove(pBorder, onTheCode);
        QTRY_COMPARE(pBorder->cursor().shape(), Qt::IBeamCursor);
        QVERIFY2(ringInk() != accent, "the swatch is ringed with the pointer away from it");
        QTest::mouseMove(pBorder, onTheSwatch);
        QTRY_COMPARE(pBorder->cursor().shape(), Qt::PointingHandCursor);
        QVERIFY2(ringInk() == accent, qPrintable(qsl("under the pointer the swatch is ringed in %1, not the accent %2").arg(ringInk().name(), accent.name())));

        QTest::mouseClick(pBorder, Qt::LeftButton, Qt::NoModifier, onTheSwatch);
        auto* pWindow = pDlg->findChild<QColorDialog*>();
        QVERIFY2(pWindow && pWindow->isVisible(), "a click on the swatch opened no colours window");
        QCOMPARE(pWindow->currentColor().rgb(), QColor(Qt::darkGreen).rgb());
        pWindow->reject();
    }

    // A code typed and taken with Return is a chosen colour, the border's previewed on the room as the colours
    // window's is, and the short form and a missing # are both understood
    void aTypedCodeIsTakenAsAChosenColour()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), qsl("#00ff00"), Qt::Key_Return);
        QVERIFY2(pDlg->isVisible(), "Return in the colour field closed the dialog");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#00FF00"));
        QVERIFY2(pDlg->pushButton_resetSymbolColor->isEnabled(), "a typed symbol colour cannot be reset");
        typeInto(borderColour(pDlg), qsl("0f0"), Qt::Key_Return);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#00FF00"));
        QCOMPARE(pRoom->mBorderColor, QColor(0, 255, 0));
        QVERIFY2(pDlg->pushButton_resetBorderColor->isEnabled(), "a typed border colour cannot be reset");

        pDlg->accept();
        QVERIFY(mEmitted.changeSymbolColor);
        QCOMPARE(mEmitted.newSymbolColor, QColor(0, 255, 0));
        QVERIFY(mEmitted.changeBorderColor);
        QCOMPARE(mEmitted.newBorderColor, QColor(0, 255, 0));
    }

    // A code that is not a whole colour when it is taken, and anything left at Escape, puts the colour back
    void anUnfinishedCodeOrEscapePutsTheColourBack()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), qsl("#12"), Qt::Key_Return);
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#FF00FF"));
        typeInto(borderColour(pDlg), qsl("#1234"), Qt::Key_Return);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        typeInto(symbolColour(pDlg), qsl("#123456"), Qt::Key_Escape);
        QVERIFY2(pDlg->isVisible(), "Escape in a field holding a typed code closed the dialog");
        QCOMPARE(symbolColour(pDlg)->text(), qsl("#FF00FF"));
        typeInto(borderColour(pDlg), qsl("#123456"), Qt::Key_Escape);
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));

        pDlg->accept();
        QVERIFY2(!mEmitted.changeSymbolColor, "a code put back was reported as a change of symbol colour");
        QVERIFY2(!mEmitted.changeBorderColor, "a code put back was reported as a change of border colour");
    }

    // Leaving the field takes the code as Return does, so a code typed and clicked away from is not lost
    void leavingTheFieldTakesTheCode()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        uiDesign::ColourValue* pBorder = borderColour(pDlg);
        QVERIFY2(focusInTheActiveDialog(pDlg, pBorder), "the border field never had the focus, so it had none to lose");
        pBorder->selectAll();
        QTest::keyClicks(pBorder, qsl("#ff0000"));
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));

        QTest::mouseClick(pDlg->lineEdit_name, Qt::LeftButton);
        QVERIFY2(pDlg->lineEdit_name->hasFocus(), "the click left the focus in the border field");
        QCOMPARE(pBorder->text(), qsl("#FF0000"));
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::red));

        pDlg->accept();
        QVERIFY(mEmitted.changeBorderColor);
        QCOMPARE(mEmitted.newBorderColor, QColor(Qt::red));
    }

    // Emptying a field and taking it is what Reset does
    void anEmptiedFieldPutsTheRoomBackToTheDefault()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mSymbol = qsl("=");
        pRoom->mSymbolColor = QColor(Qt::magenta);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(symbolColour(pDlg), QString(), Qt::Key_Return);
        QVERIFY2(symbolColour(pDlg)->showsDefault(), qPrintable(qsl("the emptied symbol field shows '%1' rather than its default").arg(symbolColour(pDlg)->text())));
        QVERIFY2(!pDlg->pushButton_resetSymbolColor->isEnabled(), "Reset is still offered once the field has reset the colour");
        typeInto(borderColour(pDlg), QString(), Qt::Key_Return);
        QVERIFY(borderColour(pDlg)->showsDefault());
        QVERIFY2(!pRoom->mBorderColor.isValid(), "emptying the border field did not reach the room");
        QVERIFY(!pDlg->pushButton_resetBorderColor->isEnabled());

        pDlg->accept();
        QVERIFY(mEmitted.changeSymbolColor);
        QVERIFY2(!mEmitted.newSymbolColor.isValid(), "an emptied symbol field has to report an invalid QColor so the default is used");
    }

    // Only the colour is typed: how see-through the border is stays as it was
    void aTypedCodeKeepsTheOpacity()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(200, 40, 80, 153);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
        typeInto(borderColour(pDlg), qsl("#00ff00"), Qt::Key_Return);
        QCOMPARE(pRoom->mBorderColor, QColor(0, 255, 0, 153));
        QCOMPARE(borderOpacity(pDlg)->value(), 60);
    }

    // The opacity box sets how see-through the border is, keeping its colour, and the swatch shows the board through it
    void theOpacityBoxSetsHowSeeThroughTheBorderIs()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        QVERIFY2(!showsTheBoardThrough(borderColour(pDlg)), "an opaque border's swatch shows the board through it");
        typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
        QVERIFY2(pDlg->isVisible(), "Return in the opacity box closed the dialog");
        QCOMPARE(pRoom->mBorderColor, QColor(0, 128, 0, 153));
        QCOMPARE(borderColour(pDlg)->text(), qsl("#008000"));
        QVERIFY2(showsTheBoardThrough(borderColour(pDlg)), "a see-through border's swatch does not show the board through it");
        QVERIFY(pDlg->pushButton_resetBorderColor->isEnabled());
    }

    // A border that is only nearly clear or nearly opaque is held at 1% or 99%, so the box shows 0% and 100% only for
    // a border that is wholly clear or wholly opaque
    void onlyAClearOrOpaqueBorderShowsAsZeroOrAHundredPercent()
    {
        const QList<std::pair<int, int>> percentForAlpha{{0, 0}, {1, 1}, {254, 99}, {255, 100}};
        for (const auto& [alpha, percent] : percentForAlpha) {
            buildMap();
            room(scmFirstRoom)->mBorderColor = QColor(0, 128, 0, alpha);
            auto* pDlg = openDialogOn({scmFirstRoom});
            const int shown = borderOpacity(pDlg)->value();
            QVERIFY2(shown == percent, qPrintable(qsl("a border of alpha %1 shows as %2%, not %3%").arg(alpha).arg(shown).arg(percent)));
            pDlg->reject();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

    // The field shows one room's colour, so an opacity set for rooms that differ in colour goes to each with its own,
    // in the preview and on OK, and a room with none of its own keeps following the map's
    void anOpacitySetForRoomsThatDifferInColourKeepsEachRoomsOwn()
    {
        auto* pDlg = openDialogOnRoomsThatDifferInBorderColour();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));

        typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
        QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(firstOwnBorder(), 153));
        QCOMPARE(room(scmSecondRoom)->mBorderColor, atAlpha(secondOwnBorder(), 153));
        QVERIFY2(!room(scmThirdRoom)->mBorderColor.isValid(), qPrintable(qsl("the room drawn in the map's border colour was given %1").arg(room(scmThirdRoom)->mBorderColor.name(QColor::HexArgb))));
        // A second step sets the opacity anew rather than on top of the first
        typeInto(borderOpacity(pDlg), qsl("30"), Qt::Key_Return);
        QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(firstOwnBorder(), 77));
        QCOMPARE(room(scmSecondRoom)->mBorderColor, atAlpha(secondOwnBorder(), 77));
        QVERIFY(!room(scmThirdRoom)->mBorderColor.isValid());

        pDlg->accept();
        QCOMPARE(mEmitted.emitCount, 1);
        QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(firstOwnBorder(), 77));
        QCOMPARE(room(scmSecondRoom)->mBorderColor, atAlpha(secondOwnBorder(), 77));
        QVERIFY2(!room(scmThirdRoom)->mBorderColor.isValid(), qPrintable(qsl("OK gave the room drawn in the map's border colour %1").arg(room(scmThirdRoom)->mBorderColor.name(QColor::HexArgb))));
    }

    void cancelAfterAnOpacitySetForRoomsThatDifferPutsEveryRoomBack()
    {
        auto* pDlg = openDialogOnRoomsThatDifferInBorderColour();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
        QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(firstOwnBorder(), 153));

        pDlg->reject();
        QCOMPARE(mEmitted.emitCount, 0);
        QCOMPARE(room(scmFirstRoom)->mBorderColor, firstOwnBorder());
        QCOMPARE(room(scmSecondRoom)->mBorderColor, secondOwnBorder());
        QVERIFY(!room(scmThirdRoom)->mBorderColor.isValid());
    }

    // Once a colour is chosen for them all, typed or in the colours window, the opacity goes with that colour to every
    // room, a room that had none of its own too
    void anOpacitySetAfterAColourIsChosenGoesWithItToEveryRoom()
    {
        for (const bool inTheColoursWindow : {false, true}) {
            const QString way = inTheColoursWindow ? qsl("chosen in the colours window") : qsl("typed");
            auto* pDlg = openDialogOnRoomsThatDifferInBorderColour();
            QVERIFY(QTest::qWaitForWindowExposed(pDlg));
            if (inTheColoursWindow) {
                QVERIFY2(chooseInTheColoursWindow(pDlg, pDlg->pushButton_setBorderColor, QColor(0, 255, 0)), "Choose opened no colours window for the border");
            } else {
                typeInto(borderColour(pDlg), qsl("#00ff00"), Qt::Key_Return);
            }
            QCOMPARE(borderColour(pDlg)->text(), qsl("#00FF00"));
            for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
                QVERIFY2(room(roomId)->mBorderColor == borderColour(pDlg)->colour(), qPrintable(qsl("a colour %1 did not reach room %2").arg(way).arg(roomId)));
            }

            typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
            for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
                QVERIFY2(room(roomId)->mBorderColor == QColor(0, 255, 0, 153),
                         qPrintable(qsl("with a colour %1, an opacity left room %2 at %3").arg(way).arg(roomId).arg(room(roomId)->mBorderColor.name(QColor::HexArgb))));
            }

            pDlg->accept();
            QVERIFY(mEmitted.changeBorderColor);
            QCOMPARE(mEmitted.newBorderColor, QColor(0, 255, 0, 153));
            for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
                QCOMPARE(room(roomId)->mBorderColor, QColor(0, 255, 0, 153));
            }
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

    // Reset decides for every room, whatever opacity was set before it
    void aResetAfterAnOpacitySetClearsEveryRoom()
    {
        auto* pDlg = openDialogOnRoomsThatDifferInBorderColour();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
        pDlg->pushButton_resetBorderColor->click();
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QVERIFY2(!room(roomId)->mBorderColor.isValid(), qPrintable(qsl("Reset left room %1 at %2").arg(roomId).arg(room(roomId)->mBorderColor.name(QColor::HexArgb))));
        }

        pDlg->accept();
        QVERIFY(mEmitted.changeBorderColor);
        QVERIFY(!mEmitted.newBorderColor.isValid());
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QVERIFY(!room(roomId)->mBorderColor.isValid());
        }
    }

    // One room's colour is the one the field shows, so an opacity set for it gives it that colour, the map's included
    void anOpacitySetForOneRoomGivesItTheColourShown()
    {
        for (const bool ownColour : {true, false}) {
            buildMap();
            mp2dMap->mAreaID = mAreaId;
            if (ownColour) {
                room(scmFirstRoom)->mBorderColor = firstOwnBorder();
            }
            auto* pDlg = openDialogOn({scmFirstRoom});
            connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
            pDlg->show();
            QVERIFY(QTest::qWaitForWindowExposed(pDlg));
            const QColor shown = ownColour ? firstOwnBorder() : mpHost->mRoomBorderColor;
            QCOMPARE(borderColour(pDlg)->colour(), shown);

            typeInto(borderOpacity(pDlg), qsl("60"), Qt::Key_Return);
            QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(shown, 153));
            pDlg->accept();
            QVERIFY2(mEmitted.changeBorderColor, "an opacity set for one room was not reported as its border colour");
            QCOMPARE(mEmitted.newBorderColor, atAlpha(shown, 153));
            QCOMPARE(room(scmFirstRoom)->mBorderColor, atAlpha(shown, 153));
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

    // Rooms with different symbols show no one colour, and a code typed for them goes to every one
    void aCodeTypedForRoomsWithDifferentSymbolsReachesThemAll()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pSymbol = symbolColour(pDlg);
        QVERIFY2(pSymbol->text().isEmpty() && pSymbol->placeholderText().isEmpty() && !pSymbol->colour().isValid(), "rooms that differ were shown a colour");
        typeInto(pSymbol, qsl("#0000ff"), Qt::Key_Return);
        QCOMPARE(pSymbol->text(), qsl("#0000FF"));
        pDlg->accept();

        QCOMPARE(room(scmFirstRoom)->mSymbolColor, QColor(Qt::blue));
        QCOMPARE(room(scmSecondRoom)->mSymbolColor, QColor(Qt::blue));
        QCOMPARE(room(scmFirstRoom)->mSymbol, qsl("!"));
        QCOMPARE(room(scmSecondRoom)->mSymbol, qsl("?"));
    }

    // The border controls preview onto the rooms themselves as they are moved,
    // so that the map behind the dialog shows the result; Cancel has to put the
    // rooms back the way it found them.
    void theBorderPreviewIsLiveAndCancelUndoesIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(pDlg->spinBox_borderThickness->value(), 2);

        pDlg->spinBox_borderThickness->setValue(6);
        QCOMPARE(pRoom->mBorderThickness, 6);
        pDlg->pushButton_resetBorderColor->click();
        QVERIFY2(!pRoom->mBorderColor.isValid(), "resetting the border colour did not reach the room");

        pDlg->reject();

        QCOMPARE(pRoom->mBorderThickness, 2);
        QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));
    }

    // Cancel and closing the window both hide the dialog, and the field takes a code still being typed as the focus
    // leaves it; the rooms must still end up as they were
    void cancelOrClosingWithACodeStillTypedLeavesTheBorderAsItWas()
    {
        for (const bool closeTheWindow : {false, true}) {
            const QString way = closeTheWindow ? qsl("closing the window") : qsl("Cancel");
            buildMap();
            TRoom* pRoom = room(scmFirstRoom);
            pRoom->mBorderColor = QColor(Qt::darkGreen);

            auto* pDlg = openDialogOn({scmFirstRoom});
            uiDesign::ColourValue* pBorder = borderColour(pDlg);
            QVERIFY2(focusInTheActiveDialog(pDlg, pBorder), "the border field never had the focus, so it had none to lose");
            pBorder->selectAll();
            QTest::keyClicks(pBorder, qsl("#ff0000"));
            QCOMPARE(pRoom->mBorderColor, QColor(Qt::darkGreen));

            // Called, not clicked: where a click gives the button the focus, the field takes the code before Cancel runs
            if (closeTheWindow) {
                QVERIFY(pDlg->close());
            } else {
                pDlg->reject();
            }
            QVERIFY2(pRoom->mBorderColor == QColor(Qt::darkGreen),
                     qPrintable(qsl("%1 left the room's border %2, the code typed but not taken").arg(way, pRoom->mBorderColor.name(QColor::HexArgb).toUpper())));
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
    }

    // Destroying the dialog, as its map going does, takes the focus from a field still holding an edit, which the
    // field then takes after the dialog's own part is gone; neither the dialog nor the room may see it
    void destroyingTheDialogWithAnEditStillTypedLeavesTheRoomAsItWas()
    {
        const QList<std::pair<QString, QString>> edits{
                {qsl("roomPropertiesBorderColour"), qsl("#ff0000")}, {qsl("roomPropertiesBorderOpacity"), qsl("60")}, {qsl("roomPropertiesSymbolColour"), qsl("#ff0000")}};
        for (const auto& [field, typed] : edits) {
            buildMap();
            TRoom* pRoom = room(scmFirstRoom);
            pRoom->mSymbol = qsl("=");
            pRoom->mSymbolColor = QColor(Qt::magenta);
            pRoom->mBorderColor = QColor(Qt::darkGreen);

            auto* pDlg = openDialogOn({scmFirstRoom});
            auto* pField = pDlg->findChild<QWidget*>(field);
            QVERIFY2(pField && focusInTheActiveDialog(pDlg, pField), qPrintable(qsl("%1 never had the focus, so it had none to lose").arg(field)));
            if (auto* pLineEdit = qobject_cast<QLineEdit*>(pField)) {
                pLineEdit->selectAll();
            } else if (auto* pSpinBox = qobject_cast<QSpinBox*>(pField)) {
                pSpinBox->selectAll();
            }
            QTest::keyClicks(pField, typed);

            delete pDlg;
            QVERIFY2(pRoom->mBorderColor == QColor(Qt::darkGreen),
                     qPrintable(qsl("destroying the dialog with %1 typed in %2 left the room's border %3").arg(typed, field, pRoom->mBorderColor.name(QColor::HexArgb).toUpper())));
            QCOMPARE(pRoom->mSymbolColor, QColor(Qt::magenta));
        }
    }

    void okKeepsThePreviewedBorderAndReportsIt()
    {
        buildMap();
        TRoom* pRoom = room(scmFirstRoom);
        pRoom->mBorderColor = QColor(Qt::darkGreen);
        pRoom->mBorderThickness = 2;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->spinBox_borderThickness->setValue(6);
        pDlg->accept();

        QCOMPARE(pRoom->mBorderThickness, 6);
        QVERIFY(mEmitted.changeBorderThickness);
        QCOMPARE(mEmitted.newBorderThickness, 6);
        QVERIFY2(!mEmitted.changeBorderColor, "the border colour was reported as changed by a thickness edit");
    }

    // Every room in the selection previews, not just the one the dialog read its
    // starting values from
    void theBorderPreviewReachesEveryRoomInTheSelection()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom, scmThirdRoom});
        int previewCount = 0;
        connect(pDlg, &dlgRoomProperties::signal_preview_border, this, [&previewCount](QSet<int>) {
            ++previewCount;
        });

        pDlg->spinBox_borderThickness->setValue(4);

        QCOMPARE(previewCount, 1);
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 4);
        }

        pDlg->reject();
        for (const int roomId : {scmFirstRoom, scmSecondRoom, scmThirdRoom}) {
            QCOMPARE(room(roomId)->mBorderThickness, 0);
        }
    }

    // Clearing the symbol is how a symbol is removed, and an empty field is a
    // decision rather than a "leave alone"
    void clearingTheSymbolFieldIsReportedAsAChange()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->clear();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbol);
        QVERIFY(mEmitted.newSymbol.isEmpty());
    }

    void resettingTheSymbolColourIsReportedAsAnInvalidColour()
    {
        buildMap();
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->pushButton_resetSymbolColor->click();
        pDlg->accept();

        QVERIFY(mEmitted.changeSymbolColor);
        QVERIFY2(!mEmitted.newSymbolColor.isValid(), "a reset symbol colour has to be an invalid QColor so the default is used");
    }

    // The symbol and its colour are two decisions, and a selection that
    // disagrees about the symbol leaves only the first of them undecided: the
    // colour still has to reach every room, and the placeholder standing in for
    // the symbols must not be written over them (#8394)
    void aSymbolColourDecidedForRoomsWithDifferentSymbolsLeavesTheSymbolsAlone()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        room(scmFirstRoom)->mSymbolColor = QColor(Qt::red);
        room(scmSecondRoom)->mSymbolColor = QColor(Qt::red);

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        QCOMPARE(pDlg->comboBox_roomSymbol->currentText(), scmMultipleValues);
        pDlg->pushButton_resetSymbolColor->click();
        pDlg->accept();

        QVERIFY2(!mEmitted.changeSymbol, "the symbols disagree, so the dialog has no symbol to report");
        QVERIFY2(mEmitted.changeSymbolColor, "the colour was reset, which is a decision about the colour alone");
        QCOMPARE(room(scmFirstRoom)->mSymbol, qsl("!"));
        QCOMPARE(room(scmSecondRoom)->mSymbol, qsl("?"));
        QVERIFY(!room(scmFirstRoom)->mSymbolColor.isValid());
        QVERIFY(!room(scmSecondRoom)->mSymbolColor.isValid());
    }

    // The dialog is not modal, so a script can delete a room in the selection
    // while it is open; Cancel must only put back the rooms that are still there
    void cancelAfterARoomIsDeletedRestoresOnlyTheRoomsLeft()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->spinBox_borderThickness->setValue(5);
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 5);

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        pDlg->reject();

        QVERIFY(!room(scmFirstRoom));
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 0);
    }

    void okAfterARoomIsDeletedAppliesOnlyToTheRoomsLeft()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));
        pDlg->spinBox_borderThickness->setValue(3);

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        pDlg->accept();

        QVERIFY(!room(scmFirstRoom));
        QCOMPARE(room(scmSecondRoom)->name, qsl("Renamed"));
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 3);
    }

    // deleteMap() and loading another map free every room the dialog was opened on
    void closingTheDialogAfterTheMapIsClearedTouchesNoRoom()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));
        pDlg->spinBox_borderThickness->setValue(3);

        map()->mapClear();
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY(!room(scmFirstRoom));
        QVERIFY(!room(scmSecondRoom));

        buildMap();
        auto* pSecondDlg = openDialogOn({scmFirstRoom});
        pSecondDlg->spinBox_borderThickness->setValue(4);
        map()->mapClear();
        pSecondDlg->reject();

        QVERIFY(!room(scmFirstRoom));
    }

    void previewAndCancelAfterARoomIdIsReusedLeaveTheNewRoomAlone()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        QVERIFY(map()->addRoom(scmFirstRoom));
        QVERIFY(map()->setRoomArea(scmFirstRoom, mAreaId));
        room(scmFirstRoom)->mBorderThickness = 7;

        pDlg->spinBox_borderThickness->setValue(5);
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 5);
        QVERIFY2(room(scmFirstRoom)->mBorderThickness == 7, "the preview reached a new room that reused the id");

        pDlg->reject();
        QCOMPARE(room(scmSecondRoom)->mBorderThickness, 0);
        QVERIFY2(room(scmFirstRoom)->mBorderThickness == 7, "Cancel restored the old room's border onto a new room that reused the id");
    }

    void okAfterARoomIdIsReusedLeavesTheNewRoomAlone()
    {
        buildMap();
        mp2dMap->mAreaID = mAreaId;
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        connect(pDlg, &dlgRoomProperties::signal_save_symbol, mp2dMap, &T2DMap::slot_setRoomProperties);
        pDlg->lineEdit_name->setText(qsl("Renamed"));

        QVERIFY(roomDB()->removeRoom(scmFirstRoom));
        QVERIFY(map()->addRoom(scmFirstRoom));
        QVERIFY(map()->setRoomArea(scmFirstRoom, mAreaId));
        pDlg->accept();

        QCOMPARE(room(scmSecondRoom)->name, qsl("Renamed"));
        QVERIFY2(room(scmFirstRoom)->name.isEmpty(), "OK renamed a new room that reused the id");
        QCOMPARE(mEmitted.roomIds, QList<int>{scmSecondRoom});
    }

    void rightClickingPastTheColorsOfAnEmptyColorListOffersNothing()
    {
        buildMap();
        map()->mCustomEnvColors.clear();

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, -1, -1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY2(!mColorSelectorOutcome.sawMenu, "there is no color under the cursor to delete");
    }

    void rightClickingPastTheColorsLeavesTheCurrentOneAlone()
    {
        buildMap();
        map()->mCustomEnvColors = {{257, QColor(Qt::red)}, {258, QColor(Qt::blue)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, 0, -1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY2(!mColorSelectorOutcome.clickLandedOnAColor, "the selector is too small to have empty space past its colors");
        QVERIFY2(!mColorSelectorOutcome.sawMenu, "there is no color under the cursor to delete");
        QCOMPARE(mColorSelectorOutcome.colorsLeftInList, 2);
        QCOMPARE(map()->mCustomEnvColors.size(), 2);
    }

    void deletingARoomColorRemovesTheOneRightClicked()
    {
        buildMap();
        map()->mCustomEnvColors = {{257, QColor(Qt::red)}, {258, QColor(Qt::blue)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        rightClickInTheRoomColorSelector(pDlg, 0, 1);

        QVERIFY(mColorSelectorOutcome.sawSelector);
        QVERIFY(mColorSelectorOutcome.clickLandedOnAColor);
        QVERIFY(mColorSelectorOutcome.sawMenu);
        QCOMPARE(mColorSelectorOutcome.colorsLeftInList, 1);
        QVERIFY(map()->mCustomEnvColors.contains(257));
        QVERIFY2(!map()->mCustomEnvColors.contains(258), "the right-clicked color was not deleted");
    }

    void cancellingTheColorSelectorKeepsTheColorPickedBefore()
    {
        buildMap();
        map()->mCustomEnvColors = {{257, QColor(Qt::red)}, {258, QColor(Qt::blue)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY2(clickAColorInTheRoomColorSelector(pDlg, 0, true), "the first click did not pick colour 257");
        QVERIFY2(clickAColorInTheRoomColorSelector(pDlg, 1, false), "the cancelled click did not pick colour 258, so Cancel went untested");
        pDlg->accept();

        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY(mEmitted.changeRoomColor);
        QCOMPARE(mEmitted.roomColorNumber, 257);
    }

    // The preview is a sample of the room as the map draws it, where a symbol
    // of several characters is shrunk to fit the room rather than cut off
    void aSymbolOfSeveralCharactersFitsInsideThePreview()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));

        const auto [contents, ink] = symbolInk(pDlg->label_preview);
        QVERIFY2(ink.isValid(), "nothing of the symbol was drawn");
        QVERIFY2(ink.left() > contents.left() && ink.right() < contents.right(),
                 qPrintable(qsl("the symbol, drawn across x %1 to %2, is cut off by the preview's edges at %3 and %4").arg(ink.left()).arg(ink.right()).arg(contents.left()).arg(contents.right())));
    }

    // Round rooms draw their symbol in the square inside the circle, smaller
    // than in a square room of the same size
    void aSymbolInARoundRoomIsDrawnSmallerThanInASquareOne()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        // Measured over the same circle as the round room, so only the size differs
        const QRect squareInk = symbolInk(pDlg->label_preview, true).ink;

        mpHost->mBubbleMode = true;
        auto restoreSquareRooms = qScopeGuard([this]() {
            mpHost->mBubbleMode = false;
        });
        pDlg->lineEdit_roomSymbol->clear();
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        const QRect roundInk = symbolInk(pDlg->label_preview, true).ink;

        QVERIFY2(squareInk.isValid() && roundInk.isValid(), "nothing of the symbol was drawn");
        QVERIFY2(roundInk.width() < squareInk.width(), qPrintable(qsl("the symbol is %1 pixels wide in a round room and %2 in a square one").arg(roundInk.width()).arg(squareInk.width())));
    }

    // The map fits a symbol to the room whatever point size the symbol font
    // was given, so the preview must not follow that size either
    void theSymbolFontPointSizeDoesNotChangeThePreview()
    {
        buildMap();
        const QFont originalFont = map()->mMapSymbolFont;
        auto restoreFont = qScopeGuard([this, originalFont]() {
            map()->mMapSymbolFont = originalFont;
        });
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("W"));
        const QImage atOriginalSize = pDlg->label_preview->grab().toImage();
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the symbol was drawn");

        for (const int pointSize : {6, 40}) {
            map()->mMapSymbolFont.setPointSize(pointSize);
            pDlg->lineEdit_roomSymbol->clear();
            pDlg->lineEdit_roomSymbol->setText(qsl("W"));
            QVERIFY2(pDlg->label_preview->grab().toImage() == atOriginalSize, qPrintable(qsl("the preview changed when the symbol font was set to %1pt").arg(pointSize)));
        }
    }

    // The preview shows the border the room will have on the map: its own colour and width where it has them, also
    // where the map draws no borders, and the map's where it has none
    void thePreviewDrawsTheRoomsOwnBorder()
    {
        buildMap();
        const bool shownBefore = mpHost->mMapperShowRoomBorders;
        mpHost->mMapperShowRoomBorders = false;
        const QColor own(0, 128, 0);
        room(scmFirstRoom)->mBorderColor = own;
        room(scmFirstRoom)->mBorderThickness = 3;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        const auto edge = [pDlg](const int inFromTheLeft) {
            const QImage image = pDlg->label_preview->grab().toImage();
            return image.pixelColor(QPointF(inFromTheLeft * image.devicePixelRatio(), pDlg->label_preview->height() / 2 * image.devicePixelRatio()).toPoint());
        };
        QCOMPARE(edge(0), own);
        QCOMPARE(edge(2), own);
        QVERIFY2(edge(4) != own, "the border is wider than the room's three steps");

        const QColor changed(0, 0, 200);
        QMetaObject::invokeMethod(pDlg, "slot_borderColorSelected", Q_ARG(QColor, changed));
        QCOMPARE(edge(0), changed);

        pDlg->spinBox_borderThickness->setValue(1);
        QCOMPARE(edge(0), changed);
        QVERIFY2(edge(2) != changed, "the preview kept the old width");

        // With neither a colour nor a width of its own, the room has the map's border, and here the map draws none
        pDlg->pushButton_resetBorderColor->click();
        pDlg->spinBox_borderThickness->setValue(0);
        QVERIFY2(edge(0) != changed, "the preview kept the border that was reset");
        QCOMPARE(edge(0), edge(4));

        mpHost->mMapperShowRoomBorders = true;
        pDlg->lineEdit_roomSymbol->setText(qsl("A"));
        QCOMPARE(edge(0), QColor(mpHost->mRoomBorderColor.rgb()));
        mpHost->mMapperShowRoomBorders = shownBefore;
    }

    // Removing the symbol must not leave the last one showing, as OK would then
    // save a room without the symbol the preview shows
    void clearingTheSymbolClearsThePreview()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(qsl("ABCDE"));
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the symbol was drawn");

        pDlg->lineEdit_roomSymbol->clear();

        QVERIFY2(!symbolInk(pDlg->label_preview).ink.isValid(), "the preview still shows the removed symbol");
        QVERIFY(pDlg->label_preview->pixmap().isNull());
    }

    // The "(Multiple values...)" entry decides no symbol, so it shows none
    void pickingTheMultipleValuesEntryClearsThePreview()
    {
        buildMap();
        room(scmFirstRoom)->mSymbol = qsl("!");
        room(scmSecondRoom)->mSymbol = qsl("?");
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QCOMPARE(pDlg->comboBox_roomSymbol->itemText(0), scmMultipleValues);
        pDlg->comboBox_roomSymbol->setCurrentIndex(1);
        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing of the picked symbol was drawn");

        pDlg->comboBox_roomSymbol->setCurrentIndex(0);

        QVERIFY2(!symbolInk(pDlg->label_preview).ink.isValid(), "the preview still shows the previously picked symbol");
        QVERIFY(pDlg->label_preview->pixmap().isNull());
    }

    // With only the symbol font allowed, the map draws a character that font
    // cannot show as the replacement character, and the preview has to too
    void aCharacterTheFontLacksIsPreviewedAsTheReplacementCharacter()
    {
        buildMap();
        map()->setOnlySymbolFontUsed(true);
        auto restoreFontMerging = qScopeGuard([this]() {
            map()->setOnlySymbolFontUsed(false);
        });
        // Unassigned in Unicode, so the symbol font has no glyph for it
        const QChar unassigned(0x0378);
        QVERIFY(!QFontMetrics(map()->mMapSymbolFont).inFontUcs4(unassigned.unicode()));

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->lineEdit_roomSymbol->setText(QString(QChar::ReplacementCharacter));
        const QImage replacement = pDlg->label_preview->grab().toImage();
        pDlg->lineEdit_roomSymbol->setText(QString(unassigned));
        const QImage missing = pDlg->label_preview->grab().toImage();

        QVERIFY2(symbolInk(pDlg->label_preview).ink.isValid(), "nothing was drawn for the missing character");
        QVERIFY2(missing == replacement, "the missing character was not previewed as the replacement character");
    }

    void userDataOfOneRoomIsListedSortedByKeyWithoutMudletsOwnKeys()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}, {qsl("area"), qsl("Omelos")}, {qsl("room.ui_showName"), qsl("false")}};

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(userDataKeys(pDlg), QStringList({qsl("area"), qsl("zone")}));
        QCOMPARE(userDataValue(pDlg, qsl("zone"))->text(), qsl("Orcland"));
    }

    void okWithoutEditsSendsNoUserData()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
    }

    void anEditedValueIsSentAlone()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}, {qsl("notes"), qsl("old")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        userDataValue(pDlg, qsl("notes"))->setText(qsl("new"));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 1);
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("notes"), qsl("new")}}));
        QCOMPARE(mEmitted.userDataRoomIds, QList<int>({scmFirstRoom}));
    }

    void aRemovedRowRemovesItsKey()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->tableWidget_userData->selectRow(0);
        pDlg->pushButton_removeUserData->click();
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("zone"), std::nullopt}}));
    }

    // An added row with no key, or with a key of Mudlet's own, is left out rather than written
    void anAddedRowIsSentUnlessItsKeyIsEmptyOrMudletsOwn()
    {
        buildMap();

        auto* pDlg = openDialogOn({scmFirstRoom});
        for (const auto& [key, value] : {std::pair{qsl("shop"), qsl("yes")}, std::pair{QString(), qsl("lost")}, std::pair{qsl("room.ui_nameColor"), qsl("red")}}) {
            pDlg->pushButton_addUserData->click();
            const int row = pDlg->tableWidget_userData->rowCount() - 1;
            pDlg->tableWidget_userData->item(row, 0)->setText(key);
            pDlg->tableWidget_userData->item(row, 1)->setText(value);
        }
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("shop"), qsl("yes")}}));
    }

    // Such a row is not saved, so typing its key says so, rather than the row going without a word on OK. The
    // description is an ordinary key, kept with the rest
    void anAddedRowWithAKeyOfTheMapsOwnSaysItWillNotBeSaved()
    {
        buildMap();

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(pDlg->label_userDataWarning->isHidden());
        pDlg->pushButton_addUserData->click();
        QTableWidgetItem* pKey = pDlg->tableWidget_userData->item(pDlg->tableWidget_userData->rowCount() - 1, 0);

        pKey->setText(qsl("description"));
        QVERIFY2(pDlg->label_userDataWarning->isHidden(), qPrintable(pDlg->label_userDataWarning->text()));

        pKey->setText(qsl("room.ui_nameColor"));
        QVERIFY(!pDlg->label_userDataWarning->isHidden());
        QVERIFY2(pDlg->label_userDataWarning->text().contains(qsl("room.ui_")), qPrintable(pDlg->label_userDataWarning->text()));

        pKey->setText(qsl("shop"));
        QVERIFY(pDlg->label_userDataWarning->isHidden());

        pKey->setText(qsl("room.ui_nameColor"));
        pDlg->tableWidget_userData->selectRow(pKey->row());
        pDlg->pushButton_removeUserData->click();
        QVERIFY2(pDlg->label_userDataWarning->isHidden(), "the warning outlived the row it was about");
    }

    void aKeyTypedWithSpacesAroundItIsSavedWithout()
    {
        buildMap();

        auto* pDlg = openDialogOn({scmFirstRoom});
        for (const auto& [key, value] : {std::pair{qsl(" shop "), qsl("yes")}, std::pair{qsl("description "), qsl("A hall.")}}) {
            pDlg->pushButton_addUserData->click();
            const int row = pDlg->tableWidget_userData->rowCount() - 1;
            pDlg->tableWidget_userData->item(row, 0)->setText(key);
            pDlg->tableWidget_userData->item(row, 1)->setText(value);
        }
        QVERIFY2(pDlg->label_userDataWarning->isHidden(), qPrintable(pDlg->label_userDataWarning->text()));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("shop"), qsl("yes")}, {qsl("description"), qsl("A hall.")}}));
    }

    // Only the first row of a key is saved, and one added after it says it will not be
    void anAddedRowRepeatingAKeyIsLeftOutAndSaysSo()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}};
        room(scmSecondRoom)->userData = {{qsl("zone"), qsl("Moonglow")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        for (const auto& [key, value] : {std::pair{qsl("zone"), qsl("a")}, std::pair{qsl("shop"), qsl("yes")}, std::pair{qsl("shop"), qsl("no")}}) {
            pDlg->pushButton_addUserData->click();
            const int row = pDlg->tableWidget_userData->rowCount() - 1;
            pDlg->tableWidget_userData->item(row, 0)->setText(key);
            pDlg->tableWidget_userData->item(row, 1)->setText(value);
        }
        const QString warning = pDlg->label_userDataWarning->text();
        QVERIFY2(!pDlg->label_userDataWarning->isHidden() && warning.contains(qsl("\"zone\"")) && warning.contains(qsl("\"shop\"")), qPrintable(warning));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("shop"), qsl("yes")}}));
    }

    void aKeyTheRoomsDisagreeOnShowsThePlaceholder()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}, {qsl("shop"), qsl("yes")}};
        room(scmSecondRoom)->userData = {{qsl("zone"), qsl("Moonglow")}, {qsl("area"), qsl("Omelos")}};
        room(scmThirdRoom)->userData = {{qsl("area"), qsl("Omelos")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        // A key only some rooms have is as undecided as one they hold different values for
        QCOMPARE(userDataKeys(pDlg), QStringList({qsl("area"), qsl("shop"), qsl("zone")}));
        for (const QString& key : {qsl("area"), qsl("shop"), qsl("zone")}) {
            QCOMPARE(userDataValue(pDlg, key)->text(), scmMultipleValues);
        }
    }

    void okLeavesAnUntouchedMixedKeyAloneAndWritesAnEditedOneToEveryRoom()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}, {qsl("notes"), qsl("a")}};
        room(scmSecondRoom)->userData = {{qsl("zone"), qsl("Moonglow")}, {qsl("notes"), qsl("b")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        QVERIFY2(valueBox(pDlg)->toPlainText().isEmpty(), qPrintable(valueBox(pDlg)->toPlainText()));
        QCOMPARE(valueBox(pDlg)->placeholderText(), scmMultipleValues);
        typeAtTheEndOfTheValue(pDlg, qsl("same"));
        QCOMPARE(userDataValue(pDlg, qsl("notes"))->text(), qsl("same"));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("notes"), qsl("same")}}));
        QCOMPARE(mEmitted.userDataRoomIds, QList<int>({scmFirstRoom, scmSecondRoom}));
    }

    void theMapperAppliesUserDataChangesToEveryRoom()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("zone"), qsl("Orcland")}, {qsl("old"), qsl("x")}};
        room(scmSecondRoom)->userData = {{qsl("zone"), qsl("Moonglow")}};

        mp2dMap->slot_setRoomUserData({{qsl("zone"), qsl("Omelos")}, {qsl("old"), std::nullopt}}, {scmFirstRoom, scmSecondRoom});

        QCOMPARE(room(scmFirstRoom)->userData, (QMap<QString, QString>{{qsl("zone"), qsl("Omelos")}}));
        QCOMPARE(room(scmSecondRoom)->userData, (QMap<QString, QString>{{qsl("zone"), qsl("Omelos")}}));
    }

    // The heading is the room, and its description is one more row of the script data
    void oneRoomShowsItsIdItsGameIdAndItsDescriptionAsARow()
    {
        buildMap();
        QVERIFY(roomDB()->setRoomHash(scmFirstRoom, qsl("17146")));
        room(scmFirstRoom)->userData = {{qsl("description"), qsl("A dim hall.\nCandles line the walls.")}, {qsl("zone"), qsl("Orcland")}};

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->label_roomIdValue->text(), qsl("Room %1").arg(scmFirstRoom));
        QVERIFY2(pDlg->label_roomIdValue->textInteractionFlags().testFlag(Qt::TextSelectableByMouse), "the room's ID cannot be selected to copy it");
        QCOMPARE(pDlg->lineEdit_hash->text(), qsl("17146"));
        QVERIFY(pDlg->lineEdit_hash->isEnabled());
        QCOMPARE(pDlg->findChild<QLabel*>(qsl("roomPropertiesHashCaption"))->text(), qsl("For games that name their rooms with text instead of a number."));
        QVERIFY2(!pDlg->findChild<QWidget*>(qsl("plainTextEdit_description")), "the description still has a field of its own");
        QCOMPARE(userDataKeys(pDlg), QStringList({qsl("description"), qsl("zone")}));
        // A cell shows the first line, the box the whole of it
        QCOMPARE(userDataValue(pDlg, qsl("description"))->text(), qsl("A dim hall.\nCandles line the walls."));
        QVERIFY(chooseRow(pDlg, qsl("description")));
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("A dim hall.\nCandles line the walls."));
    }

    void aSelectionCannotShareAGameIdAndShowsTheirDescriptionsAsMixed()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("description"), qsl("A dim hall.")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        // The list is joined the way the reader's language joins one
        QCOMPARE(pDlg->label_roomIdValue->text(), qsl("2 rooms: %1").arg(pDlg->locale().createSeparatedList({QString::number(scmFirstRoom), QString::number(scmSecondRoom)})));
        QVERIFY(!pDlg->lineEdit_hash->isEnabled());
        QCOMPARE(pDlg->lineEdit_hash->placeholderText(), qsl("One room at a time"));
        QCOMPARE(pDlg->findChild<QLabel*>(qsl("roomPropertiesHashCaption"))->text(), qsl("A hash belongs to one room."));
        QCOMPARE(userDataValue(pDlg, qsl("description"))->text(), scmMultipleValues);
        QVERIFY(chooseRow(pDlg, qsl("description")));
        QVERIFY(valueBox(pDlg)->toPlainText().isEmpty());
        QCOMPARE(valueBox(pDlg)->placeholderText(), scmMultipleValues);
        pDlg->accept();
        QCOMPARE(mEmitted.userDataEmitCount, 0);
        QCOMPARE(mEmitted.hashEmitCount, 0);
    }

    // Many rooms are named only in part, all of them in the tooltip
    void aLargeSelectionNamesTheFirstFewRoomsAndHowManyMore()
    {
        buildMap();
        QList<int> ids{scmFirstRoom, scmSecondRoom, scmThirdRoom};
        for (int roomId = 20; roomId < 25; ++roomId) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            ids << roomId;
        }

        auto* pDlg = openDialogOn(ids);

        const QLocale locale = pDlg->locale();
        QCOMPARE(pDlg->label_roomIdValue->text(), qsl("8 rooms: %1").arg(locale.createSeparatedList({qsl("11"), qsl("12"), qsl("13"), qsl("5 more")})));
        QStringList all;
        for (const int roomId : std::as_const(ids)) {
            all << QString::number(roomId);
        }
        QVERIFY2(pDlg->label_roomIdValue->toolTip().contains(locale.createSeparatedList(all)), qPrintable(pDlg->label_roomIdValue->toolTip()));
    }

    // Typed in and backspaced away again, the mixed value is back to its placeholder: the rooms keep their own
    // descriptions, and a description typed in does reach them all
    void aMixedDescriptionTypedInAndEmptiedAgainLeavesTheRoomsAlone()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("description"), qsl("A dim hall.")}};
        room(scmSecondRoom)->userData = {{qsl("description"), qsl("A bright yard.")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("description")));
        typeAtTheEndOfTheValue(pDlg, qsl("x"));
        QTest::keyClick(valueBox(pDlg), Qt::Key_Backspace);
        QCOMPARE(userDataValue(pDlg, qsl("description"))->text(), scmMultipleValues);
        pDlg->accept();
        QCOMPARE(mEmitted.userDataEmitCount, 0);

        cleanup();
        mEmitted = EmittedProperties();
        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("description")));
        replaceTheValue(pDlg, qsl("A room."));
        pDlg->accept();
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("description"), qsl("A room.")}}));
    }

    // An ordinary row: an edited description is saved, an emptied one kept empty, and Remove is what removes it
    void anEditedDescriptionIsSavedAndARemovedOneRemoved()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("description"), qsl("old")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("description")));
        replaceTheValue(pDlg, qsl("new"));
        pDlg->accept();
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("description"), qsl("new")}}));

        cleanup();
        mEmitted = EmittedProperties();
        pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("description")));
        replaceTheValue(pDlg, QString());
        pDlg->accept();
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("description"), QString()}}));

        cleanup();
        mEmitted = EmittedProperties();
        pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("description")));
        pDlg->pushButton_removeUserData->click();
        pDlg->accept();
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("description"), std::nullopt}}));
    }

    void aChangedGameIdIsSentAndWarnsWhenAnotherRoomHasIt()
    {
        buildMap();
        QVERIFY(roomDB()->setRoomHash(scmSecondRoom, qsl("500")));

        auto* pDlg = openDialogOn({scmFirstRoom});
        const QLabel* pCaption = pDlg->findChild<QLabel*>(qsl("roomPropertiesHashCaption"));
        QVERIFY(pCaption && !pCaption->isHidden());
        pDlg->lineEdit_hash->setText(qsl("500"));
        QVERIFY(!pDlg->label_hashWarning->isHidden());
        QCOMPARE(pDlg->label_hashWarning->text(), qsl("Room %1 has this hash now and will lose it.").arg(scmSecondRoom));
        QVERIFY2(pCaption->isHidden(), "the warning is shown beside the caption rather than in its place");
        pDlg->lineEdit_hash->setText(qsl("501"));
        QVERIFY(pDlg->label_hashWarning->isHidden());
        QVERIFY(!pCaption->isHidden());
        pDlg->accept();

        QCOMPARE(mEmitted.hashEmitCount, 1);
        QCOMPARE(mEmitted.hashRoomId, scmFirstRoom);
        QCOMPARE(mEmitted.newHash, qsl("501"));
    }

    // Handed a room that is already gone, the dialog edits the one left, and that room's own game ID is no conflict
    void theGameIdOfTheOneRoomLeftIsNotTakenForAnotherRoomsId()
    {
        buildMap();
        QVERIFY(roomDB()->setRoomHash(scmFirstRoom, qsl("17146")));
        auto* pDlg = new dlgRoomProperties(mpHost);
        mpDialog = pDlg;
        QHash<QString, int> usedNames;
        QHash<int, int> usedColors;
        QHash<QString, int> usedSymbols;
        QHash<int, int> usedWeights;
        QHash<bool, int> usedLockStatus;
        pDlg->init(usedNames, usedColors, usedSymbols, usedWeights, usedLockStatus, 0, {scmFirstRoom, 9999});
        QVERIFY(pDlg->lineEdit_hash->isEnabled());

        pDlg->lineEdit_hash->setText(qsl("1"));
        pDlg->lineEdit_hash->setText(qsl("17146"));
        QVERIFY2(pDlg->label_hashWarning->isHidden(), qPrintable(pDlg->label_hashWarning->text()));
    }

    // A game's own ID for a room may be the room's number, so the field takes the name scripts know it by, and the
    // caption says which games fill it in
    void theHashFieldIsNamedAndExplainedAsTheRoomsHash()
    {
        buildMap();
        QVERIFY(roomDB()->setRoomHash(scmSecondRoom, qsl("500")));

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(pDlg->label_hash->text(), qsl("Room hash"));
        QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pDlg->lineEdit_hash);
        QVERIFY(pInterface);
        QCOMPARE(pInterface->text(QAccessible::Name), qsl("Room hash"));
        QVERIFY(pDlg->lineEdit_hash->text().isEmpty());
        QCOMPARE(pDlg->lineEdit_hash->placeholderText(), qsl("None"));
        const QLabel* pCaption = pDlg->findChild<QLabel*>(qsl("roomPropertiesHashCaption"));
        QVERIFY(pCaption);
        QCOMPARE(pCaption->text(), qsl("For games that name their rooms with text instead of a number."));
        pDlg->lineEdit_hash->setText(qsl("500"));
        QCOMPARE(pDlg->label_hashWarning->text(), qsl("Room %1 has this hash now and will lose it.").arg(scmSecondRoom));
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QCOMPARE(pDlg->lineEdit_hash->placeholderText(), qsl("One room at a time"));
        QCOMPARE(pDlg->findChild<QLabel*>(qsl("roomPropertiesHashCaption"))->text(), qsl("A hash belongs to one room."));
    }

    void theMapperMovesAGameIdToTheRoomItIsGivenTo()
    {
        buildMap();
        QVERIFY(roomDB()->setRoomHash(scmSecondRoom, qsl("500")));

        mp2dMap->slot_setRoomHash(scmFirstRoom, qsl("500"));

        QCOMPARE(roomDB()->hashToRoomID.value(qsl("500")), scmFirstRoom);
        QCOMPARE(roomDB()->roomIDToHash.value(scmFirstRoom), qsl("500"));
        QVERIFY(!roomDB()->roomIDToHash.contains(scmSecondRoom));
    }

    void theRoomsAreaIsPickedAndOkWithoutAChangeMovesNothing()
    {
        buildMap();
        const int otherArea = roomDB()->addArea(qsl("Other area"));
        QVERIFY(otherArea > 0);

        auto* pDlg = openDialogOn({scmFirstRoom});

        QCOMPARE(pDlg->comboBox_area->currentData().toInt(), mAreaId);
        QVERIFY(pDlg->comboBox_area->findData(otherArea) >= 0);
        pDlg->accept();
        QCOMPARE(mEmitted.areaEmitCount, 0);
    }

    void aSelectionInDifferentAreasStartsOnThePlaceholder()
    {
        buildMap();
        const int otherArea = roomDB()->addArea(qsl("Other area"));
        QVERIFY(map()->setRoomArea(scmSecondRoom, otherArea));

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});

        QCOMPARE(pDlg->comboBox_area->currentText(), scmMultipleValues);
        pDlg->accept();
        QCOMPARE(mEmitted.areaEmitCount, 0);
    }

    void pickingAnotherAreaSendsItForEveryRoom()
    {
        buildMap();
        const int otherArea = roomDB()->addArea(qsl("Other area"));

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        pDlg->comboBox_area->setCurrentIndex(pDlg->comboBox_area->findData(otherArea));
        pDlg->accept();

        QCOMPARE(mEmitted.areaEmitCount, 1);
        QCOMPARE(mEmitted.newAreaId, otherArea);
        QCOMPARE(mEmitted.areaRoomIds, QList<int>({scmFirstRoom, scmSecondRoom}));
    }

    void theMapperMovesTheRoomsIntoTheArea()
    {
        buildMap();
        const int otherArea = roomDB()->addArea(qsl("Other area"));

        mp2dMap->slot_setRoomsArea(otherArea, {scmFirstRoom, scmSecondRoom});

        QCOMPARE(room(scmFirstRoom)->getArea(), otherArea);
        QCOMPARE(room(scmSecondRoom)->getArea(), otherArea);
        QCOMPARE(room(scmThirdRoom)->getArea(), mAreaId);
    }

    // Configure room is not modal, so by its OK the map may have another room selected, or none
    void theMapLandsOnTheMovedRoomsWhateverIsSelectedNow()
    {
        buildMap();
        const int otherArea = roomDB()->addArea(qsl("Other area"));
        QVERIFY(map()->setRoomCoordinates(scmFirstRoom, 20, 5, 0));
        QVERIFY(map()->setRoomCoordinates(scmThirdRoom, -30, 7, 2));
        mp2dMap->mMultiSelectionSet = {scmThirdRoom};

        mp2dMap->slot_setRoomsArea(otherArea, {scmFirstRoom});
        mp2dMap->mMultiSelectionSet.clear();

        QCOMPARE(mp2dMap->mMapCenterX, 20.0);
        QCOMPARE(mp2dMap->mMapCenterY, -5.0);
        QCOMPARE(mp2dMap->mMapCenterZ, 0);
    }

    void theMapperMovesTheRoomsIntoAnAreaThatSoFarHasOnlyAName()
    {
        buildMap();
        // setAreaName() names an area without making it; the first room moved there does
        const int namedArea = 4242;
        QVERIFY(roomDB()->setAreaName(namedArea, qsl("Named only")));
        QVERIFY(!roomDB()->getArea(namedArea));

        mp2dMap->slot_setRoomsArea(namedArea, {scmFirstRoom});

        QCOMPARE(room(scmFirstRoom)->getArea(), namedArea);
    }

    void okKeepsKeysThatAreBlankButAlreadyThere()
    {
        buildMap();
        // setRoomUserData() accepts an empty key, so a room can hold one
        room(scmFirstRoom)->userData = {{QString(), qsl("a")}, {qsl("  "), qsl("b")}, {qsl("zone"), qsl("Orcland")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(userDataKeys(pDlg).size(), 3);
        // nor does a row added and left without a key take the place of the blank one
        pDlg->pushButton_addUserData->click();
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
    }

    // Emptied again, a value the rooms disagreed on is back to its placeholder and the rooms keep theirs
    void aMixedValueTypedInAndEmptiedAgainChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("a")}};
        room(scmSecondRoom)->userData = {{qsl("notes"), qsl("b")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        typeAtTheEndOfTheValue(pDlg, qsl("x"));
        QCOMPARE(userDataValue(pDlg, qsl("notes"))->text(), qsl("x"));
        QTest::keyClick(valueBox(pDlg), Qt::Key_Backspace);
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
        QCOMPARE(userDataValue(pDlg, qsl("notes"))->text(), scmMultipleValues);
    }

    // Once typed over the row holds a value, but emptied when it is chosen again it is still the rooms' differing ones
    void aMixedValueTypedInAndEmptiedInALaterEditChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("a")}, {qsl("zone"), qsl("Orcland")}};
        room(scmSecondRoom)->userData = {{qsl("notes"), qsl("b")}, {qsl("zone"), qsl("Orcland")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        typeAtTheEndOfTheValue(pDlg, qsl("x"));
        QVERIFY(chooseRow(pDlg, qsl("zone")));
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("x"));
        replaceTheValue(pDlg, QString());
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
        QCOMPARE(userDataValue(pDlg, qsl("notes"))->text(), scmMultipleValues);
    }

    void aMixedValueChosenAndLeftAloneChangesNothing()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("a")}};
        room(scmSecondRoom)->userData = {{qsl("notes"), qsl("b")}};

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
        QCOMPARE(userDataValue(pDlg, qsl("notes"))->text(), scmMultipleValues);
    }

    // The box takes what is typed at once, so OK saves it wherever the focus is
    void aValueTypedInTheBoxIsSavedByOk()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("old")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        replaceTheValue(pDlg, qsl("new"));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("notes"), qsl("new")}}));
    }

    // Edited in the box and never in its cell, where a line editor would turn a value of several lines into one
    void aValueCellIsNotEditedInPlace()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("first\nsecond")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QTableWidgetItem* pValue = userDataValue(pDlg, qsl("notes"));
        QVERIFY2(!pValue->flags().testFlag(Qt::ItemIsEditable), "a value cell can be edited in place");
        pDlg->tableWidget_userData->editItem(pValue);
        QVERIFY2(!pDlg->tableWidget_userData->viewport()->findChild<QLineEdit*>(), "the value cell opened an editor");
    }

    // The box shows the chosen row whole and writes what is typed back to that row; at first no row is chosen
    void theValueBoxAndTheTableStayInStep()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("exit north"), qsl("479")}, {qsl("portals"), qsl("512:enter mirror\n513:enter pool")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(valueBox(pDlg)->isReadOnly());
        QVERIFY(valueBox(pDlg)->toPlainText().isEmpty());
        QCOMPARE(valueBox(pDlg)->placeholderText(), qsl("Choose a row to see its value"));
        QCOMPARE(valueLabel(pDlg)->text(), qsl("Value"));
        QVERIFY2(!pDlg->pushButton_removeUserData->isEnabled(), "Remove is offered with no row chosen");

        QVERIFY(chooseRow(pDlg, qsl("portals")));
        QVERIFY(!valueBox(pDlg)->isReadOnly());
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("512:enter mirror\n513:enter pool"));
        QCOMPARE(valueLabel(pDlg)->text(), qsl("Value of <code>portals</code>"));
        QVERIFY(pDlg->pushButton_removeUserData->isEnabled());
        QTextCursor cursor = valueBox(pDlg)->textCursor();
        cursor.movePosition(QTextCursor::End);
        valueBox(pDlg)->setTextCursor(cursor);
        valueBox(pDlg)->insertPlainText(qsl("\n514:jump"));
        QCOMPARE(userDataValue(pDlg, qsl("portals"))->text(), qsl("512:enter mirror\n513:enter pool\n514:jump"));

        QVERIFY(chooseRow(pDlg, qsl("exit north")));
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("479"));
        QCOMPARE(valueLabel(pDlg)->text(), qsl("Value of <code>exit north</code>"));
        QVERIFY(chooseRow(pDlg, qsl("portals")));
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("512:enter mirror\n513:enter pool\n514:jump"));

        // Removed, the chosen row takes the box with it to whichever row is chosen now
        pDlg->pushButton_removeUserData->click();
        QCOMPARE(userDataKeys(pDlg), QStringList{qsl("exit north")});
        const int row = pDlg->tableWidget_userData->currentRow();
        QCOMPARE(valueBox(pDlg)->toPlainText(), row >= 0 ? pDlg->tableWidget_userData->item(row, 1)->text() : QString());
        pDlg->accept();
        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("portals"), std::nullopt}}));
    }

    // Return on a row, or a double-click on its value, takes the reader to the box to edit it, not to OK
    void returnOrADoubleClickOnARowGoesToTheValueBox()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("notes"), qsl("old")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(showTheScriptData(pDlg));
        pDlg->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(pDlg));
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        pDlg->tableWidget_userData->setFocus();
        QTRY_VERIFY(pDlg->tableWidget_userData->hasFocus());
        QTest::keyClick(pDlg->tableWidget_userData, Qt::Key_Return);
        QVERIFY2(pDlg->isVisible(), "Return on a row closed the dialog");
        QTRY_VERIFY(valueBox(pDlg)->hasFocus());

        pDlg->tableWidget_userData->setFocus();
        QTRY_VERIFY(pDlg->tableWidget_userData->hasFocus());
        const QRect cell = pDlg->tableWidget_userData->visualItemRect(userDataValue(pDlg, qsl("notes")));
        // A double-click is the second press of two: the view acts on it only over the row the first one pressed
        QTest::mouseClick(pDlg->tableWidget_userData->viewport(), Qt::LeftButton, Qt::NoModifier, cell.center());
        QTest::mouseDClick(pDlg->tableWidget_userData->viewport(), Qt::LeftButton, Qt::NoModifier, cell.center());
        QTRY_VERIFY(valueBox(pDlg)->hasFocus());
        QCOMPARE(mEmitted.emitCount, 0);
    }

    // A key typed into an added row is taken by Tab, which goes on to its value
    void tabAfterANewKeyGoesToItsValue()
    {
        buildMap();

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(showTheScriptData(pDlg));
        pDlg->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(pDlg));
        pDlg->pushButton_addUserData->click();
        auto* pKeyEditor = pDlg->tableWidget_userData->viewport()->findChild<QLineEdit*>();
        QVERIFY2(pKeyEditor, "Add opened no editor on the new row's key");
        QTRY_VERIFY(pKeyEditor->hasFocus());
        QTest::keyClicks(pKeyEditor, qsl("shop"));
        QTest::keyClick(pKeyEditor, Qt::Key_Tab);
        QTRY_VERIFY(valueBox(pDlg)->hasFocus());
        typeAtTheEndOfTheValue(pDlg, qsl("yes"));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("shop"), qsl("yes")}}));
    }

    // The box holds a value whole however long it is, where a line edit stops at 32767 characters
    void aLongValueIsKeptWhole()
    {
        buildMap();
        const QString longValue(40000, QLatin1Char('a'));
        room(scmFirstRoom)->userData = {{qsl("notes"), longValue}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        QCOMPARE(valueBox(pDlg)->toPlainText().size(), longValue.size());
        typeAtTheEndOfTheValue(pDlg, qsl("b"));
        pDlg->accept();
        // Sizes rather than the values, which a failure would print in full
        QVERIFY(mEmitted.userDataChanges.contains(qsl("notes")));
        const QString saved = mEmitted.userDataChanges.value(qsl("notes")).value_or(QString());
        QCOMPARE(saved.size(), longValue.size() + 1);
        QVERIFY(saved.endsWith(qsl("ab")));
    }

    // Games lay out a description with non-breaking spaces and some send "\r\n", and mapper scripts match it as the
    // game sent it; every value is cared for the same way, and one nobody edited is not written at all
    void theDescriptionRoundTripsItsLineEndingsAndNonBreakingSpaces()
    {
        buildMap();
        const QString nbsp(QChar::Nbsp);
        room(scmFirstRoom)->userData = {
                {qsl("description"), qsl("A%1dim hall.\r\nCandles.").arg(nbsp)}, {qsl("notes"), qsl("Ask%1Ahmet.\r\nTwice.").arg(nbsp)}, {qsl("zone"), qsl("Orc%1land").arg(nbsp)}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("zone")));
        QVERIFY(chooseRow(pDlg, qsl("description")));
        QCOMPARE(valueBox(pDlg)->document()->toRawText(), qsl("A%1dim hall.%2Candles.").arg(nbsp, QChar(QChar::ParagraphSeparator)));
        typeAtTheEndOfTheValue(pDlg, qsl(" Lit."));
        QVERIFY(chooseRow(pDlg, qsl("notes")));
        typeAtTheEndOfTheValue(pDlg, qsl(" Thrice."));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges,
                 (QMap<QString, std::optional<QString>>{{qsl("description"), qsl("A%1dim hall.\r\nCandles. Lit.").arg(nbsp)}, {qsl("notes"), qsl("Ask%1Ahmet.\r\nTwice. Thrice.").arg(nbsp)}}));
    }

    // A JSON list or table stored with nothing between its entries is laid out to be read, in a fixed-width face, and
    // taking the layout out again gives back the very text that was stored
    void aJsonTableIsShownIndentedAndTheLayoutComesOutAgainWhole()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("areaContinents"), trickyJson()}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("areaContinents")));
        const QString shown = valueBox(pDlg)->toPlainText();
        QVERIFY2(shown.startsWith(qsl("{\n  \"name\": \"a {b} [c], d: e\",\n  \"quote\": \"say \\\"hi\\\", {ok}\",\n")), qPrintable(shown));
        QVERIFY2(shown.contains(qsl("\n  \"list\": [\n    1,\n    2.50,\n    -3e2,\n    true,\n    null,\n    {\n      \"x\": []\n    }\n  ],\n")), qPrintable(shown));
        QVERIFY2(shown.endsWith(qsl("\n  \"empty\": {}\n}")), qPrintable(shown));
        QVERIFY2(QFontInfo(valueBox(pDlg)->font()).fixedPitch(), "a JSON table is not shown in a fixed-width face");
        QCOMPARE(valueLabel(pDlg)->text(), qsl("Value of <code>areaContinents</code>, a table of 6"));

        // A line break typed between two entries is layout, so the value is still what was stored
        QTextCursor cursor = valueBox(pDlg)->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::EndOfLine);
        valueBox(pDlg)->setTextCursor(cursor);
        QTest::keyClick(valueBox(pDlg), Qt::Key_Return);
        QCOMPARE(userDataValue(pDlg, qsl("areaContinents"))->text(), trickyJson());
        QVERIFY(jsonWarning(pDlg)->isHidden());
        pDlg->accept();
        QCOMPARE(mEmitted.userDataEmitCount, 0);
    }

    // Opening a JSON value to read it writes nothing back
    void aJsonValueNobodyEditedIsNotWritten()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("ferry rooms"), qsl("[1204,1377,2290]")}, {qsl("indoors"), qsl("y")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("ferry rooms")));
        QCOMPARE(valueBox(pDlg)->toPlainText(), qsl("[\n  1204,\n  1377,\n  2290\n]"));
        QCOMPARE(valueLabel(pDlg)->text(), qsl("Value of <code>ferry rooms</code>, a list of 3"));
        QVERIFY(chooseRow(pDlg, qsl("indoors")));
        QVERIFY(chooseRow(pDlg, qsl("ferry rooms")));
        QCOMPARE(userDataValue(pDlg, qsl("ferry rooms"))->text(), qsl("[1204,1377,2290]"));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataEmitCount, 0);
    }

    // Saved in the shape it was stored in: the layout comes out again, and the keys keep their order and the numbers
    // the way they were written, where writing the table out anew would sort the one and rewrite the other
    void anEditedJsonValueKeepsItsKeyOrderAndNumbers()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("gotoMapping"), qsl("{\"forge\":479,\"bank\":1.50,\"cloister\":4.7e2,\"gallery\":[512,0.10]}")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("gotoMapping")));
        QString edited = valueBox(pDlg)->toPlainText();
        QVERIFY2(edited.contains(qsl("\"forge\": 479")), qPrintable(edited));
        edited.replace(qsl("\"forge\": 479"), qsl("\"forge\": 480"));
        replaceTheValue(pDlg, edited);
        QVERIFY(jsonWarning(pDlg)->isHidden());
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("gotoMapping"), qsl("{\"forge\":480,\"bank\":1.50,\"cloister\":4.7e2,\"gallery\":[512,0.10]}")}}));
    }

    // Once an edit no longer reads as JSON there is no telling the layout from what was typed, so it is saved as typed
    // and the box says so
    void anEditedJsonValueThatNoLongerReadsAsJsonIsSavedAsTyped()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("ferry rooms"), qsl("[1204,1377]")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(chooseRow(pDlg, qsl("ferry rooms")));
        QVERIFY(jsonWarning(pDlg)->isHidden());
        const QString typed = qsl("[\n  1204,\n  1377,\n");
        replaceTheValue(pDlg, typed);
        QVERIFY2(!jsonWarning(pDlg)->isHidden(), "nothing says the value no longer reads as JSON");
        QCOMPARE(jsonWarning(pDlg)->text(), qsl("Not valid JSON. It will be saved as typed."));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("ferry rooms"), typed}}));
    }

    // A table a script wrote with whitespace of its own, or words that only start like one, are shown as stored, and
    // an edit is saved as typed, whitespace and all
    void onlyAJsonListOrTableStoredCompactIsLaidOut()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("spaced"), qsl("{\"a\": 1, \"b\": [2, 3]}")}, {qsl("word"), qsl("[not json")}, {qsl("number"), qsl("42")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        for (const QString& key : {qsl("spaced"), qsl("word"), qsl("number")}) {
            QVERIFY(chooseRow(pDlg, key));
            QCOMPARE(valueBox(pDlg)->toPlainText(), userDataValue(pDlg, key)->text());
            QCOMPARE(valueLabel(pDlg)->text(), qsl("Value of <code>%1</code>").arg(key));
        }
        QVERIFY(chooseRow(pDlg, qsl("spaced")));
        typeAtTheEndOfTheValue(pDlg, qsl(" "));
        pDlg->accept();

        QCOMPARE(mEmitted.userDataChanges, (QMap<QString, std::optional<QString>>{{qsl("spaced"), qsl("{\"a\": 1, \"b\": [2, 3]} ")}}));
    }

    // The key column opens as wide as its keys, never past its share of the table, and is dragged at the header
    void theKeyColumnOpensFittedToItsKeysUpToItsShareAndCanBeDragged()
    {
        buildMap();
        room(scmFirstRoom)->userData = {{qsl("a"), qsl("1")}, {qsl("bb"), qsl("2")}};

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(showTheScriptData(pDlg));
        QTableWidget* pTable = pDlg->tableWidget_userData;
        QHeaderView* pHeader = pTable->horizontalHeader();
        QCOMPARE(pHeader->sectionResizeMode(0), QHeaderView::Interactive);
        QTRY_COMPARE(pTable->columnWidth(0), std::max(keysNeed(pTable), pHeader->sectionSizeHint(0)));
        // The value column takes the rest
        QCOMPARE(pTable->columnWidth(0) + pTable->columnWidth(1), pTable->viewport()->width());
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        buildMap();
        room(scmFirstRoom)->userData = {{QString(200, QLatin1Char('k')), qsl("1")}};
        pDlg = openDialogOn({scmFirstRoom});
        QVERIFY(showTheScriptData(pDlg));
        pTable = pDlg->tableWidget_userData;
        const int share = qRound(pTable->viewport()->width() * 0.45);
        QVERIFY2(keysNeed(pTable) > share, "the long key fits its share, so the cap goes untested");
        QTRY_COMPARE(pTable->columnWidth(0), share);
        QCOMPARE(pTable->horizontalHeader()->sectionResizeMode(0), QHeaderView::Interactive);
    }

    // The table is as tall as its rows and no taller, and empty it keeps one line that says so
    // Choosing a long value shrinks the table and scrolls it; the row that was clicked is the one left chosen, not
    // the row that came to stand under the pointer
    void aClickedRowStaysChosenWhenTheTableGivesWayToItsValue()
    {
        buildMap();
        QMap<QString, QString> data;
        for (int key = 0; key < 8; ++key) {
            data.insert(qsl("key %1").arg(key), qsl("value %1").arg(key));
        }
        QStringList lines;
        for (int line = 0; line < 60; ++line) {
            lines << qsl("Line %1 of a long value.").arg(line);
        }
        const int longRow = 1;
        data.insert(qsl("key %1").arg(longRow), lines.join(QLatin1Char('\n')));
        room(scmFirstRoom)->userData = data;

        auto* pDlg = openDialogOn({scmFirstRoom});
        QVERIFY2(showTheScriptData(pDlg), "the Script data tab did not come on show");
        QTableWidget* pTable = pDlg->tableWidget_userData;
        const int heightBefore = pTable->height();
        const QPoint onTheRow = pTable->visualItemRect(pTable->item(longRow, 1)).center();
        QTest::mouseClick(pTable->viewport(), Qt::LeftButton, Qt::NoModifier, onTheRow);
        QTRY_VERIFY2(pTable->height() < heightBefore, "the table did not give way to the long value");

        QCOMPARE(pTable->currentRow(), longRow);
        const auto selected = pTable->selectionModel()->selectedRows();
        QCOMPARE(selected.size(), 1);
        QCOMPARE(selected.constFirst().row(), longRow);
        QCOMPARE(pTable->visualItemRect(pTable->item(longRow, 1)).center(), onTheRow);

        // A row too far down to stay under the pointer: still the chosen one, and in sight
        auto* pLower = openDialogOn({scmFirstRoom});
        QVERIFY2(showTheScriptData(pLower), "the Script data tab did not come on show");
        pTable = pLower->tableWidget_userData;
        const int lastRow = pTable->rowCount() - 1;
        pTable->item(lastRow, 1)->setText(lines.join(QLatin1Char('\n')));
        const int tall = pTable->height();
        QTest::mouseClick(pTable->viewport(), Qt::LeftButton, Qt::NoModifier, pTable->visualItemRect(pTable->item(lastRow, 1)).center());
        QTRY_VERIFY2(pTable->height() < tall, "the table did not give way to the long value");
        QCOMPARE(pTable->currentRow(), lastRow);
        const auto lowerSelected = pTable->selectionModel()->selectedRows();
        QCOMPARE(lowerSelected.size(), 1);
        QCOMPARE(lowerSelected.constFirst().row(), lastRow);
        QVERIFY2(pTable->viewport()->rect().contains(pTable->visualItemRect(pTable->item(lastRow, 1)).center()), "the chosen row was scrolled out of sight");
    }

    void theTableIsAsTallAsItsRows()
    {
        QList<int> heights;
        int rowHeight = 0;
        for (const int keys : {0, 2, 5}) {
            buildMap();
            QMap<QString, QString> data;
            for (int key = 0; key < keys; ++key) {
                data.insert(qsl("key %1").arg(key), qsl("value"));
            }
            room(scmFirstRoom)->userData = data;

            auto* pDlg = openDialogOn({scmFirstRoom});
            QVERIFY(showTheScriptData(pDlg));
            QTableWidget* pTable = pDlg->tableWidget_userData;
            rowHeight = pTable->verticalHeader()->defaultSectionSize();
            QVERIFY2(pTable->viewport()->height() == std::max(1, keys) * rowHeight,
                     qPrintable(qsl("with %1 rows the table shows %2 pixels of them, at %3 a row").arg(keys).arg(pTable->viewport()->height()).arg(rowHeight)));
            const auto* pEmpty = pDlg->findChild<QLabel*>(qsl("roomPropertiesScriptDataEmpty"));
            QVERIFY(pEmpty);
            QCOMPARE(pEmpty->isVisible(), keys == 0);
            if (!keys) {
                QCOMPARE(pEmpty->text(), qsl("No script has kept anything with this room."));
            }
            heights << pTable->height();
            pDlg->reject();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        QCOMPARE(heights.at(2) - heights.at(1), 3 * rowHeight);

        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QCOMPARE(pDlg->findChild<QLabel*>(qsl("roomPropertiesScriptDataEmpty"))->text(), qsl("No script has kept anything with these rooms."));
    }

    // The weight box is drawn narrow, but keeps the weight's whole range: the dialog saves the weight it shows, so a
    // lower maximum would cut down one a script set
    void theWeightBoxKeepsItsRangeAndAWeightPastFourDigitsSurvivesOk()
    {
        buildMap();
        room(scmFirstRoom)->setWeight(123456);

        auto* pDlg = openDialogOn({scmFirstRoom});
        QCOMPARE(pDlg->spinBox_weight->minimum(), 1);
        QCOMPARE(pDlg->spinBox_weight->maximum(), std::numeric_limits<int>::max());
        QVERIFY2(pDlg->spinBox_weight->specialValueText().isEmpty(), qPrintable(pDlg->spinBox_weight->specialValueText()));
        QCOMPARE(pDlg->spinBox_weight->value(), 123456);
        QVERIFY2(pDlg->spinBox_weight->width() < pDlg->spinBox_weight->sizeHint().width(), "the weight box is as wide as its largest weight");
        pDlg->accept();

        QVERIFY(mEmitted.changeWeight);
        QCOMPARE(mEmitted.newWeight, 123456);
    }

    // Rooms with different names show a placeholder, and the field left empty, or emptied again, renames none of them
    void aSelectionWithDifferentNamesLeftEmptyRenamesNone()
    {
        buildMap();
        room(scmFirstRoom)->name = qsl("Armoury");
        room(scmSecondRoom)->name = qsl("Cellar");

        auto* pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QVERIFY(pDlg->lineEdit_name->text().isEmpty());
        QCOMPARE(pDlg->lineEdit_name->placeholderText(), scmMultipleValues);
        QTest::keyClicks(pDlg->lineEdit_name, qsl("x"));
        QTest::keyClick(pDlg->lineEdit_name, Qt::Key_Backspace);
        pDlg->accept();
        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY2(!mEmitted.changeName, qPrintable(qsl("an empty name field renamed the rooms to '%1'").arg(mEmitted.newName)));

        cleanup();
        mEmitted = EmittedProperties();
        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QTest::keyClicks(pDlg->lineEdit_name, qsl("Hall"));
        pDlg->accept();
        QVERIFY(mEmitted.changeName);
        QCOMPARE(mEmitted.newName, qsl("Hall"));
    }

    // The room colour is one of the map's, known by its number: shown with its swatch, chosen and never typed
    void theRoomColourShowsTheNumberOfTheMapsColour()
    {
        buildMap();
        const QMap<int, QColor> colours = map()->mCustomEnvColors;
        const auto restoreColours = qScopeGuard([this, colours]() {
            map()->mCustomEnvColors = colours;
        });
        map()->mCustomEnvColors = {{300, QColor(240, 240, 200)}, {301, QColor(Qt::blue)}};
        room(scmFirstRoom)->environment = 300;
        room(scmSecondRoom)->environment = 301;

        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        uiDesign::ColourValue* pRoom = roomColour(pDlg);
        QVERIFY(pRoom);
        QVERIFY2(pRoom->isReadOnly(), "the room colour can be typed into");
        QVERIFY(pRoom->text().isEmpty());
        QCOMPARE(pRoom->placeholderText(), qsl("300"));
        QCOMPARE(swatchOf(pRoom), QColor(240, 240, 200));
        QCOMPARE(pRoom->chooseButton(), pDlg->pushButton_setRoomColor);
        QVERIFY2(clickAColorInTheRoomColorSelector(pDlg, 1, true), "the click did not pick colour 301");
        QCOMPARE(pRoom->placeholderText(), qsl("301"));
        pDlg->reject();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        pDlg = openDialogOn({scmFirstRoom, scmSecondRoom});
        QCOMPARE(roomColour(pDlg)->placeholderText(), qsl("Multiple"));
        QVERIFY2(!roomColour(pDlg)->colour().isValid(), "rooms of different colours were shown one");
    }

    // The dialog is not modal and may be out of sight behind the main window; asking for it again brings it back, on
    // the rooms it was opened on, rather than doing nothing
    void askingForTheOpenDialogAgainBringsItBack()
    {
        buildMap();
        mp2dMap->mMultiSelectionSet = {scmFirstRoom};
        const auto clearSelection = qScopeGuard([this]() {
            mp2dMap->mMultiSelectionSet.clear();
        });
        mp2dMap->slot_showPropertiesDialog();
        QList<dlgRoomProperties*> dialogs = mp2dMap->findChildren<dlgRoomProperties*>();
        QCOMPARE(dialogs.size(), 1);
        dlgRoomProperties* pDlg = dialogs.constFirst();
        mpDialog = pDlg;
        QVERIFY(QTest::qWaitForWindowExposed(pDlg));
        pDlg->hide();
        QVERIFY(!pDlg->isVisible());

        mp2dMap->mMultiSelectionSet = {scmSecondRoom};
        mp2dMap->slot_showPropertiesDialog();
        dialogs = mp2dMap->findChildren<dlgRoomProperties*>();
        QCOMPARE(dialogs.size(), 1);
        QCOMPARE(dialogs.constFirst(), pDlg);
        QVERIFY2(pDlg->isVisible(), "asking for the dialog again left it out of sight");
        QVERIFY(QTest::qWaitForWindowActive(pDlg));
        QCOMPARE(pDlg->label_roomIdValue->text(), qsl("Room %1").arg(scmFirstRoom));
    }

    // The dialog opens on the name, so a name typed straight away and Enter rename the room
    void typingANameAndPressingEnterRenamesTheRoom()
    {
        buildMap();
        auto* pDlg = openDialogOn({scmFirstRoom});
        pDlg->show();
        QVERIFY(QTest::qWaitForWindowActive(pDlg));
        QCOMPARE(QApplication::focusWidget(), pDlg->lineEdit_name);

        QTest::keyClicks(QApplication::focusWidget(), qsl("Armoury"));
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        QCOMPARE(mEmitted.emitCount, 1);
        QVERIFY(mEmitted.changeName);
        QCOMPARE(mEmitted.newName, qsl("Armoury"));
    }
};

#include "RoomPropertiesDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(RoomPropertiesDialogTest)

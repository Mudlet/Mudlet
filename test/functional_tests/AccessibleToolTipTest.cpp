/***************************************************************************
 *   Copyright (C) 2026 by Andrew Johnson - andrew@johnson5.net            *
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
 * Qt reports a widget's tooltip, markup and all, as its accessible description
 * when it has none, and screen readers read the tags aloud (#8547).
 * widgetutils::syncAccessibleDescriptionsWithToolTips() gives each widget with
 * a rich tooltip a plain-text description instead. These cases read the
 * description through the widget's QAccessibleInterface, as a screen reader
 * gets it, because the fallback to the tooltip happens there and not in
 * QWidget::accessibleDescription().
 *
 * Run with: ctest -R AccessibleToolTipTest -V
 */

#include <QAccessible>
#include <QAction>
#include <QPushButton>
#include <QToolBar>
#include <QtTest/QtTest>

#include "utils.h"
#include "widgetutils.h"

#include "GroupedTest.h"

class AccessibleToolTipTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { widgetutils::syncAccessibleDescriptionsWithToolTips(); }

    void richToolTipIsReadAsPlainText()
    {
        QPushButton button;
        button.setToolTip(qsl("<p>Save the <b>profile</b> &amp; close it.</p><p>Nothing is sent to the game.</p>"));
        QCOMPARE(description(&button), qsl("Save the profile & close it. Nothing is sent to the game."));
    }

    void descriptionFollowsEachNewToolTip()
    {
        QPushButton button;
        button.setToolTip(utils::richText(qsl("Start logging.")));
        button.setToolTip(utils::richText(qsl("Stop logging.")));
        QCOMPARE(description(&button), qsl("Stop logging."));

        button.setToolTip(QString());
        QCOMPARE(description(&button), QString());
    }

    void plainToolTipIsLeftToQt()
    {
        QPushButton button;
        button.setToolTip(utils::richText(qsl("Rich.")));
        // Not a tag Qt knows, so QToolTip shows this as it is:
        const QString plainToolTip = qsl("Add Group (Ctrl+Shift+N) for <name> & more");
        button.setToolTip(plainToolTip);
        QCOMPARE(button.accessibleDescription(), QString());
        QCOMPARE(description(&button), plainToolTip);
    }

    void descriptionSetElsewhereIsKept()
    {
        QPushButton setFirst;
        setFirst.setAccessibleDescription(qsl("Curated"));
        setFirst.setToolTip(utils::richText(qsl("Tooltip")));
        QCOMPARE(description(&setFirst), qsl("Curated"));

        // The order setupUi() uses for a .ui widget that has both
        QPushButton setAfterToolTip;
        setAfterToolTip.setToolTip(utils::richText(qsl("Tooltip")));
        setAfterToolTip.setAccessibleDescription(qsl("Curated"));
        setAfterToolTip.setToolTip(utils::richText(qsl("Changed tooltip")));
        QCOMPARE(description(&setAfterToolTip), qsl("Curated"));
    }

    void toolBarButtonFollowsItsAction()
    {
        QToolBar toolBar;
        QAction* pAction = toolBar.addAction(qsl("Save"));
        pAction->setToolTip(utils::richText(qsl("Save the profile.")));
        QWidget* pButton = toolBar.widgetForAction(pAction);
        QVERIFY(pButton);
        QCOMPARE(description(pButton), qsl("Save the profile."));

        pAction->setToolTip(utils::richText(qsl("Saving is disabled while a profile loads.")));
        QCOMPARE(description(pButton), qsl("Saving is disabled while a profile loads."));
    }

private:
    static QString description(QWidget* pWidget)
    {
        QAccessibleInterface* pInterface = QAccessible::queryAccessibleInterface(pWidget);
        return pInterface ? pInterface->text(QAccessible::Description) : QString();
    }
};

#include "AccessibleToolTipTest.moc"
MUDLET_GROUPED_TEST_MAIN(AccessibleToolTipTest)

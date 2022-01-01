/***************************************************************************
 *   Copyright (C) 2018, 2020-2021 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
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

/***************************************************************************
 *   This class is entirely concerned with overcoming the inability to     *
 *   modify the text format for individual tabs in a QTabBar widget        *
 *   via stylesheets on an index or tab text basis.                        *
 ***************************************************************************/

#include "TTabBar.h"
#include "pre_guard.h"
#include <QPainter>
#include <QVariant>
#include "post_guard.h"


void TTabBar::setNamedTabState(const QString& tabName, const bool state, QSet<QString>& effect)
{
    bool textIsInATab = false;
    for (int i = 0, total = count(); i < total; ++i) {
        if (tabData(i).toString() == tabName) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return;
    }

    if (state) {
        effect.insert(tabName);
    } else {
        effect.remove(tabName);
    }
}

void TTabBar::setIndexedTabState(const int index, const bool state, QSet<QString>& effect)
{
    if (index < 0 || index >= count()) {
        return;
    }

    if (state) {
        effect.insert(tabData(index).toString());
    } else {
        effect.remove(tabData(index).toString());
    }
}

bool TTabBar::namedTabState(const QString& tabName, const QSet<QString>& effect) const
{
    bool textIsInATab = false;
    for (int i = 0, total = count(); i < total; ++i) {
        if (tabData(i).toString() == tabName) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return false;
    }

    return effect.contains(tabName);
}

bool TTabBar::indexedTabState(const int index, const QSet<QString>& effect) const
{
    if (index < 0 || index >= count()) {
        return false;
    }

    return effect.contains(tabData(index).toString());
}

QSize TTabBar::tabSizeHint(int index) const
{
    if (tabBold(index) || tabItalic(index) || tabUnderline(index)) {
        const QSize s = QTabBar::tabSizeHint(index);
        const QFontMetrics fm(font());
        // Note that this method must use (because it is associated with sizing
        // the text to show) the (possibly Qt modified to include an
        // accelarator) actual tabText and not the profile name that we have
        // stored in the tabData:
        const int w = fm.horizontalAdvance(tabText(index));

        QFont f = font();
        f.setBold(tabBold(index));
        f.setItalic(tabItalic(index));
        f.setUnderline(tabUnderline(index));
        const QFontMetrics bfm(f);

        const int bw = bfm.horizontalAdvance(tabText(index));

        return {s.width() - w + bw, s.height()};
    }
    return QTabBar::tabSizeHint(index);
}

QString TTabBar::tabName(const int index) const
{
    QString tabName{tabData(index).toString()};
    return tabName;
}

int TTabBar::tabIndex(const QString& tabName) const
{
    int index = -1;
    if (tabName.isEmpty()) {
        return index;
    }
    const int total = count();
    while (++index < total) {
        if (!tabData(index).toString().compare(tabName)) {
            return index;
        }
    }
    return -1;
}

void TTabBar::removeTab(int index)
{
    if (index >= 0 && index < count()) {
        setTabBold(index, false);
        setTabItalic(index, false);
        setTabUnderline(index, false);
        QTabBar::removeTab(index);
    }
}

void TTabBar::removeTab(const QString& tabName)
{
    int index = tabIndex(tabName);
    if (index > -1) {
        setTabBold(index, false);
        setTabItalic(index, false);
        setTabUnderline(index, false);
        QTabBar::removeTab(index);
    }
}

QStringList TTabBar::tabNames() const
{
    QStringList results;
    for (int i = 0, total = count(); i < total; ++i) {
        results << tabData(i).toString();
    }

    return results;
}

void TTabBar::applyPrefixToDisplayedText(const QString& tabName, const QString& prefix)
{
    int index = tabIndex(tabName);
    if (index > -1) {
        QTabBar::setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::applyPrefixToDisplayedText(int index, const QString& prefix)
{
    if (index > -1) {
        QTabBar::setTabText(index, qsl("%1%2").arg(prefix, tabData(index).toString()));
    }
}

void TTabBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);
    QStyleOptionTab opt;

    for (int i = 0, total = count(); i < total; ++i)
    {
        QFont font = painter.font();
        initStyleOption(&opt, i);
        painter.save();
        font.setBold(tabBold(i));
        font.setItalic(tabItalic(i));
        font.setUnderline(tabUnderline(i));
        painter.setFont(font);
        painter.drawControl(QStyle::CE_TabBarTabShape, opt);
        painter.drawControl(QStyle::CE_TabBarTabLabel, opt);
        painter.restore();
    }
}

/***************************************************************************
 *   Copyright (C) 2018, 2020 by Stephen Lyons - slysven@virginmedia.com   *
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
#include <QStyleOption>
#include <QPainter>
#include <QVariant>
#include "post_guard.h"

void TStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == QStyle::CE_TabBarTab) {
        QString tabName = mpTabBar->tabData(mpTabBar->tabAt(option->rect.center())).toString();
        QFont font = widget->font();
        bool isStyleChanged = false;
        if (mBoldTabsSet.contains(tabName) || mItalicTabsSet.contains(tabName) || mUnderlineTabsSet.contains(tabName)) {
            painter->save();
            font.setBold(mBoldTabsSet.contains(tabName));
            font.setItalic(mItalicTabsSet.contains(tabName));
            font.setUnderline(mUnderlineTabsSet.contains(tabName));
            isStyleChanged = true;
            painter->setFont(font);
        }

        QProxyStyle::drawControl(element, option, painter, widget);

        if (isStyleChanged) {
            painter->restore();
        }

    } else {
        QProxyStyle::drawControl(element, option, painter, widget);
    }
}

void TStyle::setNamedTabState(const QString& text, const bool state, QSet<QString>& effect)
{
    bool textIsInATab = false;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        if (mpTabBar->tabData(i).toString() == text) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return;
    }

    if (state) {
        effect.insert(text);
    } else {
        effect.remove(text);
    }
}

void TStyle::setIndexedTabState(const int index, const bool state, QSet<QString>& effect)
{
    if (index < 0 || index >= mpTabBar->count()) {
        return;
    }

    if (state) {
        effect.insert(mpTabBar->tabData(index).toString());
    } else {
        effect.remove(mpTabBar->tabData(index).toString());
    }
}

bool TStyle::namedTabState(const QString& text, const QSet<QString>& effect) const
{
    bool textIsInATab = false;
    for (int i = 0, total = mpTabBar->count(); i < total; ++i) {
        if (mpTabBar->tabData(i).toString() == text) {
            textIsInATab = true;
            break;
        }
    }

    if (!textIsInATab) {
        return false;
    }

    return effect.contains(text);
}

bool TStyle::indexedTabState(const int index, const QSet<QString>& effect) const
{
    if (index < 0 || index >= mpTabBar->count()) {
        return false;
    }

    return effect.contains(mpTabBar->tabData(index).toString());
}

QSize TTabBar::tabSizeHint(int index) const
{
    if (mStyle.tabBold(index) || mStyle.tabItalic(index) || mStyle.tabUnderline(index)) {
        const QSize s = QTabBar::tabSizeHint(index);
        const QFontMetrics fm(font());
        // Note that this method must use (because it is associated with sizing
        // the text to show) the (possibly Qt modified to include an
        // accelarator) actual tabText and not the profile name that we have
        // stored in the tabData:
        const int w = fm.width(tabText(index));

        QFont f = font();
        f.setBold(mStyle.tabBold(index));
        f.setItalic(mStyle.tabItalic(index));
        f.setUnderline(mStyle.tabUnderline(index));
        const QFontMetrics bfm(f);

        const int bw = bfm.width(tabText(index));

        return {s.width() - w + bw, s.height()};
    } else {
        return QTabBar::tabSizeHint(index);
    }
}

QString& TTabBar::tabName(const int index) const
{
    QString result{tabData(index).toString()};
    return result;
}

int TTabBar::tabIndex(const QString& name) const
{
    int index = -1;
    if (name.isEmpty()) {
        return index;
    }
    const int total = count();
    while (++index < total) {
        if (!tabData(index).toString().compare(name)) {
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

void TTabBar::removeTab(const QString& name)
{
    int index = tabIndex(name);
    if (index > -1) {
        setTabBold(index, false);
        setTabItalic(index, false);
        setTabUnderline(index, false);
        QTabBar::removeTab(index);
    }
}

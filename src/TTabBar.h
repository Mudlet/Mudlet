#ifndef TTABBAR_H
#define TTABBAR_H

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

#include "pre_guard.h"
#include <QProxyStyle>
#include <QSet>
#include <QString>
#include <QTabBar>
#include "post_guard.h"

class TStyle : public QProxyStyle
{
public:
    TStyle(QTabBar * bar) : mpTabBar(bar) {}
    ~TStyle() {}

    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = Q_NULLPTR) const;
    void setTabBold(const QString& tabName, const bool state) { setNamedTabState(tabName, state, mBoldTabsSet); }
    void setTabBold(const int index, const bool state) { setIndexedTabState(index, state, mBoldTabsSet); }
    void setTabItalic(const QString& tabName, const bool state) { setNamedTabState(tabName, state, mItalicTabsSet); }
    void setTabItalic(const int index, const bool state) { setIndexedTabState(index, state, mItalicTabsSet); }
    void setTabUnderline(const QString& tabName, const bool state) {setNamedTabState(tabName, state, mUnderlineTabsSet); }
    void setTabUnderline(const int index, const bool state) { setIndexedTabState(index, state, mUnderlineTabsSet); }
    bool tabBold(const QString& tabName) const { return namedTabState(tabName, mBoldTabsSet); }
    bool tabBold(const int index) const { return indexedTabState(index, mBoldTabsSet); }
    bool tabItalic(const QString& tabName) const { return namedTabState(tabName, mItalicTabsSet); }
    bool tabItalic(const int index) const { return indexedTabState(index, mItalicTabsSet); }
    bool tabUnderline(const QString& tabName) const { return namedTabState(tabName, mUnderlineTabsSet); }
    bool tabUnderline(const int index) const { return indexedTabState(index, mUnderlineTabsSet); }

private:
    bool indexedTabState(int, const QSet<QString>&) const;
    bool namedTabState(const QString&, const QSet<QString>&) const;
    void setNamedTabState(const QString&, bool, QSet<QString>&);
    void setIndexedTabState(int, bool, QSet<QString>&);


    QTabBar * mpTabBar;
    QSet<QString> mBoldTabsSet;
    QSet<QString> mItalicTabsSet;
    QSet<QString> mUnderlineTabsSet;
};

class TTabBar : public QTabBar
{
public:
    TTabBar(QWidget * parent)
    : QTabBar(parent)
    , mStyle(qobject_cast<QTabBar*>(this))
    {
        setStyle(&mStyle);
    }
    ~TTabBar() {}

    QSize tabSizeHint(int index) const;
    void setTabBold(const QString& tabName, const bool state) {mStyle.setTabBold(tabName, state); }
    void setTabBold(const int index, const bool state) {mStyle.setTabBold(index, state); }
    void setTabItalic(const QString& tabName, const bool state) {mStyle.setTabItalic(tabName, state); }
    void setTabItalic(const int index, const bool state) {mStyle.setTabItalic(index, state); }
    void setTabUnderline(const QString& tabName, const bool state) {mStyle.setTabUnderline(tabName, state); }
    void setTabUnderline(const int index, const bool state) {mStyle.setTabUnderline(index, state); }
    bool tabBold(const QString& tabName) const {return mStyle.tabBold(tabName);}
    bool tabBold(const int index) const {return mStyle.tabBold(index);}
    bool tabItalic(const QString& tabName) const {return mStyle.tabItalic(tabName);}
    bool tabItalic(const int index) const {return mStyle.tabItalic(index);}
    bool tabUnderline(const QString& tabName) const {return mStyle.tabUnderline(tabName);}
    bool tabUnderline(const int index) const {return mStyle.tabUnderline(index);}
    QString& tabName(const int) const;
    int tabIndex(const QString&) const;
    void removeTab(const QString&);
    void removeTab(int);

private:
    // This instance of TStyle needs a pointer to a QTabBar on instantiation:
    TStyle mStyle;
};

#endif // TTABBAR_H

#ifndef MUDLET_SETTINGSTESTHELPER_H
#define MUDLET_SETTINGSTESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
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

#include <QDir>
#include <QListWidget>
#include <QScrollArea>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QWidget>

#include "mudlet.h"

// What the eight settings tests each used to define for themselves. Free inline
// functions rather than a QObject on purpose: a header is not listed in
// test/functional_tests/CMakeLists.txt, so nothing would run moc over it and a
// Q_OBJECT here would fail to link.
//
// Nothing here may use QVERIFY or QFAIL - both expand to a bare return, so they
// are ill-formed in a function returning anything. The object names are the
// dialog's test interface, documented in docs/settings-redesign.md; spelling
// each of them once is most of the point of this file.
namespace TestSettings {

// Comfortably past the 400ms debounce even on a loaded sanitiser build, and
// what a case waits out to say that no apply happened at all
inline constexpr int scmQuietWindow = 1500;
inline constexpr int scmApplyTimeout = 10000;

inline void deleteProfileDirectory(const QString& profileName)
{
    QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

// The apply may already have run inside the interaction that scheduled it, and
// QSignalSpy::wait() only returns for a signal arriving while it is waiting
inline bool waitForApply(QSignalSpy& spy)
{
    return !spy.isEmpty() || spy.wait(scmApplyTimeout);
}

inline QListWidget* sidebar(const QWidget* pDialog)
{
    return pDialog->findChild<QListWidget*>(qsl("settingsCategoryList"));
}

inline QStackedWidget* stack(const QWidget* pDialog)
{
    return pDialog->findChild<QStackedWidget*>(qsl("settingsStack"));
}

// A category page, or a subpage under its "category_sub" key
inline QScrollArea* pageOf(const QWidget* pDialog, const QString& key)
{
    return pDialog->findChild<QScrollArea*>(qsl("settingsPage_%1").arg(key));
}

// The sidebar row a category is on, or -1 if the sidebar has none
inline int rowOf(const QWidget* pDialog, const QString& key)
{
    QListWidget* pList = sidebar(pDialog);
    if (!pList) {
        return -1;
    }
    for (int row = 0, rows = pList->count(); row < rows; ++row) {
        if (pList->item(row)->data(Qt::UserRole).toString() == key) {
            return row;
        }
    }
    return -1;
}

} // namespace TestSettings

#endif // MUDLET_SETTINGSTESTHELPER_H

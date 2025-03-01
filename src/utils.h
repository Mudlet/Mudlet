#ifndef MUDLET_UTILS_H
#define MUDLET_UTILS_H

/***************************************************************************
 *   Copyright (C) 2021 by Vadim Peretokin - vperetokin@hey.com            *
 *   Copyright (C) 2021, 2023 by Stephen Lyons - slysven@virginmedia.com   *
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
#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QEnterEvent>
#endif
#include <QDir>
#include <QString>
#include "post_guard.h"

#define qsl(s) QStringLiteral(s)

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// QMultiMapIterator replaced QMapIterator as iterator for QMultiMap in Qt6
template <typename T1, typename T2>
using QMultiMapIterator = QMapIterator<T1, T2>;

template <typename T1, typename T2>
using QMutableMultiMapIterator = QMutableMapIterator<T1, T2>;

// QEvent was derived into a specific QEnterEvent in Qt6
using TEnterEvent = QEvent;
#else
using TEnterEvent = QEnterEvent;
#endif

class utils
{
public:
    // This construct will be very useful for formatting tooltips and by
    // defining a static function/method here we can save using the same
    // qsl all over the place:
    static QString richText(const QString& text) { return qsl("<p>%1</p>").arg(text); }

    // Return a new QString with path made absolute, resolved against base and cleaned if it was relative
    // Returns path unchanged if it was already absolute or an empty string
    static QString pathResolveRelative(const QString& path, const QString& base)
    {
        if (path.size() == 0) {
            return path;
        }
        if (QDir::isAbsolutePath(path)) {
            return path;
        }
        return QDir::cleanPath(base + "/" + path);
    }
};

#endif // UPDATER_H

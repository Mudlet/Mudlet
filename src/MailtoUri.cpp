/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "MailtoUri.h"

#include <QStringList>
#include <QUrl>

#include "utils.h"

QString MailtoUri::withOnlyMessageFields(const QString& uri)
{
    if (QUrl(uri).scheme().toLower() != qsl("mailto")) {
        return uri;
    }

    const QString withoutFragment = uri.left(uri.indexOf(QLatin1Char('#')));
    const qsizetype queryStart = withoutFragment.indexOf(QLatin1Char('?'));
    if (queryStart < 0) {
        return withoutFragment;
    }

    QStringList kept;
    for (const QString& field : withoutFragment.mid(queryStart + 1).split(QLatin1Char('&'))) {
        const QString name = QUrl::fromPercentEncoding(field.left(field.indexOf(QLatin1Char('='))).toUtf8()).trimmed().toLower();
        if (name == qsl("to") || name == qsl("cc") || name == qsl("bcc") || name == qsl("subject") || name == qsl("body")) {
            // Some handlers also split a query on these
            QString value = field;
            kept.append(value.replace(QLatin1Char(';'), qsl("%3B")).replace(QLatin1Char('?'), qsl("%3F")));
        }
    }

    const QString address = withoutFragment.left(queryStart);
    return kept.isEmpty() ? address : qsl("%1?%2").arg(address, kept.join(QLatin1Char('&')));
}

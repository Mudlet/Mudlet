#ifndef MUDLET_TMARKDOWNINLINE_H
#define MUDLET_TMARKDOWNINLINE_H

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

#include <QList>
#include <QString>

#include <vector>

class TChar;
struct TMarkdownLink;

namespace TMarkdownInline {
// Takes the inline markup out of text from column from on, and the format entries with it; sets Bold, Italic,
// StrikeOut and Code on what is left. Returns the links, in the columns of the result.
QList<TMarkdownLink> render(QString& text, std::vector<TChar>& format, int from);

// Text longer than this is left as sent
inline constexpr int csmMaxInlineLength = 10000;
} // namespace TMarkdownInline

#endif // MUDLET_TMARKDOWNINLINE_H

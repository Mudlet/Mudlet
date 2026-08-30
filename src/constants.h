#ifndef MUDLET_CONSTANTS_H
#define MUDLET_CONSTANTS_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QString>

#include "utils.h"

// Application-wide values that belong to no one class, kept off the mudlet
// QMainWindow subclass so that core code reading them - the buffer, the map, the
// telnet layer - does not have to name a widget class (#8681). Anything added
// here that needs a widget belongs somewhere else.
namespace constants {

// Equivalent to QDataStream::Qt_5_12. It cannot be taken from Qt itself because
// that enumerator is not defined by the Qt versions Mudlet must still read files
// from, and every serialised Mudlet file is written at this version.
inline constexpr int qDataStreamFormat_5_12 = 18;

// The format of the timestamp shown against console text, as per
// QDateTime::toString(). Localised once during startup, so it is fixed for the
// run but not compile-time constant.
inline QString timeStampFormat = qsl("hh:mm:ss.zzz ");

// Stamped on lines that continue an earlier one, and compared against to decide
// whether a line starts a paragraph - so it has to stay distinct from anything
// timeStampFormat can produce, and the same length, or the timestamp column
// stops lining up.
inline QString blankTimeStamp = qsl("------------ ");

} // namespace constants

#endif // MUDLET_CONSTANTS_H

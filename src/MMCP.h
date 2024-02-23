/***************************************************************************
 *   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _MMCP_H_
#define _MMCP_H_

#include "pre_guard.h"
#include <QFlags>
#include "post_guard.h"

enum MMCPChatCommand {
    NameChange = 1,
    RequestConnections = 2,
    ConnectionList = 3,
    TextEveryone = 4,
    TextPersonal = 5,
    TextGroup = 6,
    Message = 7,
    DoNotDisturb = 8,
    Version = 19,
    FileStart = 20,
    FileDeny = 21,
    FileBlockRequest = 22,
    FileBlock = 23,
    FileEnd = 24,
    FileCancel = 25,
    PingRequest = 26,
    PingResponse = 27,
    PeekConnections = 28,
    PeekList = 29,
    Snoop = 30,
    SnoopData = 31,
    SnoopColor = 32,
    SideChannel = 33,
    ChannelData = 240,
    End = 255
};
Q_DECLARE_FLAGS(MMCPChatCommands, MMCPChatCommand)

namespace AnsiColors {
constexpr char const* RST = "\x1b[0m";
constexpr char const* BLD = "\x1b[1m";
constexpr char const* REV = "\x1b[7m";
constexpr char const* FBLK = "\x1b[30m";
constexpr char const* FRED = "\x1b[31m";
constexpr char const* FGRN = "\x1b[32m";
constexpr char const* FYEL = "\x1b[33m";
constexpr char const* FBLU = "\x1b[34m";
constexpr char const* FMAG = "\x1b[35m";
constexpr char const* FCYN = "\x1b[36m";
constexpr char const* FWHT = "\x1b[37m";
constexpr char const* FBLDGRY = "\x1b[1;30m";
constexpr char const* FBLDRED = "\x1b[1;31m";
constexpr char const* FBLDGRN = "\x1b[1;32m";
constexpr char const* FBLDYEL = "\x1b[1;33m";
constexpr char const* FBLDBLU = "\x1b[1;34m";
constexpr char const* FBLDMAG = "\x1b[1;35m";
constexpr char const* FBLDCYN = "\x1b[1;36m";
constexpr char const* FBLDWHT = "\x1b[1;37m";
constexpr char const* BBLK = "\x1b[40m";
constexpr char const* BRED = "\x1b[41m";
constexpr char const* BGRN = "\x1b[42m";
constexpr char const* BYEL = "\x1b[43m";
constexpr char const* BBLU = "\x1b[44m";
constexpr char const* BMAG = "\x1b[45m";
constexpr char const* BCYN = "\x1b[46m";
constexpr char const* BWHT = "\x1b[47m";
} // namespace AnsiColors

#endif

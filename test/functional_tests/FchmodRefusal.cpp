/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

#include "FchmodRefusal.h"

#include <cerrno>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
bool refuseOwnerOnlyOnOpenFiles = false;
int refusals = 0;
}

void setRefuseOwnerOnlyOnOpenFiles(const bool refuse)
{
    refuseOwnerOnlyOnOpenFiles = refuse;
}

int ownerOnlyRefusals()
{
    return refusals;
}

// Replaces libc's fchmod() for this binary, Qt's calls included, so a case can play a file system
// that refuses owner-only on an open file (leaving it 0644) but takes it by path
extern "C" int fchmod(int fd, mode_t mode) noexcept
{
    if (refuseOwnerOnlyOnOpenFiles && !(mode & (S_IRGRP | S_IROTH))) {
        ::syscall(SYS_fchmod, fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        ++refusals;
        errno = EPERM;
        return -1;
    }
    return static_cast<int>(::syscall(SYS_fchmod, fd, mode));
}

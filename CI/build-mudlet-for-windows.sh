#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2024  by John McKisson - john.mckisson@gmail.com   #
#   Copyright (C) 2023-2025  by Stephen Lyons - slysven@virginmedia.com   #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# Version: 2.1.0    Remove MINGW32 since upstream no longer supports it
#          2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow
#          1.5.0    Change BUILD_TYPE to BUILD_CONFIG to avoid clash with
#                   CI/CB system using same variable
#          1.4.0    Rewrite Makefile to use ccache.exe if available
#          1.3.0    No changes
#          1.2.0    No changes
#          1.1.0    No changes
#          1.0.0    Original version

# Script to build the Mudlet code currently checked out in
# ${GITHUB_WORKSPACE} in a MINGW64 shell

# To be used AFTER setup-windows-sdk.sh has been run; once this has completed
# successfully, package-mudlet-for-windows.sh is run by the workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 3 - Unsupported build type

if ! cmake -G "MinGW Makefiles" -D CMAKE_BUILD_TYPE=Release -D WITH_UPDATER=ON -D WITH_SENTRY=ON ..; then
    echo "cmake failed"
    exit 1
fi

if ! cmake --build . --config Release; then
    echo "cmake --build failed"
    exit 1
fi
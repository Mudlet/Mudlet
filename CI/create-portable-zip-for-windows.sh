#!/bin/bash
###########################################################################
#   Copyright (C) 2026  by Vadim Peretokin - vperetokin@hey.com           #
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

# Zips up the assembled package directory as the portable build.
#
# To be used AFTER package-mudlet-for-windows.sh has been run, and after the
# SignPath signing steps, so the portable ships the same signed binaries the
# installer does.

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2 shell type
# 4 - Package directory is missing or empty

if [ "${MSYSTEM}" != "CLANG64" ]; then
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "CLANG64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

BUILD_CONFIG="release"
GITHUB_WORKSPACE_UNIX_PATH=$(echo "${GITHUB_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g' | sed 's|C:|/c|g')
PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-${BUILD_CONFIG}"

if [ ! -d "${PACKAGE_DIR}" ] || [ -z "$(ls -A "${PACKAGE_DIR}")" ]; then
  echo "ERROR: ${PACKAGE_DIR} is missing or empty - did"
  echo "package-mudlet-for-windows.sh complete successfully?"
  exit 4
fi

# Create portable version
echo "Creating portable ZIP package..."
PORTABLE_ZIP_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/portable-${MSYSTEM}-${BUILD_CONFIG}"
if [ -d "${PORTABLE_ZIP_DIR}" ]; then
  rm -rf "${PORTABLE_ZIP_DIR}"
fi
mkdir -p "${PORTABLE_ZIP_DIR}"

# Copy all packaged files to portable directory
cp -r "${PACKAGE_DIR}"/* "${PORTABLE_ZIP_DIR}/"

# Create portable.txt file to enable portable mode (empty file)
touch "${PORTABLE_ZIP_DIR}/portable.txt"
echo "Created portable.txt file in: ${PORTABLE_ZIP_DIR}/portable.txt"

# Verify portable.txt was created
if [ -f "${PORTABLE_ZIP_DIR}/portable.txt" ]; then
  echo "portable.txt file exists and is ready for packaging"
  ls -la "${PORTABLE_ZIP_DIR}/portable.txt"
else
  echo "ERROR: portable.txt file was not created!"
  exit 1
fi

# Create the portable ZIP archive
cd "${GITHUB_WORKSPACE_UNIX_PATH}" || exit 1
PORTABLE_ZIP_NAME="Mudlet-portable-${MSYSTEM,,}.zip"

echo "Creating ZIP from directory: $(basename "${PORTABLE_ZIP_DIR}")"
echo "Contents of portable directory before ZIP creation:"
ls -la "${PORTABLE_ZIP_DIR}/" | head -20

zip -r "${PORTABLE_ZIP_NAME}" "$(basename "${PORTABLE_ZIP_DIR}")"

# Verify portable.txt is in the ZIP
echo "Verifying portable.txt is in the ZIP:"
unzip -l "${PORTABLE_ZIP_NAME}" | grep portable.txt || echo "WARNING: portable.txt not found in ZIP!"

echo ""
echo "Created portable ZIP: ${GITHUB_WORKSPACE_UNIX_PATH}/${PORTABLE_ZIP_NAME}"
echo ""
echo "   ... create-portable-zip-for-windows.sh shell script finished."
echo ""

exit 0

/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
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

#include "SemVer.h"

#include <QRegularExpression>

namespace dblsqd {

/*!
 * \class SemVer
 * \brief SemVer encapsulates a version according to
 * Semantic Versioning 2.0.
 */

/*!
 * \brief Constructs a new SemVer object from a string.
 */
SemVer::SemVer(const QString& version)
: mOriginal(version)
{
    QRegularExpression rx(getRegExp());
    QRegularExpressionMatch match = rx.match(version);
    if (match.hasMatch()) {
        mMajor = match.captured(1).toInt();
        mMinor = match.captured(2).toInt();
        mPatch = match.captured(3).toInt();
        mPrerelease = match.captured(4);
        mBuild = match.captured(5);
        mValid = true;
    }
}

/*!
 * \brief Returns true if this version is valid according to the SemVer
 * specification. Otherwise returns false.
 */
bool SemVer::isValid() const
{
    return mValid;
}

/*!
 * \brief Compares two SemVer objects.
 *
 * Returns true if the left-hand SemVer object represents a lower version
 * according to the SemVer 2.0 specification.
 * Otherwise returns false.
 * Returns false if one of the SemVer objects does not represent a valid
 * SemVer.
 * \sa isValid()
 */
bool SemVer::operator<(const SemVer& other) const
{
    if (!isValid() || !other.isValid()) {
        return false;
    }

    if (mMajor != other.mMajor) {
        return mMajor < other.mMajor;
    } else if (mMinor != other.mMinor) {
        return mMinor < other.mMinor;
    } else if (mPatch != other.mPatch) {
        return mPatch < other.mPatch;
    } else if (mPrerelease != other.mPrerelease) {
        if (mPrerelease.isEmpty()) {
            return false;
        } else if (other.mPrerelease.isEmpty()) {
            return true;
        }
        return (QString::compare(mPrerelease, other.mPrerelease) < 0);
    }
    // Build metadata is ignored for precedence per SemVer 2.0 spec
    return false;
}

QString SemVer::getRegExp()
{
    QString v = "(0|[1-9]\\d*)";
    QString p = "(?:-((?:0|[1-9A-Za-z][0-9A-Za-z]*)(?:\\.(?:0|[1-9A-Za-z][0-9A-Za-z]*))*))?";
    QString b = "(?:\\+((?:[0-9A-Za-z]*)(?:\\.(?:[0-9A-Za-z][0-9A-Za-z]*))*))?";
    return "^" + v + "\\." + v + "\\." + v + p + b + "$";
}

} // namespace dblsqd

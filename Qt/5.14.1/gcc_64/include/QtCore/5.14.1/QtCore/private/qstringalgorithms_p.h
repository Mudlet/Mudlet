/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSTRINGALGORITHMS_P_H
#define QSTRINGALGORITHMS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qstring.h"
#include "qlocale_p.h"      // for ascii_isspace

QT_BEGIN_NAMESPACE

template <typename StringType> struct QStringAlgorithms
{
    typedef typename StringType::value_type Char;
    typedef typename StringType::size_type size_type;
    typedef typename std::remove_cv<StringType>::type NakedStringType;
    static const bool isConst = std::is_const<StringType>::value;

    static inline bool isSpace(char ch) { return ascii_isspace(ch); }
    static inline bool isSpace(QChar ch) { return ch.isSpace(); }

    // Surrogate pairs are not handled in either of the functions below. That is
    // not a problem because there are no space characters (Zs, Zl, Zp) outside the
    // Basic Multilingual Plane.

    static inline StringType trimmed_helper_inplace(NakedStringType &str, const Char *begin, const Char *end)
    {
        // in-place trimming:
        Char *data = const_cast<Char *>(str.cbegin());
        if (begin != data)
            memmove(data, begin, (end - begin) * sizeof(Char));
        str.resize(end - begin);
        return std::move(str);
    }

    static inline StringType trimmed_helper_inplace(const NakedStringType &, const Char *, const Char *)
    {
        // can't happen
        Q_UNREACHABLE();
        return StringType();
    }

    static inline void trimmed_helper_positions(const Char *&begin, const Char *&end)
    {
        // skip white space from end
        while (begin < end && isSpace(end[-1]))
            --end;
        // skip white space from start
        while (begin < end && isSpace(*begin))
            begin++;
    }

    static inline StringType trimmed_helper(StringType &str)
    {
        const Char *begin = str.cbegin();
        const Char *end = str.cend();
        trimmed_helper_positions(begin, end);

        if (begin == str.cbegin() && end == str.cend())
            return str;
        if (!isConst && str.isDetached())
            return trimmed_helper_inplace(str, begin, end);
        return StringType(begin, end - begin);
    }

    static inline StringType simplified_helper(StringType &str)
    {
        if (str.isEmpty())
            return str;
        const Char *src = str.cbegin();
        const Char *end = str.cend();
        NakedStringType result = isConst || !str.isDetached() ?
                                     StringType(str.size(), Qt::Uninitialized) :
                                     std::move(str);

        Char *dst = const_cast<Char *>(result.cbegin());
        Char *ptr = dst;
        bool unmodified = true;
        forever {
            while (src != end && isSpace(*src))
                ++src;
            while (src != end && !isSpace(*src))
                *ptr++ = *src++;
            if (src == end)
                break;
            if (*src != QChar::Space)
                unmodified = false;
            *ptr++ = QChar::Space;
        }
        if (ptr != dst && ptr[-1] == QChar::Space)
            --ptr;

        int newlen = ptr - dst;
        if (isConst && newlen == str.size() && unmodified) {
            // nothing happened, return the original
            return str;
        }
        result.resize(newlen);
        return result;
    }
};

QT_END_NAMESPACE

#endif // QSTRINGALGORITHMS_P_H

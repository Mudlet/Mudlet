/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#ifndef QSTRINGITERATOR_H
#define QSTRINGITERATOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QStringIterator
{
    QString::const_iterator i, pos, e;
    Q_STATIC_ASSERT((std::is_same<QString::const_iterator, const QChar *>::value));
public:
    explicit QStringIterator(QStringView string, qsizetype idx = 0)
        : i(string.begin()),
          pos(i + idx),
          e(string.end())
    {
    }

    inline explicit QStringIterator(const QChar *begin, const QChar *end)
        : i(begin),
          pos(begin),
          e(end)
    {
    }

    inline explicit QStringIterator(const QChar *begin, int idx, const QChar *end)
        : i(begin),
          pos(begin + idx),
          e(end)
    {
    }

    inline QString::const_iterator position() const
    {
        return pos;
    }

    inline int index() const
    {
        return pos - i;
    }

    inline void setPosition(QString::const_iterator position)
    {
        Q_ASSERT_X(i <= position && position <= e, Q_FUNC_INFO, "position out of bounds");
        pos = position;
    }

    // forward iteration

    inline bool hasNext() const
    {
        return pos < e;
    }

    inline void advance()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY((pos++)->isHighSurrogate())) {
            if (Q_LIKELY(pos != e && pos->isLowSurrogate()))
                ++pos;
        }
    }

    inline void advanceUnchecked()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY((pos++)->isHighSurrogate()))
            ++pos;
    }

    inline uint peekNextUnchecked() const
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY(pos->isHighSurrogate()))
            return QChar::surrogateToUcs4(pos[0], pos[1]);

        return pos->unicode();
    }

    inline uint peekNext(uint invalidAs = QChar::ReplacementCharacter) const
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        if (Q_UNLIKELY(pos->isSurrogate())) {
            if (Q_LIKELY(pos->isHighSurrogate())) {
                const QChar *low = pos + 1;
                if (Q_LIKELY(low != e && low->isLowSurrogate()))
                    return QChar::surrogateToUcs4(*pos, *low);
            }
            return invalidAs;
        }

        return pos->unicode();
    }

    inline uint nextUnchecked()
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        const QChar cur = *pos++;
        if (Q_UNLIKELY(cur.isHighSurrogate()))
            return QChar::surrogateToUcs4(cur, *pos++);
        return cur.unicode();
    }

    inline uint next(uint invalidAs = QChar::ReplacementCharacter)
    {
        Q_ASSERT_X(hasNext(), Q_FUNC_INFO, "iterator hasn't a next item");

        const QChar uc = *pos++;
        if (Q_UNLIKELY(uc.isSurrogate())) {
            if (Q_LIKELY(uc.isHighSurrogate() && pos < e && pos->isLowSurrogate()))
                return QChar::surrogateToUcs4(uc, *pos++);
            return invalidAs;
        }

        return uc.unicode();
    }

    // backwards iteration

    inline bool hasPrevious() const
    {
        return pos > i;
    }

    inline void recede()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY((--pos)->isLowSurrogate())) {
            const QChar *high = pos - 1;
            if (Q_LIKELY(high != i - 1 && high->isHighSurrogate()))
                --pos;
        }
    }

    inline void recedeUnchecked()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY((--pos)->isLowSurrogate()))
            --pos;
    }

    inline uint peekPreviousUnchecked() const
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY(pos[-1].isLowSurrogate()))
            return QChar::surrogateToUcs4(pos[-2], pos[-1]);
        return pos[-1].unicode();
    }

    inline uint peekPrevious(uint invalidAs = QChar::ReplacementCharacter) const
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        if (Q_UNLIKELY(pos[-1].isSurrogate())) {
            if (Q_LIKELY(pos[-1].isLowSurrogate())) {
                const QChar *high = pos - 2;
                if (Q_LIKELY(high != i - 1 && high->isHighSurrogate()))
                    return QChar::surrogateToUcs4(*high, pos[-1]);
            }
            return invalidAs;
        }

        return pos[-1].unicode();
    }

    inline uint previousUnchecked()
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        const QChar cur = *--pos;
        if (Q_UNLIKELY(cur.isLowSurrogate()))
            return QChar::surrogateToUcs4(*--pos, cur);
        return cur.unicode();
    }

    inline uint previous(uint invalidAs = QChar::ReplacementCharacter)
    {
        Q_ASSERT_X(hasPrevious(), Q_FUNC_INFO, "iterator hasn't a previous item");

        const QChar uc = *--pos;
        if (Q_UNLIKELY(uc.isSurrogate())) {
            if (Q_LIKELY(uc.isLowSurrogate() && pos > i && pos[-1].isHighSurrogate()))
                return QChar::surrogateToUcs4(*--pos, uc);
            return invalidAs;
        }

        return uc.unicode();
    }
};

QT_END_NAMESPACE

#endif // QSTRINGITERATOR_H

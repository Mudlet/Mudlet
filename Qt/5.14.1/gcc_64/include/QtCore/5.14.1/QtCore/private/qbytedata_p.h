/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QBYTEDATA_P_H
#define QBYTEDATA_P_H

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
#include <qbytearray.h>

QT_BEGIN_NAMESPACE

// this class handles a list of QByteArrays. It is a variant of QRingBuffer
// that avoid malloc/realloc/memcpy.
class QByteDataBuffer
{
private:
    QList<QByteArray> buffers;
    qint64 bufferCompleteSize;
    qint64 firstPos;
public:
    QByteDataBuffer() : bufferCompleteSize(0), firstPos(0)
    {
    }

    ~QByteDataBuffer()
    {
        clear();
    }

    static inline void popFront(QByteArray &ba, qint64 n)
    {
        ba = QByteArray(ba.constData() + n, ba.size() - n);
    }

    inline void squeezeFirst()
    {
        if (!buffers.isEmpty() && firstPos > 0) {
            popFront(buffers.first(), firstPos);
            firstPos = 0;
        }
    }

    inline void append(const QByteDataBuffer& other)
    {
        if (other.isEmpty())
            return;

        buffers.append(other.buffers);
        bufferCompleteSize += other.byteAmount();

        if (other.firstPos > 0)
            popFront(buffers[bufferCount() - other.bufferCount()], other.firstPos);
    }


    inline void append(const QByteArray& bd)
    {
        if (bd.isEmpty())
            return;

        buffers.append(bd);
        bufferCompleteSize += bd.size();
    }

    inline void prepend(const QByteArray& bd)
    {
        if (bd.isEmpty())
            return;

        squeezeFirst();

        buffers.prepend(bd);
        bufferCompleteSize += bd.size();
    }

    // return the first QByteData. User of this function has to free() its .data!
    // preferably use this function to read data.
    inline QByteArray read()
    {
        squeezeFirst();
        bufferCompleteSize -= buffers.first().size();
        return buffers.takeFirst();
    }

    // return everything. User of this function has to free() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray readAll()
    {
        return read(byteAmount());
    }

    // return amount. User of this function has to free() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray read(qint64 amount)
    {
        amount = qMin(byteAmount(), amount);
        QByteArray byteData;
        byteData.resize(amount);
        read(byteData.data(), byteData.size());
        return byteData;
    }

    // return amount bytes. User of this function has to free() its .data!
    // avoid to use this, it will memcpy.
    qint64 read(char* dst, qint64 amount)
    {
        amount = qMin(amount, byteAmount());
        qint64 originalAmount = amount;
        char *writeDst = dst;

        while (amount > 0) {
            const QByteArray &first = buffers.first();
            qint64 firstSize = first.size() - firstPos;
            if (amount >= firstSize) {
                // take it completely
                bufferCompleteSize -= firstSize;
                amount -= firstSize;
                memcpy(writeDst, first.constData() + firstPos, firstSize);
                writeDst += firstSize;
                firstPos = 0;
                buffers.takeFirst();
            } else {
                // take a part of it & it is the last one to take
                bufferCompleteSize -= amount;
                memcpy(writeDst, first.constData() + firstPos, amount);
                firstPos += amount;
                amount = 0;
            }
        }

        return originalAmount;
    }

    inline char getChar()
    {
        char c;
        read(&c, 1);
        return c;
    }

    inline void clear()
    {
        buffers.clear();
        bufferCompleteSize = 0;
        firstPos = 0;
    }

    // The byte count of all QByteArrays
    inline qint64 byteAmount() const
    {
        return bufferCompleteSize;
    }

    // the number of QByteArrays
    inline int bufferCount() const
    {
        return buffers.length();
    }

    inline bool isEmpty() const
    {
        return byteAmount() == 0;
    }

    inline qint64 sizeNextBlock() const
    {
        if(buffers.isEmpty())
            return 0;
        else
            return buffers.first().size() - firstPos;
    }

    inline QByteArray& operator[](int i)
    {
        if (i == 0)
            squeezeFirst();

        return buffers[i];
    }

    inline bool canReadLine() const {
        int i = 0;
        if (i < buffers.length()) {
            if (buffers.at(i).indexOf('\n', firstPos) != -1)
                return true;
            ++i;

            for (; i < buffers.length(); i++)
                if (buffers.at(i).contains('\n'))
                    return true;
        }
        return false;
    }
};

QT_END_NAMESPACE

#endif // QBYTEDATA_P_H

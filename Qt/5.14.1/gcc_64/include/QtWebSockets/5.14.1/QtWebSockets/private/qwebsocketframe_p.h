/****************************************************************************
**
** Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebSockets module of the Qt Toolkit.
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

#ifndef QWEBSOCKETFRAME_P_H
#define QWEBSOCKETFRAME_P_H

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

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <limits.h>

#include "qwebsockets_global.h"
#include "qwebsocketprotocol.h"
#include "qwebsocketprotocol_p.h"

QT_BEGIN_NAMESPACE

class QIODevice;

const quint64 MAX_FRAME_SIZE_IN_BYTES = INT_MAX - 1;
const quint64 MAX_MESSAGE_SIZE_IN_BYTES = INT_MAX - 1;

class Q_AUTOTEST_EXPORT QWebSocketFrame
{
    Q_DECLARE_TR_FUNCTIONS(QWebSocketFrame)

public:
    QWebSocketFrame();
    QWebSocketFrame(const QWebSocketFrame &other);

    QWebSocketFrame &operator =(const QWebSocketFrame &other);

#ifdef Q_COMPILER_RVALUE_REFS
    QWebSocketFrame(QWebSocketFrame &&other);
    QWebSocketFrame &operator =(QWebSocketFrame &&other);
#endif

    void swap(QWebSocketFrame &other);

    QWebSocketProtocol::CloseCode closeCode() const;
    QString closeReason() const;
    bool isFinalFrame() const;
    bool isControlFrame() const;
    bool isDataFrame() const;
    bool isContinuationFrame() const;
    bool hasMask() const;
    quint32 mask() const;    //returns 0 if no mask
    inline bool rsv1() const { return m_rsv1; }
    inline bool rsv2() const { return m_rsv2; }
    inline bool rsv3() const { return m_rsv3; }
    QWebSocketProtocol::OpCode opCode() const;
    QByteArray payload() const;

    void clear();

    bool isValid() const;
    bool isDone() const;

    void readFrame(QIODevice *pIoDevice);

private:
    QWebSocketProtocol::CloseCode m_closeCode;
    QString m_closeReason;
    quint32 m_mask;
    QWebSocketProtocol::OpCode m_opCode;
    quint64 m_length;
    QByteArray m_payload;

    bool m_isFinalFrame;
    bool m_rsv1;
    bool m_rsv2;
    bool m_rsv3;
    bool m_isValid;

    enum ProcessingState
    {
        PS_READ_HEADER,
        PS_READ_PAYLOAD_LENGTH,
        PS_READ_MASK,
        PS_READ_PAYLOAD,
        PS_DISPATCH_RESULT,
        PS_WAIT_FOR_MORE_DATA
    } m_processingState{PS_READ_HEADER};

    ProcessingState readFrameHeader(QIODevice *pIoDevice);
    ProcessingState readFramePayloadLength(QIODevice *pIoDevice);
    ProcessingState readFrameMask(QIODevice *pIoDevice);
    ProcessingState readFramePayload(QIODevice *pIoDevice);

    void setError(QWebSocketProtocol::CloseCode code, const QString &closeReason);
    bool checkValidity();
};

QT_END_NAMESPACE

#endif // QWEBSOCKETFRAME_P_H

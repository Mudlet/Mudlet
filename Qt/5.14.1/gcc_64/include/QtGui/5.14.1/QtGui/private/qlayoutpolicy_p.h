/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QLAYOUTPOLICY_H
#define QLAYOUTPOLICY_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qnamespace.h>

#ifndef QT_NO_DATASTREAM
# include <QtCore/qdatastream.h>
#endif

QT_BEGIN_NAMESPACE


class QVariant;

class Q_GUI_EXPORT QLayoutPolicy
{
    Q_GADGET
    Q_ENUMS(Policy)

public:
    enum PolicyFlag {
        GrowFlag = 1,
        ExpandFlag = 2,
        ShrinkFlag = 4,
        IgnoreFlag = 8
    };

    enum Policy {
        Fixed = 0,
        Minimum = GrowFlag,
        Maximum = ShrinkFlag,
        Preferred = GrowFlag | ShrinkFlag,
        MinimumExpanding = GrowFlag | ExpandFlag,
        Expanding = GrowFlag | ShrinkFlag | ExpandFlag,
        Ignored = ShrinkFlag | GrowFlag | IgnoreFlag
    };

    enum ControlType {
        DefaultType      = 0x00000001,
        ButtonBox        = 0x00000002,
        CheckBox         = 0x00000004,
        ComboBox         = 0x00000008,
        Frame            = 0x00000010,
        GroupBox         = 0x00000020,
        Label            = 0x00000040,
        Line             = 0x00000080,
        LineEdit         = 0x00000100,
        PushButton       = 0x00000200,
        RadioButton      = 0x00000400,
        Slider           = 0x00000800,
        SpinBox          = 0x00001000,
        TabWidget        = 0x00002000,
        ToolButton       = 0x00004000
    };
    Q_DECLARE_FLAGS(ControlTypes, ControlType)

    QLayoutPolicy() : data(0) { }

    QLayoutPolicy(Policy horizontal, Policy vertical, ControlType type = DefaultType)
        : data(0) {
        bits.horPolicy = horizontal;
        bits.verPolicy = vertical;
        setControlType(type);
    }
    Policy horizontalPolicy() const { return static_cast<Policy>(bits.horPolicy); }
    Policy verticalPolicy() const { return static_cast<Policy>(bits.verPolicy); }
    ControlType controlType() const;

    void setHorizontalPolicy(Policy d) { bits.horPolicy = d; }
    void setVerticalPolicy(Policy d) { bits.verPolicy = d; }
    void setControlType(ControlType type);

    Qt::Orientations expandingDirections() const {
        Qt::Orientations result;
        if (verticalPolicy() & ExpandFlag)
            result |= Qt::Vertical;
        if (horizontalPolicy() & ExpandFlag)
            result |= Qt::Horizontal;
        return result;
    }

    void setHeightForWidth(bool b) { bits.hfw = b;  }
    bool hasHeightForWidth() const { return bits.hfw; }
    void setWidthForHeight(bool b) { bits.wfh = b;  }
    bool hasWidthForHeight() const { return bits.wfh; }

    bool operator==(const QLayoutPolicy& s) const { return data == s.data; }
    bool operator!=(const QLayoutPolicy& s) const { return data != s.data; }

    int horizontalStretch() const { return static_cast<int>(bits.horStretch); }
    int verticalStretch() const { return static_cast<int>(bits.verStretch); }
    void setHorizontalStretch(int stretchFactor) { bits.horStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }
    void setVerticalStretch(int stretchFactor) { bits.verStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }

    void transpose();


private:
#ifndef QT_NO_DATASTREAM
    friend QDataStream &operator<<(QDataStream &, const QLayoutPolicy &);
    friend QDataStream &operator>>(QDataStream &, QLayoutPolicy &);
#endif
    QLayoutPolicy(int i) : data(i) { }

    union {
        struct {
            quint32 horStretch : 8;
            quint32 verStretch : 8;
            quint32 horPolicy : 4;
            quint32 verPolicy : 4;
            quint32 ctype : 5;
            quint32 hfw : 1;
            quint32 wfh : 1;
            quint32 padding : 1;   // feel free to use
        } bits;
        quint32 data;
    };
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLayoutPolicy::ControlTypes)

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &, const QLayoutPolicy &);
QDataStream &operator>>(QDataStream &, QLayoutPolicy &);
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QLayoutPolicy &);
#endif

inline void QLayoutPolicy::transpose() {
    Policy hData = horizontalPolicy();
    Policy vData = verticalPolicy();
    int hStretch = horizontalStretch();
    int vStretch = verticalStretch();
    setHorizontalPolicy(vData);
    setVerticalPolicy(hData);
    setHorizontalStretch(vStretch);
    setVerticalStretch(hStretch);
}

QT_END_NAMESPACE

#endif // QLAYOUTPOLICY_H

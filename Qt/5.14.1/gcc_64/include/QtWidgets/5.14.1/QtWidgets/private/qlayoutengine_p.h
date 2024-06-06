/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QLAYOUTENGINE_P_H
#define QLAYOUTENGINE_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qlayoutitem.h"
#include "QtWidgets/qstyle.h"

QT_BEGIN_NAMESPACE

template <typename T> class QVector;

struct QLayoutStruct
{
    inline void init(int stretchFactor = 0, int minSize = 0) {
        stretch = stretchFactor;
        minimumSize = sizeHint = minSize;
        maximumSize = QLAYOUTSIZE_MAX;
        expansive = false;
        empty = true;
        spacing = 0;
    }

    int smartSizeHint() {
        return (stretch > 0) ? minimumSize : sizeHint;
    }
    int effectiveSpacer(int uniformSpacer) const {
        Q_ASSERT(uniformSpacer >= 0 || spacing >= 0);
        return (uniformSpacer >= 0) ? uniformSpacer : spacing;
    }

    // parameters
    int stretch;
    int sizeHint;
    int maximumSize;
    int minimumSize;
    int spacing;
    bool expansive;
    bool empty;

    // temporary storage
    bool done;

    // result
    int pos;
    int size;
};


Q_WIDGETS_EXPORT void qGeomCalc(QVector<QLayoutStruct> &chain, int start, int count,
                            int pos, int space, int spacer = -1);
Q_WIDGETS_EXPORT QSize qSmartMinSize(const QSize &sizeHint, const QSize &minSizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy);
Q_WIDGETS_EXPORT QSize qSmartMinSize(const QWidgetItem *i);
Q_WIDGETS_EXPORT QSize qSmartMinSize(const QWidget *w);
Q_WIDGETS_EXPORT QSize qSmartMaxSize(const QSize &sizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy, Qt::Alignment align = nullptr);
Q_WIDGETS_EXPORT QSize qSmartMaxSize(const QWidgetItem *i, Qt::Alignment align = nullptr);
Q_WIDGETS_EXPORT QSize qSmartMaxSize(const QWidget *w, Qt::Alignment align = nullptr);

Q_WIDGETS_EXPORT int qSmartSpacing(const QLayout *layout, QStyle::PixelMetric pm);

/*
  Modify total maximum (max), total expansion (exp), and total empty
  when adding boxmax/boxexp.

  Expansive boxes win over non-expansive boxes.
  Non-empty boxes win over empty boxes.
*/
static inline void qMaxExpCalc(int & max, bool &exp, bool &empty,
                               int boxmax, bool boxexp, bool boxempty)
{
    if (exp) {
        if (boxexp)
            max = qMax(max, boxmax);
    } else {
        if (boxexp || (empty && (!boxempty || max == 0)))
            max = boxmax;
        else if (empty == boxempty)
            max = qMin(max, boxmax);
    }
    exp = exp || boxexp;
    empty = empty && boxempty;
}

QT_END_NAMESPACE

#endif // QLAYOUTENGINE_P_H

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

#ifndef QSPLITTER_P_H
#define QSPLITTER_P_H

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
#include "private/qframe_p.h"

QT_REQUIRE_CONFIG(splitter);

QT_BEGIN_NAMESPACE

static const uint Default = 2;

class QSplitterLayoutStruct
{
public:
    QRect rect;
    int sizer;
    uint collapsed : 1;
    uint collapsible : 2;
    QWidget *widget;
    QSplitterHandle *handle;

    QSplitterLayoutStruct() : sizer(-1), collapsed(false), collapsible(Default), widget(nullptr), handle(nullptr) {}
    ~QSplitterLayoutStruct() { delete handle; }
    int getWidgetSize(Qt::Orientation orient);
    int getHandleSize(Qt::Orientation orient);
    int pick(const QSize &size, Qt::Orientation orient)
    { return (orient == Qt::Horizontal) ? size.width() : size.height(); }
};

class QSplitterPrivate : public QFramePrivate
{
    Q_DECLARE_PUBLIC(QSplitter)
public:
    QSplitterPrivate() :
#if QT_CONFIG(rubberband)
                         rubberBand(nullptr),
#endif
                         opaque(true), firstShow(true),
                         childrenCollapsible(true), compatMode(false), handleWidth(-1), blockChildAdd(false), opaqueResizeSet(false) {}
    ~QSplitterPrivate();

#if QT_CONFIG(rubberband)
    QPointer<QRubberBand> rubberBand;
#endif
    mutable QList<QSplitterLayoutStruct *> list;
    Qt::Orientation orient;
    bool opaque : 8;
    bool firstShow : 8;
    bool childrenCollapsible : 8;
    bool compatMode : 8;
    int handleWidth;
    bool blockChildAdd;
    bool opaqueResizeSet;

    inline int pick(const QPoint &pos) const
    { return orient == Qt::Horizontal ? pos.x() : pos.y(); }
    inline int pick(const QSize &s) const
    { return orient == Qt::Horizontal ? s.width() : s.height(); }

    inline int trans(const QPoint &pos) const
    { return orient == Qt::Vertical ? pos.x() : pos.y(); }
    inline int trans(const QSize &s) const
    { return orient == Qt::Vertical ? s.width() : s.height(); }

    void init();
    void recalc(bool update = false);
    void doResize();
    void storeSizes();
    void getRange(int index, int *, int *, int *, int *) const;
    void addContribution(int, int *, int *, bool) const;
    int adjustPos(int, int, int *, int *, int *, int *) const;
    bool collapsible(QSplitterLayoutStruct *) const;
    bool collapsible(int index) const
    { return (index < 0 || index >= list.size()) ? true : collapsible(list.at(index)); }
    QSplitterLayoutStruct *findWidget(QWidget *) const;
    void insertWidget_helper(int index, QWidget *widget, bool show);
    QSplitterLayoutStruct *insertWidget(int index, QWidget *);
    void doMove(bool backwards, int pos, int index, int delta,
                bool mayCollapse, int *positions, int *widths);
    void setGeo(QSplitterLayoutStruct *s, int pos, int size, bool allowCollapse);
    int findWidgetJustBeforeOrJustAfter(int index, int delta, int &collapsibleSize) const;
    void updateHandles();
    void setSizes_helper(const QList<int> &sizes, bool clampNegativeSize = false);
    bool shouldShowWidget(const QWidget *w) const;

};

class QSplitterHandlePrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QSplitterHandle)
public:
    QSplitterHandlePrivate() : s(nullptr), orient(Qt::Horizontal), mouseOffset(0), opaq(false), hover(false), pressed(false) {}

    inline int pick(const QPoint &pos) const
    { return orient == Qt::Horizontal ? pos.x() : pos.y(); }

    QSplitter *s;
    Qt::Orientation orient;
    int mouseOffset;
    bool opaq    : 1;
    bool hover   : 1;
    bool pressed : 1;
};

QT_END_NAMESPACE

#endif

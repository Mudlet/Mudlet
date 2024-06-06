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

#ifndef QGRAPHICSPROXYWIDGET_H
#define QGRAPHICSPROXYWIDGET_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qgraphicswidget.h>

QT_REQUIRE_CONFIG(graphicsview);

QT_BEGIN_NAMESPACE

class QGraphicsProxyWidgetPrivate;

class Q_WIDGETS_EXPORT QGraphicsProxyWidget : public QGraphicsWidget
{
    Q_OBJECT
public:
    QGraphicsProxyWidget(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = Qt::WindowFlags());
    ~QGraphicsProxyWidget();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    QRectF subWidgetRect(const QWidget *widget) const;

    void setGeometry(const QRectF &rect) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    enum {
        Type = 12
    };
    int type() const override;

    QGraphicsProxyWidget *createProxyForChildWidget(QWidget *child);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    bool event(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
#endif

#if QT_CONFIG(draganddrop)
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;
#endif

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void grabMouseEvent(QEvent *event) override;
    void ungrabMouseEvent(QEvent *event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QGraphicsSceneWheelEvent *event) override;
#endif

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool focusNextPrevChild(bool next) override;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;

    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;

protected Q_SLOTS:
    QGraphicsProxyWidget *newProxyWidget(const QWidget *);

private:
    Q_DISABLE_COPY(QGraphicsProxyWidget)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QGraphicsProxyWidget)
    Q_PRIVATE_SLOT(d_func(), void _q_removeWidgetSlot())

    friend class QWidget;
    friend class QWidgetPrivate;
    friend class QGraphicsItem;
};

QT_END_NAMESPACE

#endif

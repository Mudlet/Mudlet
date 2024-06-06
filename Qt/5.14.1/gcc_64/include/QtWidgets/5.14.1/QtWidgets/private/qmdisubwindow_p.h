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

#ifndef QMDISUBWINDOW_P_H
#define QMDISUBWINDOW_P_H

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
#include "qmdisubwindow.h"

#include <QStyle>
#include <QStyleOptionTitleBar>
#if QT_CONFIG(menubar)
#include <QMenuBar>
#endif
#if QT_CONFIG(sizegrip)
#include <QSizeGrip>
#endif
#include <QPointer>
#include <QDebug>
#include <private/qwidget_p.h>

QT_REQUIRE_CONFIG(mdiarea);

QT_BEGIN_NAMESPACE

class QVBoxLayout;
class QMouseEvent;

namespace QMdi {
template<typename T>
class ControlElement : public T
{
public:
    ControlElement(QMdiSubWindow *child) : T(child, nullptr)
    {
        Q_ASSERT(child);
        mdiChild = child;
    }

    void *qt_metacast(const char *classname) override
    {
        if (classname && strcmp(classname, "ControlElement") == 0)
            return this;
        return nullptr;
    }

    QPointer<QMdiSubWindow> mdiChild;
};

class ControlContainer : public QObject
{
public:
    ControlContainer(QMdiSubWindow *mdiChild);
    ~ControlContainer();

#if QT_CONFIG(menubar)
    void showButtonsInMenuBar(QMenuBar *menuBar);
    void removeButtonsFromMenuBar(QMenuBar *menuBar = nullptr);
    QMenuBar *menuBar() const { return m_menuBar; }
#endif
    void updateWindowIcon(const QIcon &windowIcon);
    QWidget *controllerWidget() const { return m_controllerWidget; }
    QWidget *systemMenuLabel() const { return m_menuLabel; }

private:
    QPointer<QWidget> previousLeft;
    QPointer<QWidget> previousRight;
#if QT_CONFIG(menubar)
    QPointer<QMenuBar> m_menuBar;
#endif
    QPointer<QWidget> m_controllerWidget;
    QPointer<QWidget> m_menuLabel;
    QPointer<QMdiSubWindow> mdiChild;
};
} // namespace QMdi

class QMdiSubWindowPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMdiSubWindow)
public:
    // Enums and typedefs.
    enum Operation {
        None,
        Move,
        TopResize,
        BottomResize,
        LeftResize,
        RightResize,
        TopLeftResize,
        TopRightResize,
        BottomLeftResize,
        BottomRightResize
    };

    enum ChangeFlag {
        HMove = 0x01,
        VMove = 0x02,
        HResize = 0x04,
        VResize = 0x08,
        HResizeReverse = 0x10,
        VResizeReverse = 0x20
    };

    enum WindowStateAction {
        RestoreAction,
        MoveAction,
        ResizeAction,
        MinimizeAction,
        MaximizeAction,
        StayOnTopAction,
        CloseAction,
        /* Add new states _above_ this line! */
        NumWindowStateActions
    };

    struct OperationInfo {
        uint changeFlags;
        Qt::CursorShape cursorShape;
        QRegion region;
        bool hover;
        OperationInfo(uint changeFlags, Qt::CursorShape cursorShape, bool hover = true)
            : changeFlags(changeFlags),
              cursorShape(cursorShape),
              hover(hover)
        {}
    };

    typedef QMap<Operation, OperationInfo> OperationInfoMap;

    QMdiSubWindowPrivate();

    // Variables.
    QPointer<QWidget> baseWidget;
    QPointer<QWidget> restoreFocusWidget;
    QPointer<QMdi::ControlContainer> controlContainer;
#if QT_CONFIG(sizegrip)
    QPointer<QSizeGrip> sizeGrip;
#endif
#if QT_CONFIG(rubberband)
    QRubberBand *rubberBand;
#endif
    QPoint mousePressPosition;
    QRect oldGeometry;
    QSize internalMinimumSize;
    QSize userMinimumSize;
    QSize restoreSize;
    bool resizeEnabled;
    bool moveEnabled;
    bool isInInteractiveMode;
#if QT_CONFIG(rubberband)
    bool isInRubberBandMode;
#endif
    bool isShadeMode;
    bool ignoreWindowTitleChange;
    bool ignoreNextActivationEvent;
    bool activationEnabled;
    bool isShadeRequestFromMinimizeMode;
    bool isMaximizeMode;
    bool isWidgetHiddenByUs;
    bool isActive;
    bool isExplicitlyDeactivated;
    int keyboardSingleStep;
    int keyboardPageStep;
    int resizeTimerId;
    Operation currentOperation;
    QStyle::SubControl hoveredSubControl;
    QStyle::SubControl activeSubControl;
    Qt::FocusReason focusInReason;
    OperationInfoMap operationMap;
    QPointer<QMenu> systemMenu;
#ifndef QT_NO_ACTION
    QPointer<QAction> actions[NumWindowStateActions];
#endif
    QMdiSubWindow::SubWindowOptions options;
    QString lastChildWindowTitle;
    QPalette titleBarPalette;
    QString windowTitle;
    QFont font;
    QIcon menuIcon;
    QStyleOptionTitleBar cachedStyleOptions;
    QString originalTitle;

    // Slots.
    void _q_updateStaysOnTopHint();
    void _q_enterInteractiveMode();
    void _q_processFocusChanged(QWidget *old, QWidget *now);

    // Functions.
    void leaveInteractiveMode();
    void removeBaseWidget();
    void initOperationMap();
#if QT_CONFIG(menu)
    void createSystemMenu();
#endif
    void updateCursor();
    void updateDirtyRegions();
    void updateGeometryConstraints();
    void updateMask();
    void setNewGeometry(const QPoint &pos);
    void setMinimizeMode();
    void setNormalMode();
    void setMaximizeMode();
    void setActive(bool activate, bool changeFocus = true);
    void processClickedSubControl();
    QRegion getRegion(Operation operation) const;
    Operation getOperation(const QPoint &pos) const;
    QStyleOptionTitleBar titleBarOptions() const;
    void ensureWindowState(Qt::WindowState state);
    int titleBarHeight(const QStyleOptionTitleBar &options) const;
    void sizeParameters(int *margin, int *minWidth) const;
    bool drawTitleBarWhenMaximized() const;
#if QT_CONFIG(menubar)
    QMenuBar *menuBar() const;
    void showButtonsInMenuBar(QMenuBar *menuBar);
    void removeButtonsFromMenuBar();
#endif
    void updateWindowTitle(bool requestFromChild);
#if QT_CONFIG(rubberband)
    void enterRubberBandMode();
    void leaveRubberBandMode();
#endif
    QPalette desktopPalette() const;
    void updateActions();
    void setFocusWidget();
    bool restoreFocus();
    void storeFocusWidget();
    void setWindowFlags(Qt::WindowFlags windowFlags) override;
    void setVisible(WindowStateAction, bool visible = true);
#ifndef QT_NO_ACTION
    void setEnabled(WindowStateAction, bool enable = true);
#if QT_CONFIG(menu)
    void addToSystemMenu(WindowStateAction, const QString &text, const char *slot);
#endif
#endif // QT_NO_ACTION
    QSize iconSize() const;
#if QT_CONFIG(sizegrip)
    void setSizeGrip(QSizeGrip *sizeGrip);
    void setSizeGripVisible(bool visible = true) const;
#endif
    void updateInternalWindowTitle();
    QString originalWindowTitle();
    void setNewWindowTitle();

    inline int titleBarHeight() const
    {
        Q_Q(const QMdiSubWindow);
        if (!parent || q->windowFlags() & Qt::FramelessWindowHint
            || (q->isMaximized() && !drawTitleBarWhenMaximized())) {
            return 0;
        }
        QStyleOptionTitleBar options = titleBarOptions();
        int height = options.rect.height();
        if (hasBorder(options))
            height += q->isMinimized() ? 8 : 4;
        return height;
    }

    inline QStyle::SubControl getSubControl(const QPoint &pos) const
    {
        Q_Q(const QMdiSubWindow);
        QStyleOptionTitleBar titleBarOptions = this->titleBarOptions();
        return q->style()->hitTestComplexControl(QStyle::CC_TitleBar, &titleBarOptions, pos, q);
    }

    inline void setNewGeometry(QRect *geometry)
    {
        Q_Q(QMdiSubWindow);
        Q_ASSERT(parent);
        geometry->setSize(geometry->size().expandedTo(internalMinimumSize));
#if QT_CONFIG(rubberband)
        if (isInRubberBandMode)
            rubberBand->setGeometry(*geometry);
        else
#endif
            q->setGeometry(*geometry);
    }

    inline bool hasBorder(const QStyleOptionTitleBar &options) const
    {
        Q_Q(const QMdiSubWindow);
        return !q->style()->styleHint(QStyle::SH_TitleBar_NoBorder, &options, q);
    }

    inline bool autoRaise() const
    {
        Q_Q(const QMdiSubWindow);
        return q->style()->styleHint(QStyle::SH_TitleBar_AutoRaise, nullptr, q);
    }

    inline bool isResizeOperation() const
    {
        return currentOperation != None && currentOperation != Move;
    }

    inline bool isMoveOperation() const
    {
        return currentOperation == Move;
    }
};

QT_END_NAMESPACE

#endif // QMDISUBWINDOW_P_H

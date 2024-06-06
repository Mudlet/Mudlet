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

#ifndef QCOMBOBOX_P_H
#define QCOMBOBOX_P_H

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
#include "QtWidgets/qcombobox.h"

#include "QtWidgets/qabstractslider.h"
#include "QtWidgets/qapplication.h"
#include "QtWidgets/qitemdelegate.h"
#include "QtGui/qstandarditemmodel.h"
#include "QtWidgets/qlineedit.h"
#include "QtWidgets/qlistview.h"
#include "QtGui/qpainter.h"
#include "QtWidgets/qstyle.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qpair.h"
#include "QtCore/qtimer.h"
#include "private/qwidget_p.h"
#include "QtCore/qpointer.h"
#if QT_CONFIG(completer)
#include "QtWidgets/qcompleter.h"
#endif
#include "QtGui/qevent.h"
#include "QtCore/qdebug.h"

#include <limits.h>

QT_REQUIRE_CONFIG(combobox);

QT_BEGIN_NAMESPACE

class QAction;
class QPlatformMenu;

class QComboBoxListView : public QListView
{
    Q_OBJECT
public:
    QComboBoxListView(QComboBox *cmb = nullptr) : combo(cmb) {}

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        resizeContents(viewport()->width(), contentsSize().height());
        QListView::resizeEvent(event);
    }

    QStyleOptionViewItem viewOptions() const override
    {
        QStyleOptionViewItem option = QListView::viewOptions();
        option.showDecorationSelected = true;
        if (combo)
            option.font = combo->font();
        return option;
    }

    void paintEvent(QPaintEvent *e) override
    {
        if (combo) {
            QStyleOptionComboBox opt;
            opt.initFrom(combo);
            opt.editable = combo->isEditable();
            if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
                //we paint the empty menu area to avoid having blank space that can happen when scrolling
                QStyleOptionMenuItem menuOpt;
                menuOpt.initFrom(this);
                menuOpt.palette = palette();
                menuOpt.state = QStyle::State_None;
                menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
                menuOpt.menuRect = e->rect();
                menuOpt.maxIconWidth = 0;
                menuOpt.tabWidth = 0;
                QPainter p(viewport());
                combo->style()->drawControl(QStyle::CE_MenuEmptyArea, &menuOpt, &p, this);
            }
        }
        QListView::paintEvent(e);
    }

private:
    QComboBox *combo;
};


class QStandardItemModel;

class Q_AUTOTEST_EXPORT QComboBoxPrivateScroller : public QWidget
{
    Q_OBJECT

public:
    QComboBoxPrivateScroller(QAbstractSlider::SliderAction action, QWidget *parent)
        : QWidget(parent), sliderAction(action)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        setAttribute(Qt::WA_NoMousePropagation);
    }
    QSize sizeHint() const override {
        return QSize(20, style()->pixelMetric(QStyle::PM_MenuScrollerHeight));
    }

protected:
    inline void stopTimer() {
        timer.stop();
    }

    inline void startTimer() {
        timer.start(100, this);
        fast = false;
    }

    void enterEvent(QEvent *) override {
        startTimer();
    }

    void leaveEvent(QEvent *) override {
        stopTimer();
    }
    void timerEvent(QTimerEvent *e) override {
        if (e->timerId() == timer.timerId()) {
            emit doScroll(sliderAction);
            if (fast) {
                emit doScroll(sliderAction);
                emit doScroll(sliderAction);
            }
        }
    }
    void hideEvent(QHideEvent *) override {
        stopTimer();
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        // Enable fast scrolling if the cursor is directly above or below the popup.
        const int mouseX = e->pos().x();
        const int mouseY = e->pos().y();
        const bool horizontallyInside = pos().x() < mouseX && mouseX < rect().right() + 1;
        const bool verticallyOutside = (sliderAction == QAbstractSlider::SliderSingleStepAdd) ?
                                        rect().bottom() + 1 < mouseY : mouseY < pos().y();

        fast = horizontallyInside && verticallyOutside;
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        QStyleOptionMenuItem menuOpt;
        menuOpt.init(this);
        menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
        menuOpt.menuRect = rect();
        menuOpt.maxIconWidth = 0;
        menuOpt.tabWidth = 0;
        menuOpt.menuItemType = QStyleOptionMenuItem::Scroller;
        if (sliderAction == QAbstractSlider::SliderSingleStepAdd)
            menuOpt.state |= QStyle::State_DownArrow;
        p.eraseRect(rect());
        style()->drawControl(QStyle::CE_MenuScroller, &menuOpt, &p);
    }

Q_SIGNALS:
    void doScroll(int action);

private:
    QAbstractSlider::SliderAction sliderAction;
    QBasicTimer timer;
    bool fast;
};

class Q_WIDGETS_EXPORT QComboBoxPrivateContainer : public QFrame
{
    Q_OBJECT

public:
    QComboBoxPrivateContainer(QAbstractItemView *itemView, QComboBox *parent);
    QAbstractItemView *itemView() const;
    void setItemView(QAbstractItemView *itemView);
    int spacing() const;
    int topMargin() const;
    int bottomMargin() const { return topMargin(); }
    void updateTopBottomMargin();

    QTimer blockMouseReleaseTimer;
    QBasicTimer adjustSizeTimer;
    QPoint initialClickPosition;

public Q_SLOTS:
    void scrollItemView(int action);
    void hideScrollers();
    void updateScrollers();
    void viewDestroyed();

protected:
    void changeEvent(QEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void timerEvent(QTimerEvent *timerEvent) override;
    void leaveEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QStyleOptionComboBox comboStyleOption() const;

Q_SIGNALS:
    void itemSelected(const QModelIndex &);
    void resetButton();

private:
    QComboBox *combo;
    QAbstractItemView *view;
    QComboBoxPrivateScroller *top;
    QComboBoxPrivateScroller *bottom;
    bool maybeIgnoreMouseButtonRelease;
    QElapsedTimer popupTimer;

    friend class QComboBox;
    friend class QComboBoxPrivate;
};

class Q_AUTOTEST_EXPORT QComboMenuDelegate : public QAbstractItemDelegate
{ Q_OBJECT
public:
    QComboMenuDelegate(QObject *parent, QComboBox *cmb) : QAbstractItemDelegate(parent), mCombo(cmb) {}

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QStyleOptionMenuItem opt = getStyleOption(option, index);
        painter->fillRect(option.rect, opt.palette.window());
        mCombo->style()->drawControl(QStyle::CE_MenuItem, &opt, painter, mCombo);
    }
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        QStyleOptionMenuItem opt = getStyleOption(option, index);
        return mCombo->style()->sizeFromContents(
            QStyle::CT_MenuItem, &opt, option.rect.size(), mCombo);
    }

private:
    QStyleOptionMenuItem getStyleOption(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const;
    QComboBox *mCombo;
};

// ### Qt6: QStyledItemDelegate ?
// Note that this class is intentionally not using QStyledItemDelegate
// Vista does not use the new theme for combo boxes and there might
// be other side effects from using the new class
class Q_AUTOTEST_EXPORT QComboBoxDelegate : public QItemDelegate
{ Q_OBJECT
public:
    QComboBoxDelegate(QObject *parent, QComboBox *cmb) : QItemDelegate(parent), mCombo(cmb) {}

    static bool isSeparator(const QModelIndex &index) {
        return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
    }
    static void setSeparator(QAbstractItemModel *model, const QModelIndex &index) {
        model->setData(index, QString::fromLatin1("separator"), Qt::AccessibleDescriptionRole);
        if (QStandardItemModel *m = qobject_cast<QStandardItemModel*>(model))
            if (QStandardItem *item = m->itemFromIndex(index))
                item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
    }

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        if (isSeparator(index)) {
            QRect rect = option.rect;
            if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(option.widget))
                rect.setWidth(view->viewport()->width());
            QStyleOption opt;
            opt.rect = rect;
            mCombo->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, painter, mCombo);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        if (isSeparator(index)) {
            int pm = mCombo->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, mCombo);
            return QSize(pm, pm);
        }
        return QItemDelegate::sizeHint(option, index);
    }
private:
    QComboBox *mCombo;
};

class Q_AUTOTEST_EXPORT QComboBoxPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QComboBox)
public:
    QComboBoxPrivate();
    ~QComboBoxPrivate();
    void init();
    QComboBoxPrivateContainer* viewContainer();
    void updateLineEditGeometry();
    Qt::MatchFlags matchFlags() const;
    void _q_editingFinished();
    void _q_returnPressed();
    void _q_complete();
    void _q_itemSelected(const QModelIndex &item);
    bool contains(const QString &text, int role);
    void emitActivated(const QModelIndex&);
    void _q_emitHighlighted(const QModelIndex&);
    void _q_emitCurrentIndexChanged(const QModelIndex &index);
    void _q_modelDestroyed();
    void _q_modelReset();
#if QT_CONFIG(completer)
    void _q_completerActivated(const QModelIndex &index);
#endif
    void _q_resetButton();
    void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void _q_updateIndexBeforeChange();
    void _q_rowsInserted(const QModelIndex & parent, int start, int end);
    void _q_rowsRemoved(const QModelIndex & parent, int start, int end);
    void updateArrow(QStyle::StateFlag state);
    bool updateHoverControl(const QPoint &pos);
    QRect popupGeometry(int screen = -1) const;
    QStyle::SubControl newHoverControl(const QPoint &pos);
    int computeWidthHint() const;
    QSize recomputeSizeHint(QSize &sh) const;
    void adjustComboBoxSize();
    QString itemText(const QModelIndex &index) const;
    QIcon itemIcon(const QModelIndex &index) const;
    int itemRole() const;
    void updateLayoutDirection();
    void setCurrentIndex(const QModelIndex &index);
    void updateDelegate(bool force = false);
    void keyboardSearchString(const QString &text);
    void modelChanged();
    void updateViewContainerPaletteAndOpacity();
    void updateFocusPolicy();
    void showPopupFromMouseEvent(QMouseEvent *e);

#ifdef Q_OS_MAC
    void cleanupNativePopup();
    bool showNativePopup();
    struct IndexSetter {
        int index;
        QComboBox *cb;

        void operator()(void)
        {
            cb->setCurrentIndex(index);
            cb->d_func()->emitActivated(cb->d_func()->currentIndex);
        }
    };
#endif

    QAbstractItemModel *model;
    QLineEdit *lineEdit;
    QComboBoxPrivateContainer *container;
    QComboBox::InsertPolicy insertPolicy;
    QComboBox::SizeAdjustPolicy sizeAdjustPolicy;
    int minimumContentsLength;
    QSize iconSize;
    uint shownOnce : 1;
    uint autoCompletion : 1;
    uint duplicatesEnabled : 1;
    uint frame : 1;
    uint padding : 26;
    int maxVisibleItems;
    int maxCount;
    int modelColumn;
    bool inserting;
    mutable QSize minimumSizeHint;
    mutable QSize sizeHint;
    QStyle::StateFlag arrowState;
    QStyle::SubControl hoverControl;
    QRect hoverRect;
    QPersistentModelIndex currentIndex;
    QPersistentModelIndex root;
    Qt::CaseSensitivity autoCompletionCaseSensitivity;
    int indexBeforeChange;
#ifdef Q_OS_MAC
    QPlatformMenu *m_platformMenu;
#endif
#if QT_CONFIG(completer)
    QPointer<QCompleter> completer;
#endif
    static QPalette viewContainerPalette(QComboBox *cmb)
    { return cmb->d_func()->viewContainer()->palette(); }
};

QT_END_NAMESPACE

#endif // QCOMBOBOX_P_H

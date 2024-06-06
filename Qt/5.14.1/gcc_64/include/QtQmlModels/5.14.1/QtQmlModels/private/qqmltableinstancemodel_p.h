/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLTABLEINSTANCEMODEL_P_H
#define QQMLTABLEINSTANCEMODEL_P_H

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

#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p_p.h>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class QQmlTableInstanceModel;
class QQmlAbstractDelegateComponent;

class QQmlTableInstanceModelIncubationTask : public QQDMIncubationTask
{
public:
    QQmlTableInstanceModelIncubationTask(
            QQmlTableInstanceModel *tableInstanceModel
            , QQmlDelegateModelItem* modelItemToIncubate
            , IncubationMode mode)
        : QQDMIncubationTask(nullptr, mode)
        , modelItemToIncubate(modelItemToIncubate)
        , tableInstanceModel(tableInstanceModel) {
        clear();
    }

    void statusChanged(Status status) override;
    void setInitialState(QObject *object) override;

    QQmlDelegateModelItem *modelItemToIncubate = nullptr;
    QQmlTableInstanceModel *tableInstanceModel = nullptr;
};

class Q_QMLMODELS_PRIVATE_EXPORT QQmlTableInstanceModel : public QQmlInstanceModel
{
    Q_OBJECT

public:

    enum ReusableFlag {
        NotReusable,
        Reusable
    };

    QQmlTableInstanceModel(QQmlContext *qmlContext, QObject *parent = nullptr);
    ~QQmlTableInstanceModel() override;

    void useImportVersion(int minorVersion);

    int count() const override { return m_adaptorModel.count(); }
    int rows() const { return m_adaptorModel.rowCount(); }
    int columns() const { return m_adaptorModel.columnCount(); }

    bool isValid() const override { return true; }

    bool canFetchMore() const { return m_adaptorModel.canFetchMore(); }
    void fetchMore() { m_adaptorModel.fetchMore(); }

    QVariant model() const;
    void setModel(const QVariant &model);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    const QAbstractItemModel *abstractItemModel() const override;

    QObject *object(int index, QQmlIncubator::IncubationMode incubationMode = QQmlIncubator::AsynchronousIfNested) override;
    ReleaseFlags release(QObject *object) override { return release(object, NotReusable); }
    ReleaseFlags release(QObject *object, ReusableFlag reusable);
    void cancel(int) override;

    void insertIntoReusableItemsPool(QQmlDelegateModelItem *modelItem);
    QQmlDelegateModelItem *takeFromReusableItemsPool(const QQmlComponent *delegate);
    void drainReusableItemsPool(int maxPoolTime);
    int poolSize() { return m_reusableItemsPool.size(); }
    void reuseItem(QQmlDelegateModelItem *item, int newModelIndex);

    QQmlIncubator::Status incubationStatus(int index) override;

    QVariant variantValue(int, const QString &) override { Q_UNREACHABLE(); return QVariant(); }
    void setWatchedRoles(const QList<QByteArray> &) override { Q_UNREACHABLE(); }
    int indexOf(QObject *, QObject *) const override { Q_UNREACHABLE(); return 0; }

Q_SIGNALS:
    void itemPooled(int index, QObject *object);
    void itemReused(int index, QObject *object);

private:
    QQmlComponent *resolveDelegate(int index);

    QQmlAdaptorModel m_adaptorModel;
    QQmlAbstractDelegateComponent *m_delegateChooser = nullptr;
    QQmlComponent *m_delegate = nullptr;
    QPointer<QQmlContext> m_qmlContext;
    QQmlDelegateModelItemMetaType *m_metaType;

    QHash<int, QQmlDelegateModelItem *> m_modelItems;
    QList<QQmlDelegateModelItem *> m_reusableItemsPool;
    QList<QQmlIncubator *> m_finishedIncubationTasks;

    void incubateModelItem(QQmlDelegateModelItem *modelItem, QQmlIncubator::IncubationMode incubationMode);
    void incubatorStatusChanged(QQmlTableInstanceModelIncubationTask *dmIncubationTask, QQmlIncubator::Status status);
    void deleteIncubationTaskLater(QQmlIncubator *incubationTask);
    void deleteAllFinishedIncubationTasks();
    QQmlDelegateModelItem *resolveModelItem(int index);

    void dataChangedCallback(const QModelIndex &begin, const QModelIndex &end, const QVector<int> &roles);

    static bool isDoneIncubating(QQmlDelegateModelItem *modelItem);
    static void deleteModelItemLater(QQmlDelegateModelItem *modelItem);

    friend class QQmlTableInstanceModelIncubationTask;
};

QT_END_NAMESPACE

#endif // QQMLTABLEINSTANCEMODEL_P_H

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

#ifndef QQMLDELEGATECOMPONENT_P_H
#define QQMLDELEGATECOMPONENT_P_H

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

#include <private/qtqmlmodelsglobal_p.h>
#include <qqmlcomponent.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

// TODO: consider making QQmlAbstractDelegateComponent public API
class QQmlAbstractDelegateComponentPrivate;
class QQmlAdaptorModel;
class Q_QMLMODELS_PRIVATE_EXPORT QQmlAbstractDelegateComponent : public QQmlComponent
{
    Q_OBJECT
public:
    QQmlAbstractDelegateComponent(QObject *parent = nullptr);
    ~QQmlAbstractDelegateComponent() override;

    virtual QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = 0) const = 0;

signals:
    void delegateChanged();

protected:
    QVariant value(QQmlAdaptorModel *adaptorModel,int row, int column, const QString &role) const;

private:
    Q_DECLARE_PRIVATE(QQmlAbstractDelegateComponent)
    Q_DISABLE_COPY(QQmlAbstractDelegateComponent)
};

class Q_QMLMODELS_PRIVATE_EXPORT QQmlDelegateChoice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY roleValueChanged)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
    Q_PROPERTY(int index READ row WRITE setRow NOTIFY indexChanged)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
public:
    QVariant roleValue() const;
    void setRoleValue(const QVariant &roleValue);

    int row() const;
    void setRow(int r);

    int column() const;
    void setColumn(int c);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    virtual bool match(int row, int column, const QVariant &value) const;

signals:
    void roleValueChanged();
    void rowChanged();
    void indexChanged();
    void columnChanged();
    void delegateChanged();
    void changed();

private:
    QVariant m_value;
    int m_row = -1;
    int m_column = -1;
    QQmlComponent *m_delegate = nullptr;
};

class Q_QMLMODELS_PRIVATE_EXPORT QQmlDelegateChooser : public QQmlAbstractDelegateComponent
{
    Q_OBJECT
    Q_PROPERTY(QString role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QQmlListProperty<QQmlDelegateChoice> choices READ choices CONSTANT)
    Q_CLASSINFO("DefaultProperty", "choices")

public:
    QString role() const { return m_role; }
    void setRole(const QString &role);

    virtual QQmlListProperty<QQmlDelegateChoice> choices();
    static void choices_append(QQmlListProperty<QQmlDelegateChoice> *, QQmlDelegateChoice *);
    static int choices_count(QQmlListProperty<QQmlDelegateChoice> *);
    static QQmlDelegateChoice *choices_at(QQmlListProperty<QQmlDelegateChoice> *, int);
    static void choices_clear(QQmlListProperty<QQmlDelegateChoice> *);

    QQmlComponent *delegate(QQmlAdaptorModel *adaptorModel, int row, int column = -1) const override;

signals:
    void roleChanged();

private:
    QString m_role;
    QList<QQmlDelegateChoice *> m_choices;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlDelegateChoice)
QML_DECLARE_TYPE(QQmlDelegateChooser)

#endif // QQMLDELEGATECOMPONENT_P_H

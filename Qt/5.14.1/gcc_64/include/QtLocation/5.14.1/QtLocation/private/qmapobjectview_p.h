/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAPOBJECTVIEW_P_H
#define QMAPOBJECTVIEW_P_H

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

#include <QtLocation/private/qlocationglobal_p.h>
#include <QtLocation/private/qgeomapobject_p.h>
#include <QQmlComponent>
#include <QVector>

QT_BEGIN_NAMESPACE

class QQmlDelegateModel;
class QMapObjectViewPrivate;
class QQmlChangeSet;
class Q_LOCATION_PRIVATE_EXPORT QMapObjectView : public QGeoMapObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_INTERFACES(QQmlParserStatus)
public:
    QMapObjectView(QObject *parent = nullptr);
    ~QMapObjectView() override;

    // QGeoMapObject interface
    QList<QGeoMapObject *> geoMapObjectChildren() const override;
    void setMap(QGeoMap *map) override;

    // QQmlParserStatus interface
    void classBegin() override;
    void componentComplete() override;

    QVariant model() const;
    void setModel(QVariant model);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent * delegate);

public Q_SLOTS:
    // The dynamic API that matches Map.add/remove MapItem
    void addMapObject(QGeoMapObject *object);
    void removeMapObject(QGeoMapObject *object);

signals:
    void modelChanged(QVariant model);
    void delegateChanged(QQmlComponent * delegate);

protected Q_SLOTS:
    void destroyingItem(QObject *object);
    void initItem(int index, QObject *object);
    void createdItem(int index, QObject *object);
    void modelUpdated(const QQmlChangeSet &changeSet, bool reset);

protected:
    void addMapObjectToMap(QGeoMapObject *object, int index);
    void removeMapObjectFromMap(int index);
    void flushDelegateModel();
    void flushUserAddedMapObjects();

    QQmlDelegateModel *m_delegateModel = nullptr;
    QVector<QPointer<QGeoMapObject>> m_instantiatedMapObjects;
    QVector<QPointer<QGeoMapObject>> m_pendingMapObjects; // for items instantiated before the map is set
    QVector<QPointer<QGeoMapObject>> m_userAddedMapObjects; // A third list containing the objects dynamically added through addMapObject
    bool m_creatingObject = false;
};

QT_END_NAMESPACE

#endif // QMAPOBJECTVIEW_P_H

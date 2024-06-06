/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#ifndef QT3DRENDER_CAMERALENS_P_H
#define QT3DRENDER_CAMERALENS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <Qt3DRender/private/qt3drender_global_p.h>
#include <Qt3DCore/private/qcomponent_p.h>
#include <Qt3DCore/private/qnodecommand_p.h>

#include "qcameralens.h"

#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

struct CameraLensCommand
{
    QString name;
    QVariant data;
    Qt3DCore::QNodeCommand::CommandId commandId;

    inline operator bool() const { return !name.isEmpty(); }
};

inline bool operator ==(const CameraLensCommand &a, const CameraLensCommand &b) noexcept
{
    return a.name == b.name && a.data == b.data && a.commandId == b.commandId;
}

inline bool operator !=(const CameraLensCommand &a, const CameraLensCommand &b) noexcept
{
    return !(a == b);
}

class Q_3DRENDERSHARED_PRIVATE_EXPORT QCameraLensPrivate : public Qt3DCore::QComponentPrivate
{
public:
    QCameraLensPrivate();

    inline void updateProjectionMatrix()
    {
        switch (m_projectionType) {
        case QCameraLens::OrthographicProjection:
            updateOrthographicProjection();
            break;
        case QCameraLens::PerspectiveProjection:
            updatePerpectiveProjection();
            break;
        case QCameraLens::FrustumProjection:
            updateFrustumProjection();
            break;
        case QCameraLens::CustomProjection:
            break;
        }
    }

    Q_DECLARE_PUBLIC(QCameraLens)

    QCameraLens::ProjectionType m_projectionType;

    float m_nearPlane;
    float m_farPlane;

    float m_fieldOfView;
    float m_aspectRatio;

    float m_left;
    float m_right;
    float m_bottom;
    float m_top;

    mutable QMatrix4x4 m_projectionMatrix;

    float m_exposure;

    CameraLensCommand m_pendingViewAllCommand;
    void processViewAllCommand(Qt3DCore::QNodeCommand::CommandId commandId, const QVariant &data);

private:
    inline void updatePerpectiveProjection()
    {
        Q_Q(QCameraLens);
        m_projectionMatrix.setToIdentity();
        m_projectionMatrix.perspective(m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane);
        Q_EMIT q->projectionMatrixChanged(m_projectionMatrix);
    }

    inline void updateOrthographicProjection()
    {
        Q_Q(QCameraLens);
        m_projectionMatrix.setToIdentity();
        m_projectionMatrix.ortho(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        Q_EMIT q->projectionMatrixChanged(m_projectionMatrix);
    }

    inline void updateFrustumProjection()
    {
        Q_Q(QCameraLens);
        m_projectionMatrix.setToIdentity();
        m_projectionMatrix.frustum(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        Q_EMIT q->projectionMatrixChanged(m_projectionMatrix);
    }
};

struct QCameraLensData
{
    QMatrix4x4 projectionMatrix;
    float exposure;
};

} // namespace Qt3DRender

QT_END_NAMESPACE

#endif // QT3DRENDER_CAMERALENS_P_H

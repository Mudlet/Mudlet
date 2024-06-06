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

#ifndef QT3DCORE_QASPECTJOB_P_H
#define QT3DCORE_QASPECTJOB_P_H

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

#include <QtCore/QWeakPointer>

#include <Qt3DCore/private/qt3dcore_global_p.h>
#include <Qt3DCore/qt3dcore-config.h>

QT_BEGIN_NAMESPACE

namespace Qt3DCore {

class QAspectJob;
class QAspectManager;

#if QT_CONFIG(qt3d_profile_jobs)
struct FrameHeader
{
    FrameHeader()
        : frameId(0)
        , jobCount(0)
        , frameType(WorkerJob)
    {
    }

    enum FrameType {
        WorkerJob = 0,
        Submission
    };

    quint32 frameId;
    quint16 jobCount;
    quint16 frameType; // Submission or worker job
};

union JobId
{
    quint32 typeAndInstance[2];
    quint64 id;
};

struct JobRunStats
{
    JobRunStats()
    {
        jobId.id = 0;
    }

    qint64 startTime;
    qint64 endTime;
    JobId jobId;
    // QAspectJob subclasses should properly populate the jobId
    quint64 threadId;
};
#endif

class Q_3DCORE_PRIVATE_EXPORT QAspectJobPrivate
{
public:
    QAspectJobPrivate();
    virtual ~QAspectJobPrivate();

    static QAspectJobPrivate *get(QAspectJob *job);
    static const QAspectJobPrivate *get(const QAspectJob *job);

    virtual void postFrame(QAspectManager *aspectManager);

    QVector<QWeakPointer<QAspectJob> > m_dependencies;
#if QT_CONFIG(qt3d_profile_jobs)
    JobRunStats m_stats;
#endif
};

} // Qt3D

#define Q_DJOB(Class) \
    Class##Private *d = static_cast<Class##Private *>(Qt3DCore::QAspectJobPrivate::get(this))

#if QT_CONFIG(qt3d_profile_jobs)

#include <Qt3DCore/private/qaspectjob_p.h>

#define SET_JOB_RUN_STAT_TYPE(job, type, instance) \
    Qt3DCore::QAspectJobPrivate::get(job)->m_stats.jobId.typeAndInstance[0] = type; \
    Qt3DCore::QAspectJobPrivate::get(job)->m_stats.jobId.typeAndInstance[1] = instance;

#else

#define SET_JOB_RUN_STAT_TYPE(job, type, instance) \
    Q_UNUSED(job) \
    Q_UNUSED(type) \
    Q_UNUSED(instance)

#endif

QT_END_NAMESPACE

#endif // QT3DCORE_QASPECTJOB_P_H

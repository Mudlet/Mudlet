/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QRECYCLEPOOL_P_H
#define QRECYCLEPOOL_P_H

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

QT_BEGIN_NAMESPACE

#define QRECYCLEPOOLCOOKIE 0x33218ADF

template<typename T, int Step>
class QRecyclePoolPrivate
{
public:
    QRecyclePoolPrivate()
    : recyclePoolHold(true), outstandingItems(0), cookie(QRECYCLEPOOLCOOKIE),
      currentPage(nullptr), nextAllocated(nullptr)
    {
    }

    bool recyclePoolHold;
    int outstandingItems;
    quint32 cookie;

    struct PoolType : public T {
        union {
            QRecyclePoolPrivate<T, Step> *pool;
            PoolType *nextAllocated;
        };
    };

    struct Page {
        Page *nextPage;
        unsigned int free;
        union {
            char array[Step * sizeof(PoolType)];
            qint64 q_for_alignment_1;
            double q_for_alignment_2;
        };
    };

    Page *currentPage;
    PoolType *nextAllocated;

    inline T *allocate();
    static inline void dispose(T *);
    inline void releaseIfPossible();
};

template<typename T, int Step = 1024>
class QRecyclePool
{
public:
    inline QRecyclePool();
    inline ~QRecyclePool();

    inline T *New();
    template<typename T1>
    inline T *New(const T1 &);
    template<typename T1>
    inline T *New(T1 &);

    static inline void Delete(T *);

private:
    QRecyclePoolPrivate<T, Step> *d;
};

template<typename T, int Step>
QRecyclePool<T, Step>::QRecyclePool()
: d(new QRecyclePoolPrivate<T, Step>())
{
}

template<typename T, int Step>
QRecyclePool<T, Step>::~QRecyclePool()
{
    d->recyclePoolHold = false;
    d->releaseIfPossible();
}

template<typename T, int Step>
T *QRecyclePool<T, Step>::New()
{
    T *rv = d->allocate();
    new (rv) T;
    return rv;
}

template<typename T, int Step>
template<typename T1>
T *QRecyclePool<T, Step>::New(const T1 &a)
{
    T *rv = d->allocate();
    new (rv) T(a);
    return rv;
}

template<typename T, int Step>
template<typename T1>
T *QRecyclePool<T, Step>::New(T1 &a)
{
    T *rv = d->allocate();
    new (rv) T(a);
    return rv;
}

template<typename T, int Step>
void QRecyclePool<T, Step>::Delete(T *t)
{
    t->~T();
    QRecyclePoolPrivate<T, Step>::dispose(t);
}

template<typename T, int Step>
void QRecyclePoolPrivate<T, Step>::releaseIfPossible()
{
    if (recyclePoolHold || outstandingItems)
        return;

    Page *p = currentPage;
    while (p) {
        Page *n = p->nextPage;
        free(p);
        p = n;
    }

    delete this;
}

template<typename T, int Step>
T *QRecyclePoolPrivate<T, Step>::allocate()
{
    PoolType *rv = nullptr;
    if (nextAllocated) {
        rv = nextAllocated;
        nextAllocated = rv->nextAllocated;
    } else if (currentPage && currentPage->free) {
        rv = (PoolType *)(currentPage->array + (Step - currentPage->free) * sizeof(PoolType));
        currentPage->free--;
    } else {
        Page *p = (Page *)malloc(sizeof(Page));
        p->nextPage = currentPage;
        p->free = Step;
        currentPage = p;

        rv = (PoolType *)currentPage->array;
        currentPage->free--;
    }

    rv->pool = this;
    ++outstandingItems;
    return rv;
}

template<typename T, int Step>
void QRecyclePoolPrivate<T, Step>::dispose(T *t)
{
    PoolType *pt = static_cast<PoolType *>(t);
    Q_ASSERT(pt->pool && pt->pool->cookie == QRECYCLEPOOLCOOKIE);

    QRecyclePoolPrivate<T, Step> *This = pt->pool;
    pt->nextAllocated = This->nextAllocated;
    This->nextAllocated = pt;
    --This->outstandingItems;
    This->releaseIfPossible();
}

QT_END_NAMESPACE

#endif // QRECYCLEPOOL_P_H

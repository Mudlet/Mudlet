/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtXmlPatterns module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Patternist_RemovalIterator_H
#define Patternist_RemovalIterator_H

#include <private/qitem_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{

    /**
     * @short Removes one items at a specified position from an input QAbstractXmlForwardIterator.
     *
     * RemoveIterator removes an item from a sequence at a certain position,
     * while retaining the pull-based characteristic of being an QAbstractXmlForwardIterator itself. The
     * RemovalIterator's constructor is passed an QAbstractXmlForwardIterator, the QAbstractXmlForwardIterator to
     * remove from, and the position of the item to remove. When calling the RemovalIterator's
     * functions, it acts as an ordinary QAbstractXmlForwardIterator, taking into account that
     * one item is removed from the source QAbstractXmlForwardIterator.
     *
     * The RemovalIterator class contains the central business logic for implementing the
     * <tt>fn:remove()</tt> function, whose definition therefore specifies the detailed behaviors
     * of RemovalIterator.
     *
     * @see <a href="http://www.w3.org/TR/xpath-functions/#func-remove">XQuery 1.0
     * and XPath 2.0 Functions and Operators, 15.1.8 fn:remove</a>
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_iterators
     */
    class RemovalIterator : public Item::Iterator
    {
    public:

        /**
         * Creates an RemovalIterator.
         *
         * @param target the QAbstractXmlForwardIterator containing the sequence of items
         * which the item at position @p position should be removed from.
         * @param position the position of the item to remove. Must be
         * 1 or larger.
         */
        RemovalIterator(const Item::Iterator::Ptr &target,
                        const xsInteger position);

        virtual Item next();
        virtual Item current() const;
        virtual xsInteger position() const;

        /**
         * The QAbstractXmlForwardIterator's count is computed by subtracting one from the source
         * QAbstractXmlForwardIterator's count.
         */
        virtual xsInteger count();

        virtual Item::Iterator::Ptr copy() const;

    private:
        const Item::Iterator::Ptr m_target;
        const xsInteger m_removalPos;
        Item m_current;
        xsInteger m_position;
    };
}

QT_END_NAMESPACE

#endif

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

#ifndef Patternist_Template_H
#define Patternist_Template_H

#include <QSharedData>
#include <QVector>

#include <private/qdynamiccontext_p.h>
#include <private/qexpression_p.h>
#include <private/qsourcelocationreflection_p.h>
#include <private/qtemplateinvoker_p.h>
#include <private/qvariabledeclaration_p.h>

QT_BEGIN_NAMESPACE

namespace QPatternist
{
    /**
     * @short Contains data related to a template.
     *
     * A Template is associated with a mode, by being housed
     * inside a TemplateMode instance.
     *
     * Template has role very similar to UserFunction.
     *
     * @see TemplateMode
     * @see TemplatePattern
     * @see UserFunction
     * @author Frans Englich <frans.englich@nokia.com>
     * @ingroup Patternist_expressions
     * @since 4.5
     */
    class Q_AUTOTEST_EXPORT Template : public QSharedData, public SourceLocationReflection

    {
    public:
        typedef QExplicitlySharedDataPointer<Template> Ptr;
        typedef QVector<Template::Ptr> Vector;

        inline Template(const ImportPrecedence ip,
                        const SequenceType::Ptr &reqType) : importPrecedence(ip)
                                                          , m_reqType(reqType)
        {
        }
        ~Template();

        Expression::Ptr body;

        /**
         * Returns @c this.
         */
        virtual const SourceLocationReflection* actualReflection() const;

        const ImportPrecedence importPrecedence;

        VariableDeclaration::List templateParameters;

        /**
         * If @p isCallTemplate, the caller is @c xsl:call-template, as opposed
         * to for instance @c xsl:apply-templates. This affects error
         * reporting.
         */
        DynamicContext::Ptr createContext(const TemplateInvoker *const invoker,
                                         const DynamicContext::Ptr &context,
                                         const bool isCallTemplate) const;

        /**
         * Since we have our template parameters in templateParameters, we need
         * this separate step to do the regular phases:
         * Expression::typeCheck(), and Expression::compress().
         */
        void compileParameters(const StaticContext::Ptr &context);

        /**
         * A value which takes into account the body and its template
         * parameters.
         */
        Expression::Properties properties() const;

        Expression::Properties dependencies() const;

        static void raiseXTSE0680(const ReportContext::Ptr &context,
                                  const QXmlName &name,
                                  const SourceLocationReflection *const reflection);

    private:
        DynamicContext::TemplateParameterHash parametersAsHash() const;
        const SequenceType::Ptr m_reqType;
    };
}

QT_END_NAMESPACE

#endif

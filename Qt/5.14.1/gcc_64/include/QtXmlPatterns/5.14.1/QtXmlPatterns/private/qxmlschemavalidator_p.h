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

#ifndef QXMLSCHEMAVALIDATOR_P_H
#define QXMLSCHEMAVALIDATOR_P_H

#include "qabstractmessagehandler.h"
#include "qabstracturiresolver.h"
#include <private/qautoptr_p.h>
#include <private/qcoloringmessagehandler_p.h>
#include "qxmlschema.h"
#include <private/qxmlschema_p.h>

#include <private/qxsdschemacontext_p.h>
#include <private/qxsdschema_p.h>

#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE

class QXmlSchemaValidatorPrivate
{
public:
    QXmlSchemaValidatorPrivate(const QXmlSchema &schema)
        : m_namePool(schema.namePool())
        , m_userMessageHandler(0)
        , m_uriResolver(0)
        , m_userNetworkAccessManager(0)
    {
        setSchema(schema);

        const QXmlSchemaPrivate *p = schema.d;

        // initialize the environment properties with the ones from the schema

        if (p->m_userNetworkAccessManager) // schema has user defined network access manager
            m_userNetworkAccessManager = p->m_userNetworkAccessManager;
        else
            m_networkAccessManager = p->m_networkAccessManager;

        if (p->m_userMessageHandler) // schema has user defined message handler
            m_userMessageHandler = p->m_userMessageHandler;
        else
            m_messageHandler = p->m_messageHandler;

        m_uriResolver = p->m_uriResolver;
    }

    void setSchema(const QXmlSchema &schema)
    {
        // use same name pool as the schema
        m_namePool = schema.namePool();
        m_schema = schema.d->m_schemaParserContext->schema();
        m_schemaDocumentUri = schema.documentUri();

        // create a new schema context
        m_context = QPatternist::XsdSchemaContext::Ptr(new QPatternist::XsdSchemaContext(m_namePool.d));
        m_context->m_schemaTypeFactory = schema.d->m_schemaContext->m_schemaTypeFactory;
        m_context->m_builtinTypesFacetList = schema.d->m_schemaContext->m_builtinTypesFacetList;

        m_originalSchema = schema;
    }

    QXmlNamePool                                                     m_namePool;
    QAbstractMessageHandler*                                         m_userMessageHandler;
    const QAbstractUriResolver*                                      m_uriResolver;
    QNetworkAccessManager*                                           m_userNetworkAccessManager;
    QPatternist::ReferenceCountedValue<QAbstractMessageHandler>::Ptr m_messageHandler;
    QPatternist::ReferenceCountedValue<QNetworkAccessManager>::Ptr   m_networkAccessManager;

    QXmlSchema                                                       m_originalSchema;
    QPatternist::XsdSchemaContext::Ptr                               m_context;
    QPatternist::XsdSchema::Ptr                                      m_schema;
    QUrl                                                             m_schemaDocumentUri;
};

QT_END_NAMESPACE

#endif

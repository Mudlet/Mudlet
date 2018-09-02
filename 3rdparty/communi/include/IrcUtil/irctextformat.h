/*
  Copyright (C) 2008-2016 The Communi Project

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IRCTEXTFORMAT_H
#define IRCTEXTFORMAT_H

#include <IrcGlobal>
#include <QtCore/qurl.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>

IRC_BEGIN_NAMESPACE

class IrcPalette;
class IrcTextFormatPrivate;

class IRC_UTIL_EXPORT IrcTextFormat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(IrcPalette* palette READ palette CONSTANT)
    Q_PROPERTY(QString urlPattern READ urlPattern WRITE setUrlPattern)
    Q_PROPERTY(QString plainText READ plainText)
    Q_PROPERTY(QString html READ html)
    Q_PROPERTY(QList<QUrl> urls READ urls)
    Q_ENUMS(SpanFormat)

public:
    explicit IrcTextFormat(QObject* parent = 0);
    virtual ~IrcTextFormat();

    IrcPalette* palette() const;

    QString urlPattern() const;
    void setUrlPattern(const QString& pattern);

    enum SpanFormat { SpanStyle, SpanClass };

    SpanFormat spanFormat() const;
    void setSpanFormat(SpanFormat format);

    Q_INVOKABLE QString toHtml(const QString& text) const;
    Q_INVOKABLE QString toPlainText(const QString& text) const;

    QString plainText() const;
    QString html() const;
    QList<QUrl> urls() const;

public Q_SLOTS:
    void parse(const QString& text);

private:
    QScopedPointer<IrcTextFormatPrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcTextFormat)
    Q_DISABLE_COPY(IrcTextFormat)
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcTextFormat*))
Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcTextFormat::SpanFormat))

#endif // IRCTEXTFORMAT_H

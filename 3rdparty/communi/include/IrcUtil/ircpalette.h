/*
  Copyright (C) 2008-2020 The Communi Project

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

#ifndef IRCPALETTE_H
#define IRCPALETTE_H

#include <IrcGlobal>
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qscopedpointer.h>

IRC_BEGIN_NAMESPACE

class IrcPalettePrivate;

class IRC_UTIL_EXPORT IrcPalette : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString white READ white WRITE setWhite)
    Q_PROPERTY(QString black READ black WRITE setBlack)
    Q_PROPERTY(QString blue READ blue WRITE setBlue)
    Q_PROPERTY(QString green READ green WRITE setGreen)
    Q_PROPERTY(QString red READ red WRITE setRed)
    Q_PROPERTY(QString brown READ brown WRITE setBrown)
    Q_PROPERTY(QString purple READ purple WRITE setPurple)
    Q_PROPERTY(QString orange READ orange WRITE setOrange)
    Q_PROPERTY(QString yellow READ yellow WRITE setYellow)
    Q_PROPERTY(QString lightGreen READ lightGreen WRITE setLightGreen)
    Q_PROPERTY(QString cyan READ cyan WRITE setCyan)
    Q_PROPERTY(QString lightCyan READ lightCyan WRITE setLightCyan)
    Q_PROPERTY(QString lightBlue READ lightBlue WRITE setLightBlue)
    Q_PROPERTY(QString pink READ pink WRITE setPink)
    Q_PROPERTY(QString gray READ gray WRITE setGray)
    Q_PROPERTY(QString lightGray READ lightGray WRITE setLightGray)

public:
    ~IrcPalette() override;

    QMap<int, QString> colorNames() const;
    void setColorNames(const QMap<int, QString>& names);

    QString colorName(int color, const QString& fallback = QStringLiteral("black")) const;
    void setColorName(int color, const QString& name);

    QString white() const;
    void setWhite(const QString& color);

    QString black() const;
    void setBlack(const QString& color);

    QString blue() const;
    void setBlue(const QString& color);

    QString green() const;
    void setGreen(const QString& color);

    QString red() const;
    void setRed(const QString& color);

    QString brown() const;
    void setBrown(const QString& color);

    QString purple() const;
    void setPurple(const QString& color);

    QString orange() const;
    void setOrange(const QString& color);

    QString yellow() const;
    void setYellow(const QString& color);

    QString lightGreen() const;
    void setLightGreen(const QString& color);

    QString cyan() const;
    void setCyan(const QString& color);

    QString lightCyan() const;
    void setLightCyan(const QString& color);

    QString lightBlue() const;
    void setLightBlue(const QString& color);

    QString pink() const;
    void setPink(const QString& color);

    QString gray() const;
    void setGray(const QString& color);

    QString lightGray() const;
    void setLightGray(const QString& color);

private:
    friend class IrcTextFormat;
    explicit IrcPalette(QObject* parent);

    QScopedPointer<IrcPalettePrivate> d_ptr;
    Q_DECLARE_PRIVATE(IrcPalette)
    Q_DISABLE_COPY(IrcPalette)
};

IRC_END_NAMESPACE

Q_DECLARE_METATYPE(IRC_PREPEND_NAMESPACE(IrcPalette*))

#endif // IRCPALETTE_H

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

#ifndef IRCNETWORK_P_H
#define IRCNETWORK_P_H

#include "ircnetwork.h"

#include <QSet>
#include <QHash>
#include <QString>
#include <QPointer>

IRC_BEGIN_NAMESPACE

class IrcNetworkPrivate
{
    Q_DECLARE_PUBLIC(IrcNetwork)

public:
    IrcNetworkPrivate();

    void setInfo(const QHash<QString, QString>& info);
    void setAvailableCapabilities(const QSet<QString>& capabilities);
    void setActiveCapabilities(const QSet<QString>& capabilities);

    void setName(const QString& name);
    void setModes(const QStringList& modes);
    void setPrefixes(const QStringList& prefixes);
    void setChannelTypes(const QStringList& types);
    void setStatusPrefixes(const QStringList& prefixes);

    static QString getPrefix(const QString& str, const QStringList& prefixes);
    static QString removePrefix(const QString& str, const QStringList& prefixes);

    static IrcNetwork* create(IrcConnection* connection)
    {
        return new IrcNetwork(connection);
    }

    static IrcNetworkPrivate* get(const IrcNetwork* network)
    {
        return network->d_ptr.data();
    }

    IrcNetwork* q_ptr = nullptr;
    QPointer<IrcConnection> connection;
    bool initialized = false;
    QString name;
    QStringList modes, prefixes, channelTypes, channelModes, statusPrefixes;
    QHash<QString, int> numericLimits, modeLimits, channelLimits, targetLimits;
    QSet<QString> availableCaps, requestedCaps, activeCaps;
    bool skipCapabilityValidation = false;
};

IRC_END_NAMESPACE

#endif // IRCNETWORK_P_H

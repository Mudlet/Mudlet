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

#ifndef IRCMESSAGECOMPOSER_P_H
#define IRCMESSAGECOMPOSER_P_H

#include <IrcGlobal>
#include <QtCore/qstack.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

IRC_BEGIN_NAMESPACE

class IrcMessage;
class IrcConnection;
class IrcNumericMessage;

class IrcMessageComposer : public QObject
{
    Q_OBJECT

public:
    IrcMessageComposer(IrcConnection* connection);

    static bool isComposed(int code);

    void composeMessage(IrcNumericMessage* message);

Q_SIGNALS:
    void messageComposed(IrcMessage* message);

private:
    void finishCompose(IrcMessage* message);
    void replaceParam(int index, const QString& param);

    struct Data {
        IrcConnection* connection;
        QStack<IrcMessage*> messages;
    } d;
};

IRC_END_NAMESPACE

#endif // IRCMESSAGECOMPOSER_P_H

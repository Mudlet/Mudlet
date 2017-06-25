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

#ifndef IRCTOKEN_P_H
#define IRCTOKEN_P_H

#include "ircglobal.h"
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

IRC_BEGIN_NAMESPACE

class IrcToken
{
public:
    IrcToken() : idx(-1), pos(-1) { }
    IrcToken(int index, int position, const QString& text)
        : idx(index), pos(position), str(text) { }

    bool isValid() const { return idx != -1; }
    int index() const { return idx; }
    int position() const { return pos; }
    int length() const { return str.length(); }
    QString text() const { return str; }

private:
    int idx;
    int pos;
    QString str;
    friend class IrcTokenizer;
};

class IrcTokenizer
{
public:
    IrcTokenizer(const QString& str = QString());

    int count() const;
    bool isEmpty() const;
    QList<IrcToken> tokens() const;
    IrcToken at(int index) const;
    IrcTokenizer mid(int index) const;

    void clear();
    void replace(int index, const QString& text);
    IrcToken find(int pos) const;
    QString toString() const;

private:
    int len;
    QList<IrcToken> t;
};

IRC_END_NAMESPACE

#endif // IRCTOKEN_P_H

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

#include "ircutil.h"

IRC_BEGIN_NAMESPACE

/*!
    \file ircutil.h
    \brief \#include &lt;IrcUtil&gt;
 */

/*!
    \namespace IrcUtil
    \ingroup util
    \brief Module meta-type registration.
 */

namespace IrcUtil {

    /*!
        Registers IrcUtil types to the %Qt meta-system.

        \sa IrcCore::registerMetaTypes(), IrcModel::registerMetaTypes(), qRegisterMetaType()
     */
    void registerMetaTypes()
    {
        qRegisterMetaType<IrcCommandParser*>("IrcCommandParser*");
        qRegisterMetaType<IrcCompleter*>("IrcCompleter*");
        qRegisterMetaType<IrcLagTimer*>("IrcLagTimer*");
        qRegisterMetaType<IrcPalette*>("IrcPalette*");
        qRegisterMetaType<IrcTextFormat*>("IrcTextFormat*");
    }
}

IRC_END_NAMESPACE

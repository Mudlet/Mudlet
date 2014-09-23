/*
* Copyright (C) 2008-2012 J-P Nurmi <jpnurmi@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "connectionwizard.h"
#include "userwizardpage.h"
#include "serverwizardpage.h"
#include "connectionwizardpage.h"

ConnectionWizard::ConnectionWizard(QWidget* parent) : QWizard(parent)
{
    setWindowTitle(tr("Connect"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setPage(UserPage, new UserWizardPage(this));
    setPage(ServerPage, new ServerWizardPage(this));
    setPage(ConnectionPage, new ConnectionWizardPage(this));
    setOption(NoDefaultButton, false);
}

ConnectionInfo ConnectionWizard::connection() const
{
    ConnectionInfo conn;
    conn.nick = static_cast<UserWizardPage*>(page(UserPage))->nickName();
    conn.real = static_cast<UserWizardPage*>(page(UserPage))->realName();
    conn.host = static_cast<ServerWizardPage*>(page(ServerPage))->hostName();
    conn.port = static_cast<ServerWizardPage*>(page(ServerPage))->port();
    conn.secure = static_cast<ServerWizardPage*>(page(ServerPage))->isSecure();
    conn.pass = static_cast<ServerWizardPage*>(page(ServerPage))->password();
    conn.name = static_cast<ConnectionWizardPage*>(page(ConnectionPage))->connectionName();
    return conn;
}

void ConnectionWizard::setConnection(const ConnectionInfo& conn)
{
    static_cast<UserWizardPage*>(page(UserPage))->setNickName(conn.nick);
    static_cast<UserWizardPage*>(page(UserPage))->setRealName(conn.real);
    static_cast<ServerWizardPage*>(page(ServerPage))->setHostName(conn.host);
    static_cast<ServerWizardPage*>(page(ServerPage))->setPort(conn.port);
    static_cast<ServerWizardPage*>(page(ServerPage))->setSecure(conn.secure);
    static_cast<ServerWizardPage*>(page(ServerPage))->setPassword(conn.pass);
    static_cast<ConnectionWizardPage*>(page(ConnectionPage))->setConnectionName(conn.name);
}

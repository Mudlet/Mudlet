/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "glwidget_integration.h"
#include "Host.h"

QOpenGLWidget* GLWidgetFactory::createGLWidget(TMap* pMap, Host* pHost, QWidget* parent)
{
    if (pHost && pHost->getUseModern3DMapper()) {
        return new ModernGLWidget(pMap, pHost, parent);
    }
    return new GLWidget(pMap, pHost, parent);
}

bool GLWidgetFactory::isCorrectWidgetType(QOpenGLWidget* widget, Host* pHost)
{
    if (!widget || !pHost) {
        return false;
    }
    
    bool shouldUseModern = pHost->getUseModern3DMapper();
    bool isModern = dynamic_cast<ModernGLWidget*>(widget) != nullptr;
    
    return shouldUseModern == isModern;
}

QString GLWidgetFactory::getWidgetTypeName(QOpenGLWidget* widget)
{
    if (!widget) {
        return QStringLiteral("null");
    }

    if (dynamic_cast<ModernGLWidget*>(widget)) {
        return QStringLiteral("ModernGLWidget");
    }
    if (dynamic_cast<GLWidget*>(widget)) {
        return QStringLiteral("GLWidget");
    }
    return QStringLiteral("Unknown");
}

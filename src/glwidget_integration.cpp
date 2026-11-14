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

#if defined(Q_OS_MACOS)
#include <QSurfaceFormat>
#include <QWindow>
#endif

void GLWidgetFactory::matchWindowTo3DView([[maybe_unused]] QWidget* parent, [[maybe_unused]] const bool modern)
{
#if defined(Q_OS_MACOS)
    // macOS shares nothing between a core profile context and a legacy one, and a window composes its 3D view through a context made to its own
    // format when its first 3D view arrives; unshared, the window shows a shrunken picture of itself where the map should be
    const auto profiled = [modern](QSurfaceFormat format) {
        format.setVersion(modern ? 3 : 2, modern ? 3 : 0);
        format.setProfile(modern ? QSurfaceFormat::CoreProfile : QSurfaceFormat::NoProfile);
        return format;
    };
    // Windows made later, such as a floated map, take the profile of the last 3D view made, as does a classic view's own context
    QSurfaceFormat::setDefaultFormat(profiled(QSurfaceFormat::defaultFormat()));
    if (QWindow* pWindow = parent ? parent->window()->windowHandle() : nullptr; pWindow && pWindow->surfaceType() != QSurface::OpenGLSurface) {
        pWindow->setFormat(profiled(pWindow->requestedFormat()));
    }
#endif
}

QOpenGLWidget* GLWidgetFactory::createGLWidget(TMap* pMap, Host* pHost, QWidget* parent)
{
    const bool modern = pHost && pHost->getUseModern3DMapper();
    matchWindowTo3DView(parent, modern);
    if (modern) {
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

void GLWidgetFactory::setViewCenter(QOpenGLWidget* widget, const int areaId, const int x, const int y, const int z)
{
    if (auto* glWidget = dynamic_cast<GLWidget*>(widget)) {
        glWidget->setViewCenter(areaId, x, y, z);
    } else if (auto* modernWidget = dynamic_cast<ModernGLWidget*>(widget)) {
        modernWidget->setViewCenter(areaId, x, y, z);
    }
}

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "TNullConsoleFrontend.h"

#include "Host.h"
#include "TLabelModel.h"

#include <QStringList>

TNullConsoleFrontend::TNullConsoleFrontend(Host* pHost)
: mpHost(pHost)
{
}

TNullConsoleFrontend::~TNullConsoleFrontend() = default;

void TNullConsoleFrontend::dropWindows()
{
    while (!mSubConsoles.empty()) {
        removeSubConsole(mSubConsoles.begin()->first);
    }
    while (!mLabels.empty()) {
        removeLabel(mLabels.begin()->first);
    }
}

QString TNullConsoleFrontend::userWindowOrMain(const QString& windowname) const
{
    return mpHost->windowRegistry().hasDockWidget(windowname) ? windowname : QString();
}

void TNullConsoleFrontend::createLabel(const QString& windowname, const QString& name, int x, int y, int width, int height, bool, bool)
{
    auto pLabel = std::make_unique<TLabelModel>(mpHost, name);
    pLabel->mGeometry = QRect(x, y, width, height);
    pLabel->mVisible = true;
    mpHost->windowRegistry().registerLabel(name, pLabel.get());
    mLabels[name] = {std::move(pLabel), userWindowOrMain(windowname)};
}

void TNullConsoleFrontend::deleteLabel(const QString& name)
{
    removeLabel(name);
}

void TNullConsoleFrontend::removeLabel(const QString& name)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return;
    }
    mpHost->windowRegistry().deregisterLabel(name, it->second.pModel.get());
    mLabels.erase(it);
}

bool TNullConsoleFrontend::setLabelText(const QString& name, const QString& text)
{
    const auto it = mLabels.find(name);
    if (it == mLabels.end()) {
        return false;
    }
    it->second.pModel->mText = text;
    return true;
}

TConsoleModel& TNullConsoleFrontend::addSubConsole(const QString& name, const TWindowRegistry::SubConsoleKind kind, const QString& windowname)
{
    // As resolveConsoleModel() and the TConsole constructor set up a sub-console's model
    auto pModel = std::make_unique<TConsoleModel>(mpHost);
    pModel->mConsoleName = name;
    pModel->mScriptAddressable = true;
    pModel->mScrollBarEnabled = false;
    TConsoleModel& model = *pModel;
    mpHost->windowRegistry().registerSubConsole(name, &model, kind);
    mSubConsoles[name] = {std::move(pModel), userWindowOrMain(windowname)};
    return model;
}

void TNullConsoleFrontend::removeSubConsole(const QString& name)
{
    const auto it = mSubConsoles.find(name);
    if (it == mSubConsoles.end()) {
        return;
    }
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (registry.subConsoleKind(name) == TWindowRegistry::SubConsoleKind::UserWindow) {
        QStringList contents;
        for (const auto& [childName, child] : mSubConsoles) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeSubConsole(childName);
        }
        contents.clear();
        for (const auto& [childName, child] : mLabels) {
            if (child.userWindow == name) {
                contents << childName;
            }
        }
        for (const QString& childName : contents) {
            removeLabel(childName);
        }
        registry.deregisterDockWidget(name);
    }
    registry.deregisterSubConsole(name, it->second.pModel.get());
    mSubConsoles.erase(it);
}

void TNullConsoleFrontend::createBuffer(const QString& name)
{
    TConsoleModel& model = addSubConsole(name, TWindowRegistry::SubConsoleKind::Buffer, QString());
    model.setWrapAt(mpHost->mWrapAt);
    model.setIndentCount(mpHost->mWrapIndentCount);
    model.setHangingIndentCount(mpHost->mWrapHangingIndentCount);
}

void TNullConsoleFrontend::addMiniConsole(const QString& windowname, const QString& name, int x, int y, int width, int height)
{
    addSubConsole(name, TWindowRegistry::SubConsoleKind::MiniConsole, windowname);
    mpHost->windowRegistry().setSubConsoleGeometry(name, QRect(x, y, width, height));
    mpHost->windowRegistry().setSubConsoleVisible(name, true);
}

void TNullConsoleFrontend::deleteMiniConsole(const QString& name)
{
    if (mSubConsoles.count(name)) {
        mpHost->windowRegistry().forgetUserWindowSize(name);
        removeSubConsole(name);
    }
}

void TNullConsoleFrontend::closeSubConsole(const QString& name)
{
    if (!mSubConsoles.count(name)) {
        return;
    }
    if (mpHost->isClosingDown()) {
        removeSubConsole(name);
    } else {
        mpHost->windowRegistry().setSubConsoleVisible(name, false);
    }
}

void TNullConsoleFrontend::openUserWindow(const QString& name, bool, bool, const QString&)
{
    TWindowRegistry& registry = mpHost->windowRegistry();
    if (!mSubConsoles.count(name)) {
        addSubConsole(name, TWindowRegistry::SubConsoleKind::UserWindow, QString());
        registry.registerDockWidget(name);
        registry.setUserWindowTitle(name, name);
        registry.setUserWindowStyleSheet(name, mpHost->mProfileStyleSheet);
    }
    registry.setSubConsoleVisible(name, true);
}

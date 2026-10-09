#ifndef MUDLET_TAPPFRONTEND_H
#define MUDLET_TAPPFRONTEND_H

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

class Host;
class QString;

// The application shell as core code sees it. The mudlet main window implements it;
// core code reaches it through instance() instead of naming the window class.
class TAppFrontend
{
public:
    // nullptr until the main window exists, and again from the end of its destructor.
    static TAppFrontend* instance() { return smpInstance; }
    static void setInstance(TAppFrontend* frontend) { smpInstance = frontend; }

    // The profile in the active tab, or nullptr if there is none or it has no console yet.
    virtual Host* getActiveHost() = 0;
    virtual void announce(const QString& text, const QString& processing, bool isPlain) = 0;
    virtual void showOptionsDialog(const QString& tab, Host* pHost) = 0;
    virtual void showConnectionDialog() = 0;
    virtual void handleTelnetUri(const QString& uri) = 0;
    // Only the profile in the active tab drives the menu's checkbox.
    virtual void setCompactInputLineChecked(Host* pHost, bool checked) = 0;

protected:
    // The main window owns itself, so nothing deletes it through this interface.
    ~TAppFrontend() = default;

private:
    inline static TAppFrontend* smpInstance = nullptr;
};

#endif // MUDLET_TAPPFRONTEND_H

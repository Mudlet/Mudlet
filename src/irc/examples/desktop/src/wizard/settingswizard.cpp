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

#include "settingswizard.h"
#include "generalwizardpage.h"
#include "shortcutswizardpage.h"
#include "colorswizardpage.h"
#include "messageswizardpage.h"

SettingsWizard::SettingsWizard(QWidget* parent) : QWizard(parent)
{
    setWindowTitle(tr("Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setPage(GeneralPage, new GeneralWizardPage(this));
    setPage(ShortcutsPage, new ShortcutsWizardPage(this));
    setPage(MessagesPage, new MessagesWizardPage(this));
    setPage(ColorsPage, new ColorsWizardPage(this));

    setOption(HaveCustomButton1, true);
    setButtonText(QWizard::CustomButton1, tr("Reset"));
    connect(button(QWizard::CustomButton1), SIGNAL(clicked()), this, SLOT(resetCurrentPage()));

    setOption(NoDefaultButton, false);
}

Settings SettingsWizard::settings() const
{
    Settings settings;
    settings.font = static_cast<GeneralWizardPage*>(page(GeneralPage))->font().toString();
    settings.layout = static_cast<GeneralWizardPage*>(page(GeneralPage))->layout();
    settings.language = static_cast<GeneralWizardPage*>(page(GeneralPage))->language();
    settings.maxBlockCount = static_cast<GeneralWizardPage*>(page(GeneralPage))->maxBlockCount();
    settings.timeStamp = static_cast<GeneralWizardPage*>(page(GeneralPage))->timeStamp();
    settings.stripNicks = static_cast<GeneralWizardPage*>(page(GeneralPage))->stripNicks();
    settings.shortcuts = static_cast<ShortcutsWizardPage*>(page(ShortcutsPage))->shortcuts();
    settings.messages = static_cast<MessagesWizardPage*>(page(MessagesPage))->messages();
    settings.highlights = static_cast<MessagesWizardPage*>(page(MessagesPage))->highlights();
    settings.colors = static_cast<ColorsWizardPage*>(page(ColorsPage))->colors();
    return settings;
}

void SettingsWizard::setSettings(const Settings& settings)
{
    setGeneralSettings(settings);
    setShortcutSettings(settings);
    setMessageSettings(settings);
    setColorSettings(settings);
}

void SettingsWizard::setGeneralSettings(const Settings& settings)
{
    QFont font;
    if (!settings.font.isEmpty())
        font.fromString(settings.font);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setFont(font);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setLayout(settings.layout);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setLanguage(settings.language);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setMaxBlockCount(settings.maxBlockCount);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setTimeStamp(settings.timeStamp);
    static_cast<GeneralWizardPage*>(page(GeneralPage))->setStripNicks(settings.stripNicks);
}

void SettingsWizard::setShortcutSettings(const Settings& settings)
{
    static_cast<ShortcutsWizardPage*>(page(ShortcutsPage))->setShortcuts(settings.shortcuts);
}

void SettingsWizard::setMessageSettings(const Settings& settings)
{
    static_cast<MessagesWizardPage*>(page(MessagesPage))->setMessages(settings.messages);
    static_cast<MessagesWizardPage*>(page(MessagesPage))->setHighlights(settings.highlights);
}

void SettingsWizard::setColorSettings(const Settings& settings)
{
    static_cast<ColorsWizardPage*>(page(ColorsPage))->setColors(settings.colors);
}

void SettingsWizard::resetCurrentPage()
{
    Settings settings; // default values
    switch (currentId())
    {
        case GeneralPage: setGeneralSettings(settings); break;
        case ShortcutsPage: setShortcutSettings(settings); break;
        case MessagesPage: setMessageSettings(settings); break;
        case ColorsPage: setColorSettings(settings); break;
        default: Q_ASSERT(false); break;
    }
}

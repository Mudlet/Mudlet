-- After editing this file or mudlet-base-ui.xml, rebuild mudlet-base-ui.mpackage:
-- it is a zip of config.lua + mudlet-base-ui.xml + .mudlet/Icon/mudlet.png.
mpackage = [[mudlet-base-ui]]
author = [[Mudlet Makers]]
icon = [[mudlet.png]]
title = [[A starter interface with health bars, map and chat, built from what your game provides.]]
description = [[# Mudlet base UI

A modest starter interface for players new to Mudlet: an adjustable dock
with your map, tabbed chat (All/Tells/Channels with unread counters) and
health, mana, movement and experience gauges - built only from data your
game actually provides (GMCP, MSDP, or recognisable prompt and score
lines). Nothing appears until the game sends something to show.

No section is stuck in the dock: drag the map, the chat window or the
health bars out by its header row and it will sit wherever you drop it
over the game's text. The sections left behind share out the space it
had, and where you leave each one is remembered.

It comes in three looks - Midnight, Ember and Hardlight. Click the ◐ in
the dock's title bar to try each in turn; a look colors the dock and,
if you have not chosen a background color of your own, the background
the game's text sits on as well.

Commands:

```
> baseui           -- show status and options
> baseui hide      -- remove the interface (remembered between sessions)
> baseui show      -- bring it back
> baseui dock      -- put every section back in the dock
> baseui ember     -- change the look (midnight, ember or hardlight)
```

When a game installs an interface of its own, this one quietly stands
aside - `baseui show` brings it back if you prefer it.
]]
version = [[1.6.1]]
created = "2026-07-25T12:00:00+00:00"

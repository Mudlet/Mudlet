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

Commands:

```
> baseui        -- show status and options
> baseui hide   -- remove the interface (remembered between sessions)
> baseui show   -- bring it back
```

When a game installs an interface of its own, this one quietly stands
aside - `baseui show` brings it back if you prefer it.
]]
version = [[1.0.0]]
created = "2026-07-25T12:00:00+00:00"

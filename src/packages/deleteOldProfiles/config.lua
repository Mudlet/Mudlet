mpackage = [[deleteOldProfiles]]
author = [[Mudlet Default Package]]
icon = [[mudlet.png]]
title = [[Remove excess backup files.]]
description = [[# deleteOldProfiles Package

Mudlet continuiously creates backups of important data.  This can result in a lot
of files.  This package deletes old profiles, maps and modules in the 
"current", "map" and "moduleBackups" folders of the Mudlet home directory that are
no longer required.

The commands are;

```
> delete old profiles [days]
> delete old maps [days]
> delete old modules[days]
```

Days is optional, the default is 31 days.

The following files are NOT deleted:

- Files newer than the amount of days specified, or 31 days if not specified.
- One file for every month before that. Specifically: The first available file of every month prior to this.

```
-- Examples: 
> delete old profiles   -- deletes profiles older than 31 days  
> delete old maps 10    -- deletes maps older than 10 days
```
]]
version = [[3]]
created = "2024-08-24T08:26:45+02:00"

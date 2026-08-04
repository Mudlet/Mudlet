mpackage = [[echo]]
author = [[Mudlet Default Package]]
icon = [[mudlet.png]]
title = [[A set of aliases to test triggers on the command line.]]
description = [[# Echo Package

The echo package provides a means of testing triggers via the command line with four command aliases;
`` `echo, `cecho, `decho, `hecho``.  

All act as if the given text came from the game itself and will fire any matching triggers.  

See [Triggers](https://wiki.mudlet.org/w/Manual:Introduction#Triggers) for further information on matching text.
  
## `echo Alias

Displays text on the screen and tells all matching triggers to fire.  For coloring use one
of the other functions mentioned below.

```
-- examples
> `echo text - displays text on the main screen and tells all matching triggers to fire
> `echo This is a sample line from the game$$And this is a new line.
```
See [echo](https://wiki.mudlet.org/w/Manual:Lua_Functions#echo), [feedTriggers](https://wiki.mudlet.org/w/Manual:Lua_Functions#feedTriggers), 
  
## `cecho Alias

Like echo, but you can add color information using color names and ANSI values.

```
-- example: color format is <foreground:background>
> `cecho <green:red>green on red<r> reset$$<124:100>foreground of ANSI124 and background of ANSI100<r>
```
See [cecho](https://wiki.mudlet.org/w/Manual:Lua_Functions#cecho), [cfeedTriggers](https://wiki.mudlet.org/w/Manual:Lua_Functions#cfeedTriggers).

## `decho Alias

Like cecho, but you can add color information using <r,g,b> format.

```
-- example
> `decho <0,128,0:128,0,0>green on red<r> reset
```
See [decho](https://wiki.mudlet.org/w/Manual:Lua_Functions#decho), [dfeedTriggers](https://wiki.mudlet.org/w/Manual:Lua_Functions#dfeedTriggers).

## `hecho Alias

Like cecho, but you can add color information using hex #RRGGBB format.

```
-- example
> `hecho #008000,800000green on red#r reset
```
See [hecho](https://wiki.mudlet.org/w/Manual:Lua_Functions#hecho), [hfeedTriggers](https://wiki.mudlet.org/w/Manual:Lua_Functions#hfeedTriggers).
]]
version = [[1]]
created = "2024-08-24T08:27:19+02:00"

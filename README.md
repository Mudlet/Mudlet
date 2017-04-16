# Mudlet

[![Build Status](https://travis-ci.org/Mudlet/Mudlet.svg?branch=development)](https://travis-ci.org/Mudlet/Mudlet) [![GitHub stars](https://img.shields.io/github/stars/Mudlet/Mudlet.svg)](https://github.com/Mudlet/Mudlet/stargazers)
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Mudlet/Mudlet?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Mudlet is a quality MUD client, designed to take mudding to a new level.

It’s a new breed of a client on the MUD scene – with an intuitive user interface, a specially designed scripting framework, and a very fast text display. That, along with cross-platform capability and an open-source development model results in a very likable MUD client.

## Easy to use client

We’re big on usability, and as such, creating an easy to use client and interface is one of the defining goals of the project. This applies to both the power users and plain gamers – everyone will feel at home with Mudlet, without having to waste too much time figuring out how to do something. Comments on display improvement are always welcome, and we’ll continue refining the design to perfection continually.

## Designed for speed

Mudlet is designed to be very fast and efficient right from the start, and we’re building it to be so. It’s scripting engine is designed to scale to large systems without bogging down – and the text display is designed to handle thousands of lines under a second. All in all, we are very serious about Mudlet being quick – and take all chances to make it so.

## Powerful Scripting

Mudlet features a scripting framework using [Lua](https://www.lua.org/) – a small, fast and efficient scripting language. This allows Mudlet to leverage the existing community and large ecosystem of existing packages for Lua without the many drawbacks of creating a Mudlet specific scripting language. Best of all, the Lua API is seamlessly integrated in Mudlet and shared by all aliases, triggers, scripts, keybindings, buttons and other Mudlet components.

## Cross-Platform love

We believe in making Mudlet available to people on all major platforms, and we work on keeping cross-platform compatibility right from start. Mudlet will be available on Linux (both 32bit and 64bit), Windows, and Macs.

## And many other things…

Such as package creation / sharing, 3D mapper are in our plans. You’re welcome to suggest ideas, too!

# About This Fork Repository

__Current Version:__ *3.0.1-dev-fae8b*  

This Fork of [Mudlet 3.x](https://github.com/Mudlet/Mudlet "Mudlet 3.x Official Repository") intended to expand IRC and other functionality in Mudlet, mainly for the needs of [Botman - A 7 Days To Die Server Manager](http://www.botman.nz/ "Botman, A 7 Days To Die Server Manager").  
This branch contains code which has been compiled and lightly tested but is based on Development branch source code and may not always be stable.  
  
The code here has been built & tested on Ubuntu 16.04.  

## Compatibility Differences

* Lua Function `sendIrc()` now returns 1 on successful send or 0 on errors / not ready for sending. Used to return nil.  
* Lua Function `connectToServer()` now accepts a third optional argument `saveToProfile` (boolean). If `saveToProfile` is set to true the connection information given are saved to the active profile.  Example Usage: `connectToServer('12.34.56.78', 13004, true)`   

## Noteworthy Changes

* IRC Settings are now available in the Profile Preferences/Settings window, and save per-profile.  
* The IRC Chat now connects to `localhost:6667` until configured by profile settings or Lua function calls.  
* The IRC Chat window will not open unless you have loaded a connection profile.  
* The IRC client is now started automatically by calls to `sendIrc()` from lua, unless disabled manually.  
* The IRC client will attempt to reconnect to the current connected IRC server if an unexpected disconnection occurs.  
* The IRC can be disabled completely by setting the IRC Nickname setting to *"DisableMudletIRC"* (without " marks.)  
* The IRC can also be stopped and started manually by issuing commands `/stopirc` and `/startirc` in Mudlet's IRC chat window.  
* New Lua functions for interacting with the IRC Client functions and configurations.  
* More 'slash' commands supported by the IRC Client.  
* New Lua functions for saving and loading window layouts to the active profile.

## New IRC Window Commands

* `/help` - Shows IRC Command help text.  
* `/join <Channel>` - Join a channel.  
* `/part <Channel>` - Leave a channel.  
* `/listusers [Channel]` - Updates the nicklist for the Main channel or the channel specified.  
* `/listchans` - Lists currently Joined channels, including the Main channel.  
* `/setchan [Channel]` - Sets the Main IRC Channel, leaving the previous main channel.  
* `/joinserver <hostname>[:<port>]` - Changes the current server connection settings and reconnects if needed.  
* `/whois <Nick>` - Requests Whois information about given Nick.  
* `/whowas <Nick>` - Requests Whowas information about the given Nick.  
* `/topic <Channel> [New Topic]` - Gets the topic for given Channel, optionally sets Topic on Channel.  
* `/mode <Target> [New Modes]` - Gets the modes of Target (a valid Channel or the IRC Client Nick) and optionally can set modes on Target.  
* `/oper <User> <Password>` - Sends the OPER command to the IRC server using given User and Password credentials. *__Note:__ This is a server-wide OPER, not a CHANOP.*  
* `/op <Nick> [Channel]` - Grants CHANOP to given Nick in the given Channel. If Channel is omitted the Main channel is used.  
* `/deop <Nick> [Channel]` - Revokes CHANOP from given Nick in the given Channel. If Channel is omitted the Main channel is used.  

## New Lua Functions

* `ircGetHost()` - Gets the sessions IRC Host configuration as a string with the format <host>:<port>.  
* `ircGetNick()` - Gets the sessions IRC Nick name configuration as a string.
* `ircGetChannel()` - Gets the sessions Main IRC Channel configuration as a string.  
* `ircSetHost( Host, Port )` - Sets the active Host (and optional Port) used for the IRC client. Returns nil.  
* `ircSetNick( NickName )` - Sets the active IRC NickName. Returns nil.  
* `ircSetChannel( Channel )` - Sets the Main IRC Channel to send and operate on. Returns nil.  
* `ircSaveSessionConfigs()` - Saves the session IRC configuration to the profile configuration files.  
* `ircJoin( Channel )` - Attempts to join an additional Channel. Returns int, 1 on assumed success, 0 on local failure. *__Note:__ Return value does not indicate actual Join.*  
* `ircPart( Channel )` - Attempts to leave a Channel previously joined, or closes connection if all channels are left.  Returns int, 1 on assumed success and 0 on local failure. *__Note:__ Return value does not indicate actual Part.*  
* `ircGetChannels()` - Returns a string of currently joined channels, delimited by spaces. Returns an empty string when not connected.  
* `ircGetNicks( Channel )` - Requests the Nick list from the Channel given, activates `sysIrcStatusMessage` event when the list has been fetched.  Returns string, a cached Nick list for the Channel if available or empty string if not.  
* `ircWhoIs( Nick )` - Requests IRC Whois information for the given Nick. Results are passed back via the `sysIrcStatusMessage` event.  
* `ircWhoWas( Nick )` - Requests IRC Whowas information for the given Nick. Results are passed back via the `sysIrcStatusMessage` event.  
* `ircOper( User, Password )` - Sends the OPER command to the IRC Server. Results are passed back via the `sysIrcStatusMessage` event.  
* `ircOp( Nick, [Channel] )` - Grants CHANOP to given Nick in the given Channel. If Channel is omitted the Main Channel will be used. *__Note:__ Requires OPER or CHANOP first.*  
* `ircDeOp( Nick, [Channel])` - Revokes CHANOP from given Nick in the given Channel. If Channel is omitted the Main Channel will be used. *__Note:__ Requires OPER or CHANOP first.*   
* `ircIsOper()` - Returns a boolean value denoting if the client has successfully been granted OPER on the IRC Server.  
* `ircIsChanOp( [Channel] )` - Returns a boolean value denoting if the client has been granted CHANOP on the given Channel. If Channel is omitted the default Main IRC channel is used.  
* `ircMode( Target, [Modes] )` - Will get or set Modes of Target, Target must be a valid Channel name or the current Nick of the IRC Client. Results are passed back via the `sysIrcStatusMessage` event. *__Note:__ Requires OPER or CHANOP first.*  
* `ircTopic( Channel, [Message] )` - Will get or set the Topic message of the given Channel. Results are passed back via the `sysIrcStatusMessage` event. *__Note:__ Requires OPER or CHANOP first.*   
* `ircReconnect()` - Will simply and forcefully disconnect the IRC Client and then reconnect using the IRC settings in the active profile.  Returns nil.    
* `saveWindowLayout( [Version] )` - Saves the current window layout to the active profile, Version integer is option and defaults to 0.  
* `loadWindowLayout( [Version] )` - Loads a window layout from the active profile if it exists, Version integer is optional and defaults to 0.  
* `saveProfile()` - Saves the currently active profile to disk. Unsaved items may not be saved.  
* `closeMudlet()` - Closes the Mudlet application completely, does not save profile data or prompt the user to save it before closing.  
  
## New Lua Events

* `sysIrcStatusMessage` - Event much like `sysIrcMessage` but for handling IRC Client status updates. This event is only activated when certain functions are called, however the event may fire several times per each function call until a specific IRC Code # is received from the server which ends the event sequence.  
   Three string type arguments are passed to this event's handler function:  
    -- *Subject*  is usualy a Channel name or Nick name.  
    -- *Data*  is usually the text sent back from the IRC Server, re-formatted in some cases.  
    -- *Code*  is always a number, indicating the type of IRC message sent from the IRC server.  
   The event is activated by the following Lua functions:  
   * `ircGetNicks( Channel )` - Passes the Channel name as `Subject`, Nick list as `Data`, and the IRC Code #.   
   * `ircWhoIs( Nick )` - Passes the Nick name as `Subject`, Whois line as `Data`, and the IRC Code #.  
   * `ircWhoWas( Nick )` - Passes the Nick name as `Subject`, Whowas line as `Data`, and the IRC Code #.  
   * `ircOper( User, Password )` - Passes the client Nick as `Subject`, the status text as `Data`, and the IRC Code #.
   * `ircMode( Target, [Modes] )` - Passes Target back as `Subject`, the MODE response line as `Data`, and the IRC Code #.  
   * `ircTopic( Channel, [Message] )` - Passes Channel back as `Subject`, the Topic query response line as `Data` and the IRC Code #.  
   





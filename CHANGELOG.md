# [Unreleased] 4.12 - in development

This version will be the first to use this changelog file for gathering notes

 # [Unreleased] 4.11 - to be released soon

This release is currently prepared outside of this changelog file

# 4.10 – extra command lines, background images, and more

This release has been a long time coming, thanks to the vagaries of obtaining a new code signing certificate for our Windows builds. Consequently there are a lot of changes which made it into Mudlet 4.10. Let’s go over some of the highlights!

## What is Mudlet?

It’s a platform with which you can play multiplayer, pure-text RPGs – called MUDs, precursors to today’s MMORPGs. There are thousands of unique worlds online – [download](https://www.mudlet.org/download/) Mudlet for free on Windows/macOS/Linux, join one and enjoy!

## Extra command lines

You’ve been asking for it, you waited patiently, now thanks to [Edru](https://github.com/Edru2/) you can have… multiple command lines! Whenever you want a second one for role-playing, your chat window to have configuration, or anything else, this is now possible:

![](https://wiki.mudlet.org/images/0/0c/AdditionalCommandLine.png)

To turn on a commandline in a miniconsole, [see here](https://wiki.mudlet.org/w/Manual:Geyser#Enable_and_use_your_miniconsole_command_line). To create a stand-alone, free floating command line, [see here](https://wiki.mudlet.org/w/Manual:Geyser#Geyser.CommandLine).

## Background images for miniconsoles

You know what else you’ve been waiting and asking for? Background images on miniconsoles! Edru has been on a roll!

![enter image description here](https://www.mudlet.org/wp-content/uploads/2020/11/Selection_363.png)

This also means that the usual c/d/heho color formatting functions now accept transparency as a parameter. [See here](https://wiki.mudlet.org/w/Manual:Geyser#Change_your_miniconsole_background_image) on how to set a background image.

This also means that you can try setting a background image for the main window as well – but you might find that games often hardcode the black background, so it’s not ideal.

## Math in Geyser

Have you been struggling to get your Geyser elements lined up **just right** and wished you could nudge it over just a few pixels? Been avoiding Geyser and using another option so you could have math in your constraints? Well, now you can use Geyser and receive all of its updates and use math in your constraints.

This means for example if you have a 32×32 label you want to center you can do so by using Label1:move(“50%-16px“,“50%+10px“), or using “50%-16px” for both the x and y when making the label.

**Note:** this improvement will also find your typos! For example if you had “95%%” before, that (invalid) constraint won’t work anymore. Find these bugs easily by searching for %%.

## Other Geyser Improvements

Geyser.Label also allows you to use “nocolor” as the text color, which will then allow for coloring the text via CSS. It also picked up the Geyser.Label:rawEcho(txt) function, which skips over all the formatting assistance normally provided by Geyser.Label:echo().

Geyser.MiniConsole gains Geyser.MiniConsole:display() which works just like regular display(), but outputs in the miniconsole instead of your main display.
Tired of copying things to a miniconsole and not being able to click them? Wish you could keep your MXP links intact in a miniconsole? Well, now you can! Edru once again stepped up to the plate for this one.

## New color functions for feedTriggers

Have you struggled to test your color triggers? Tired of hand-crafting escape sequences when using feedTriggers()? Well, Demonnic was even if you weren’t, so he added some new functions for working with feedTriggers. decho2ansi() and hecho2ansi() will take any decho or hecho string and return an ansi colored string. The functions are used by the new feedTriggers variants, dfeedTriggers and hfeedTriggers which allow for you to send colored text to the trigger engine for testing your color triggers. Also added is cfeedTriggers, which uses color names but uses a limited color table which corresponds to the ansi standard colors, IE black, red, green, yellow, blue, magenta, cyan, white, and lightBlack, lightRed, etc. You can also use a number from 0-255 to get the ansi 256 color equivalent.

## Multi-view forever!

Now you can open ALL THE PROFILES! and move between them with your mouse and keyboard shortcuts without it turning multiview off. Have fun multiplaying!

![](https://www.mudlet.org/wp-content/uploads/2020/11/Selection_368-1024x446.png)

## Telnet CHARSET support

Game admins will be happy about this feature – it is now possible to automatically set Mudlet’s text encoding – for example, to be utf8 in case your game supports emojis – using the [CHARSET protocol](https://tools.ietf.org/html/rfc2066). Big thanks to Tamarindo from [StickMUD](https://www.stickmud.com/) for pulling that one in.

## Music/sound in MXP

Tamarindo also went ahead and added sound/music support to Mudlet’s MXP implementation. That said, there’s a much more modern [Mud Client Media Protocol](https://wiki.mudlet.org/w/Special:MyLanguage/Standards:MUD_Client_Media_Protocol) that you should aim to use 😉

## Scrollbar improvements

You’ll not be surprised to hear this one is Edru as well. Had you noticed Mudlet’s scrollbars behaving a bit… oddly? Jumping around, not resizing itself, stuff like that? Well, Edru’s put a lot of work into this and it should all work much more smoothly now.
 

## macOS 10.13 High Sierra

Apple will stop supporting 10.13 High Sierra in November 2020, and Mudlet will have to follow suit. Thus Mudlet 4.10 will be the last version to support 10.13, which had a good run of 3 years!

## Credits

Thanks to all coders that have contributed to this massive release: Andre Castellanos, Damian Monogue, Eraene, Fae, Gustavo Sousa, Ian Adkins, Jonathan Mohrbacher, Kebap, keneanung, Manuel Wegmann, Matthias Urlichs, Mike Conley, Stack, Stephen Lyons, and Vadim Peretokin.

Thanks to all translators to translated Mudlet into their language: Alan Sneath (alsneath), Anubisa, emeraldboy, Hsin Hsiang Peng (Hsins), Kae (itsTheFae), Leris, Marco “M0lid3us” Tironi (wiploo), rodeos, Vadim Peretokin (vperetokin), vingi, and 王AQ (Anselmus).

This massive piece of work would not be possible without all of you :)

## Changelog

**added:**

- ability to display room names on the map (no auto-align them yet)
- an event to catch all events – `*`
- background image for miniconsoles is now possible, same as labels
- background image for the main window is now possible
- English spellchecking dictionaries are now included
- game admins can now set encoding via telnet CHARSET (so utf8 for emojis)
- miniconsoles can now have commandlines in them
- music is now supported in MXP
- option not to highlight text (history) when pressing the ↑ button
- option to use your own pathfinding algorithm instead of the built-in one
- you can now create standalone commandlines now allow you to ask the player for info
- transparency in now supported by cecho/decho/hecho
- you can now use math in Geyser constraints, ie “50% + 3px”

**added & improved functions:**

- ansi2string() given a string with ANSI codes in it, remove them all
- cfeedTriggers(), dfeedTriggers(), and hfeedTriggers() to enable testing color triggers
- createCommandLine(), enableCommandLine() for creating custom commandlines
- enableHorizontalScrollBar() to show a horizontal scrollbar in miniconsoles as needed
- getClipboardText() and setClipboardText() for interacting with the clipboard
- getFgColor() and getBgColor() now report how many characters the same color applies to
- getHTTP() downloadFile’s cousin and allows you to pass custom headers, joins putHTTP(), postHTTP() family for consistency
- Geyser.Label:createRightClickMenu() to create menus in labels
- Geyser.MiniConsole:display() to display straight into a miniconsole
- Geyser.UserWindow:setStyleSheet(), setUserWindowStyleSheet() to adjust the userwindow title and borders – great for dark themes
- hecho() can now set the background color only, without affecting foreground
- isActive() and exists() now work with buttons
- keepcolor parameter to replaceWildcard() and replaceAll() functions
- purgeMediaCache() to remove MCMP & MSP media files
- setCmdLineStyleSheet() to customize the font, colors, position of the command line
- setMapBackgroundColor() to change background color of the map from the usual black
- setMapRoomExitsColor() to customize the color of room exits drawn on the map
- spairs() which iterates over a table in a sorted way
- tempAlias() and tempKey() now can take Lua functions and not just text
- tempAnsiColor() trigger now works as it should

**improved:**

- ‘errors’ button text is now capitalized
- a message is now shown when a replay ends
- Client.Media – package messages themselves are no longer case sensitive
- compact input line option is now per-profile
- errors console now shows a horizontal scrollbar when needed
- exit line in generic mapper is now case-insensitive, and a few other triggers improved
- in case of compiling Mudlet yourself without TTS support, TTS functions won’t error anymore
- links at the end of the line now won’t be clickable in the empty space
- more accurate row and column reporting through NAWS protocol
- Mudlet logo artwork, now sharper and better-looking. Dev and PTB builds are now clearly distinguishable
- Mudlet’s savefiles are now saved with a more sorting-friendly name
- Mudlet’s usual, development and PTB icons got improved
- multiview tabbars aren’t re-arrangeable anymore, as doing so didn’t re-arrange the views anyhow
- profile history in connection dialog now translates dates
- scrolling now uses the upper pane’s height, so you aren’t missing text when scrolling, or getting too much of it
- shrinking rooms in the mapper will keep everything in its place
- string.split() now works with many more characters
- stylesheets can now manipulate color of Geyser Labels
- tabs and tab stops in profile preferences now make sense, which is a bit friendlier for a11y
- upped the limit of regex capture groups in aliases and triggers to 33
- zooming the mouse in and out in 2D mapper will keep the position more stable

**fixed:**

- ‘map config’ alias in generic mapper now works
- appendBuffer() now keeps links, so copying text with links in it to miniconsoles works
- crash when closing a profile with toolbars
- crash when selecting/copying text
- crash trying to move too many rooms at once via right-click
- fixed MXP with values containing =
- Materia Magica’s MXP now works better with Mudlet
- multiview now stays when you change focus between profiles
- resetProfile() to work again
- static items in HBox/VBoxes won’t be resized anymore when the box changes size
- sysDataSendEvent will no longer reveal the password
- vertical scrollbar now moves when you scroll with PgUp/PgDown
- vertical scrollbar now shrinks in size as you get more text

**infrastructure:**

- added some useful defaults for Visual Studio Code’s C++ integration
- building with libzip 1.70.0 that lacks version numbers now works
- cleaned up IRC code
- debug output now spams a bit less for unprintable codepoints
- deleted Github Action for clearing whitespace (more spam than help)
- enabled Github’s new CodeQL security code scanning
- fixed crash importing map with custom exit lines on Qt 5.14+ (macOS)
- internal code that only works with the main window is now in a separate class of its own, making the programmers happier
- Lua binding code cleaned up and improved for consistency and code style, now more fresh!
- Mudlet can now be built & run remotely in the browser with Github Codespaces
- Mudlet can now be built in MSYS2 on Windows
- Mudlet’s wiki is now upgraded to latest and greatest
- new PTBs are only built on days where there are new changes
- updated Linux builds to use Qt 5.12.9

**4.10.1** fixes the slowness in the 2D mapper, the keepcolor feature in replace(), emoji width are back as they were before, and the new CHARSET function accepts ASCII and ISO variants (for game admins).


# 4.9 – Public Test Builds & ANSI art

We’ve improved Mudlet’s editor and highlighting as well as even more improved art display. Also, you can now every day receive new Mudlet developments with our Public Test Build (PTB) version!

## Public Test Builds

While these have been prepared for a few months now, PTB are finally mature enough to be mentioned here. You can now always play with the most recent Mudlet developments enabled, if you don’t want to wait for the next official release.

New PTB versions are built nightly, hence their icons are dark, whereas new developments start with a dim red sun light. See immediately which is which via distinct icons both before and after you started Mudlet.

[![](https://www.mudlet.org/wp-content/uploads/2020/06/Application-icons.png)](https://www.mudlet.org/wp-content/uploads/2020/06/Application-icons.png)

Mudlet PTB will be installed right next to your regular Mudlet. Meanwhile, the original release version will stay unchanged, so you can always go back. Find the latest PTB on top of Mudlet’s snapshot website: **[https://make.mudlet.org/snapshots/?platform=all&source=ptb](https://make.mudlet.org/snapshots/?platform=all&source=ptb)**

Please beware: Even though we test new features, the PTB may actually still break sometimes. Please report any doubts or feedback to us, so we can fix things long before the official release happens. This is why we’re doing this: for better Mudlet quality.

## Improved ANSI art display

While Mudlet 4.8 started CP437 support, we received a [request for help in Mudlet forums](https://forums.mudlet.org/viewtopic.php?f=9&t=22887). A player wondered why Mudlet would not display their game’s sweet ANSI art upon connection as expected. SlySven investigated and fixed the issue, even surpassing the original request:

[![](https://www.mudlet.org/wp-content/uploads/2020/06/cp437.gif)](https://www.mudlet.org/wp-content/uploads/2020/06/cp437.gif)

Please feel free to let us know any display problems you may find with Mudlet in your games, so we can look into them as well. Or even better, send us a fix to review! ;-)

## Highlight foreground or background only

Even non-tech-savy users can easily use triggers in Mudlet to highlight interesting words or phrases happening in their games. The default highlighting will use red foreground and yellow background color.

[![](https://www.mudlet.org/wp-content/uploads/2020/06/highlight.png)](https://www.mudlet.org/wp-content/uploads/2020/06/highlight.png)

With the new Mudlet version, you can now choose to only use one of them for highlighting and keep the other one as-is. In the example, keep the (maybe different) foreground colors, just use yellow background.

## Editor placeholder text

Mudlet’s feature-rich script-editor (edbee) just learned a new trick: The initial comment “put your Lua code here” will vanish, as soon as you start typing. No more left over fragments at the beginning of your code scripts!

[![editor placeholder text](https://www.mudlet.org/wp-content/uploads/2020/06/placeholder.gif)](https://www.mudlet.org/wp-content/uploads/2020/06/placeholder.gif)

## Credits

Thanks to all coders: Damian Monogue, Edru2, Gustavo Sousa, Kebap, Manuel Wegmann, Nicholas Molen, Stephen Lyons, and Vadim Peretokin.

Thanks to all translators: Alan Sneath (alsneath), Leris, Marco “M0lid3us” Tironi (wiploo), rodeos, and Vadim Peretokin for their work on this every day​.

## Changelog

**added:**

-   Public Test Build (PTB) versions of Mudlet for macOS and Linux
-   New icons for Mudlet’s Public Test Builds and development versions
-   IRE mapper automatically installed for Starmourn as well
-   Lua code in Mudlet can now be translated in Crowdin
-   Option to keep color highlighting, so you aren’t forced to specify both fore and background colors
-   New sysWindowMousePress and sysWindowMouseRelease events for user windows

**improved:**

-   A single echo’s size was raised from the 10k character limit in the last release to a million characters
-   Check for updates will check for updates more regularly
-   The generic mapper script will automatically run ‘find prompt’
-   The generic mapper script will handle room titles with embedded mini ASCII maps on the side
-   Geyser HBox/VBox are now much quicker
-   Geyser now stores stylesheet in the object
-   Mudlet’s website is now shown in the Discord line
-   MXP support rewritten, now supports colours
-   Faster in-script search in the code editor for big scripts – won’t search on first two characters anymore
-   Support graphics on Durismud (mud.durismud.com)

**fixed:**

-   Prevent crash when closing a profile that hasn’t successfully connected
-   Event handlers continue running now after one of them had an error
-   Locking and minimization on creation is working now as expected for Adjustable Containers
-   Module sync can’t be enabled for mpackages, as those aren’t supported for syncing yet
-   Improved pixel precision in Adjustable Containers
-   Replays now process cr+null correctly
-   Time units aren’t translated in the UI anymore, making it work for other languages
-   Userwindows can now still show up if previously closed/hidden
-   Ctrl+C breaking when you have played a lot

**infrastructure:**

-   Linux builds now use Qt 5.12.8
-   Removed unused code

**4.9.1** fixes issues with the MXP links in miniconsoles and hboxes/vboxes of 0 width, which prevented some UIs from loading.


# 4.8 – Drag and drop packages, custom cursors, and amazing maps

This is an amazing release that adds drag and drop packages, custom cursors, draggable labels and more! Mudlet is now more powerful than ever for building stunning, modern interfaces for MUDs.

## Drag and drop packages

Thanks to Edru, who massively improved Geyser and Userwindows in the [last update](http://mudlet.org/4-6), you can now drag and drop packages into Mudlet to install them. Pretty simple!

[![](https://www.mudlet.org/wp-content/uploads/2020/04/drag-and-drop-package-install.gif)](https://www.mudlet.org/wp-content/uploads/2020/04/drag-and-drop-package-install.gif)

Credit: [Eraene’s DarkTheme](https://forums.mudlet.org/viewtopic.php?f=6&t=22728)

Along with this comes a [sysDropEvent](https://wiki.mudlet.org/w/Manual:Event_Engine#sysDropEvent), so you can code a custom action when a file is dropped into Mudlet. If you’re a package author, remember you can make use of the [sysInstall](https://wiki.mudlet.org/w/Manual:Event_Engine#sysInstall) event to notify that your package has been installed. Enjoy!

## Carrion Fields added

[This game](http://www.carrionfields.net/) has put together a seriously impressive Mudlet package and we’re honoured to have them added to Mudlet officially! Double-click on CF in the Connection screen to play :)

[![](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_591-1024x572.png)](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_591.png)

Carrion Fields is a unique blend of high-caliber roleplay and complex, hardcore player-versus-player combat that has been running continuously, and 100% free, for over 25 years.

Choose from among 21 races, 17 highly customizable classes, and several cabals and religions to suit your playstyle and the story you want to tell. Our massive, original world is full of secrets and envied limited objects that take skill to acquire and great care to keep.

We like to think of ourselves as the Dark Souls of MUDs, with a community that is supportive of new players – unforgiving though our world may be. Join us for a real challenge and real rewards: adrenalin-pumping battles, memorable quests run by our volunteer immortal staff, and stories that will stick with you for a lifetime.

## Adjustable Containers

Edru didn’t just stop with drag and drop – [adjustable containers](https://wiki.mudlet.org/w/Manual:Geyser#Adjustable.Container) are in as well! What does it mean? It means labels/miniconsoles that you can move around and reposition just by dragging! No code needed.

[![](https://www.mudlet.org/wp-content/uploads/2020/05/adjustable-container.gif)](https://www.mudlet.org/wp-content/uploads/2020/05/adjustable-container.gif)

Try this code in Mudlet:

testCon = testCon or Adjustable.Container:new({name="testContainer"})  
testLabel = Geyser.Label:new({x=0, y=0, height="100%", width="100%", color="green"},testCon)

## Search within the script

Dicene also added a cool feature – you can now hit **Ctrl+F** to search within the script only, not your entire Mudlet profile!

[![](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_583.png)](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_583.png)

He didn’t just stop there either – you can now toggle whenever you’d like to search within variables, default off so it’s quicker:

[![](https://www.mudlet.org/wp-content/uploads/2020/05/Workspace-1_585.png)](https://www.mudlet.org/wp-content/uploads/2020/05/Workspace-1_585.png)

## CP437 encoding now supported

Thanks to SlySven, the CP437 encoding is now supported. Along with a custom font, [Medievia](http://www.medievia.com/) makes use of this to draw amazing custom maps :)

[![CP437 encoding with a custom font](https://user-images.githubusercontent.com/62970949/80146038-2a957480-857f-11ea-860e-e127feeec9c1.JPG)](https://user-images.githubusercontent.com/62970949/80146038-2a957480-857f-11ea-860e-e127feeec9c1.JPG)

Check out a [few more](https://user-images.githubusercontent.com/62970949/80146037-29fcde00-857f-11ea-9ae0-40f316bc5687.JPG) [screenshots](https://user-images.githubusercontent.com/62970949/80146039-2b2e0b00-857f-11ea-9f13-03c0ea082276.JPG).

## Custom cursors

[![](https://www.mudlet.org/wp-content/uploads/2020/05/custom-cursors-demo.gif)](https://www.mudlet.org/wp-content/uploads/2020/05/custom-cursors-demo.gif)

Yep, custom cursors are here! Curious? Download, drag, and drop [this demo package](https://wiki.mudlet.org/w/File:Cursor_grid.zip) into Mudlet to try all the cursors out :)

Big thanks to Edru for adding these in.

## New API features

gcms added MXP as another format that you can get data / events from, just like GMCP. [Check it out](https://wiki.mudlet.org/w/Manual:Supported_Protocols#MXP)!

demonnic did a pass over the table functions – [table.contains()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.contains) can now check within multiple nested tables, and [table.collect()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.collect), [table.n_collect()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.n_collect), [table.matches()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.matches), [table.n_matches()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.n_matches) to make working with tables easier.

Thanks to Edru, you can now manipulate the syncing of a module with [enableModuleSync()](https://wiki.mudlet.org/w/Manual:Miscellaneous_Functions#enableModuleSync), [disableModuleSync()](https://wiki.mudlet.org/w/Manual:Lua_Functions#disableModuleSync), and [getModuleSync()](https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleSync), and add/edit the Scripts as you see them in Mudlet with [enableScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScript), [disableScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScript), [setScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#setScript), [getScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#getScript), [permScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#permScript), and [appendScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#appendScript).

Edru also improved on the userwindows – you can now choose whenever they should be popped out or docked at start, and if docked, which area should they be in. Autodocking while dragging can now be disabled, too!

A few more improvements are also present – see the full changelog below.

## Geyser:show() fixed

As mentioned [two months ago](https://www.mudlet.org/2020/03/4-6-geyser-geyser-geyser/), we’ve fixed an issue where you could still :show() the child of a hidden container. If your UI relied on this broken behaviour, make sure to update it!

## Script editor sizes

Alias/trigger/script editors are no longer huge by default, and they’ll remember their sizes as well. Thanks to dicene for covering this off!

## Polish translation ![⭐](https://s.w.org/images/core/emoji/13.0.1/svg/2b50.svg =250px)

Big thanks to mwarzec for translating the entirety of Mudlet into Polish! In his own words, it was possible thanks to:

home.. sweet home.. and no reasons to go outside..

[![](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_589-1024x329.png)](https://www.mudlet.org/wp-content/uploads/2020/05/Selection_589.png)

## Did I miss 4.7?

We skipped 4.7 because that was the [April Fools](https://mudlet.org/4-7) version ;)

## Credits

Thanks to all coders who made this release amazing! atari2600tim, Damian Monogue, Edru2, gcms, Ian Adkins, Jim Tryon, Kebap, keneanung, Mike Conley, Richard Moffitt, Slobodan Terzić, Stephen Lyons, and Vadim Peretokin.

Thanks to all translators: DarkApocalypse , Dawid Chomaniuk (pd.chomaniuk), Jelle Z. (jelle619), Leris, Marco “M0lid3us” Tironi (wiploo), mwarzec, vingi, and thomazleventhal for their work in this.

## Changelog

**added:**

-   ‘sysDropEvent’ event that allows drag and drop over labels/miniconsoles/main window
-   Adjustable Containers are now part of Mudlet. This means nice labels you can drag to move without having to code it
-   CLI option –profile to load a specific profile on start
-   [copy2decho()](https://wiki.mudlet.org/w/Manual:UI_Functions#copy2decho) and [copy2html()](https://wiki.mudlet.org/w/Manual:UI_Functions#copy2html) utility functions
-   CP437 encoding is now supported
-   [enableModuleSync()](https://wiki.mudlet.org/w/Manual:Lua_Functions#enableModuleSync), [disableModuleSync()](https://wiki.mudlet.org/w/Manual:Lua_Functions#disableModuleSync), and [getModuleSync()](https://wiki.mudlet.org/w/Manual:Lua_Functions#getModuleSync)
-   [enableScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#enableScript), [disableScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#disableScript), [setScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#setScript), [getScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#getScript), [permScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#permScript), and [appendScript()](https://wiki.mudlet.org/w/Manual:Lua_Functions#appendScript)
-   getMudletInfo() to show debugging information about Mudlet
-   [Geyser.changeContainer()](https://www.mudlet.org/geyser/files/geyser/GeyserGeyser.html#Geyser:changeContainer) to change the parent of a label/window
-   Geyser.container raiseAll/lowerAll()
-   [Geyser:setCursor()](https://www.mudlet.org/geyser/files/geyser/GeyserLabel.html#Geyser.Label:setCursor), [Geyser:setCustomCursor()](https://www.mudlet.org/geyser/files/geyser/GeyserLabel.html#Geyser.Label:setCustomCursor), and [Geyser:resetCursor()](https://www.mudlet.org/geyser/files/geyser/GeyserLabel.html#Geyser.Label:resetCursor) (and non-Geyser equivalents) to set a custom cursor over a label
-   [mxp events](https://wiki.mudlet.org/w/Manual:Supported_Protocols#MXP) and an ‘mxp.’ table for MXP content (similar to GMCP)
-   [set](https://wiki.mudlet.org/w/Manual:UI_Functions#setMapWindowTitle)/[resetMapWindowTitle()](https://wiki.mudlet.org/w/Manual:UI_Functions#resetMapWindowTitle) to change the name of the mapper
-   [set](https://wiki.mudlet.org/w/Manual:UI_Functions#setUserWindowTitle)/[resetUserWindowTitle()](https://wiki.mudlet.org/w/Manual:UI_Functions#resetUserWindowTitle) and shorter titles by default
-   [setFgColor()](https://www.mudlet.org/geyser/files/geyser/GeyserGauge.html#Geyser.Gauge:setFgColor) for Gauges
-   [setWindow()](https://wiki.mudlet.org/w/Manual:UI_Functions#setWindow) to change the parent of a label/window
-   [table.collect()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.collect), [table.n_collect()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.n_collect), [table.matches()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.matches), [table.n_matches()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.n_matches) to make working with tables easier
-   you can now drag and drop packages into Mudlet to install them!
-   you can now search just within the script, and not the whole of Mudlet, with **Ctrl+F**. Enter/Shift+Enter navigate results
-   Mapper is a Geyser object now as well under [Geyser.Mapper](https://wiki.mudlet.org/w/Manual:Geyser#Create_Map_Window)

**improved:**

-   echo() is now limited to 10k characters on a single line
-   [Geyser:setOnEnter](https://www.mudlet.org/geyser/files/geyser/GeyserLabel.html#Geyser.Label:setOnEnter)/[setOnLeave()](https://www.mudlet.org/geyser/files/geyser/GeyserLabel.html#Geyser.Label:setOnLeave) now support tables of arguments like other functions
-   [setLabelClick](https://wiki.mudlet.org/w/Manual:UI_Functions#setLabelClickCallback) and others now accept Lua functions directly
-   3D mapper is only loaded when clicked on now
-   Mudlet’s language is auto-detected and set for new players automatically
-   Noto Font only included where it’s useful (Linux), smaller Mudlet size for macOS and Windows
-   searching for variables in editor is now optional and defaults to off (faster search)
-   [setConsoleBufferSize()](https://wiki.mudlet.org/w/Manual:Lua_Functions#setConsoleBufferSize) won’t allow you to set the limit to be bigger than your computer’s/processes memory anymore
-   splash screens are now different for official / dev / public test build versions
-   [table.contains()](https://wiki.mudlet.org/w/Manual:Table_Functions#table.contains) can now check for multiple items
-   [tempComplexRegexTrigger()](https://wiki.mudlet.org/w/Manual:Mudlet_Object_Functions#tempComplexRegexTrigger) won’t throw an error if expiration is set to 0
-   the collapse/expand icon for search is now sharper
-   there’s now a frame around the search line in Mudlet
-   trigger/script/etc sizes in the editor are now better by default and will remember their position
-   updated algorithm for calculating the width of a character
-   updated Noto Font to a newer version
-   updater window now mentions ‘update’ explicitly
-   UserWindows: you can now choose dockPosition on start
-   UserWindows: you can now disable automatic docking
-   removed getMudletLuaDefaultPaths(). This is a rare case where nobody at all was using the function and it wasn’t needed, so no backwards compat is broken

**fixed:**

-   echoLink() fixed to show text right away
-   logging in other languages will now record characters properly
-   MXP now handles &text; tag correctly
-   toolbar turning black on old macOS when 3D mapper window is opened
-   many issues with [Geyser Flyout](https://wiki.mudlet.org/w/Manual:Geyser#Flyout_Labels) labels fixed
-   Geyser:show() won’t allow showing children of a hidden parent anymore

**infrastructure:**

-   big thanks to TheFae for refreshing the frontpage and the downloads page on the website
-   added use of Github Actions to automate infrastructure tasks (updating dependencies, translations, etc)
-   added use of Github Actions to build Mudlet in macOS and Windows
-   createButton() deprecated, now uses an internal wrapper
-   text available for translation improved, some \n’s removed

**4.8.1** restores Geyser’s :new() behaviour to be as-is and adds :new2() which won’t make a hidden element appear.


<!--

# 4.x – this is just a template: copy above and edit later

This is the summary paragraph, briefly explaining some of the most important changes already

## Feature #1

Explain a big change in more detail, add links to further details or developers, or images to help comparison, etc.

## Feature #2

Feature #2 explanation

## Feature #3

etc.

## Credits

Thanks to all coders who made this release amazing! (remove these brackets and add commiters' names instead)

Thanks to all translators for their work in this: (remove these brackets and add translators' names instead)

## Changelog

**added:**

-   every change must have a line in this or one of the next sections, even if not featured above 
-   put a concise description for each feature added, may use links and formatting but no images

**improved:**

-   if the change did not add anything new, but improved existing features, this is the correct section

**fixed:**

-   describe bugs fixed in this section (not to be confused with prior buggy state)

**infrastructure:**

-   this section grabs all changes not to Mudlet app itself, but website, wiki, forums, etc. as well as development tools

**4.x.y** this line explains how a minor release version after 4.x proper improved upon it, most probably some bug hotfix


> Written with [StackEdit](https://stackedit.io/).

-->

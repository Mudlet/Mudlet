## Core principle: powerful simplicity

Mudlet's interface prioritizes a clean, premium aesthetic that gets out of the way of gameplay to implement [Mudlet's vision](https://www.mudlet.org/about/). The default experience should feel polished and uncluttered, avoiding the overwhelming feature-heavy interfaces common in similar clients.

### Pure players: 

* they're here to play the game, add an occasional alias or trigger, and look for [ready-made packages](https://packages.mudlet.org/) if they need something more complex
	* they are the silent majority: they spend their time in the games, not Mudlet Discord
- needs: connect quickly, use simple and intuitive interface, play comfortably
- preferred interface: clean main window with essential controls only. They tend to prefer Mudlet to have the simplicity of a web browser rather than richness of IDE.
- scriptability: they rely on script-focused developers to be able to create packages for them that adapt the interface to their needs
- richness in settings dialog: if they open the settings dialog and can't make sense of it because it's brimming with options, that will not just reflect on the settings dialog only - but on the application as a whole: "this is confusing, complicated, and thus Mudlet is not for me. I need a simpler client"

### Script-focused developers: 

* they're here to improve their gaming experience with heavy scripting. Some even go off into the deep end and script more than play the game
	* they're the visible minority: they hang out in Mudlet Discord and are vocal about their needs
- needs: interface customization, powerful scripting tools
- preferred interface: mostly lean towards powerful interfaces rich with information and desire a lot of options. They tend to prefer Mudlet to have the richness of an IDE rather than the simplicity of a web browser.
- scriptability: they require Mudlet to be scriptable and customisable via the API to create packages that address unique needs in a game

## Design principles

### Clarity over features

Default to hiding advanced options. Present only what users need when they need it. Visual richness - as in, lots of information - should be opt-in, not opt-out.

### Progressive disclosure

The main interface serves casual players with a premium, distraction-free experience. Power users can access advanced features through dedicated areas like the scripting editor, by building their own interfaces, or through enabling options.

### Intuitive hierarchy

- Main window: minimal, game-focused interface with clean text display
- Scripting editor: comprehensive toolset presented in an organized, approachable way
- Settings: organized by user type and frequency of use, without overwhelming the pure players by the amount of options available

### Quality over quantity

Better to have fewer, well-integrated features than many features that are stand-alone and don't cooperate together well in the bigger picture of the interface. Each interface element should serve a clear purpose and maintain visual consistency. Perfection is achieved not when there is nothing left to add, but nothing left to take away.

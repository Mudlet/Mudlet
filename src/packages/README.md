# Default packages

Packages Mudlet preinstalls into new profiles. Each one gets a directory holding
its `.mpackage` archive plus the sources that archive is built from:

```
src/packages/echo/
    config.lua      package metadata: name, author, version, description, icon
    echo.xml        the triggers, aliases and scripts, as Mudlet exports them
    echo.mpackage   what actually ships - a zip of the two files above and the icon
```

The archive is what Mudlet installs, so **editing a source file does nothing
until the archive is rebuilt**:

```bash
cd src/packages/echo
zip echo.mpackage config.lua echo.xml
```

That updates the members in place and leaves the icon under `.mudlet/Icon/`
alone. `CI/check-mpackage-sync.lua` fails the build if the two ever disagree.

Bump `version` in `config.lua` whenever a package changes. Several of these are
published to the [package repository](https://github.com/Mudlet/mudlet-package-repository),
which syncs them weekly and only offers players an update when the version goes
up - the same check enforces this.

Two packages here are maintained elsewhere and synced in by
`.github/workflows/update-3rdparty.yml`: `mpkg` (from the package repository)
and the [IRE mapper](https://github.com/IRE-Mudlet-Mapping/ire-mapping-script),
which upstream publishes as a bare `src/mudlet-mapper.xml` rather than a
package. Change them upstream: the next sync PR reverts any edit made only here.

Which games get which package is decided in `HostManager::defaultPackages()`,
and every archive needs an entry in `src/mudlet.qrc` to be compiled into Mudlet.

## Declaring a phone-sized interface

A package whose windows fit a phone's screen says so in `config.lua`:

```lua
mobile = [[yes]]
```

`mobile = true` works too, as does any case of "yes" or "true". The package
exporter writes the line when its "Its interface fits a phone screen" box is
ticked. Mudlet for Android and Mudlet Web can then leave out, on a small
screen, the labels, gauges, mini consoles, user windows, command lines, text
and scroll boxes, borders, main background image and style sheets that come
from packages without it, unless the player asks for them. A package's mappers,
buttons and `addCommand()` entries stay either way, and so does everything the
profile's own scripts make. Desktop Mudlet shows everything as before.

What a library package makes counts as the package that calls it, when that
package lists the library in its `dependencies`, so a library needs no line of
its own. What Geyser makes counts as the package whose code asked for it.

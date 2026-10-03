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
and the IRE mapper, which upstream publishes as a bare `src/mudlet-mapper.xml`
rather than a package.

Which games get which package is decided in `mudlet::setupPreInstallPackages()`,
and every archive needs an entry in `src/mudlet.qrc` to be compiled into Mudlet.

# Package fixtures

Fixture packages and modules for `Package_spec.lua`, plus one for
`Trigger_spec.lua`. They are deliberately tiny (the largest archive is about
1 KB) and every one of them is named `mudlet-spec-*` so anything they leave
behind is obviously test-owned.

`sources/` holds the readable source of each fixture; the `.mpackage` files next
to this README are those directories zipped up. `.mpackage` files are zip
archives, so never edit one in place - change the source and rebuild:

```sh
./build-fixtures.sh
```

The archives are committed instead of being zipped when the specs run because
busted runs on every platform Mudlet builds on and a `zip` tool is not there on
all of them. `build-fixtures.sh` forces the timestamps and passes `-X`, so
rebuilding unchanged sources with Info-ZIP reproduces the committed archives
byte for byte; another zip implementation may well write different bytes for the
same contents, which is harmless as long as the archives are only rebuilt
deliberately.

| fixture | what it is for |
| --- | --- |
| `mudlet-spec-minimal` | valid package: `config.lua`, one alias, one script |
| `mudlet-spec-resources` | valid package that also ships a `resources/` folder with a nested subfolder |
| `mudlet-spec-module` | installed as a module; its script counts its own compiles so a reload is observable |
| `mudlet-spec-selfuninstall` | package whose event handler uninstalls its own package (regression #9557) |
| `mudlet-spec-noconfig` | archive with a package XML but no `config.lua`, so the name comes from the file name |
| `mudlet-spec-emptyarchive` | archive with neither `config.lua` nor a package XML |
| `mudlet-spec-notazip.mpackage` | not a zip archive at all, for the unpacking error path |
| `sources/mudlet-spec-xmlonly` | bare package XML, installed without any archive around it |
| `sources/mudlet-spec-colorfilter` | bare package XML with colour-pattern and perl children under filter parents; the colour children are the part no Lua API can build |

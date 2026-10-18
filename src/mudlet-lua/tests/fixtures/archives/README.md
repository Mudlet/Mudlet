# Archive fixtures

Zip archives for the `unzip()` specs in `LuaGlobal_spec.lua`. Some of their
entries cannot be produced by zipping a folder, so `build-fixtures.py` writes
them all directly; never edit an archive in place - change the script and rebuild:

```sh
python3 build-fixtures.py
```

| fixture | what it is for |
| --- | --- |
| `unzip-escape.zip` | entries whose names climb out of the destination (`../`, `..\`, a leading `/`, a Windows drive) next to ones that stay inside it |
| `unzip-empty.zip` | empty files at the top level and inside a folder, beside bare folder entries |
| `unzip-collide.zip` | a file entry and a folder entry, each named like the temporary file the next entry is written through |
| `unzip-large.zip` | an entry larger than the 1 MiB `unzip()` reads at a time |
| `unzip-long-name.zip` | an entry with a 255-byte name, too long for a temporary file's suffix |
| `unzip-corrupt.zip` | an entry whose recorded CRC does not match its data |

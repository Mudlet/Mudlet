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
| `unzip-collide.zip` | an entry named like the temporary file the next entry is written through |
| `unzip-corrupt.zip` | an entry whose recorded CRC does not match its data |

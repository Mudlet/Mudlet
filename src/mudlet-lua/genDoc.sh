#!/bin/sh
# wipe all old html files
cd mudlet-lua-doc/ || exit 1
find . -name "*.html" -type f

# generate documentation
cd ../lua/ || exit 1
ldoc --style "$(pwd)" --project Geyser --not_luadoc --dir ../mudlet-lua-doc/files geyser/

# maintain compatibility with old luadoc-generated links.
# luadoc created geyser/, ldoc makes modules/
cd ../mudlet-lua-doc/files || exit 1
find . -type f -exec sed -i 's@modules/@geyser/@g' {} +
rm -rf geyser/
mv modules geyser

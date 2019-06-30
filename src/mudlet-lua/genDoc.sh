#!/bin/sh
# wipe all old html files
cd mudlet-lua-doc/ || exit
find . -name "*.html" -type f

# generate documentation
cd ../lua/ || exit
ldoc --style "$(pwd)" --project Geyser --not_luadoc --dir ../mudlet-lua-doc/files geyser/

# maintain compatibility with old luadoc-generated links.
# luadoc created geyser/, ldoc makes modules/
cd ../mudlet-lua-doc/files || exit
find . -type f -exec sed -i 's@modules/@geyser/@g' {} +
rm -rf geyser/
mv modules geyser

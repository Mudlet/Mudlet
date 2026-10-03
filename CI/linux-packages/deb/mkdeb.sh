#!/usr/bin/env bash
# Runs in the packager stage: turns the compiled tree into /build/out/*.deb.
set -euo pipefail

: "${PKG_VERSION:?}"
SRC=/build/Mudlet
STAGE=/build/stage
OUT=/build/out
ROCKS=/build/rocks
PACKAGE=mudlet
MAINTAINER="Vadim Peretokin <vadim.peretokin@mudlet.org>"
rm -rf "$STAGE" "$OUT"
mkdir -p "$STAGE" "$OUT"

# shellcheck disable=SC1091
. /etc/os-release
# A per-distribution ~codename suffix keeps versions distinct if a package reaches the wrong suite;
# ~git (not +git) sorts a snapshot before its release.
UPSTREAM="${PKG_VERSION}${PKG_SNAPSHOT:+~git${PKG_SNAPSHOT}}"
VERSION="${UPSTREAM}-${PKG_RELEASE:-1}~${VERSION_CODENAME}"
ARCH="$(dpkg --print-architecture)"
echo "==> packaging ${PACKAGE} ${VERSION} (${ARCH})"

cmake --install "$SRC/build" --prefix "$STAGE/usr"

# add_subdirectory() adopts vendored projects' install() rules, which would ship their dev files (#10871).
# EXCLUDE_FROM_ALL keeps them out, but that is undocumented CMake behaviour, so check the staged tree.
# Drop rather than refuse: this also packages tags that predate that CMake fix.
mapfile -d '' -t DEVELOPMENT_FILES < <(cd "$STAGE" && find . \( -path './usr/include' \
  -o -name '*.a' -o -name '*.cmake' -o -name '*.h' -o -name '*.hpp' -o -name '*.la' \
  -o -name '*.pc' -o -path '*/cmake/*' -o \( -type l -name '*.so' \) \) -prune -print0)
if [[ ${#DEVELOPMENT_FILES[@]} -gt 0 ]]; then
  echo "==> dropping development files staged into the package:" >&2
  printf '      %s\n' "${DEVELOPMENT_FILES[@]}" >&2
  echo "    a vendored project's install() rules reached this tree - add EXCLUDE_FROM_ALL to" >&2
  echo "    its add_subdirectory() in CMakeLists.txt, as 3rdparty/qt-tags-widget has" >&2
  (cd "$STAGE" && rm -rf -- "${DEVELOPMENT_FILES[@]}")
  find "$STAGE" -mindepth 1 -type d -empty -delete
else
  echo "==> no development files staged"
fi

# EXCLUDE_FROM_ALL fails by silently dropping files, so also check what the install must produce.
mapfile -t STAGED_TOP < <(cd "$STAGE/usr" && find . -mindepth 1 -maxdepth 1 -printf '%f\n' | sort)
if [[ "${STAGED_TOP[*]}" != "bin share" || ! -x "$STAGE/usr/bin/mudlet" || ! -d "$STAGE/usr/share/mudlet/lua" ]]; then
  echo "the install staged no usable Mudlet: expected bin/mudlet and share/mudlet/lua below" >&2
  echo "bin and share only, found '${STAGED_TOP[*]}'" >&2
  exit 1
fi

if [[ -d "$SRC/translations/lua" ]]; then
  mkdir -p "$STAGE/usr/share/mudlet/lua/translations"
  cp -a "$SRC/translations/lua/." "$STAGE/usr/share/mudlet/lua/translations/"
fi

# A rock file already owned by a distribution package would collide with it
bundle_rocks() {
  local src="$1" dest="$2" rel target owner
  [[ -d "$src" ]] || return 0
  while IFS= read -r -d '' rel; do
    rel="${rel#"$src"/}"
    target="$dest/$rel"
    if owner="$(dpkg -S "$target" 2>/dev/null)"; then
      echo "    skip (owned by ${owner%%:*}): $target"
      continue
    fi
    install -Dm644 "$src/$rel" "$STAGE$target"
  done < <(find "$src" -type f -print0)
}
echo "==> bundling luarocks modules"
bundle_rocks "$ROCKS/lib/lua/5.1" /usr/lib/lua/5.1
bundle_rocks "$ROCKS/share/lua/5.1" /usr/share/lua/5.1

while IFS= read -r -d '' f; do
  case "$(file -b "$f")" in
    *ELF*executable*|*ELF*shared\ object*) ;;
    *) continue ;;
  esac
  strip --strip-unneeded "$f"
  # luarocks adds an rpath to the system library dir, which lintian flags and nothing needs
  if [[ -n "$(patchelf --print-rpath "$f")" ]]; then
    patchelf --remove-rpath "$f"
  fi
done < <(find "$STAGE/usr" -type f -print0)

echo "==> resolving shared library dependencies"
SHLIB_DIR="$(mktemp -d)"
mkdir -p "$SHLIB_DIR/debian"
printf 'Source: %s\n\nPackage: %s\nArchitecture: any\n' "$PACKAGE" "$PACKAGE" > "$SHLIB_DIR/debian/control"
mapfile -t ELF_FILES < <(
  find "$STAGE/usr" -type f -print | while read -r f; do
    case "$(file -b "$f")" in *ELF*) printf '%s\n' "$f" ;; esac
  done
)
SHLIB_DEPENDS="$(cd "$SHLIB_DIR" && dpkg-shlibdeps -O --ignore-missing-info "${ELF_FILES[@]}" |
  sed -nE 's/^shlibs:Depends=//p')"
DEPENDS="${SHLIB_DEPENDS}, lua5.1, lua-filesystem, lua-lpeg, ca-certificates"
echo "    Depends: $DEPENDS"

mkdir -p "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<EOF
Package: ${PACKAGE}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Installed-Size: $(du -ks "$STAGE/usr" | cut -f1)
Depends: ${DEPENDS}
Section: games
Priority: optional
Homepage: https://www.mudlet.org
Description: Cross-platform, open source MUD client
 Mudlet is a MUD client for playing online text-based games (MUDs), with a
 Lua scripting engine, a mapper, triggers, aliases, timers and GUI scripting.
EOF

for script in postinst postrm; do
  case "$script" in
    postinst) when='[ "$1" = configure ]' ;;
    postrm) when='[ "$1" = remove ] || [ "$1" = purge ]' ;;
  esac
  cat > "$STAGE/DEBIAN/$script" <<EOF
#!/bin/sh
set -e
if $when; then
  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications || true
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -f -t /usr/share/icons/hicolor || true
  fi
fi
exit 0
EOF
  chmod 755 "$STAGE/DEBIAN/$script"
done

DOCDIR="$STAGE/usr/share/doc/${PACKAGE}"
mkdir -p "$DOCDIR"
install -m644 "$SRC/COPYING" "$DOCDIR/copyright"
cat > "$DOCDIR/changelog.Debian" <<EOF
${PACKAGE} (${VERSION}) ${VERSION_CODENAME}; urgency=medium

  * Build of Mudlet ${UPSTREAM} from commit ${BUILD_COMMIT:-unknown}.

 -- ${MAINTAINER}  $(date -uR)
EOF
gzip -9n "$DOCDIR/changelog.Debian"

find "$STAGE" -type d -exec chmod 755 {} +
(cd "$STAGE" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' | sort -z | xargs -0 md5sum > DEBIAN/md5sums)

DEB_FILE="${OUT}/${PACKAGE}_${VERSION}_${ARCH}.deb"
dpkg-deb --root-owner-group -Zxz --build "$STAGE" "$DEB_FILE"
dpkg-deb --info "$DEB_FILE"

#!/usr/bin/env bash
# Runs in the packager stage: turns the compiled tree into /build/out/*.rpm.
set -euo pipefail

: "${PKG_VERSION:?}"
SRC=/build/Mudlet
STAGE=/build/stage
OUT=/build/out
ROCKS=/build/rocks
TOPDIR=/build/rpmbuild
PACKAGE=mudlet
PACKAGER="Vadim Peretokin <vadim.peretokin@mudlet.org>"
rm -rf "$STAGE" "$OUT" "$TOPDIR"
mkdir -p "$STAGE" "$OUT" "$TOPDIR/SPECS"

# A snapshot's Release starts with 0. so it sorts before the release of its version
if [[ -n "${PKG_SNAPSHOT:-}" ]]; then
  RELEASE="0.${PKG_SNAPSHOT%%.*}git${PKG_SNAPSHOT#*.}"
else
  RELEASE="${PKG_RELEASE:-1}"
fi
echo "==> packaging ${PACKAGE} ${PKG_VERSION}-${RELEASE} ($(uname -m))"

cmake --install "$SRC/build" --prefix "$STAGE/usr"

if [[ -d "$SRC/translations/lua" ]]; then
  mkdir -p "$STAGE/usr/share/mudlet/lua/translations"
  cp -a "$SRC/translations/lua/." "$STAGE/usr/share/mudlet/lua/translations/"
fi

# A rock file already owned by a distribution package would collide with it
LIBDIR="$(rpm --eval '%{_libdir}')"
bundle_rocks() {
  local src="$1" dest="$2" rel target owner
  [[ -d "$src" ]] || return 0
  while IFS= read -r -d '' rel; do
    rel="${rel#"$src"/}"
    target="$dest/$rel"
    if owner="$(rpm -qf --qf '%{NAME}' "$target" 2>/dev/null)"; then
      echo "    skip (owned by $owner): $target"
      continue
    fi
    install -Dm644 "$src/$rel" "$STAGE$target"
  done < <(find "$src" -type f -print0)
}
echo "==> bundling luarocks modules"
bundle_rocks "$ROCKS/lib/lua/5.1" "$LIBDIR/lua/5.1"
bundle_rocks "$ROCKS/lib64/lua/5.1" "$LIBDIR/lua/5.1"
bundle_rocks "$ROCKS/share/lua/5.1" /usr/share/lua/5.1

while IFS= read -r -d '' f; do
  case "$(file -b "$f")" in
    *ELF*executable*|*ELF*shared\ object*) ;;
    *) continue ;;
  esac
  strip --strip-unneeded "$f"
  # rpmbuild's check-rpaths rejects the rpath luarocks links modules with
  if [[ -n "$(patchelf --print-rpath "$f")" ]]; then
    patchelf --remove-rpath "$f"
  fi
done < <(find "$STAGE/usr" -type f -print0)

LICENSE_PATH="/usr/share/licenses/${PACKAGE}/COPYING"
install -Dm644 "$SRC/COPYING" "$STAGE$LICENSE_PATH"

# Claim every shipped file, but only the directories no installed package owns,
# so the package never fights the filesystem or Lua packages over a directory
FILELIST="$TOPDIR/SPECS/${PACKAGE}.files"
: > "$FILELIST"
while IFS= read -r -d '' d; do
  rel="${d#"$STAGE"}"
  [[ -z "$rel" ]] && continue
  rpm -qf "$rel" >/dev/null 2>&1 && continue
  printf '%%dir "%s"\n' "$rel" >> "$FILELIST"
done < <(find "$STAGE" -type d -print0)
while IFS= read -r -d '' f; do
  rel="${f#"$STAGE"}"
  if [[ "$rel" == "$LICENSE_PATH" ]]; then
    printf '%%license "%s"\n' "$rel" >> "$FILELIST"
  else
    printf '"%s"\n' "$rel" >> "$FILELIST"
  fi
done < <(find "$STAGE" \( -type f -o -type l \) -print0)

# No %prep or %build: the tree is already compiled. rpmbuild derives the library
# Requires from the ELF files, so only the Lua runtime is listed by hand.
SPEC="$TOPDIR/SPECS/${PACKAGE}.spec"
cat > "$SPEC" <<EOF
%global debug_package %{nil}
%global __brp_mangle_shebangs %{nil}

Name:           ${PACKAGE}
Version:        ${PKG_VERSION}
Release:        ${RELEASE}%{?dist}
Summary:        Cross-platform, open source MUD client
License:        GPL-2.0-or-later
URL:            https://www.mudlet.org
Packager:       ${PACKAGER}
Requires:       compat-lua
Requires:       lua5.1-filesystem
Requires:       lua5.1-lpeg
Requires:       ca-certificates

%description
Mudlet is a MUD client for playing online text-based games (MUDs), with a
Lua scripting engine, a mapper, triggers, aliases, timers and GUI scripting.

%install
cp -a %{_stagedir}/. %{buildroot}/

%post
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database -q %{_datadir}/applications || :
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -q -f -t %{_datadir}/icons/hicolor || :
fi

%postun
if [ \$1 -eq 0 ]; then
  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q %{_datadir}/applications || :
  fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q -f -t %{_datadir}/icons/hicolor || :
  fi
fi

%files -f ${FILELIST}

%changelog
* $(LC_ALL=C date -u '+%a %b %d %Y') ${PACKAGER} - ${PKG_VERSION}-${RELEASE}
- Build of Mudlet ${PKG_VERSION} from commit ${BUILD_COMMIT:-unknown}.
EOF

rpmbuild -bb "$SPEC" \
  --define "_topdir $TOPDIR" \
  --define "_stagedir $STAGE" \
  --define "_rpmdir $OUT" \
  --define "_build_id_links none"

find "$OUT" -mindepth 2 -name '*.rpm' -exec mv -t "$OUT" {} +
find "$OUT" -mindepth 1 -type d -empty -delete
rpm -qip "$OUT"/*.rpm

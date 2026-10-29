#!/bin/bash

set -e

notarize () {

  local file="$1" output
  shift
  for i in {1..3}; do
    echo "Trying to notarize $(basename "${file}") (attempt ${i})"
    if output=$(xcrun notarytool submit "${file}" --apple-id "$APPLE_USERNAME" --password "$APPLE_PASSWORD" --team-id "$APPLE_TEAM_ID" --wait "$@" 2>&1); then
      echo "${output}"
      # A completed submission that Apple rejected can still exit 0
      if grep -q '^ *status: Invalid' <<< "${output}"; then
        echo "::warning::Apple rejected $(basename "${file}") - run notarytool log with the id above for the reasons"
        return 1
      fi
      echo "Successfully notarized $(basename "${file}")"
      return 0
    fi
    echo "${output}"
  done
  return 1
}

sign_and_notarize () {

  local appBundle="$1"
  codesign --deep -o runtime -s "$IDENTITY" "${appBundle}"
  echo "Signed final .dmg"

  notarize "${appBundle}" || echo "::warning::could not notarize ${appBundle}"
}

sign_app_bundle () {

  local appBundle="$1"
  echo "Signing app bundle: ${appBundle}"
  # -f forces a re-sign: the portable flow adds portable.txt to the bundle, which
  # invalidates the signature applied during the main dmg flow. Return non-zero on
  # failure so callers can skip the portable rather than ship a broken bundle.
  # The entitlements make-installer.sh signed in (src/mudlet.entitlements) are
  # kept: a re-sign without --preserve-metadata drops them, and with them the
  # microphone.
  if ! codesign -f -s "$IDENTITY" -o runtime --timestamp --preserve-metadata=entitlements "${appBundle}"; then
    echo "::warning::failed to codesign ${appBundle}"
    return 1
  fi
  echo "Successfully signed app bundle"

  echo "Notarizing app bundle"
  # notarytool needs an archive (zip/dmg/pkg), not a bare .app bundle.
  local notarizeZip="${appBundle}.notarize.zip"
  ditto -c -k --keepParent "${appBundle}" "${notarizeZip}"
  local notarized="false"
  if notarize "${notarizeZip}"; then
    notarized="true"
  fi
  rm -f "${notarizeZip}"

  if [ "${notarized}" = "true" ]; then
    echo "Stapling notarization ticket to app bundle"
    xcrun stapler staple "${appBundle}" || echo "::warning::failed to staple notarization ticket to ${appBundle}"
  else
    echo "::warning::could not notarize ${appBundle}; shipping a signed but un-notarized portable app bundle"
  fi
}

# Mudlet's signature will vouch for these files, so they must be exactly the ones pinned here.
fetch_pinned () {

  local url="$1" expected="$2" dest="$3" actual
  if ! curl -fsSL --retry 3 -o "${dest}" "${url}"; then
    echo "::warning::could not download ${url}"
    return 1
  fi
  actual=$(shasum -a 256 "${dest}" | awk '{print $1}')
  if [ "${actual}" != "${expected}" ]; then
    echo "::error::${url} has sha256 ${actual}, expected ${expected} - refusing to sign it"
    return 1
  fi
}

# The signed app's library validation refuses the engines' own ad-hoc signed builds,
# so publish copies signed by Mudlet. Called from a condition, so set -e is off here.
build_speech_engines_asset () {

  local outDir="$1" assetBase="$2"
  local sherpaVersion="1.13.5" onnxruntimeVersion="1.27.1" voskVersion="0.3.42"
  local sherpaArch sherpaSha256
  case "${ARCH}" in
    arm64)
      sherpaArch="arm64"
      sherpaSha256="bd0e13f42eb1e2aa7174a2816198f2cff3b586fa09a7140f5504d8c1ab1cb19e"
      ;;
    x86_64)
      sherpaArch="x64"
      sherpaSha256="c1a65c52458a59ac0fc861733a42bd2c870940e6b23a77eb528aa3c3e6f0b85f"
      ;;
    *)
      echo "::warning::no speech engine builds are pinned for ${ARCH}"
      return 1
      ;;
  esac
  local sherpaName="sherpa-onnx-v${sherpaVersion}-osx-${sherpaArch}-shared-no-tts-lib"
  local downloads="${outDir}/downloads"
  local stage="${outDir}/${assetBase}"
  local zip="${outDir}/${assetBase}.zip"

  echo "=== Building ${assetBase}.zip ==="
  rm -rf "${outDir}"
  mkdir -p "${downloads}" "${stage}/sherpa-onnx" "${stage}/vosk" || return 1

  fetch_pinned "https://github.com/k2-fsa/sherpa-onnx/releases/download/v${sherpaVersion}/${sherpaName}.tar.bz2" \
    "${sherpaSha256}" "${downloads}/sherpa-onnx.tar.bz2" || return 1
  fetch_pinned "https://github.com/alphacep/vosk-api/releases/download/v${voskVersion}/vosk-osx-${voskVersion}.zip" \
    "65395f196c9d0583d79949142b25560acaf9c295f36284e18433097f3adb0ea1" "${downloads}/vosk.zip" || return 1
  # Neither archive carries the license texts that redistributing it requires
  fetch_pinned "https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/v${sherpaVersion}/LICENSE" \
    "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30" "${stage}/sherpa-onnx/LICENSE" || return 1
  fetch_pinned "https://raw.githubusercontent.com/microsoft/onnxruntime/v${onnxruntimeVersion}/LICENSE" \
    "2f07c72751aed99790b8a4869cf2311df85a860b22ded05fa22803587a48922c" "${stage}/sherpa-onnx/LICENSE-onnxruntime" || return 1
  fetch_pinned "https://raw.githubusercontent.com/microsoft/onnxruntime/v${onnxruntimeVersion}/ThirdPartyNotices.txt" \
    "0e07b95f3a8d6230037707c5c4a2b554d12c4cb67369669ac255635528ffcee2" "${stage}/sherpa-onnx/ThirdPartyNotices-onnxruntime.txt" || return 1
  fetch_pinned "https://raw.githubusercontent.com/alphacep/vosk-api/v${voskVersion}/COPYING" \
    "bb618af833c817ab01c1564bd3ec75ba4b4f9cf30910a4a2168e30a674ee8cc6" "${stage}/vosk/LICENSE" || return 1

  # The c-api finds onnxruntime through its @loader_path rpath, so the two must share a folder
  tar -xjf "${downloads}/sherpa-onnx.tar.bz2" -C "${stage}/sherpa-onnx" --strip-components 2 \
    "${sherpaName}/lib/libsherpa-onnx-c-api.dylib" "${sherpaName}/lib/libonnxruntime.dylib" || return 1
  unzip -q -j "${downloads}/vosk.zip" "vosk-osx-${voskVersion}/libvosk.dylib" -d "${stage}/vosk" || return 1

  # The keychain locks after an hour idle, and the DMG's notarization can take most of that
  security unlock-keychain -p travis "${KEYCHAIN}" || return 1
  local lib
  for lib in "${stage}/sherpa-onnx/libsherpa-onnx-c-api.dylib" "${stage}/sherpa-onnx/libonnxruntime.dylib" "${stage}/vosk/libvosk.dylib"; do
    if ! codesign --force --timestamp -o runtime -s "$IDENTITY" "${lib}" || ! codesign --verify --strict "${lib}"; then
      echo "::warning::failed to sign ${lib}"
      return 1
    fi
  done

  cat > "${stage}/README.txt" <<EOF || return 1
Speech engines for Mudlet on macOS (${ARCH})

sherpa-onnx ${sherpaVersion} (with onnxruntime ${onnxruntimeVersion}) and Vosk ${voskVersion}, unmodified
from their authors' releases but signed by Mudlet. Mudlet's releases and PTBs
only load engine libraries signed by Mudlet, so use these rather than the
downloads from the engines' own pages.

Install either engine or both, then restart Mudlet:

  sherpa-onnx: copy the two .dylib files from sherpa-onnx/ into
               ~/.config/mudlet/sherpa-onnx-lib/
  Vosk:        copy libvosk.dylib from vosk/ into
               ~/.config/mudlet/vosk-lib/

Create the folder if it does not exist. Those are the default locations:
"lua stt.getLibraryPath()" prints the folder in use for the engine Mudlet
would pick, which is sherpa-onnx once it is installed and Vosk otherwise.

sherpa-onnx and Vosk are licensed under the Apache License 2.0 and onnxruntime
under the MIT License; the license texts are in each engine's folder.
EOF

  ditto -c -k --keepParent "${stage}" "${zip}" || return 1
  # Bare dylibs cannot be stapled: Gatekeeper fetches the ticket online when a quarantined copy loads
  notarize "${zip}" --timeout 15m || return 1
  (cd "${outDir}" && shasum -a 256 "$(basename "${zip}")") > "${zip}.sha256" || return 1

  {
    echo "SPEECH_ENGINES_ASSET_PATH=${zip}"
    echo "SPEECH_ENGINES_ASSET_SHA256_PATH=${zip}.sha256"
  } >> "$GITHUB_ENV"
}

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

if [ "${DEPLOY}" = "deploy" ]; then

  git clone https://github.com/Mudlet/installers.git "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/osx"

  # setup macOS keychain for code signing on ptb/release builds.
  if [ -n "$MACOS_SIGNING_PASS" ]; then
    KEYCHAIN=build.keychain
    security create-keychain -p travis $KEYCHAIN
    security default-keychain -s $KEYCHAIN
    security unlock-keychain -p travis $KEYCHAIN
    security set-keychain-settings -t 3600 -u $KEYCHAIN
    security import Certificates.p12 -k $KEYCHAIN -P "$MACOS_SIGNING_PASS" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s -k travis $KEYCHAIN
    export IDENTITY="Developer ID Application"
    echo "Imported identity:"
    security find-identity
    echo "----"
  fi

  if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${public_test_build}" != "true" ]; then
    echo "== Creating a snapshot build =="
    appBaseName="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}"
    if [ -n "${GITHUB_REPOSITORY}" ]; then
      mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    else
      mv "${BUILD_DIR}/Mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    fi

    ./make-installer.sh "${appBaseName}.app"

    if [ -n "$MACOS_SIGNING_PASS" ]; then
      sign_and_notarize "${HOME}/Desktop/${appBaseName}.dmg"
    fi

    echo "=== ... later, via Github ==="
    # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
    mkdir -p "${BUILD_DIR}/upload/"
    mv "${HOME}/Desktop/${appBaseName}.dmg" "${BUILD_DIR}/upload/"
    {
      echo "FOLDER_TO_UPLOAD=${BUILD_DIR}/upload"
      echo "UPLOAD_FILENAME=${appBaseName}"
    } >> "$GITHUB_ENV"
    DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID"
  else # ptb/release build
    app="${BUILD_DIR}/build/mudlet.app"
    if [ "${public_test_build}" == "true" ]; then

      # Skip duplicate check if this is a manually forced build
      if [[ "${GITHUB_FORCE_BUILD}" == "true" ]]; then
        echo "== Forced build requested, skipping duplicate PTB check =="
      else
        # don't swallow a gh failure (e.g. missing GH_TOKEN) - warning and
        # building beats silently shipping a duplicate PTB
        if existing_ptb_tags=$(gh release list --repo "${GITHUB_REPOSITORY}" --limit 100 \
              --json tagName --jq '.[].tagName | select(contains("-ptb-"))'); then
          while IFS= read -r existing_tag; do
            [[ -n "${existing_tag}" ]] || continue
            # prefix-compare as git can vary the abbreviated hash length per run
            existing_commit="${existing_tag##*-}"
            if [[ -n "${BUILD_COMMIT}" ]] \
                && { [[ "${BUILD_COMMIT}" == "${existing_commit}"* ]] \
                  || [[ "${existing_commit}" == "${BUILD_COMMIT}"* ]]; }; then
              # A PTB exists for this commit, but the platforms (and each macOS
              # arch) build in parallel and share one release - only skip if this
              # arch's .dmg is already published. Otherwise a sibling build (or a
              # re-run) created the release first and we still owe it our asset,
              # so one build racing ahead or failing never blocks the others.
              if gh release view "${existing_tag}" --repo "${GITHUB_REPOSITORY}" \
                    --json assets --jq '.assets[].name' 2>/dev/null \
                    | grep -q -- "-${ARCH}\.dmg$"; then
                echo "== PTB ${existing_tag} already has the ${ARCH} macOS asset for ${BUILD_COMMIT}, skipping rebuild =="
                exit 0
              fi
              echo "== PTB ${existing_tag} exists for ${BUILD_COMMIT} but has no ${ARCH} macOS asset yet - building to add it =="
              break
            fi
          done <<< "${existing_ptb_tags}"
        else
          echo "::warning::Could not list releases to check for duplicate PTB (is GH_TOKEN set?) - proceeding with build"
        fi
      fi

      echo "== Creating a public test build =="
      if [ -n "${GITHUB_REPOSITORY}" ]; then
        mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/Mudlet PTB.app"
      else
        mv "$app" "source/build/Mudlet PTB.app"
      fi

      app="${BUILD_DIR}/Mudlet PTB.app"
    else
      echo "== Creating a release build =="
    fi

    if [ "${public_test_build}" == "true" ]; then
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}" "$app"
    else
      ./make-installer.sh -r "${VERSION}-${ARCH}" "$app"
    fi

    if [ ! -z "$MACOS_SIGNING_PASS" ]; then
      if [ "${public_test_build}" == "true" ]; then
        sign_and_notarize "${HOME}/Desktop/Mudlet PTB.dmg"
      else
        sign_and_notarize "${HOME}/Desktop/Mudlet.dmg"
      fi
    fi

    if [ "${public_test_build}" == "true" ]; then
      RELEASE_ARTIFACT="${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}.dmg"
      mv "${HOME}/Desktop/Mudlet PTB.dmg" "${RELEASE_ARTIFACT}"
    else
      RELEASE_ARTIFACT="${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg"
      mv "${HOME}/Desktop/Mudlet.dmg" "${RELEASE_ARTIFACT}"
    fi

    echo "=== Generating SHA256 checksum for GitHub Release ==="
    RELEASE_ARTIFACT_BASENAME=$(basename "${RELEASE_ARTIFACT}")
    (cd "$(dirname "${RELEASE_ARTIFACT}")" && shasum -a 256 "${RELEASE_ARTIFACT_BASENAME}") > "${RELEASE_ARTIFACT}.sha256"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Setting up for Github upload ==="
      mkdir -p "${BUILD_DIR}/upload/"
      mv "${RELEASE_ARTIFACT}" "${BUILD_DIR}/upload/"
      mv "${RELEASE_ARTIFACT}.sha256" "${BUILD_DIR}/upload/"
      {
        echo "FOLDER_TO_UPLOAD=${BUILD_DIR}/upload"
        echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}"
        echo "RELEASE_ASSET_PATH=${BUILD_DIR}/upload/$(basename "${RELEASE_ARTIFACT}")"
        echo "RELEASE_ASSET_SHA256_PATH=${BUILD_DIR}/upload/$(basename "${RELEASE_ARTIFACT}").sha256"
        echo "VERSION=${VERSION}"
        echo "MUDLET_VERSION_BUILD=${MUDLET_VERSION_BUILD}"
        echo "BUILD_COMMIT=${BUILD_COMMIT}"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID"
    else
      {
        echo "RELEASE_ASSET_PATH=${RELEASE_ARTIFACT}"
        echo "RELEASE_ASSET_SHA256_PATH=${RELEASE_ARTIFACT}.sha256"
        echo "VERSION=${VERSION}"
        echo "MUDLET_VERSION_BUILD=${MUDLET_VERSION_BUILD}"
        echo "BUILD_COMMIT=${BUILD_COMMIT}"
      } >> "$GITHUB_ENV"

      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      if ! scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg" "mudmachine@make.mudlet.org:${DEPLOY_PATH}"; then
        echo "::error::failed to upload Mudlet-${VERSION}-${ARCH}.dmg to make.mudlet.org"
        exit 1
      fi
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-${ARCH}.dmg"

      # Retry to tolerate the delay between scp landing on make.mudlet.org and the
      # file being served at www.mudlet.org - the publish step is not instantaneous.
      verify_attempt=0
      until curl --output /dev/null --silent --head --fail "$DEPLOY_URL"; do
        verify_attempt=$((verify_attempt + 1))
        if [ "$verify_attempt" -ge 18 ]; then
          echo "::warning::release uploaded to make.mudlet.org but not yet downloadable at $DEPLOY_URL after $verify_attempt attempts (CloudFlare/publish lag); the scp above is the authoritative upload check"
          break
        fi
        echo "Release not yet available at $DEPLOY_URL, retrying in 10s (attempt $verify_attempt/18)..."
        sleep 10
      done

      SHA256SUM=$(shasum -a 256 "${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg" | awk '{print $1}')

      # Create portable version (compressed .app bundle)
      echo "=== Creating portable version ==="
      PORTABLE_NAME="Mudlet-${VERSION}-${ARCH}-portable"
      cd "${HOME}/Desktop"

      # Find the app dynamically to avoid path issues
      APP_PATH=$(find "${BUILD_DIR}" -name "mudlet.app" -type d | head -1)
      if [ -n "$APP_PATH" ]; then
        APP_DIR=$(dirname "$APP_PATH")
        echo "Found mudlet.app at: $APP_PATH"
        MACOS_DIR="${APP_PATH}/Contents/MacOS"
        if [ -d "$MACOS_DIR" ]; then
          touch "${MACOS_DIR}/portable.txt"
          echo "Created portable.txt at: ${MACOS_DIR}/portable.txt"
        else
          echo "Error: Could not find MacOS directory at: $MACOS_DIR"
          exit 1
        fi

        # Sign the app bundle for the portable version. portable.txt currently
        # lives in Contents/MacOS/, which breaks codesign; until the marker is
        # moved to a codesign-safe location (to be fixed on the development
        # branch), the macOS portable is best-effort: skip it on signing failure
        # rather than fail the whole release build.
        PORTABLE_READY="true"
        if [ -n "$MACOS_SIGNING_PASS" ]; then
          echo "Signing app bundle for portable version"
          if ! sign_app_bundle "$APP_PATH"; then
            echo "::warning::macOS portable signing failed; skipping the macOS portable for this build"
            PORTABLE_READY="false"
          fi
        fi

        if [ "${PORTABLE_READY}" = "true" ]; then
          tar -czf "${PORTABLE_NAME}.tar.gz" -C "$APP_DIR" "mudlet.app"
        fi
      else
        echo "Error: Could not find mudlet.app anywhere in ${BUILD_DIR}"
        echo "Directory contents:"
        find "${BUILD_DIR}" -name "*.app" -type d
        exit 1
      fi

      if [ "${PORTABLE_READY}" = "true" ]; then
        PORTABLE_SHA256SUM=$(shasum -a 256 "${HOME}/Desktop/${PORTABLE_NAME}.tar.gz" | awk '{print $1}')

        echo "=== Uploading portable version ==="
        if ! scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/${PORTABLE_NAME}.tar.gz" "mudmachine@make.mudlet.org:${DEPLOY_PATH}"; then
          echo "::error::failed to upload ${PORTABLE_NAME}.tar.gz to make.mudlet.org"
          exit 1
        fi
        PORTABLE_DEPLOY_URL="https://www.mudlet.org/wp-content/files/${PORTABLE_NAME}.tar.gz"

        verify_attempt=0
        until curl --output /dev/null --silent --head --fail "$PORTABLE_DEPLOY_URL"; do
          verify_attempt=$((verify_attempt + 1))
          if [ "$verify_attempt" -ge 18 ]; then
            echo "::warning::portable uploaded to make.mudlet.org but not yet downloadable at $PORTABLE_DEPLOY_URL after $verify_attempt attempts (CloudFlare/publish lag)"
            break
          fi
          echo "Portable release not yet available at $PORTABLE_DEPLOY_URL, retrying in 10s (attempt $verify_attempt/18)..."
          sleep 10
        done
      fi

      if [ "${ARCH}" = "arm64" ]; then
        FILE_CATEGORY="4"
      else
        FILE_CATEGORY="3"
      fi

      # Get current timestamp
      current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r day month year hour minute second <<< "$current_timestamp"

      # Upload regular DMG version
      curl --retry 5 -X POST 'https://make.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$DEPLOY_URL" \
      -F "file_name=Mudlet ${VERSION}-${ARCH} (macOS)" \
      -F "file_des=sha256: $SHA256SUM" \
      -F "file_cat=${FILE_CATEGORY}" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"

      # Upload portable version (only when the portable was successfully built)
      if [ "${PORTABLE_READY}" = "true" ]; then
      curl --retry 5 -X POST 'https://make.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$PORTABLE_DEPLOY_URL" \
      -F "file_name=Mudlet ${VERSION}-${ARCH} (macOS Portable)" \
      -F "file_des=sha256: $PORTABLE_SHA256SUM" \
      -F "file_cat=${FILE_CATEGORY}" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"
      fi

    fi

    # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/mac/${ARCH_DBLSQD}" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit
      changelog=$(lua "${SOURCE_DIR}/CI/generate-changelog.lua" --mode ptb --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}" || true

      # release registration and uploading will be manual for the time being
    else
      echo "=== Registering release with Dblsqd ==="
      # Non-fatal: dblsqd is legacy (GitHub releases are the primary distribution now),
      # and the dblsqd release may not be pre-created. Do not let it block the build.
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach mac:${ARCH_DBLSQD} "${DEPLOY_URL}" || echo "::warning::dblsqd push failed for mac:${ARCH_DBLSQD} - continuing (GitHub release is the primary distribution)"
    fi

    # Last, so a slow notarization here holds up only the GitHub release, not mudlet.org or dblsqd
    if [ -n "$MACOS_SIGNING_PASS" ]; then
      if ! build_speech_engines_asset "${BUILD_DIR}/speech-engines" "${RELEASE_ARTIFACT_BASENAME%-"${ARCH}".dmg}-speech-engines-${ARCH}"; then
        echo "::warning::skipping the signed speech engines asset for ${ARCH} - the .dmg is unaffected"
      fi
    fi
  fi

  # delete keychain just in case
  if [ ! -z "$MACOS_SIGNING_PASS" ]; then
    security delete-keychain $KEYCHAIN
  fi

  export DEPLOY_URL
fi

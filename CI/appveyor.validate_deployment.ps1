if ($Env:APPVEYOR_REPO_TAG -ne "true") {
  echo "Not a release build - skipping release validation."
  exit 0
}

function error([string]$message) {
  echo "$message"
  exit 1
}

function validate_qmake() {
  $VALID_QMAKE = select-string -path src/mudlet.pro -pattern "^VERSION ? = ?(\d+\.\d+\.\d+)$"
  if ($VALID_QMAKE.Matches.Success -ne $True) {
    error "mudlet.pro's VERSION variable isn't formatted following the semantic versioning rules in a release build."
  }

  $VALID_BUILD = select-string -path src/mudlet.pro -pattern ' +BUILD ? = ? ("")'
  if ($VALID_BUILD.Matches.Success -ne $True) {
    error "mudlet.pro's BUILD variable isn't set to `"`" as it should be in a release build."
  }
}

function validate_cmake() {
  $VALID_CMAKE = select-string -path CMakeLists.txt -pattern "set\(APP_VERSION (\d+\.\d+\.\d+)\)$"
  if ($VALID_CMAKE.Matches.Success -ne $True) {
    error "CMakeLists.txt's APP_VERSION variable isn't formatted following the semantic versioning rules in a release build."
  }

  $VALID_BUILD = select-string -path CMakeLists.txt -pattern 'set\(APP_BUILD ("")\)$'
  if ($VALID_BUILD.Matches.Success -ne $True) {
    error "CMakeLists.txt APP_BUILD variable isn't set to `"`" as it should be in a release build."
  }
}

function validate_updater_environment_variable() {
  if ($Env:WITH_UPDATER -eq "NO") {
    error "Updater is disabled in a release build."
  }
}

validate_qmake
validate_cmake
validate_updater_environment_variable

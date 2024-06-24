# Shell Commands to prepare
step 1: ensuring app bundle is signed
codesign --deep --force --verify --verbose --sign "Developer ID Application: Your Name (TEAMID)" /path/to/your.app

step 2: create a zip file for notarization
ditto -c -k --keepParent /path/to/your.app /path/to/your.zip

step 3: submit app for notarization
xcrun altool --notarize-app -f /path/to/your.zip --primary-bundle-id "com.your.bundle.id" -u "your-apple-id" -p "app-specific-password"

step 4: check status
xcrun altool --notarization-history 0 -u "your-apple-id" -p "app-specific-password"

step 5: staple
xcrun stapler staple /path/to/your.app

# Implementation Steps in Your Project:
Modify your build scripts to use altool or notarize-cli.
Update your CI/CD pipeline to include the notarization step using the chosen tool.
Test the entire process from signing to notarization to ensure everything works smoothly.

#!/bin/bash

APP_NAME="YourApp"
APP_BUNDLE="com.your.bundle.id"
APPLE_ID="your-apple-id"
APP_PASSWORD="app-specific-password"

#Sign the app
codesign --deep --force --verify --verbose --sign "Developer ID Application: Your Name (TEAMID)" /path/to/$APP_NAME.app

#Create a zip of the app
ditto -c -k --keepParent /path/to/$APP_NAME.app /path/to/$APP_NAME.zip

#Submit for notarization
xcrun altool --notarize-app -f /path/to/$APP_NAME.zip --primary-bundle-id "$APP_BUNDLE" -u "$APPLE_ID" -p "$APP_PASSWORD"

#Wait and check status


# Staple notarization
xcrun stapler staple /path/to/$APP_NAME.app

# Verify stapling
spctl -a -v /path/to/$APP_NAME.app


#!/bin/bash

FILENAME="$1"

if [ $# -ne 1 ]; then
	echo "Usage: $0 FILENAME";
	exit;
fi

if [ ! -f "${REPO_ROOT}"/build/bin/qrenderccccc.app/Contents/MacOS/qrenderccccc ] || [ ! -f "${REPO_ROOT}"/build/bin/renderccccccmd ]; then
	echo "ERROR: Missing qrenderccccc.app or renderccccccmd builds";
	exit 1;
fi

if ! which convert > /dev/null 2>&1; then
	echo "ERROR: Require imagemagick for packaging step";
	echo "       brew install imagemagick";
	exit 1;
fi

if ! which create-dmg > /dev/null 2>&1; then
	echo "ERROR: Require create-dmg for packaging step";
	echo "       brew install create-dmg";
	exit 1;
fi

# create final bundle folder
mkdir -p "${REPO_ROOT}"/dist/Renderccccc.app

# copy in qrenderccccc bundle
cp -R "${REPO_ROOT}"/build/bin/qrenderccccc.app/* "${REPO_ROOT}"/dist/Renderccccc.app/

# copy in renderccccccmd
cp "${REPO_ROOT}"/build/bin/renderccccccmd "${REPO_ROOT}"/dist/Renderccccc.app/Contents/MacOS/

# copy in plugins
if [ -d "${REPO_ROOT}"/plugins-macos ]; then
	cp -R "${REPO_ROOT}"/plugins-macos "${REPO_ROOT}/dist/Renderccccc.app/Contents/plugins"
else
	echo "WARNING: Plugins not present. Download and extract https://renderccccc.org/plugins.tgz in root folder";
fi

# copy in all of the android files.
mkdir -p "${REPO_ROOT}/dist/Renderccccc.app/Contents/plugins/android/"

if ls "${REPO_ROOT}"/build-android*/bin/*.apk; then
	cp "${REPO_ROOT}"/build-android*/bin/*.apk "${REPO_ROOT}/dist/Renderccccc.app/Contents/plugins/android/"
else
	echo "WARNING: Android build not present. Build arm32 and arm64 apks in build-android-arm{32,64} folders";
fi

# Create dmg background image
convert -size 600x300 xc:white \
	      -fill '#3BB779' -draw "rectangle 0,0 600,100" \
	      -fill white -pointsize 24 -gravity north \
	      -annotate +0+50 "Drag qrenderccccc to your Applications folder." \
	      /tmp/rdbackground.png

rm -rf "${REPO_ROOT}"/package
mkdir "${REPO_ROOT}"/package

create-dmg --volname "$FILENAME" \
	         --volicon "${REPO_ROOT}"/dist/Renderccccc.app/Contents/Resources/Renderccccc.icns \
	         --background /tmp/rdbackground.png \
	         --window-pos 200 120 --window-size 600 350 --icon-size 100 \
	         --icon Renderccccc.app 200 190 \
	         --app-drop-link 400 185 \
	         "${REPO_ROOT}"/package/"${FILENAME}".dmg "${REPO_ROOT}"/dist/


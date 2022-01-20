#! /bin/sh

BASEDIR=$(dirname $0)

EXPORT_PATH="$BASEDIR/../build-mac/OctetViolins"
AUDIO_PATH="$HOME/Library/Audio/Plug-Ins"

if [ ! -d "$EXPORT_PATH" ]
then
  mkdir "$EXPORT_PATH"
fi

if [ ! -d "$EXPORT_PATH/OSX" ]
then
  mkdir "$EXPORT_PATH/OSX"
fi

# cp "$BASEDIR/../manual/OctetViolins_User_Guide.pdf" "$EXPORT_PATH/" || exit 1

"$BASEDIR/dist-mac-notarize.sh" "$EXPORT_PATH" OctetViolins.component "$AUDIO_PATH/Components" || exit 1
"$BASEDIR/dist-mac-notarize.sh" "$EXPORT_PATH" OctetViolins.vst "$AUDIO_PATH/VST" || exit 1
"$BASEDIR/dist-mac-notarize.sh" "$EXPORT_PATH" OctetViolins.vst3 "$AUDIO_PATH/VST3" || exit 1

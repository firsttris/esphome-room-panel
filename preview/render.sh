#!/bin/sh
# Rendert Screenshots der Vorschau (lokal im Container esphome-sdl, in CI direkt).
# Aufruf im Repo-Verzeichnis:  sh preview/render.sh mila.yml
set -e
ROOM=${1:-mila.yml}
OUT=preview/shots
mkdir -p "$OUT"
# Vorschau-Konfiguration: Hardware durch SDL ersetzen, Beispielwerte dazu
sed -e 's#device: !include base/device.yml#device: !include preview/sdl.yml\n  demo: !include preview/demo.yml#' \
    -e 's#^\(  device_name: .*\)$#\1-preview#' "$ROOM" > .preview.yml
esphome compile .preview.yml
BIN=$(find .esphome/build -path '*preview*' -name program -type f | head -1)
Xvfb :99 -screen 0 480x480x24 >/dev/null 2>&1 &
XVFB=$!
export DISPLAY=:99
sleep 1
"$BIN" >/dev/null 2>&1 &
APP=$!
trap 'kill $APP $XVFB 2>/dev/null || true' EXIT
sleep 4
tap() { xdotool mousemove "$1" "$2" mousedown 1 sleep 0.2 mouseup 1; }
shot() { import -window root "$OUT/$1.png"; }
shot licht
tap 355 180   # Licht "Mitte" antippen
sleep 1
shot licht-tippen
tap 238 439; sleep 1.2; shot rolllaeden
tap 387 439; sleep 1.2; shot klima

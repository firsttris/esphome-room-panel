#!/bin/sh
# Rendert Screenshots der Vorschau (lokal im Container esphome-sdl, in CI direkt).
# Aufruf im Repo-Verzeichnis:  sh preview/render.sh mila.yml
set -e
ROOM=${1:-mila.yml}
NAME=$(basename "$ROOM" .yml)
OUT=preview/shots/$NAME
mkdir -p "$OUT"
# Vorschau-Konfiguration: Hardware durch SDL ersetzen, Beispielwerte dazu
sed -e 's#device: !include base/device.yml#device: !include preview/sdl.yml\n  demo: !include preview/demo-'"$NAME"'.yml#' \
    -e 's#^\(  device_name: .*\)$#\1-preview\n  preview: true#' \
    -e 's#^  show_fps: .*#  show_fps: "false"#' "$ROOM" > .preview.yml
esphome compile .preview.yml
DEV=$(sed -n 's/^  device_name: //p' "$ROOM")-preview
BIN=.esphome/build/$DEV/.pioenvs/$DEV/program
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
tap 355 180   # Licht antippen: Aufnahme mitten im Pendeln
sleep 0.3
shot licht-tippen
sleep 1
# Mitte der Reiter unten (drei oder vier Reiter)
if grep -q '^  tabs: 4' "$ROOM"; then
  tap 182 439; sleep 1.2; shot rolllaeden
  tap 294 439; sleep 1.2; shot klima
  tap 406 439; sleep 1.2; shot heizung
else
  tap 238 439; sleep 1.2; shot rolllaeden
  tap 387 439; sleep 1.2; shot klima
fi

# Optional (GIF=1): kurzer Rundgang als animiertes GIF für das README.
# Koordinaten passen zu mila.yml (4 Lichter, 6 Rollläden, Klima).
if [ -n "$GIF" ]; then
  kill $APP 2>/dev/null || true
  sleep 0.5
  "$BIN" >/dev/null 2>&1 &
  APP=$!
  sleep 3
  ffmpeg -loglevel error -y -f x11grab -draw_mouse 0 -framerate 20 -video_size 480x480 -i :99 -t 9.5 "$OUT/demo.mp4" &
  REC=$!
  sleep 0.8; tap 355 180    # Licht "Mitte" an: Glühen, Welle, Pendeln
  sleep 1.6; tap 124 180    # Licht "Links" aus
  sleep 1.6; tap 238 439    # Reiter Rollläden
  sleep 1.2; tap 205 177    # "Dachfenster links" runter: Behang fährt
  sleep 1.5; tap 387 439    # Reiter Klima
  sleep 1.0; tap 435 119    # Modus Kühlen: Farbe wechselt, Lüfter dreht
  wait $REC
  ffmpeg -loglevel error -y -i "$OUT/demo.mp4" \
    -vf "fps=15,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=none:diff_mode=rectangle" \
    "$OUT/demo.gif"
  rm -f "$OUT/demo.mp4"
fi

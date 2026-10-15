#!/bin/sh
# Social-Preview-Bild (1280 × 640) für GitHub aus den Screenshots in docs/.
# Hochladen von Hand: Settings → General → Social preview.
set -e
F600=$(ls .esphome/font/Outfit@600@*.ttf | head -1)
F400=$(ls .esphome/font/Outfit@400@*.ttf | head -1)
convert -size 1280x640 gradient:'#141B2A-#0A0E16' \
  -font "$F600" -pointsize 64 -fill '#EEF2F8' -annotate +60+118 'ESPHome Room Panel' \
  -font "$F400" -pointsize 28 -fill '#8892A8' -annotate +62+168 'Animated Home Assistant touch panel · LVGL · Guition ESP32-S3-4848S040' \
  \( docs/mila/licht.png -resize 340x340 \) -geometry +60+230 -composite \
  \( docs/mila/rolllaeden.png -resize 340x340 \) -geometry +470+230 -composite \
  \( docs/schlafzimmer/klima.png -resize 340x340 \) -geometry +880+230 -composite \
  docs/social-preview.png

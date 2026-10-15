// Animationen und Zustände des Raum-Panels (eingebunden über esphome.includes)
//
// Die Widgets entstehen in YAML; hier wird nur gemalt und animiert.
// Kinder-Reihenfolge der Widgets ist fest (siehe tiles/*.yml) und wird
// über lv_obj_get_child() angesprochen.
#pragma once

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <lvgl.h>
#include "esphome/core/log.h"

namespace panel {

// ---------------------------------------------------------------------------
// Farben (wie im Mockup)
// ---------------------------------------------------------------------------
constexpr uint32_t CARD = 0x182031, CARD2 = 0x222B3E, LINE = 0x2C3650;
constexpr uint32_t TEXT = 0xEEF2F8, MUTE = 0x8892A8, DIM = 0x5C667C;
constexpr uint32_t AMBER = 0xFFB547, AMBER_DEEP = 0x2B1B06, AMBER_BG = 0x3A2911, AMBER_LINE = 0x7A5A2A;
constexpr uint32_t SKY = 0x5CC8FF, SKY_BG = 0x1D3A52;
constexpr uint32_t NAV_NEUTRAL = 0xDDE3EE;

// Mischt von `from` nach `to`, t = 0..255
inline lv_color_t mix(uint32_t from, uint32_t to, int32_t t) {
  return lv_color_mix(lv_color_hex(to), lv_color_hex(from), (uint8_t) LV_CLAMP(0, t, 255));
}

inline lv_obj_t *child(lv_obj_t *o, int32_t i) { return lv_obj_get_child(o, i); }

inline void set_hidden(lv_obj_t *o, bool hidden) {
  if (hidden != lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
    if (hidden)
      lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
  }
}

// Temperatur mit Komma, z. B. "22,5°"
inline void set_temp(lv_obj_t *label, float v, const char *suffix = "°") {
  char buf[16];
  if (std::isnan(v)) {
    snprintf(buf, sizeof(buf), "--%s", suffix);
  } else {
    snprintf(buf, sizeof(buf), "%.1f%s", v, suffix);
    if (char *dot = strchr(buf, '.'))
      *dot = ',';
  }
  lv_label_set_text(label, buf);
}

inline void set_int(lv_obj_t *label, float v, const char *suffix) {
  char buf[16];
  if (std::isnan(v))
    snprintf(buf, sizeof(buf), "--%s", suffix);
  else
    snprintf(buf, sizeof(buf), "%d%s", (int) lroundf(v), suffix);
  lv_label_set_text(label, buf);
}

inline void anim(void *var, lv_anim_exec_xcb_t cb, int32_t from, int32_t to, uint32_t ms,
                 lv_anim_path_cb_t path = lv_anim_path_ease_out) {
  lv_anim_delete(var, cb);
  if (ms == 0 || from == to) {
    cb(var, to);
    return;
  }
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, var);
  lv_anim_set_exec_cb(&a, cb);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_duration(&a, ms);
  lv_anim_set_path_cb(&a, path);
  lv_anim_start(&a);
}

// ---------------------------------------------------------------------------
// Drück-Effekt: Kachel schrumpft kurz um 5 %
// ---------------------------------------------------------------------------
inline void press(lv_obj_t *o, bool down) {
  lv_obj_set_style_transform_pivot_x(o, lv_obj_get_width(o) / 2, 0);
  lv_obj_set_style_transform_pivot_y(o, lv_obj_get_height(o) / 2, 0);
  lv_obj_set_style_transform_scale(o, down ? 243 : 256, 0);
}

// ---------------------------------------------------------------------------
// Welle: Kreis wächst vom Finger aus und blendet aus
// ---------------------------------------------------------------------------
inline void ripple_cb(void *var, int32_t v) {  // v = 0..1000
  auto *o = (lv_obj_t *) var;
  auto c = (intptr_t) lv_obj_get_user_data(o);
  int32_t cx = (c >> 16) & 0xFFFF, cy = c & 0xFFFF;
  int32_t d = 24 + v * 300 / 1000;
  lv_obj_set_size(o, d, d);
  lv_obj_set_pos(o, cx - d / 2, cy - d / 2);
  lv_obj_set_style_bg_opa(o, 90 - v * 90 / 1000, 0);
}

inline void ripple_done(lv_anim_t *a) { lv_obj_delete_async((lv_obj_t *) a->var); }

inline void ripple(lv_obj_t *tile, uint32_t color) {
  lv_indev_t *indev = lv_indev_active();
  if (indev == nullptr)
    return;
  lv_point_t p;
  lv_indev_get_point(indev, &p);
  lv_area_t area;
  lv_obj_get_coords(tile, &area);
  int32_t cx = LV_CLAMP(0, p.x - area.x1, 0x7FFF), cy = LV_CLAMP(0, p.y - area.y1, 0x7FFF);

  lv_obj_t *o = lv_obj_create(tile);
  lv_obj_remove_style_all(o);
  lv_obj_add_flag(o, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
  lv_obj_set_user_data(o, (void *) (intptr_t) ((cx << 16) | cy));
  ripple_cb(o, 0);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, o);
  lv_anim_set_exec_cb(&a, ripple_cb);
  lv_anim_set_values(&a, 0, 1000);
  lv_anim_set_duration(&a, 550);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_set_completed_cb(&a, ripple_done);
  lv_anim_start(&a);
}

// ---------------------------------------------------------------------------
// Licht-Kacheln
// Kinder: 0 = Icon-Blase (Kind 0: Icon), 1 = Punkt, 2 = Name, 3 = Zustand
// ---------------------------------------------------------------------------
struct LightState {
  int32_t level = 0;  // 0 = aus, 255 = an (Zwischenwerte während der Animation)
  bool on = false;
  bool known = false;
  bool swing_left = false;  // Pendel-Richtung wechselt bei jedem Schalten
};

inline std::map<lv_obj_t *, LightState> lights;
inline lv_obj_t *light_count_num = nullptr;   // "2"
inline lv_obj_t *light_count_text = nullptr;  // " von 4 an"
inline lv_obj_t *sleep_lights = nullptr;      // Ruhemodus: "2 Lichter an"

// Lichtschein: einmal als Bild vorberechnet und beim Schalten nur ein- und
// ausgeblendet. Ein weicher Schatten an der Kachel müsste in jedem Bild der
// Animation neu gezeichnet werden – das ist auf dem ESP32-S3 zu langsam.
constexpr int32_t GLOW_M = 12;  // Rand um die Kachel (Schatten 34 px, Spread -6 → 11 px)

struct GlowBuf {
  int32_t w, h;
  lv_draw_buf_t *buf;
};
inline std::vector<GlowBuf> glow_bufs;  // ein Bild je Kachelgröße
inline std::map<lv_obj_t *, lv_obj_t *> glows;

inline lv_draw_buf_t *glow_buf(lv_obj_t *tile) {
  int32_t w = lv_obj_get_width(tile), h = lv_obj_get_height(tile);
  for (auto &g : glow_bufs)
    if (g.w == w && g.h == h)
      return g.buf;
  lv_draw_buf_t *buf = lv_draw_buf_create(w + 2 * GLOW_M, h + 2 * GLOW_M, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
  if (buf == nullptr)
    return nullptr;
  lv_draw_buf_clear(buf, nullptr);
  // In einem unsichtbaren Canvas den Schatten der Kachel einmal malen
  lv_obj_t *cv = lv_canvas_create(lv_layer_top());
  lv_obj_add_flag(cv, LV_OBJ_FLAG_HIDDEN);
  lv_canvas_set_draw_buf(cv, buf);
  lv_layer_t layer;
  lv_canvas_init_layer(cv, &layer);
  lv_draw_rect_dsc_t d;
  lv_draw_rect_dsc_init(&d);
  d.bg_opa = LV_OPA_TRANSP;
  d.border_width = 0;
  d.radius = lv_obj_get_style_radius(tile, LV_PART_MAIN);
  d.shadow_color = lv_color_hex(0xFFB547);
  d.shadow_width = 34;
  d.shadow_spread = -6;
  d.shadow_opa = LV_OPA_COVER;
  lv_area_t a = {GLOW_M, GLOW_M, GLOW_M + w - 1, GLOW_M + h - 1};
  lv_draw_rect(&layer, &d, &a);
  lv_canvas_finish_layer(cv, &layer);
  lv_obj_delete(cv);
  glow_bufs.push_back({w, h, buf});
  return buf;
}

// Schein-Objekt hinter der Kachel (beim ersten Einschalten angelegt)
inline lv_obj_t *glow_of(lv_obj_t *tile) {
  auto it = glows.find(tile);
  if (it != glows.end())
    return it->second;
  lv_obj_t *parent = lv_obj_get_parent(tile);
  lv_obj_update_layout(parent);
  lv_draw_buf_t *buf = glow_buf(tile);
  if (buf == nullptr)
    return glows[tile] = nullptr;
  lv_obj_t *g = lv_canvas_create(parent);
  lv_canvas_set_draw_buf(g, buf);
  lv_obj_add_flag(g, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_remove_flag(g, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(g, lv_obj_get_x(tile) - GLOW_M, lv_obj_get_y(tile) - GLOW_M);
  lv_obj_move_to_index(g, lv_obj_get_index(tile));  // direkt unter die Kachel
  lv_obj_set_style_image_opa(g, 0, 0);
  return glows[tile] = g;
}

inline void light_paint(lv_obj_t *tile, int32_t v) {
  lv_obj_set_style_bg_color(tile, mix(CARD, AMBER_BG, v), 0);
  lv_obj_set_style_border_color(tile, mix(LINE, AMBER_LINE, v), 0);
  lv_obj_t *g = v > 0 ? glow_of(tile) : (glows.count(tile) ? glows[tile] : nullptr);
  if (g != nullptr)
    lv_obj_set_style_image_opa(g, v * 95 / 255, 0);
  lv_obj_t *bubble = child(tile, 0);
  lv_obj_set_style_bg_color(bubble, mix(CARD2, AMBER, v), 0);
  lv_obj_set_style_text_color(child(bubble, 0), mix(DIM, AMBER_DEEP, v), 0);
  lv_obj_t *dot = child(tile, 1);
  lv_obj_set_style_bg_color(dot, mix(LINE, AMBER, v), 0);
  lv_obj_set_style_text_color(child(tile, 3), mix(MUTE, AMBER, v), 0);
}

inline void light_anim_cb(void *var, int32_t v) {
  lights[(lv_obj_t *) var].level = v;
  light_paint((lv_obj_t *) var, v);
}

inline int lights_on() {
  int n = 0;
  for (auto &it : lights)
    if (it.second.on)
      n++;
  return n;
}

inline void lights_changed() {
  int on = lights_on(), total = (int) lights.size();
  char buf[32];
  if (light_count_num != nullptr) {
    snprintf(buf, sizeof(buf), "%d", on);
    lv_label_set_text(light_count_num, buf);
  }
  if (light_count_text != nullptr) {
    snprintf(buf, sizeof(buf), " von %d an", total);
    lv_label_set_text(light_count_text, buf);
  }
  if (sleep_lights != nullptr) {
    if (on == 0)
      lv_label_set_text(sleep_lights, "Alles aus");
    else {
      snprintf(buf, sizeof(buf), "%d %s an", on, on == 1 ? "Licht" : "Lichter");
      lv_label_set_text(sleep_lights, buf);
    }
  }
}

inline void light_register(lv_obj_t *tile) {
  if (lights.find(tile) == lights.end()) {
    lights[tile] = LightState{};
    light_paint(tile, 0);
    lights_changed();
  }
}

// Pendelleuchte wie im ccu-addon-mui (fx-swing): 9°, -6°, 3°, -1°, Ruhe in 1,1 s.
// v = 0..1100 ms, Vorzeichen der Richtung steckt in den User-Daten des Icons.
inline void swing_cb(void *var, int32_t v) {
  static const int32_t T[] = {0, 198, 462, 726, 924, 1100};  // 0 %, 18 %, 42 %, 66 %, 84 %, 100 %
  static const int32_t A[] = {0, 90, -60, 30, -10, 0};       // Winkel in 0,1°
  int i = 0;
  while (i < 4 && v > T[i + 1])
    i++;
  int32_t t = (v - T[i]) * 1024 / (T[i + 1] - T[i]);
  // weich zwischen den Stützpunkten (Kosinus-Interpolation)
  float f = (1.0f - cosf(t / 1024.0f * 3.14159265f)) / 2.0f;
  int32_t angle = A[i] + (int32_t) lroundf((A[i + 1] - A[i]) * f);
  intptr_t dir = (intptr_t) lv_obj_get_user_data((lv_obj_t *) var);
  lv_obj_set_style_transform_rotation((lv_obj_t *) var, dir < 0 ? -angle : angle, 0);
}

// Pendeln? swing = "true"/"false" aus der Raum-Datei, sonst nur beim Deckenlicht-Symbol
inline bool wants_swing(const char *icon, const char *swing) {
  if (strcmp(swing, "true") == 0 || strcmp(swing, "True") == 0)
    return true;
  if (strcmp(swing, "false") == 0 || strcmp(swing, "False") == 0)
    return false;
  return strcmp(icon, "\U000F0769") == 0;  // ceiling-light
}

inline void swing(lv_obj_t *tile, bool left) {
  lv_obj_t *icon = child(child(tile, 0), 0);
  lv_obj_set_style_transform_pivot_x(icon, lv_obj_get_width(icon) / 2, 0);
  lv_obj_set_style_transform_pivot_y(icon, 0, 0);  // aufgehängt am oberen Rand
  lv_obj_set_user_data(icon, (void *) (intptr_t) (left ? -1 : 1));
  lv_anim_delete(icon, swing_cb);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, icon);
  lv_anim_set_exec_cb(&a, swing_cb);
  lv_anim_set_values(&a, 0, 1100);
  lv_anim_set_duration(&a, 1100);
  lv_anim_set_path_cb(&a, lv_anim_path_linear);
  lv_anim_start(&a);
}

inline void light_set(lv_obj_t *tile, bool on, bool swings = false) {
  auto &s = lights[tile];
  if (s.known && s.on == on)
    return;
  bool first = !s.known;
  s.on = on;
  s.known = true;
  lv_label_set_text(child(tile, 3), on ? "An" : "Aus");
  anim(tile, light_anim_cb, s.level, on ? 255 : 0, first ? 0 : 380);
  if (swings && !first) {
    swing(tile, s.swing_left);
    s.swing_left = !s.swing_left;
  }
  lights_changed();
}

inline bool light_is_on(lv_obj_t *tile) { return lights[tile].on; }

// ---------------------------------------------------------------------------
// Rollläden
// Karte: 0 = Fenster (Kind 0: Sonne, Kind 1: Behang), 1 = Name, 2 = Position,
//        3 = Fahrt-Hinweis, 4 = Knopfleiste (hoch, stopp, runter)
// ---------------------------------------------------------------------------
// Innenhöhe des Fensters (64 px normal, mehr bei hohen Karten)
inline int32_t window_inner_h(lv_obj_t *shutter) {
  int32_t h = lv_obj_get_content_height(lv_obj_get_parent(shutter));
  return h > 0 ? h : 64;
}

struct CoverState {
  float pos = NAN;  // 0 = zu, 100 = offen
  int dir = 0;      // 1 = öffnet, -1 = schließt
  uint32_t travel_ms = 30000;
};

inline std::map<lv_obj_t *, CoverState> covers;
inline lv_obj_t *cover_count_num = nullptr;
inline lv_obj_t *cover_count_text = nullptr;

inline void shutter_cb(void *var, int32_t v) { lv_obj_set_height((lv_obj_t *) var, v); }
inline void blink_cb(void *var, int32_t v) { lv_obj_set_style_text_opa((lv_obj_t *) var, v, 0); }

inline void covers_changed() {
  int moving = 0, n = 0;
  float sum = 0;
  for (auto &it : covers) {
    if (it.second.dir != 0)
      moving++;
    if (!std::isnan(it.second.pos)) {
      sum += it.second.pos;
      n++;
    }
  }
  char buf[32];
  if (cover_count_num != nullptr && cover_count_text != nullptr) {
    if (moving > 0) {
      snprintf(buf, sizeof(buf), "%d", moving);
      lv_label_set_text(cover_count_num, buf);
      lv_label_set_text(cover_count_text, " in Fahrt");
    } else {
      if (n > 0)
        snprintf(buf, sizeof(buf), "%d %%", (int) lroundf(sum / n));
      else
        snprintf(buf, sizeof(buf), "--");
      lv_label_set_text(cover_count_num, buf);
      lv_label_set_text(cover_count_text, " offen im Schnitt");
    }
  }
}

inline void cover_register(lv_obj_t *card, uint32_t travel_s) {
  if (covers.find(card) == covers.end()) {
    CoverState s;
    s.travel_ms = travel_s * 1000;
    covers[card] = s;
  }
}

// Zeichnet Texte, Knöpfe und startet die Behang-Animation
inline void cover_render(lv_obj_t *card) {
  auto &s = covers[card];
  lv_obj_t *window = child(card, 0);
  lv_obj_t *shutter = child(window, 1);
  lv_obj_t *pos = child(card, 2);
  lv_obj_t *hint = child(card, 3);
  lv_obj_t *buttons = child(card, 4);

  char buf[16];
  if (std::isnan(s.pos))
    snprintf(buf, sizeof(buf), "--");
  else if (s.pos >= 99)
    snprintf(buf, sizeof(buf), "Offen");
  else if (s.pos <= 1)
    snprintf(buf, sizeof(buf), "Zu");
  else
    snprintf(buf, sizeof(buf), "%d %%", (int) lroundf(s.pos));
  lv_label_set_text(pos, buf);

  lv_label_set_text(hint, s.dir > 0 ? "öffnet …" : (s.dir < 0 ? "schließt …" : ""));
  lv_obj_update_layout(pos);
  lv_obj_align_to(hint, pos, LV_ALIGN_OUT_RIGHT_BOTTOM, 8, -3);
  if (s.dir != 0) {
    if (lv_anim_get(hint, blink_cb) == nullptr) {
      lv_anim_t a;
      lv_anim_init(&a);
      lv_anim_set_var(&a, hint);
      lv_anim_set_exec_cb(&a, blink_cb);
      lv_anim_set_values(&a, 60, 255);
      lv_anim_set_duration(&a, 500);
      lv_anim_set_reverse_duration(&a, 500);
      lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
      lv_anim_start(&a);
    }
  } else {
    lv_anim_delete(hint, blink_cb);
    lv_obj_set_style_text_opa(hint, 255, 0);
  }

  for (int i = 0; i < 3; i++) {
    lv_obj_t *b = child(buttons, i);
    bool act = (i == 0 && s.dir > 0) || (i == 2 && s.dir < 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(act ? SKY_BG : CARD2), 0);
    lv_obj_set_style_text_color(child(b, 0), lv_color_hex(act ? SKY : TEXT), 0);
  }

  // Behang: Höhe = Anteil "zu"
  int32_t full = window_inner_h(shutter);
  int32_t now = lv_obj_get_style_height(shutter, LV_PART_MAIN);
  if (s.dir > 0) {
    anim(shutter, shutter_cb, now, 0, s.travel_ms * now / full, lv_anim_path_linear);
  } else if (s.dir < 0) {
    anim(shutter, shutter_cb, now, full, s.travel_ms * (full - now) / full, lv_anim_path_linear);
  } else if (!std::isnan(s.pos)) {
    int32_t target = (int32_t) lroundf(full * (100.0f - s.pos) / 100.0f);
    anim(shutter, shutter_cb, now, target, 450);
  }
  covers_changed();
}

inline void cover_position(lv_obj_t *card, float pos) {
  auto &s = covers[card];
  if (std::isnan(pos))
    return;
  s.pos = pos;
  if (s.dir != 0) {
    // Während der Fahrt: Behang auf die gemeldete Position setzen, Fahrt läuft weiter
    lv_obj_t *shutter = child(child(card, 0), 1);
    lv_anim_delete(shutter, shutter_cb);
    lv_obj_set_height(shutter, (int32_t) lroundf(window_inner_h(shutter) * (100.0f - pos) / 100.0f));
  }
  cover_render(card);
}

// Zustand aus HA: "opening", "closing", "open", "closed"
inline void cover_state(lv_obj_t *card, const std::string &state) {
  auto &s = covers[card];
  int dir = state == "opening" ? 1 : (state == "closing" ? -1 : 0);
  if (dir == s.dir && dir != 0)
    return;
  s.dir = dir;
  cover_render(card);
}

// Knopf gedrückt: sofort anzeigen, HA bestätigt später
inline void cover_command(lv_obj_t *card, int dir) {
  auto &s = covers[card];
  lv_obj_t *shutter = child(child(card, 0), 1);
  if (dir == 0) {
    // Stopp: Behang bleibt, wo er gerade ist
    lv_anim_delete(shutter, shutter_cb);
    int32_t h = lv_obj_get_style_height(shutter, LV_PART_MAIN);
    s.pos = 100.0f - h * 100.0f / window_inner_h(shutter);
  }
  if ((dir > 0 && s.pos >= 99) || (dir < 0 && s.pos <= 1))
    dir = 0;
  s.dir = dir;
  cover_render(card);
}

// ---------------------------------------------------------------------------
// Klima
// ---------------------------------------------------------------------------
constexpr int MODE_COUNT = 6, FAN_COUNT = 7;
inline const char *const MODE_KEYS[MODE_COUNT] = {"off", "heat", "cool", "heat_cool", "dry", "fan_only"};
inline const char *const MODE_NAMES[MODE_COUNT] = {"Aus", "Heizen", "Kühlen", "Auto", "Trocken", "Lüften"};
inline const uint32_t MODE_COLORS[MODE_COUNT] = {0x6B7488, 0xFF7A45, 0x46C2FF, 0xB392FF, 0x3DD6C0, 0xA9B6CB};
inline const char *const FAN_KEYS[FAN_COUNT] = {"auto", "quiet", "1", "2", "3", "4", "5"};
inline const char *const FAN_NAMES[FAN_COUNT] = {"Auto", "Leise", "Stufe 1", "Stufe 2", "Stufe 3", "Stufe 4", "Stufe 5"};
inline const uint32_t FAN_MS[FAN_COUNT] = {1400, 2800, 2200, 1700, 1300, 1000, 700};
inline const int FAN_LEVEL[FAN_COUNT] = {0, 1, 1, 2, 3, 4, 5};

struct Ac {
  int mode = 0;
  float target = NAN;
  int fan = 0;
  uint32_t color = 0x6B7488;  // aktuelle Modusfarbe (für Leiste und Ring)
  uint32_t color_from = 0x6B7488, color_to = 0x6B7488;
  uint32_t fan_ms = 0;
  uint32_t edited_ms = 0;  // letzte Bedienung am Panel
  // Widgets
  lv_obj_t *arc = nullptr, *halo = nullptr, *mode_label = nullptr, *target_label = nullptr;
  lv_obj_t *modes = nullptr, *fan_icon = nullptr, *fan_text = nullptr, *fan_bars = nullptr;
};
inline Ac ac;

inline int mode_index(const std::string &s) {
  for (int i = 0; i < MODE_COUNT; i++)
    if (s == MODE_KEYS[i])
      return i;
  return 0;
}

inline int fan_index(const std::string &s) {
  for (int i = 0; i < FAN_COUNT; i++)
    if (s == FAN_KEYS[i])
      return i;
  return 0;
}

// Bedienung am Panel blockiert HA-Werte kurz, damit nichts zurückspringt
inline bool ac_editing() { return ac.edited_ms != 0 && lv_tick_elaps(ac.edited_ms) < 6000; }
inline void ac_touch() { ac.edited_ms = lv_tick_get(); }

inline void nav_paint();
inline uint32_t tab_colors[4] = {AMBER, SKY, NAV_NEUTRAL, NAV_NEUTRAL};  // Farbe je Reiter

inline void ac_color_cb(void *, int32_t v) {
  lv_color_t c = mix(ac.color_from, ac.color_to, v);
  bool on = ac.mode != 0;
  if (ac.arc != nullptr) {
    lv_obj_set_style_arc_color(ac.arc, on ? c : lv_color_hex(0x4A546A), LV_PART_INDICATOR);
    lv_obj_set_style_border_color(ac.arc, on ? c : lv_color_hex(0x4A546A), LV_PART_KNOB);
  }
  if (ac.halo != nullptr)
    lv_obj_set_style_bg_color(ac.halo, c, 0);
  if (ac.mode_label != nullptr)
    lv_obj_set_style_text_color(ac.mode_label, on ? c : lv_color_hex(MUTE), 0);
  if (ac.fan_icon != nullptr)
    lv_obj_set_style_text_color(ac.fan_icon, on ? c : lv_color_hex(0xA9B6CB), 0);
}

inline void fan_spin_cb(void *var, int32_t v) { lv_obj_set_style_transform_rotation((lv_obj_t *) var, v, 0); }
inline void halo_cb(void *var, int32_t v) { lv_obj_set_style_bg_opa((lv_obj_t *) var, v, 0); }

inline void ac_render() {
  bool on = ac.mode != 0;
  uint32_t color = MODE_COLORS[ac.mode];

  // Farbe weich überblenden
  if (color != ac.color) {
    ac.color_from = ac.color;
    ac.color_to = color;
    ac.color = color;
    lv_anim_delete(&ac, ac_color_cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &ac);
    lv_anim_set_exec_cb(&a, ac_color_cb);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, 450);
    lv_anim_start(&a);
  } else {
    ac_color_cb(&ac, 255);
  }
  if (ac.halo != nullptr)
    anim(ac.halo, halo_cb, lv_obj_get_style_bg_opa(ac.halo, LV_PART_MAIN), on ? 34 : 0, 450);

  if (ac.mode_label != nullptr) {
    char buf[24];
    if (on)
      snprintf(buf, sizeof(buf), "%s auf", MODE_NAMES[ac.mode]);
    else
      snprintf(buf, sizeof(buf), "Aus · Ziel");
    lv_label_set_text(ac.mode_label, buf);
  }
  if (ac.target_label != nullptr)
    set_temp(ac.target_label, ac.target, "");
  if (ac.arc != nullptr && !std::isnan(ac.target) && !lv_obj_has_state(ac.arc, LV_STATE_PRESSED))
    lv_arc_set_value(ac.arc, (int32_t) lroundf(ac.target * 2));

  // Modus-Knöpfe: Kinder 0 = Icon, 1 = Text
  if (ac.modes != nullptr) {
    for (int i = 0; i < MODE_COUNT; i++) {
      lv_obj_t *b = child(ac.modes, i);
      if (b == nullptr)
        break;
      bool sel = i == ac.mode;
      uint32_t c = MODE_COLORS[i];
      lv_obj_set_style_border_color(b, lv_color_hex(sel ? c : LINE), 0);
      lv_obj_set_style_bg_color(b, sel ? mix(CARD, c, 45) : lv_color_hex(CARD), 0);
      lv_color_t tc = lv_color_hex(sel ? c : MUTE);
      lv_obj_set_style_text_color(child(b, 0), tc, 0);
      lv_obj_set_style_text_color(child(b, 1), tc, 0);
    }
  }

  // Lüfter: Text, Balken, Drehung
  if (ac.fan_text != nullptr) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%s", FAN_NAMES[ac.fan]);
    lv_label_set_text(ac.fan_text, buf);
  }
  if (ac.fan_bars != nullptr) {
    int level = FAN_LEVEL[ac.fan];
    for (int i = 0; i < 5; i++) {
      lv_obj_t *bar = child(ac.fan_bars, i);
      bool lit = on && (level == 0 || i < level);
      lv_obj_set_style_bg_color(bar, lit ? lv_color_hex(color) : lv_color_hex(LINE), 0);
      lv_obj_set_style_bg_opa(bar, lit && level == 0 ? 120 : 255, 0);
    }
  }
  if (ac.fan_icon != nullptr) {
    uint32_t ms = on ? FAN_MS[ac.fan] : 0;
    if (ms != ac.fan_ms) {
      ac.fan_ms = ms;
      lv_anim_delete(ac.fan_icon, fan_spin_cb);
      if (ms == 0) {
        lv_obj_set_style_transform_rotation(ac.fan_icon, 0, 0);
      } else {
        // Drehpunkt = Mitte des 32 × 32 großen Symbols (siehe tiles/climate.yml)
        lv_obj_set_style_transform_pivot_x(ac.fan_icon, 16, 0);
        lv_obj_set_style_transform_pivot_y(ac.fan_icon, 16, 0);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, ac.fan_icon);
        lv_anim_set_exec_cb(&a, fan_spin_cb);
        lv_anim_set_values(&a, 0, 3600);
        lv_anim_set_duration(&a, ms);
        lv_anim_set_path_cb(&a, lv_anim_path_linear);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
      }
    }
  }
  tab_colors[2] = on ? color : NAV_NEUTRAL;
  nav_paint();
}

// ---------------------------------------------------------------------------
// Thermostat (z. B. Fußbodenheizung über Homematic)
// ---------------------------------------------------------------------------
constexpr int TH_MODE_COUNT = 3;
inline const char *const TH_KEYS[TH_MODE_COUNT] = {"auto", "heat", "off"};
inline const char *const TH_NAMES[TH_MODE_COUNT] = {"Zeitplan", "Manuell", "Aus"};
inline const uint32_t TH_COLORS[TH_MODE_COUNT] = {0xFFA36B, 0xFF7A45, 0x6B7488};
constexpr uint32_t HEAT = 0xFF7A45, HEAT_SOFT = 0xFFA36B, OFF_GREY = 0x4A546A;

struct Thermo {
  int mode = 0;
  float target = NAN;
  bool heating = false;  // hvac_action == heating
  bool boost = false;    // preset_mode == boost
  uint32_t edited_ms = 0;
  int tab = 2;  // Reiter, auf dem das Thermostat liegt (2 = dritter, 3 = vierter)
  uint32_t color = OFF_GREY, color_from = OFF_GREY, color_to = OFF_GREY;
  lv_obj_t *arc = nullptr, *halo = nullptr, *mode_label = nullptr, *target_label = nullptr;
  lv_obj_t *modes = nullptr, *boost_btn = nullptr, *status_icon = nullptr, *status_text = nullptr;
};
inline Thermo th;

inline int th_mode_index(const std::string &s) {
  for (int i = 0; i < TH_MODE_COUNT; i++)
    if (s == TH_KEYS[i])
      return i;
  return 0;
}

inline bool th_editing() { return th.edited_ms != 0 && lv_tick_elaps(th.edited_ms) < 6000; }
inline void th_touch() { th.edited_ms = lv_tick_get(); }

inline void th_color_cb(void *, int32_t v) {
  lv_color_t c = mix(th.color_from, th.color_to, v);
  if (th.arc != nullptr) {
    lv_obj_set_style_arc_color(th.arc, c, LV_PART_INDICATOR);
    lv_obj_set_style_border_color(th.arc, c, LV_PART_KNOB);
  }
  if (th.halo != nullptr)
    lv_obj_set_style_bg_color(th.halo, c, 0);
  if (th.mode_label != nullptr)
    lv_obj_set_style_text_color(th.mode_label, th.mode == 2 ? lv_color_hex(MUTE) : c, 0);
}

// Knopf hervorheben: Kinder 0 = Icon, 1 = Text (weitere Kinder bleiben unverändert)
inline void paint_choice(lv_obj_t *b, bool sel, uint32_t c) {
  lv_obj_set_style_border_color(b, lv_color_hex(sel ? c : LINE), 0);
  lv_obj_set_style_bg_color(b, sel ? mix(CARD, c, 45) : lv_color_hex(CARD), 0);
  lv_color_t tc = lv_color_hex(sel ? c : MUTE);
  lv_obj_set_style_text_color(child(b, 0), tc, 0);
  lv_obj_set_style_text_color(child(b, 1), tc, 0);
}

inline void th_render() {
  bool off = th.mode == 2;
  bool warm = th.heating || th.boost;
  uint32_t color = off ? OFF_GREY : (warm ? HEAT : HEAT_SOFT);

  if (color != th.color) {
    th.color_from = th.color;
    th.color_to = color;
    th.color = color;
    lv_anim_delete(&th, th_color_cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &th);
    lv_anim_set_exec_cb(&a, th_color_cb);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, 450);
    lv_anim_start(&a);
  } else {
    th_color_cb(&th, 255);
  }
  if (th.halo != nullptr)
    anim(th.halo, halo_cb, lv_obj_get_style_bg_opa(th.halo, LV_PART_MAIN), !off && warm ? 34 : 0, 450);

  if (th.mode_label != nullptr) {
    const char *text = off ? "Aus · Ziel"
                           : th.boost ? "Boost aktiv"
                                      : th.heating ? "Heizt auf" : (th.mode == 0 ? "Zeitplan · Ziel" : "Manuell · Ziel");
    lv_label_set_text(th.mode_label, text);
  }
  if (th.target_label != nullptr)
    set_temp(th.target_label, th.target, "");
  if (th.arc != nullptr && !std::isnan(th.target) && !lv_obj_has_state(th.arc, LV_STATE_PRESSED))
    lv_arc_set_value(th.arc, (int32_t) lroundf(th.target * 2));

  if (th.modes != nullptr) {
    for (int i = 0; i < TH_MODE_COUNT; i++) {
      lv_obj_t *b = child(th.modes, i);
      if (b != nullptr)
        paint_choice(b, i == th.mode, TH_COLORS[i]);
    }
  }
  if (th.boost_btn != nullptr)
    paint_choice(th.boost_btn, th.boost, HEAT);
  if (th.status_icon != nullptr)
    lv_obj_set_style_text_color(th.status_icon, lv_color_hex(th.heating ? HEAT : MUTE), 0);
  if (th.status_text != nullptr)
    lv_label_set_text(th.status_text, th.heating ? "Heizt gerade" : (off ? "Aus" : "Ruht"));

  tab_colors[th.tab] = off ? NAV_NEUTRAL : color;
  nav_paint();
}

// ---------------------------------------------------------------------------
// Navigation unten: gleitende Markierung mit Nachfedern
// ---------------------------------------------------------------------------
inline int page = 0;
inline lv_obj_t *nav_indicator = nullptr;
inline lv_obj_t *nav_buttons[4] = {nullptr, nullptr, nullptr, nullptr};
constexpr int32_t NAV_X0 = 5;
inline int32_t nav_step = 149;  // 112 bei vier Reitern

inline void nav_x_cb(void *var, int32_t v) { lv_obj_set_x((lv_obj_t *) var, v); }

inline void nav_paint() {
  for (int i = 0; i < 4; i++) {
    if (nav_buttons[i] == nullptr)
      continue;
    lv_obj_set_style_text_color(nav_buttons[i], lv_color_hex(i == page ? tab_colors[i] : MUTE), 0);
  }
  if (nav_indicator != nullptr) {
    lv_obj_set_style_border_color(nav_indicator, lv_color_hex(tab_colors[page]), 0);
    lv_obj_set_style_shadow_color(nav_indicator, lv_color_hex(tab_colors[page]), 0);
  }
}

inline void nav_go(int p) {
  page = p;
  if (nav_indicator != nullptr)
    anim(nav_indicator, nav_x_cb, lv_obj_get_x(nav_indicator), NAV_X0 + p * nav_step, 420, lv_anim_path_overshoot);
  nav_paint();
}

// ---------------------------------------------------------------------------
// Datum auf Deutsch
// ---------------------------------------------------------------------------
inline void format_date(char *buf, size_t len, int wday, int mday, int month) {
  static const char *const WD[7] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
  static const char *const MO[12] = {"Januar", "Februar", "März",      "April",   "Mai",      "Juni",
                                     "Juli",   "August",  "September", "Oktober", "November", "Dezember"};
  // ESPTime: day_of_week 1 = Sonntag, month 1..12
  snprintf(buf, len, "%s, %d. %s", WD[(wday + 6) % 7], mday, MO[(month + 11) % 12]);
}

// ---------------------------------------------------------------------------
// FPS-Anzeige: fertige Bilder pro Sekunde, Zeit pro Bild (davon Kopieren in die
// Anzeige, inkl. Drehen) und Anteil des Bildschirms, der pro Bild neu gezeichnet wird
// ---------------------------------------------------------------------------
inline bool show_fps = true;
inline lv_obj_t *fps_label = nullptr;
inline uint32_t fps_frames = 0, fps_busy = 0, fps_t0 = 0, fps_window = 0, fps_flush = 0, fps_f0 = 0;
inline uint64_t fps_px = 0;

inline void fps_init(const lv_font_t *font) {
  lv_display_t *disp = lv_display_get_default();
  lv_display_add_event_cb(disp, [](lv_event_t *) { fps_t0 = lv_tick_get(); }, LV_EVENT_RENDER_START, nullptr);
  lv_display_add_event_cb(disp, [](lv_event_t *) {
    fps_frames++;
    fps_busy += lv_tick_elaps(fps_t0);
  }, LV_EVENT_RENDER_READY, nullptr);
  lv_display_add_event_cb(disp, [](lv_event_t *) { fps_f0 = lv_tick_get(); }, LV_EVENT_FLUSH_START, nullptr);
  lv_display_add_event_cb(disp, [](lv_event_t *) { fps_flush += lv_tick_elaps(fps_f0); }, LV_EVENT_FLUSH_FINISH, nullptr);
  lv_display_add_event_cb(disp, [](lv_event_t *e) {
    auto *a = (const lv_area_t *) lv_event_get_param(e);
    if (a)
      fps_px += (uint64_t) lv_area_get_width(a) * lv_area_get_height(a);
  }, LV_EVENT_INVALIDATE_AREA, nullptr);
  fps_window = lv_tick_get();
  if (show_fps) {
    fps_label = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_font(fps_label, font, 0);
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0x5C667C), 0);
    lv_label_set_text(fps_label, "-- fps");
    lv_obj_align(fps_label, LV_ALIGN_TOP_MID, 0, 2);
  }
}

inline void fps_update() {
  uint32_t ms = lv_tick_elaps(fps_window);
  if (ms < 900)
    return;
  float fps = fps_frames * 1000.0f / ms;
  float per = fps_frames ? (float) fps_busy / fps_frames : 0;
  float copy = fps_frames ? (float) fps_flush / fps_frames : 0;
  lv_display_t *disp = lv_display_get_default();
  float share = fps_frames ? 100.0f * fps_px / fps_frames /
                                 ((float) lv_display_get_horizontal_resolution(disp) * lv_display_get_vertical_resolution(disp))
                           : 0;
  char buf[64];
  snprintf(buf, sizeof buf, "%d fps · %.1f ms (Kopie %.1f) · %d %%", (int) lroundf(fps), per, copy, (int) lroundf(share));
  if (fps_label)
    lv_label_set_text(fps_label, buf);
  if (show_fps)
    ESP_LOGI("panel", "%s", buf);
  fps_frames = fps_busy = fps_flush = 0;
  fps_px = 0;
  fps_window = lv_tick_get();
}

}  // namespace panel

#pragma once

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor KEY_SECOND_COLOR;
  GColor KEY_BG_COLOR;
  GColor KEY_RING_COLOR;
  bool KEY_DATE;
  bool KEY_SECONDS;
} ClaySettings;

typedef struct {
  int color_circle_thickness;
  int center_outer_circle_radius;
  int center_inner_circle_radius;
  int hour_hand_width;
  float minute_hand_width;
  float second_hand_width;
  int hour_hand_circle_radius;
  int seconds_hand_circle_radius;
  int hour_hand_length;
  int minute_hand_length;
  int seconds_hand_length;
} BaseScaleValues;

extern const BaseScaleValues BASE_SCALE;

typedef enum {
  SCREEN_TYPE_OG_RECT,
  SCREEN_TYPE_OG_ROUND,
  SCREEN_TYPE_RECT_V2,
  SCREEN_TYPE_ROUND_V2
} ScreenType;

void clay_default_settings(); 
void clay_load_settings();
void clay_save_settings();

ScreenType get_watch_type(void);
void watch_type_init(void);

static inline int get_font_pixel_height(GFont font, const char *test_str)
{

  // const char *test_str = "Ag";

  // Create a large bounds box so text isn't constrained
  GRect max_bounds = GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);

  // Measure text layout
  GSize size = graphics_text_layout_get_content_size(
      test_str,
      font,
      max_bounds,
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft);

  APP_LOG(APP_LOG_LEVEL_INFO, "Font height: %d", size.h);

  return size.h;
}

static inline int get_font_pixel_width(GFont font, const char *test_str)
{

  // const char *test_str = "Ag";

  // Create a large bounds box so text isn't constrained
  GRect max_bounds = GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT);

  // Measure text layout
  GSize size = graphics_text_layout_get_content_size(
      test_str,
      font,
      max_bounds,
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft);

  APP_LOG(APP_LOG_LEVEL_INFO, "Font width: %d px for string: %s", size.h, test_str);

  return size.w;
}
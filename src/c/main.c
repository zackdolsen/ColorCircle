#include "pebble.h"
#include "main.h"
#include <stdlib.h>

#define UNIT_TESTING 0

#define COLORS PBL_IF_COLOR_ELSE(true, false)
#define ROUND PBL_IF_ROUND_ELSE(true, false)
#define ANTIALIASING true
#define INVERT true

#define ANIMATION_DURATION 1000
#define ANIMATION_DELAY 800
#define WATCH_TYPE get_watch_type()

typedef struct
{
  int hours;
  int minutes;
  int seconds;
  int day;
} Time;

ClaySettings settings;

/*************** GLOBALS ***************/
// drawing variables
static Window *s_main_window;
static Layer *s_canvas_layer;
static GPoint s_center;
static Time s_current_time;
static bool s_animating = false;
static float ui_scale;
static GFont s_gfont_date;
static int prev_hours = -1;
static GColor ring_color = GColorMediumSpringGreen;

// Scaling variables for layout, see UIHelper.c for the default values
static int FINAL_RADIUS;
static int color_circle_radius;        // becomes final radius through animation
static int color_circle_thickness;     // thickness of the colored ring
static int center_outer_circle_radius; // outer center circle radius
static int center_inner_circle_radius; // inner center circle radius
static int hour_hand_width;            // width of hour hand
static int minute_hand_width;          // width of minute hand
static int second_hand_width;          // width of second hand
static int hour_hand_circle_radius;    // radius of circle at end of hour hand
static int seconds_hand_circle_radius; // radius of circle at end of second hand
static int hour_hand_length;
static int minute_hand_length;
static int seconds_hand_length;
static int date_circle_radius;
static int hour_dots_radius;

// Lengths of hands for animating. Start at 0 ad grow to final length
static int s_hour_length = 0;
static int s_minute_length = 0;
static int s_seconds_length = 0;
static float s_anim_progress = 0.0f; // 0.0 to 1.0 to show animation percentage

// date variables
static int s_date_circle_radius = 0; // used for animating circle
static int s_date_text_height;
static int s_date_text_width;
static int s_y_text_offset; // offset for text to center it in the circle
static int s_x_text_offset; // offset for text to center it in the circle

// Battery and bluetooth status
static bool battery_charging = false;
static int battery_percent = 20;
static bool bluetooth_connected;

/*********** ANIMATION HANDLERS ***********/
static void animation_started(Animation *anim, void *context)
{
  s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context)
{
  s_animating = false;
}

static void animate(int duration, int delay, AnimationImplementation *impl, bool handlers)
{
  Animation *anim = animation_create();
  animation_set_duration(anim, duration);
  animation_set_delay(anim, delay);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  animation_set_implementation(anim, impl);
  if (handlers)
  {
    animation_set_handlers(anim, (AnimationHandlers){.started = animation_started, .stopped = animation_stopped}, NULL);
  }

  animation_schedule(anim);
}

GColor randomGColor()
{
  GColor tcolor;
  bool isValidColor = false;
  do
  {
    tcolor = GColorFromRGB(
        rand() % 256,
        rand() % 256,
        rand() % 256);

    if (tcolor.argb != GColorBlack.argb && tcolor.argb != GColorWhite.argb && tcolor.argb != settings.KEY_BG_COLOR.argb && tcolor.argb != settings.KEY_SECOND_COLOR.argb)
    {
      isValidColor = true;
    }
    else
    {
      APP_LOG(APP_LOG_LEVEL_INFO, "Invalid color generated: %d", tcolor.argb);
    }
  } while (!isValidColor);

  return tcolor;
}

void update_random_ring_color(bool force)
{
  if (settings.KEY_RANDOM_COLOR != 2 && settings.KEY_RANDOM_COLOR != 3)
  {
    return;
  }

  if (!force)
  {
    if (settings.KEY_RANDOM_COLOR == 2 && prev_hours == s_current_time.minutes)
    {
      return;
    }

    if (settings.KEY_RANDOM_COLOR == 3 && prev_hours == s_current_time.hours)
    {
      return;
    }
  }

  ring_color = randomGColor();
  settings.KEY_RING_COLOR = ring_color;
  prev_hours = (settings.KEY_RANDOM_COLOR == 2) ? s_current_time.minutes : s_current_time.hours;
  clay_save_settings();

  if (s_canvas_layer != NULL)
  {
    layer_mark_dirty(s_canvas_layer);
  }
}

void update_tick_subscription(void)
{
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(settings.KEY_SECONDS ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void handle_battery(BatteryChargeState charge_state)
{
  battery_charging = charge_state.is_charging;
  battery_percent = charge_state.charge_percent;

  APP_LOG(APP_LOG_LEVEL_INFO, "Battery status: charging=%s, percent=%d", battery_charging ? "true" : "false", battery_percent);

  layer_mark_dirty(s_canvas_layer);
}

/************* TIME & HANDS *************/
void tick_handler(struct tm *tick_time, TimeUnits changed)
{
  s_current_time.hours = tick_time->tm_hour;
  s_current_time.hours -= (s_current_time.hours > 12) ? 12 : 0;
  s_current_time.minutes = tick_time->tm_min;
  s_current_time.seconds = tick_time->tm_sec;
  s_current_time.day = tick_time->tm_mday;

  if (settings.KEY_RANDOM_COLOR == 2)
  {
    if (prev_hours != tick_time->tm_min)
    {
      update_random_ring_color(true);
    }
  }
  else if (settings.KEY_RANDOM_COLOR == 3)
  {
    if (prev_hours != tick_time->tm_hour)
    {
      update_random_ring_color(true);
    }
  }

  layer_mark_dirty(s_canvas_layer);
}

/************* DRAWING *************/
static void update_proc(Layer *layer, GContext *ctx)
{
  // GRect bounds = layer_get_bounds(layer);
  GRect bounds = layer_get_unobstructed_bounds(layer);
  s_center = grect_center_point(&bounds);

  // default hand colors (overridden by white background)
  GColor hand_gcolor = GColorWhite;
  GColor hourdot_gcolor = GColorBlack;

  // if background is white, make hands black for contrast and flip accent color
  if (settings.KEY_BG_COLOR.argb == GColorWhite.argb)
  {
    hand_gcolor = GColorBlack;
    hourdot_gcolor = GColorWhite;
  }

  // Black background
  graphics_context_set_fill_color(ctx, settings.KEY_BG_COLOR);
  if (UNIT_TESTING && ROUND)
  {
    graphics_context_set_fill_color(ctx, GColorDarkGray);
  }
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw colored ring
  // determined colors for ring based on color screen and settings
  if (COLORS)
  {
    graphics_context_set_stroke_color(ctx, settings.KEY_RING_COLOR);
  }
  else
  {
    graphics_context_set_stroke_color(ctx, hand_gcolor);
  }
  graphics_context_set_stroke_width(ctx, color_circle_thickness);
  graphics_context_set_antialiased(ctx, ANTIALIASING);
  graphics_draw_circle(ctx, s_center, color_circle_radius);

  // Draw minute dots
  if (settings.KEY_MIN_DOTS)
  {
    for (int i = 0; i < 60; i++)
    {
      int32_t minute_angle = (int32_t)(TRIG_MAX_ANGLE * i / 60.0f);
      GPoint minute_dot = {
          .x = (int16_t)(sin_lookup(minute_angle) * (color_circle_radius) / TRIG_MAX_RATIO) + s_center.x,
          .y = (int16_t)(-cos_lookup(minute_angle) * (color_circle_radius) / TRIG_MAX_RATIO) + s_center.y};

      graphics_context_set_fill_color(ctx, hourdot_gcolor);
      graphics_fill_circle(ctx, minute_dot, 1);
    }
  }

  // Draw hour dots
  if (settings.KEY_HOUR_DOTS)
  {
    for (int i = 0; i < 12; i++)
    {
      int32_t hour_angle = (int32_t)(TRIG_MAX_ANGLE * i / 12.0f);
      GPoint hour_dot = {
          .x = (int16_t)(sin_lookup(hour_angle) * (color_circle_radius) / TRIG_MAX_RATIO) + s_center.x,
          .y = (int16_t)(-cos_lookup(hour_angle) * (color_circle_radius) / TRIG_MAX_RATIO) + s_center.y};

      graphics_context_set_fill_color(ctx, hourdot_gcolor);
      graphics_fill_circle(ctx, hour_dot, hour_dots_radius);
    }
  }

  // Compute angles
  float anim_hours = s_animating ? s_anim_progress * s_current_time.hours : s_current_time.hours;
  float anim_minutes = s_animating ? s_anim_progress * s_current_time.minutes : s_current_time.minutes;
  float anim_seconds = s_animating ? s_anim_progress * s_current_time.seconds : s_current_time.seconds;

  // Compute angles from individual float values (no wraparound)
  int32_t minute_angle = (int32_t)(TRIG_MAX_ANGLE * anim_minutes / 60.0f);
  int32_t hour_angle = (int32_t)(TRIG_MAX_ANGLE * (anim_hours * 60.0f + anim_minutes) / 720.0f); // 12*60
  int32_t second_angle = (int32_t)(TRIG_MAX_ANGLE * anim_seconds / 60.0f);

  int32_t real_time_hour_angle = TRIG_MAX_ANGLE * (s_current_time.hours * 60 + s_current_time.minutes) / (12 * 60);

  // APP_LOG(APP_LOG_LEVEL_INFO, "real_time_hour_angle: %d", real_time_hour_angle);

  if (settings.KEY_DATE)
  {
    // Prepare day string
    char day_str[4];
    snprintf(day_str, sizeof(day_str), "%d", s_current_time.day);

    // Position inside the circle, on the right, relative to current ring radius
    int16_t x_offset = (FINAL_RADIUS + color_circle_thickness / 2) / 2; // halfway to the edge of the ring;

    if (real_time_hour_angle >= 8000 && real_time_hour_angle <= 22000)
    {
      x_offset = -1 * x_offset;
    }

    GPoint day_pos = GPoint(s_center.x + x_offset, s_center.y);

    if (s_date_circle_radius > 1)
    {
      graphics_context_set_stroke_width(ctx, color_circle_thickness / 4);
      graphics_draw_circle(ctx, day_pos, s_date_circle_radius);
      // graphics_context_set_fill_color(ctx, settings.KEY_RING_COLOR);
      // graphics_fill_circle(ctx, day_pos, s_date_circle_radius);
    }

    // Draw the day
    graphics_context_set_text_color(ctx, hand_gcolor);
    if (s_date_circle_radius >= s_date_text_height / 2 && s_date_circle_radius >= s_date_text_width / 2)
    { // only have text if circle can fit it
      graphics_draw_text(ctx, day_str, s_gfont_date,
                         GRect(day_pos.x - s_date_text_width / 2 - s_x_text_offset, day_pos.y - s_date_text_height / 2 - s_y_text_offset, s_date_text_width, s_date_text_height), // bounding box
                         GTextOverflowModeWordWrap,
                         GTextAlignmentCenter,
                         NULL);
    }
  }

  // Hour hand
  GPoint hour_hand = {
      .x = (int16_t)(sin_lookup(hour_angle) * s_hour_length / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(hour_angle) * s_hour_length / TRIG_MAX_RATIO) + s_center.y};

  // Minute hand
  GPoint minute_hand = {
      .x = (int16_t)(sin_lookup(minute_angle) * s_minute_length / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(minute_angle) * s_minute_length / TRIG_MAX_RATIO) + s_center.y};

  // Second hand
  GPoint second_hand = {
      .x = (int16_t)(sin_lookup(second_angle) * s_seconds_length / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(second_angle) * s_seconds_length / TRIG_MAX_RATIO) + s_center.y};

  // Draw hour and minute hands
  graphics_context_set_stroke_color(ctx, hand_gcolor);
  graphics_context_set_stroke_width(ctx, hour_hand_width);
  graphics_draw_line(ctx, s_center, hour_hand);

  graphics_context_set_stroke_width(ctx, minute_hand_width);
  graphics_draw_line(ctx, s_center, minute_hand);

  // Draw Outer center circle
  graphics_context_set_fill_color(ctx, hand_gcolor);
  graphics_fill_circle(ctx, s_center, center_outer_circle_radius);

  // draw second hand
  if (settings.KEY_SECONDS)
  {
    // determines colors for second hand and second hand circles based on color screen.
    if COLORS
    {
      graphics_context_set_fill_color(ctx, settings.KEY_SECOND_COLOR);
      graphics_context_set_stroke_color(ctx, settings.KEY_SECOND_COLOR);
    }
    else
    {
      graphics_context_set_fill_color(ctx, settings.KEY_BG_COLOR);
      graphics_context_set_stroke_color(ctx, hand_gcolor);
    }

    graphics_context_set_stroke_width(ctx, second_hand_width);
    graphics_draw_line(ctx, s_center, second_hand);

    GPoint second_circle = {
        .x = (int16_t)(sin_lookup(second_angle) * color_circle_radius / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(second_angle) * color_circle_radius / TRIG_MAX_RATIO) + s_center.y};

    if (color_circle_radius > 2 * center_outer_circle_radius)
    {
      graphics_fill_circle(ctx, second_circle, seconds_hand_circle_radius);
    }
  }

  // draw inner center circle
  if (settings.KEY_SECONDS && COLORS)
  {
    graphics_context_set_fill_color(ctx, settings.KEY_SECOND_COLOR);
  }
  else
  {
    graphics_context_set_fill_color(ctx, settings.KEY_BG_COLOR); // prev black
  }
  graphics_fill_circle(ctx, s_center, center_inner_circle_radius);

  // draw hour hand circle
  GPoint hour_circle = {
      .x = (int16_t)(sin_lookup(hour_angle) * (s_hour_length - hour_hand_circle_radius) / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(hour_angle) * (s_hour_length - hour_hand_circle_radius) / TRIG_MAX_RATIO) + s_center.y};

  if (color_circle_radius > 2 * center_outer_circle_radius)
  {

    // //battery percent color change - not working
    // if (battery_percent <= 20)
    // {
    //   hand_gcolor = GColorRed;
    // }
    // else if (battery_charging)
    // {
    //   hand_gcolor = GColorGreen;
    // }

    graphics_context_set_fill_color(ctx, hourdot_gcolor);
    graphics_fill_circle(ctx, hour_circle, hour_hand_circle_radius);
  }

  // APP_LOG(APP_LOG_LEVEL_INFO, "s_hour_length = %d", s_hour_length);
  // APP_LOG(APP_LOG_LEVEL_INFO, "s_minute_length = %d", s_minute_length);
  // APP_LOG(APP_LOG_LEVEL_INFO, "s_seconds_length = %d", s_seconds_length);
}

/************* WINDOW HANDLERS *************/
static void window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  // GRect bounds = layer_get_bounds(window_layer);
  GRect bounds = layer_get_unobstructed_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  battery_state_service_subscribe(handle_battery);
  handle_battery(battery_state_service_peek());
}

static void window_unload(Window *window)
{
  battery_state_service_unsubscribe();
  layer_destroy(s_canvas_layer);
}

/************* APP MESSAGE HANDLER *************/
static void in_received_handler(DictionaryIterator *iter, void *context)
{
  // Ring Color
  Tuple *color_t = dict_find(iter, MESSAGE_KEY_KEY_RING_COLOR);
  if (color_t)
  {
    settings.KEY_RING_COLOR = GColorFromHEX(color_t->value->int32);
  }

  // Background Color
  Tuple *color_bg_t = dict_find(iter, MESSAGE_KEY_KEY_BG_COLOR);
  if (color_bg_t)
  {
    settings.KEY_BG_COLOR = GColorFromHEX(color_bg_t->value->int32);
  }

  // Seconds Color
  Tuple *color_s_t = dict_find(iter, MESSAGE_KEY_KEY_SECOND_COLOR);
  if (color_s_t)
  {
    settings.KEY_SECOND_COLOR = GColorFromHEX(color_s_t->value->int32);
  }

  // Second hand
  Tuple *second_hand_t = dict_find(iter, MESSAGE_KEY_KEY_SECONDS);
  if (second_hand_t)
  {
    settings.KEY_SECONDS = second_hand_t->value->int32 == 1;
    update_tick_subscription();
  }

  // Date
  Tuple *date_t = dict_find(iter, MESSAGE_KEY_KEY_DATE);
  if (date_t)
  {
    settings.KEY_DATE = date_t->value->int32 == 1;
  }

  // Random Color
  Tuple *random_color_t = dict_find(iter, MESSAGE_KEY_KEY_RANDOM_COLOR);
  if (random_color_t)
  {
    if (random_color_t->type == TUPLE_CSTRING)
    {
      settings.KEY_RANDOM_COLOR = atoi(random_color_t->value->cstring);
    }
    else
    {
      settings.KEY_RANDOM_COLOR = random_color_t->value->int32;
    }
  }

  // Hour Dots
  Tuple *hour_dots_t = dict_find(iter, MESSAGE_KEY_KEY_HOUR_DOTS);
  if (hour_dots_t)
  {
    settings.KEY_HOUR_DOTS = hour_dots_t->value->int32 == 1;
  }

  // Minute Dots
  Tuple *min_dots_t = dict_find(iter, MESSAGE_KEY_KEY_MIN_DOTS);
  if (min_dots_t)
  {
    settings.KEY_MIN_DOTS = min_dots_t->value->int32 == 1;
  }

  if (settings.KEY_RANDOM_COLOR == 2 || settings.KEY_RANDOM_COLOR == 3)
  {
    update_random_ring_color(true);
  }

  clay_save_settings();

  layer_mark_dirty(s_canvas_layer);
}

/************* UI SCALING *************/
static void set_scale()
{
  color_circle_thickness = BASE_SCALE.color_circle_thickness * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "color_circle_thickness = %d", color_circle_thickness);

  center_outer_circle_radius = BASE_SCALE.center_outer_circle_radius * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "center_outer_circle_radius = %d", center_outer_circle_radius);

  center_inner_circle_radius = BASE_SCALE.center_inner_circle_radius * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "center_inner_circle_radius = %d", center_inner_circle_radius);

  hour_hand_width = BASE_SCALE.hour_hand_width * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_hand_width = %d", hour_hand_width);

  minute_hand_width = BASE_SCALE.minute_hand_width * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "minute_hand_width = %d", (int)minute_hand_width);

  second_hand_width = BASE_SCALE.second_hand_width * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "second_hand_width = %d", (int)second_hand_width);

  hour_hand_circle_radius = BASE_SCALE.hour_hand_circle_radius * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_hand_circle_radius = %d", hour_hand_circle_radius);

  seconds_hand_circle_radius = BASE_SCALE.seconds_hand_circle_radius * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "seconds_hand_circle_radius = %d", seconds_hand_circle_radius);

  hour_hand_length = BASE_SCALE.hour_hand_length * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_hand_length = %d", hour_hand_length);

  minute_hand_length = BASE_SCALE.minute_hand_length * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "minute_hand_length = %d", minute_hand_length);

  seconds_hand_length = BASE_SCALE.seconds_hand_length * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "seconds_hand_length = %d", seconds_hand_length);

  hour_dots_radius = BASE_SCALE.hour_dots_radius * ui_scale;
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_dots_radius = %d", hour_dots_radius);

  date_circle_radius = (s_date_text_width + 8) / 2;
  APP_LOG(APP_LOG_LEVEL_INFO, "date_circle_radius = %d", date_circle_radius);
}

/************* ANIMATION UPDATES *************/
static int anim_percentage(AnimationProgress dist_normalized, int max)
{
  return (int)((float)dist_normalized / ANIMATION_NORMALIZED_MAX * max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized)
{
  color_circle_radius = anim_percentage(dist_normalized, FINAL_RADIUS);
  s_date_circle_radius = anim_percentage(dist_normalized, date_circle_radius);
  layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized)
{
  s_anim_progress = (float)dist_normalized / ANIMATION_NORMALIZED_MAX;
  layer_mark_dirty(s_canvas_layer);
}

static void hour_length_update(Animation *anim, AnimationProgress dist_normalized)
{
  s_hour_length = anim_percentage(dist_normalized, hour_hand_length);

  layer_mark_dirty(s_canvas_layer);
}

static void minute_length_update(Animation *anim, AnimationProgress dist_normalized)
{
  s_minute_length = anim_percentage(dist_normalized, minute_hand_length);
  layer_mark_dirty(s_canvas_layer);
}

static void seconds_length_update(Animation *anim, AnimationProgress dist_normalized)
{
  s_seconds_length = anim_percentage(dist_normalized, seconds_hand_length);
  layer_mark_dirty(s_canvas_layer);
}

/************* INIT / DEINIT *************/
static void init()
{
  srand(time(NULL));

  watch_type_init();
  clay_load_settings();

  // set up FINAL_RADIUS based on watch type
  FINAL_RADIUS = PBL_DISPLAY_HEIGHT * 56 / 168; // original ratio for basalt
  APP_LOG(APP_LOG_LEVEL_INFO, "FINAL_RADIUS = %d", FINAL_RADIUS);

  // determine UI scale factor
  ui_scale = (float)PBL_DISPLAY_HEIGHT / (float)168; // 168 is basalt height
  APP_LOG(APP_LOG_LEVEL_INFO, "UI Scale x1000 = %ld", (int32_t)(ui_scale * 1000));

  // text size for screen size
  if (WATCH_TYPE == SCREEN_TYPE_OG_RECT || WATCH_TYPE == SCREEN_TYPE_OG_ROUND)
  {
    s_gfont_date = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    s_y_text_offset = 3;
    s_x_text_offset = 0;
  }
  else
  {
    s_gfont_date = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    s_y_text_offset = 5;
    s_x_text_offset = -1;
  }

  s_date_text_width = get_font_pixel_width(s_gfont_date, "30");
  s_date_text_height = get_font_pixel_height(s_gfont_date, "29");

  set_scale();

  app_message_open(128, 0);
  app_message_register_inbox_received(in_received_handler);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = window_load,
                                                .unload = window_unload});
  window_stack_push(s_main_window, true);

  // Set initial time
  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  prev_hours = (settings.KEY_RANDOM_COLOR == 2) ? time_now->tm_min : ((settings.KEY_RANDOM_COLOR == 3) ? time_now->tm_hour : -1);
  ring_color = settings.KEY_RING_COLOR;
  tick_handler(time_now, MINUTE_UNIT);

  update_tick_subscription();

  // Ring animation
  AnimationImplementation radius_impl = {.update = radius_update};
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

  // Hour hand length
  AnimationImplementation hour_impl = {.update = hour_length_update};
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &hour_impl, false);

  // Minute hand length
  AnimationImplementation minute_impl = {.update = minute_length_update};
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &minute_impl, false);

  // Second hand length
  AnimationImplementation second_impl = {.update = seconds_length_update};
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &second_impl, false);

  // Animate angles (hands rotation)
  AnimationImplementation hands_impl = {.update = hands_update};
  animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
}

static void deinit()
{
  tick_timer_service_unsubscribe();
  // app_message_close();
  window_destroy(s_main_window);
  app_message_deregister_callbacks();
}

int main()
{
  init();
  app_event_loop();
  deinit();
}

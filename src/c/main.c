#include <pebble.h>

#define KEY_INVERT 0
#define KEY_COLOR 1

#define HAND_MARGIN  10
#define FINAL_RADIUS 56

#define ANIMATION_DURATION 700
#define ANIMATION_DELAY    800

typedef struct {
  int hours;
  int minutes;
  int seconds;
} Time;

/*************** GLOBALS ***************/
static Window *s_main_window;
static Layer *s_canvas_layer;

static GPoint s_center;
static Time s_last_time, s_anim_time;
static int s_radius = 0;
static int t_radius = 7;
static double u_radius = 3.5;
static int c_radius = 2;
static bool s_animating = false;
static bool active;
static GColor ringColor;

static int s_hour_length = 0;
static int s_minute_length = 0;
static int s_seconds_length = 0; // we already talked about this

/*********** ANIMATION HANDLERS ***********/
static void animation_started(Animation *anim, void *context) {
  s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  s_animating = false;
}

static void animate(int duration, int delay, AnimationImplementation *impl, bool handlers) {
  Animation *anim = animation_create();
  animation_set_duration(anim, duration);
  animation_set_delay(anim, delay);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  animation_set_implementation(anim, impl);
  if (handlers) {
    animation_set_handlers(anim, (AnimationHandlers) {
      .started = animation_started,
      .stopped = animation_stopped
    }, NULL);
  }
  animation_schedule(anim);
}

/************* TIME & HANDS *************/
static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;
  s_last_time.seconds = tick_time->tm_sec;
  layer_mark_dirty(s_canvas_layer);
}

static int hours_to_minutes(int hours) {
  return (int)((float)hours / 12.0f * 60.0f);
}


/************* DRAWING *************/
static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Black background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw colored ring
  graphics_context_set_stroke_color(ctx, ringColor);
  graphics_context_set_stroke_width(ctx, 9);
  graphics_context_set_antialiased(ctx, true);
  graphics_draw_circle(ctx, s_center, s_radius);
  
  // Determine current time (or animation)
  Time draw_time = s_animating ? s_anim_time : s_last_time;

  // Compute angles
  int32_t minute_angle = TRIG_MAX_ANGLE * draw_time.minutes / 60;
  int32_t hour_angle = TRIG_MAX_ANGLE * draw_time.hours / 12 + (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);
  int32_t second_angle = TRIG_MAX_ANGLE * draw_time.seconds / 60;

  // Compute hand endpoints
  // GPoint minute_hand = {
  //   .x = (int16_t)(sin_lookup(minute_angle) * (s_radius - HAND_MARGIN + 24) / TRIG_MAX_RATIO) + s_center.x,
  //   .y = (int16_t)(-cos_lookup(minute_angle) * (s_radius - HAND_MARGIN + 24) / TRIG_MAX_RATIO) + s_center.y
  // };
  // GPoint hour_hand = {
  //   .x = (int16_t)(sin_lookup(hour_angle) * (s_radius - 2 * HAND_MARGIN + 6) / TRIG_MAX_RATIO) + s_center.x,
  //   .y = (int16_t)(-cos_lookup(hour_angle) * (s_radius - 2 * HAND_MARGIN + 6) / TRIG_MAX_RATIO) + s_center.y
  // };
  // GPoint second_hand = {
  //   .x = (int16_t)(sin_lookup(second_angle) * (s_radius - HAND_MARGIN + 32) / TRIG_MAX_RATIO) + s_center.x,
  //   .y = (int16_t)(-cos_lookup(second_angle) * (s_radius - HAND_MARGIN + 32) / TRIG_MAX_RATIO) + s_center.y
  // };

  // Hour hand
GPoint hour_hand = {
  .x = (int16_t)(sin_lookup(hour_angle) * s_hour_length / TRIG_MAX_RATIO) + s_center.x,
  .y = (int16_t)(-cos_lookup(hour_angle) * s_hour_length / TRIG_MAX_RATIO) + s_center.y
};

// Minute hand
GPoint minute_hand = {
  .x = (int16_t)(sin_lookup(minute_angle) * s_minute_length / TRIG_MAX_RATIO) + s_center.x,
  .y = (int16_t)(-cos_lookup(minute_angle) * s_minute_length / TRIG_MAX_RATIO) + s_center.y
};

// Second hand
GPoint second_hand = {
  .x = (int16_t)(sin_lookup(second_angle) * s_seconds_length / TRIG_MAX_RATIO) + s_center.x,
  .y = (int16_t)(-cos_lookup(second_angle) * s_seconds_length / TRIG_MAX_RATIO) + s_center.y
};


  // Draw hands
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 8);
  if (s_radius > 2 * HAND_MARGIN) graphics_draw_line(ctx, s_center, hour_hand);

  graphics_context_set_stroke_width(ctx, 4);
  if (s_radius > HAND_MARGIN) graphics_draw_line(ctx, s_center, minute_hand);
  
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 2);

  if (s_radius > HAND_MARGIN) graphics_draw_line(ctx, s_center, second_hand);

  // Draw center circles
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, t_radius);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_center, u_radius);

  GPoint hour_circle = {
    .x = (int16_t)(sin_lookup(hour_angle) * (s_radius - 2 * HAND_MARGIN + 4) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (s_radius - 2 * HAND_MARGIN + 4) / TRIG_MAX_RATIO) + s_center.y
  };
  if (s_radius > 2 * HAND_MARGIN) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, hour_circle, c_radius);
  }
}

/************* WINDOW HANDLERS *************/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&bounds);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

/************* APP MESSAGE HANDLER *************/
static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tuple = dict_find(iter, KEY_COLOR);
  if (tuple) {
    ringColor = GColorFromHEX(tuple->value->int32);
    layer_mark_dirty(s_canvas_layer);
  }
}

/************* ANIMATION UPDATES *************/
static int anim_percentage(AnimationProgress dist_normalized, int max) {
  return (int)((float)dist_normalized / ANIMATION_NORMALIZED_MAX * max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized) {
  s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);
  layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
  s_anim_time.hours = anim_percentage(dist_normalized, s_last_time.hours);
  s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);
  s_anim_time.seconds = anim_percentage(dist_normalized, s_last_time.seconds);
  layer_mark_dirty(s_canvas_layer);
}

static void hour_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_hour_length = anim_percentage(dist_normalized, s_radius - 2 * HAND_MARGIN + 6);
  layer_mark_dirty(s_canvas_layer);
}

static void minute_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_minute_length = anim_percentage(dist_normalized, s_radius - HAND_MARGIN + 24);
  layer_mark_dirty(s_canvas_layer);
}

static void seconds_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_seconds_length = anim_percentage(dist_normalized, s_radius - HAND_MARGIN + 32);
  layer_mark_dirty(s_canvas_layer);
}

/************* INIT / DEINIT *************/
static void init() {
  srand(time(NULL));

  ringColor = GColorFromHEX(0x00FFAA);  // set default ring color
  active = persist_read_bool(KEY_INVERT);

  app_message_open(64, 0);
  app_message_register_inbox_received(in_received_handler);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

  // Set initial time
  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  tick_handler(time_now, MINUTE_UNIT);

  //tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  // Animate ring and hands
  // AnimationImplementation radius_impl = { .update = radius_update };
  // animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

  // AnimationImplementation hands_impl = { .update = hands_update };
  // animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);

  // Ring animation (already exists)
AnimationImplementation radius_impl = { .update = radius_update };
animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

// Hour hand length
AnimationImplementation hour_impl = { .update = hour_length_update };
animate(ANIMATION_DURATION, ANIMATION_DELAY, &hour_impl, false);

// Minute hand length
AnimationImplementation minute_impl = { .update = minute_length_update };
animate(ANIMATION_DURATION, ANIMATION_DELAY, &minute_impl, false);

// Second hand length
AnimationImplementation second_impl = { .update = seconds_length_update };
animate(ANIMATION_DURATION, ANIMATION_DELAY, &second_impl, false);

// Animate angles (hands rotation)
AnimationImplementation hands_impl = { .update = hands_update };
animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
  persist_write_bool(KEY_INVERT, active);
  app_message_deregister_callbacks();
}

int main() {
  init();
  app_event_loop();
  deinit();
}

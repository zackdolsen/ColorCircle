#include <pebble.h>

#define KEY_INVERT 0
#define KEY_COLOR 1
#define UNIT_TESTING 1

//variables to be added to clay
#define KEY_SECONDS 1
#define KEY_DATE 0

#define COLORS       PBL_IF_COLOR_ELSE(true, false)
#define ROUND       PBL_IF_ROUND_ELSE(true, false)
#define ANTIALIASING true

#define HAND_MARGIN  10


#define ANIMATION_DURATION 700
#define ANIMATION_DELAY    800

typedef struct {
  int hours;
  int minutes;
  int seconds;
  int day;
} Time;

/*************** GLOBALS ***************/
static Window *s_main_window;
static Layer *s_canvas_layer;

static GPoint s_center;
static Time s_last_time, s_anim_time;
static bool s_animating = false;
static bool active;
static GColor ringColor;
static float ui_scale;

//Scaling variables for layout
static int FINAL_RADIUS;
static int color_circle_radius = 0; //becomes final radius through animation
static int color_circle_thickness = 9; //thickness of the colored ring
static int center_outer_circle_radius = 7; //outer center circle radius
static int center_inner_circle_radius = 3; //inner center circle radius
static int hour_hand_width = 8; //width of hour hand
static int minute_hand_width = 4; //width of minute hand
static int second_hand_width = 1; //width of second hand
static int hour_hand_circle_radius = 2; //radius of circle at end of hour hand
static int seconds_hand_circle_radius = 3; //radius of circle at end of second hand

// Lengths of hands for animating. Start at 0 ad grow to final length
static int s_hour_length = 0;
static int s_minute_length = 0;
static int s_seconds_length = 0; 

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
  s_last_time.day = tick_time->tm_mday;
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
  if (UNIT_TESTING && ROUND) {
    graphics_context_set_fill_color(ctx, GColorDarkGray);
  }
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw colored ring
  graphics_context_set_stroke_color(ctx, ringColor);
  graphics_context_set_stroke_width(ctx, color_circle_thickness);
  graphics_context_set_antialiased(ctx, ANTIALIASING);
  graphics_draw_circle(ctx, s_center, color_circle_radius);
  
  // Determine current time (or animation)
  Time draw_time = s_animating ? s_anim_time : s_last_time;

  // Compute angles
  int32_t minute_angle = TRIG_MAX_ANGLE * draw_time.minutes / 60;
  int32_t hour_angle = TRIG_MAX_ANGLE * (draw_time.hours * 60 + draw_time.minutes) / (12 * 60);
  int32_t second_angle = TRIG_MAX_ANGLE * draw_time.seconds / 60;


  if(KEY_DATE) {
    // Prepare day string
    char day_str[4];
    snprintf(day_str, sizeof(day_str), "%d", s_last_time.day);

    // Position inside the circle, on the right, relative to current ring radius
    int16_t x_offset = (color_circle_radius + center_outer_circle_radius) / 2 ;  // halfway to the edge of the ring;
    GPoint day_pos = GPoint(s_center.x + x_offset, s_center.y);

    // Draw the day
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, day_str, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                   GRect(day_pos.x-15, day_pos.y-10, 30, 18),  // bounding box
                   GTextOverflowModeWordWrap,
                   GTextAlignmentCenter,
                   NULL);
  }

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

  // Draw hour and minute hands
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, hour_hand_width);
  if (color_circle_radius > 2 * HAND_MARGIN) graphics_draw_line(ctx, s_center, hour_hand);

  graphics_context_set_stroke_width(ctx, minute_hand_width);
  if (color_circle_radius > HAND_MARGIN) graphics_draw_line(ctx, s_center, minute_hand);
  
  

  // Draw Outer center circle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, center_outer_circle_radius);

  //draw second hand
  if(KEY_SECONDS) {
    if COLORS {
      graphics_context_set_stroke_color(ctx, GColorRed);
    } else {
      graphics_context_set_stroke_color(ctx, GColorWhite);
    }
    graphics_context_set_stroke_width(ctx, second_hand_width);
  

    if (color_circle_radius > HAND_MARGIN) graphics_draw_line(ctx, s_center, second_hand);

    GPoint second_circle = {
      .x = (int16_t)(sin_lookup(second_angle) * color_circle_radius / TRIG_MAX_RATIO) + s_center.x,
      .y = (int16_t)(-cos_lookup(second_angle) * color_circle_radius / TRIG_MAX_RATIO) + s_center.y
    };

    if (color_circle_radius > 2 * HAND_MARGIN) {
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_circle(ctx, second_circle, seconds_hand_circle_radius);
  }
  }

  //draw inner center circle
  if(KEY_SECONDS) {
    graphics_context_set_fill_color(ctx, GColorRed);
  } else {
    graphics_context_set_fill_color(ctx, GColorBlack);
  }
  graphics_fill_circle(ctx, s_center, center_inner_circle_radius);

  GPoint hour_circle = {
    .x = (int16_t)(sin_lookup(hour_angle) * (color_circle_radius - 2 * HAND_MARGIN + 4) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (color_circle_radius - 2 * HAND_MARGIN + 4) / TRIG_MAX_RATIO) + s_center.y
  };

  if (color_circle_radius > 2 * HAND_MARGIN) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, hour_circle, hour_hand_circle_radius);
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

/************* UI SCALING *************/
static void set_scale(){
  color_circle_thickness = 9 * ui_scale; //thickness of the colored ring
  APP_LOG(APP_LOG_LEVEL_INFO, "color_circle_thickness = %d", color_circle_thickness);

  center_outer_circle_radius = 7 * ui_scale; //outer center circle radius
  APP_LOG(APP_LOG_LEVEL_INFO, "center_outer_circle_radius = %d", center_outer_circle_radius);

  center_inner_circle_radius = 3 * ui_scale; //inner center circle radius
  APP_LOG(APP_LOG_LEVEL_INFO, "center_inner_circle_radius = %d", center_inner_circle_radius);

  hour_hand_width = 8 * ui_scale; //width of hour hand
  hour_hand_width = color_circle_thickness * 82 / 100 * ui_scale; //width of hour hand
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_hand_width = %d", hour_hand_width);

  minute_hand_width = 4.7 * ui_scale; //width of minute hand
  minute_hand_width = color_circle_thickness * 64 / 100 * ui_scale; //width of minute hand
  APP_LOG(APP_LOG_LEVEL_INFO, "minute_hand_width = %d", minute_hand_width);

  second_hand_width = 1.7 * ui_scale; //width of second hand
  // second_hand_width = color_circle_thickness * 14 / 100 * ui_scale; //width of second hand
  APP_LOG(APP_LOG_LEVEL_INFO, "second_hand_width = %d", second_hand_width);

  hour_hand_circle_radius = 2 * ui_scale; //radius of circle at end of hour hand
  APP_LOG(APP_LOG_LEVEL_INFO, "hour_hand_circle_radius = %d", hour_hand_circle_radius);

  seconds_hand_circle_radius = 3 * ui_scale; //radius of circle at end of second hand
  APP_LOG(APP_LOG_LEVEL_INFO, "seconds_hand_circle_radius = %d", seconds_hand_circle_radius);


}

/************* ANIMATION UPDATES *************/
static int anim_percentage(AnimationProgress dist_normalized, int max) {
  return (int)((float)dist_normalized / ANIMATION_NORMALIZED_MAX * max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized) {
  color_circle_radius = anim_percentage(dist_normalized, FINAL_RADIUS);
  color_circle_radius = anim_percentage(dist_normalized, FINAL_RADIUS);
  layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
  s_anim_time.hours = anim_percentage(dist_normalized, s_last_time.hours);
  s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);
  s_anim_time.seconds = anim_percentage(dist_normalized, s_last_time.seconds);
  layer_mark_dirty(s_canvas_layer);
}

static void hour_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_hour_length = anim_percentage(dist_normalized, color_circle_radius - 2 * HAND_MARGIN + 6);
  layer_mark_dirty(s_canvas_layer);
}

static void minute_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_minute_length = anim_percentage(dist_normalized, color_circle_radius - HAND_MARGIN + 24);
  layer_mark_dirty(s_canvas_layer);
}

static void seconds_length_update(Animation *anim, AnimationProgress dist_normalized) {
  s_seconds_length = anim_percentage(dist_normalized, color_circle_radius - HAND_MARGIN + 35);
  layer_mark_dirty(s_canvas_layer);
}

/************* INIT / DEINIT *************/
static void init() {
  srand(time(NULL));

  //set up FINAL_RADIUS based on watch type
  FINAL_RADIUS = PBL_DISPLAY_HEIGHT * 56 / 168; //original ratio for basalt
  APP_LOG(APP_LOG_LEVEL_INFO, "FINAL_RADIUS = %d", FINAL_RADIUS);

  //determine UI scale factor
  ui_scale = (float)PBL_DISPLAY_HEIGHT / (float)168; //168 is basalt height
  APP_LOG(APP_LOG_LEVEL_INFO, "UI Scale x1000 = %ld", (int32_t)(ui_scale * 1000));


  set_scale();

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

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  // Ring animation 
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

#include "pebble.h"
#include "main.h"

static ScreenType s_watch_type;
static bool initialized = false;

ClaySettings settings;

const BaseScaleValues BASE_SCALE = {
  .color_circle_thickness = 9,
  .center_outer_circle_radius = 7,
  .center_inner_circle_radius = 3,
  .hour_hand_width = 8,
  .minute_hand_width = 4.7f,
  .second_hand_width = 1.7f,
  .hour_hand_circle_radius = 2,
  .seconds_hand_circle_radius = 3,
  .hour_hand_length = 42,
  .minute_hand_length = 70,
  .seconds_hand_length = 81,
  .hour_dots_radius = 2.3f,
};

// Initialize the default settings
void clay_default_settings() {
  settings.KEY_RING_COLOR = GColorMediumSpringGreen;
  settings.KEY_SECONDS = true;
  settings.KEY_DATE = true;
  settings.KEY_BG_COLOR = GColorBlack;
  settings.KEY_SECOND_COLOR = GColorRed;
  settings.KEY_RANDOM_COLOR = 1;
  settings.KEY_HOUR_DOTS = false;
  settings.KEY_MIN_DOTS = false;
}

// Read settings from persistent storage
void clay_load_settings() {
  // Load the default settings
  clay_default_settings();
  // Read settings from persistent storage, if they exist
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

void clay_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  // Update the display based on new settings
}


void watch_type_init()
{
    if (initialized)
    {
        return;
    }

    if (PBL_DISPLAY_HEIGHT < 200)
    {
        s_watch_type = PBL_IF_RECT_ELSE(SCREEN_TYPE_OG_RECT, SCREEN_TYPE_OG_ROUND);
    }
    else
    {
        s_watch_type = PBL_IF_RECT_ELSE(SCREEN_TYPE_RECT_V2, SCREEN_TYPE_ROUND_V2);
    }
    switch (s_watch_type)
    {
    case SCREEN_TYPE_OG_RECT:
        APP_LOG(APP_LOG_LEVEL_INFO, "Pebble type determined to be SCREEN_TYPE_OG_RECT");
        break;
    case SCREEN_TYPE_OG_ROUND:
        APP_LOG(APP_LOG_LEVEL_INFO, "Pebble type determined to be SCREEN_TYPE_OG_ROUND");
        break;
    case SCREEN_TYPE_RECT_V2:
        APP_LOG(APP_LOG_LEVEL_INFO, "Pebble type determined to be SCREEN_TYPE_RECT_V2");
        break;
    case SCREEN_TYPE_ROUND_V2:
        APP_LOG(APP_LOG_LEVEL_INFO, "Pebble type determined to be SCREEN_TYPE_ROUND_V2");
        break;
    default:
        APP_LOG(APP_LOG_LEVEL_INFO, "Pebble type determined to be UNKNOWN");
        break;
    }
    return;
}


ScreenType get_watch_type()
{
    return s_watch_type;
}
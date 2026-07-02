# Color Circle

Color Circle is a Pebble watchface that draws a colorful circular time display with optional animated elements and configurable colors.

## Features

- native Pebble watchface built for Pebble SDK 3
- configurable ring color, background color, and second hand color
- optional random ring color changes every minute or every hour
- optional display of seconds, date, hour dots, and minute dots
- supports Aplite, Basalt, Chalk, Diorite, Emery, Flint, and Gabbro platforms
- uses Clay-based settings UI for in-app configuration

## Requirements

- Pebble SDK 3 with `pebble` command-line tools installed
- A Pebble-compatible watch or emulator
- Node.js and npm only required if working with the JavaScript config or package metadata

## Installation

1. Install the Pebble SDK if you have not already.
2. Clone or open this repository in your workspace.
3. Build the watchface with:

```bash
pebble build
```

4. Install to a connected watch or emulator with:

```bash
pebble install --phone <ip-address>
```

or use the Pebble app workflow for your development environment.

## Configuration

This watchface exposes the following settings in the Clay config screen:

- **Ring Color** (`KEY_RING_COLOR`): color of the outer time ring
- **Background Color** (`KEY_BG_COLOR`): watchface background
- **Seconds Hand Color** (`KEY_SECOND_COLOR`): color of the seconds hand
- **Random Ring Color** (`KEY_RANDOM_COLOR`): Off / Every Minute / Every Hour
- **Show Seconds** (`KEY_SECONDS`): toggle seconds hand visibility
- **Show Date** (`KEY_DATE`): toggle the date display inside the face
- **Show Hour Dots** (`KEY_HOUR_DOTS`): draw hour indicator dots around the ring
- **Show Minute Dots** (`KEY_MIN_DOTS`): draw minute indicator dots around the ring

When random ring color is enabled, the app chooses a new ring color that is not black, white, or the configured background/second hand color.

## Project Structure

- `package.json` — project metadata and Pebble settings
- `wscript` — Pebble SDK build script and platform targets
- `src/c/main.c` — main watchface application code
- `src/c/main.h` — shared watchface definitions
- `src/c/UIhelper.c` — UI layout helper functions and default sizing
- `src/pkjs/config.js` — Clay configuration screen definition
- `resources/images/` — watchface image assets
- `build/` — generated build artifacts and platform output

## Development Notes

- The app uses `clay_save_settings()` to persist settings after random ring color updates.
- The watchface redraws when time ticks, Bluetooth connection changes, or battery state updates.
- The `src/pkjs/config.js` file defines the configuration UI and message keys used by the native watchface.

## License

This repository does not include a license file. If you want to reuse or distribute the watchface, please contact the original author or add a license to the repository.

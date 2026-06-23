'use strict';

/* eslint-disable quotes */

module.exports = [
  { 
    "type": "heading", 
    "defaultValue": "Color Circle" 
  }, 
  { 
    "type": "text", 
    "defaultValue": "These are the current settings" 
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Watchface Colors"
      },
      {
        "type": "color",
        "messageKey": "KEY_RING_COLOR",      // Matches your C code
        "defaultValue": "0x00FFAA",        // Pebble color name
        "label": "Ring Color",
        "sunlight": false,
        "capabilities": ["COLOR"]

      },
      {
        "type": "color",
        "messageKey": "KEY_BG_COLOR",      // Matches your C code
        "defaultValue": "0x000000",        // Pebble color name
        "label": "Background Color",
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "KEY_SECOND_COLOR",      // Matches your C code
        "defaultValue": "0xFF0000",        // Pebble color name
        "label": "Seconds Hand Color",
        "sunlight": false,
        "capabilities": ["COLOR"]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Watchface Settings"
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SECONDS",      // Matches your C code
        "defaultValue": true,        // Pebble color name
        "label": "Show seconds",
        "description": "Turning on may lead to decreased battery life"
      },
      {
        "type": "toggle",
        "messageKey": "KEY_DATE",      // Matches your C code
        "defaultValue": true,        // Pebble color name
        "label": "Show date"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
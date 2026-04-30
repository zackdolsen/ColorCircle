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
        "messageKey": "KEY_COLOR",      // Matches your C code
        "defaultValue": "0x00FFAA",        // Pebble color name
        "label": "Circle Color",
        "sunlight": false
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




// module.exports = [
//   {
//     "type": "section",
//     "items": [
//       {
//         "type": "color",
//         "messageKey": "KEY_COLOR",
//         "defaultValue": "0xFF0000",
//         "label": "Circle Color"
//       },
//       {
//         "type": "toggle",
//         "messageKey": "KEY_INVERT",
//         "defaultValue": false,
//         "label": "Invert Colors"
//       }
//     ]
//   }
// ];
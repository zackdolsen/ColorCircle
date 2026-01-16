'use strict';

/* eslint-disable quotes */

module.exports = [
  { 
    "type": "heading", 
    "defaultValue": "Settings" 
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
        "defaultValue": "Folly",        // Pebble color name
        "label": "Circle Color",
        "sunlight": true
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
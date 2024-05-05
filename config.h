#ifndef CONFIG_H
#define CONFIG_H

const unsigned long BOT_MTBS = 10000; // mean time between scan messages

// Top left of the map for mapping to LEDs
#define START_LAT 57.940524
#define START_LON -5.972884
// Bottom right of the map for mapping to LEDs
#define END_LAT 49.773646
#define END_LON 1.483017


// Out of range location (Azkaban)
#define OUT_OF_RANGE_LAT 57.574840
#define OUT_OF_RANGE_LON 0.742179

// LED Count
#define LED_ROWS 96
#define LED_COLS 48

#endif
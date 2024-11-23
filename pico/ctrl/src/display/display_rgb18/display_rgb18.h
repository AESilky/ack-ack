/**
 * Copyright 2023-24 AESilky
 *
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _DISPLAY_ILI_H_
#define _DISPLAY_ILI_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include "../display.h"

/**
 * @brief RGB 18-bit Color Structure (6R,6G,6B)
 * @ingroup display
 *
 * ILI Controller chips use a structure with the six bits of each color
 * in the upper 6 bits of each of the R, G, and B bytes. Therefore,
 * the lower 2 bits of each byte are 0. As such, the values will be like
 * 0, 4, 8, C, 10, 14, 18, 1C, 24, 20, 28, 2C, 30, 34, 38, 3C, etc.
 */
     typedef struct RGB18_BYTES_ {
         uint8_t r;
         uint8_t g;
         uint8_t b;
     } rgb18_t;

     /** @brief BLACK RGB-18 Constant */
     extern const rgb18_t RGB18_BLACK;
     /** @brief BLUE RGB-18 Constant */
     extern const rgb18_t RGB18_BLUE;
     /** @brief GREEN RGB-18 Constant */
     extern const rgb18_t RGB18_GREEN;
     /** @brief CYAN RGB-18 Constant */
     extern const rgb18_t RGB18_CYAN;
     /** @brief RED RGB-18 Constant */
     extern const rgb18_t RGB18_RED;
     /** @brief MAGENTA RGB-18 Constant */
     extern const rgb18_t RGB18_MAGENTA;
     /** @brief BROWN RGB-18 Constant */
     extern const rgb18_t RGB18_BROWN;
     /** @brief WHITE RGB-18 Constant */
     extern const rgb18_t RGB18_WHITE;
     /** @brief GREY RGB-18 Constant */
     extern const rgb18_t RGB18_GREY;
     /** @brief LIGHT BLUE RGB-18 Constant */
     extern const rgb18_t RGB18_LT_BLUE;
     /** @brief LIGHT GREEN RGB-18 Constant */
     extern const rgb18_t RGB18_LT_GREEN;
     /** @brief LIGHT CYAN RGB-18 Constant */
     extern const rgb18_t RGB18_LT_CYAN;
     /** @brief ORANGE RGB-18 Constant */
     extern const rgb18_t RGB18_ORANGE;
     /** @brief LIGHT MAGENTA RGB-18 Constant */
     extern const rgb18_t RGB18_LT_MAGENTA;
     /** @brief YELLOW RGB-18 Constant */
     extern const rgb18_t RGB18_YELLOW;
     /** @brief WHITE (BRIGHT) RGB-18 Constant */
     extern const rgb18_t RGB18_BR_WHITE;

     /** @brief Blank element (R, G, or B) for an RGB-18 value */
#define RGB18_ELM_BLANK (0x00)

/**
 * @brief Create rgb18 data value from individual Red, Green, and Blue values.
 * @ingroup display
 *
 * The rgb18 values are 6 bits each, so valid element values are 0-63. Element
 * values greater than 64 may result in unexpected colors.
 *
 * @param r Red element value (0-63)
 * @param g Green element value (0-63)
 * @param b Blue element value (0-63)
 * @return rgb18_t Generated RGB-18 structure
 */
static inline rgb18_t rgb_to_rgb18(uint8_t r, uint8_t g, uint8_t b) {
    rgb18_t rgb18 = { r << 2,g << 2,b << 2 };
    return rgb18;
}

/**
 * @brief Get the Red element of an RGB-18 adjusted to the 0-63 range.
 * @ingroup display
 *
 * RGB-18 values store the elements shifted 2 bits up. This returns the
 * element value adjusted down to the range 0-64.
 *
 * @param rgb RGB-18 value
 * @return uint8_t The adjusted element value
 */
static inline uint8_t red_from_rgb18(rgb18_t rgb) {
    return (rgb.r >> 2);
}

/**
 * @brief Get the Green element of an RGB-18 adjusted to the 0-63 range.
 * @ingroup display
 *
 * RGB-18 values store the elements shifted 2 bits up. This returns the
 * element value adjusted down to the range 0-64.
 *
 * @param rgb RGB-18 value
 * @return uint8_t The adjusted element value
 */
static inline uint8_t green_from_rgb18(rgb18_t rgb) {
    return (rgb.g >> 2);
}

/**
 * @brief Get the Blue element of an RGB-18 adjusted to the 0-63 range.
 * @ingroup display
 *
 * RGB-18 values store the elements shifted 2 bits up. This returns the
 * element value adjusted down to the range 0-64.
 *
 * @param rgb RGB-18 value
 * @return uint8_t The adjusted element value
 */
static inline uint8_t blue_from_rgb18(rgb18_t rgb) {
    return (rgb.b >> 2);
}

// /** @brief Red-5-bits Green-6-bits Blue-5-bits (16 bit unsigned) */
// typedef uint16_t rgb16_t; // R5G6B5
/**
 * @brief Get a RGB-18 (R6G6B6) value from a Color-16 (0-15 Color number)
 * @ingroup display
 *
 * @param cn16 Color-16 number to get a RGB-16 (R5G6B5) value for
 */
extern rgb18_t rgb18_from_color16(colorn16_t cn16);



/**
 * @brief Get a pointer to a buffer large enough to hold
 * one scan line for the ILI display.
 * @ingroup display
 *
 * This can be used to put RGB data into to be written to the
 * screen. The `gfxd_line_paint` can be called to put the
 * line on the screen.
 *
 * The buffer holds `ILI_WIDTH` rgb18_t values.
 */
extern rgb18_t* gfxd_get_line_buf();

/**
 * @brief Paint a buffer of rgb18_t values to one horizontal line
 * of the screen. The buffer passed in must be at least `ILI_WIDTH`
 * rgb18_t values in size.
 * @ingroup display
 *
 * @param line 0-based line number to paint (must be less than `ILI_HEIGHT`).
 * @param buf pointer to a rgb18_t buffer of data (must be at least 'ILI_WIDTH`)
 */
extern void gfxd_line_paint(uint16_t line, rgb18_t* buf);

/**
 * @brief Clear the entire screen using a single color.
 * @ingroup display
 *
 * @param color The color to fill the screen with.
 * @param force True to force a write to the screen. Otherwise, the screen is written
 *              to only if the screen is thought to be 'dirty'.
 */
extern void gfxd_screen_clr(rgb18_t color, bool force);

/**
 * @brief Clear the entire screen using a single color.
 * @ingroup display
 *
 * @param color The Color-16 (color number) to fill the screen with.
 * @param force True to force a write to the screen. Otherwise, the screen is written
 *              to only if the screen is thought to be 'dirty'.
 */
extern void gfxd_screen_clr_c16(colorn16_t color, bool force);

/**
 * @brief The height of the display screen (pixel lines).
 * @ingroup display
 *
 * @return uint16_t Pixel lines
 */
extern uint16_t gfxd_screen_height();

/**
 * @brief Turn the screen (display) on/off.
 * @ingroup display
 *
 * @param on True to turn on, false to turn off
 */
extern void gfxd_screen_on(bool on);

/**
 * @brief Paint the screen with the RGB-18 contents of a buffer.
 * @ingroup display
 *
 * Uses the buffer of RGB data to paint the screen into the screen window.
 * Set the screen window using `gfxd_window_set_area`.
 *
 * @param data RGB-18 pixel data buffer (1 rgb value for each pixel to paint)
 * @param pixels Number of pixels (size of the data buffer in rgb_t's)
 */
extern void gfxd_screen_paint(const rgb18_t* rgb_pixel_data, uint16_t pixels);

/**
 * @brief The width of the display screen (pixel columns).
 * @ingroup display
 *
 * @return uint16_t Pixel columns
 */
extern uint16_t gfxd_screen_width();

/**
 * @brief Exit scroll mode to normal mode.
 * @ingroup display
 *
 * This puts the screen back into normal display mode (no scroll area) and
 * sets the window to full screen.
 */
extern void gfxd_scroll_exit(void);

/**
 * @brief Set the scroll area. It is between the top fixed area and the
 * bottom fixed area.
 * @ingroup display
 *
 * NOTE: Hardware scrolling only works when the display is in portrait mode
 * (MADCTL MV (bit 5) = 0)
 *
 * @see gfxd_scroll_set_start
 *
 * @param top_fixed_lines Number of fixed lines at the top of the screen
 * @param bottom_fixed_lines Number of fixed lines at the bottom of the screen
 */
extern void gfxd_scroll_set_area(uint16_t top_fixed_lines, uint16_t bottom_fixed_lines);

/**
 * @brief Set the frame memory start line for the scroll area.
 * @ingroup display
 *
 * ILI9341:
 * The ILI9341 datasheet doesn't do a great job of describing the scroll functionality
 * and this value, as there is only an example for when the top and bottom fixed areas
 * are 0. For that case, this value needs to range from 0 to 319 (the display height
 * in portrait mode (MADCTL-MV (bit 5) = 0) a requirement of the ILI9341)),
 * and it defines the frame memory line that is displayed at the top of the
 * display (line 0 of the display).
 *
 * ILI9488:
 * ZZZ
 *
 * When the top, and/or bottom fixed areas are non-zero, the scroll start needs to be
 * greater than the top fixed line and less than the bottom fixed line. If it is set
 * within either the top or bottom fixed areas it is the same as being set at the
 * minimum or maximum limit.
 *
 * @see gfxd_scroll_set_area
 *
 * @param line The line within the frame memory to display at the top of the scroll area.
 */
extern void gfxd_scroll_set_start(uint16_t line);

/**
 * @brief Set the screen update window and position the start at x,y.
 * @ingroup display
 *
 * Sets the update window area on the screen. This is the area that RGB data
 * will be updated into using `gfxd_screen_paint`.
 */
extern void gfxd_window_set_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * @brief Set the screen update window to the full screen, and position the
 * start at 0,0.
 * @ingroup display
 *
 * Sets the update window to the full screen and positions the start at 0,0.
 */
extern void gfxd_window_set_fullscreen(void);


/**
 * @brief Fill an RGB18 buffer with an RGB18 value.
 * @ingroup display
 *
 * @param rgb_buf The buffer to fill
 * @param rgb The value to fill with
 * @param size The size of the buffer (in rgb18_t's)
 */
extern void rgb18_buf_fill(rgb18_t* rgb_buf, rgb18_t rgb, size_t size);

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_ILI_H_


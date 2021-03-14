#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "driver.pio.h"

/*
 * A frame is a group of 14 LEDs in the shape of a letter K.
 *
 * A frame is laid out in memory as follows:
 *
 *
 *  0          10
 *  1        9
 *  2     8
 *  3  7
 *  4    11
 *  5       12
 *  6          13
 *
 * This linear layout is stored in member `p` of `struct frame_t`.
 *
 */

#define FRAME_LEN 14
typedef struct frame_t { uint32_t p[FRAME_LEN]; } frame_t;

/*
 * We reshape a logical frame on write to match the hardware (the breadboard setup
 * uses a ring of 24 LEDs, of which we're using 14, instead of trying to breadboard a whole
 * bunch of LEDs in the right order and pattern. )
 *
 *
 * frame_map stores the mapping of frame_t.p indexes to pixel positions on the LED chain.
 */

// TODO: ifdefs for breadboard vs PCB
#define PIXEL_COUNT  24
static int frame_map[] = {
    21,
    22,
    23,
    0,
    1,
    2,
    3,
    12,
    8,
    9,
    10,
    16,
    15,
    14
};

// PCB layout has LED string connected to pin GP0
// Breadboard config uses this for UART

// TODO: ifdefs for breadboard vs PCB
#define LED_PIN 2


// gamma correction table
// taken from https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix

const uint8_t pixel_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
    115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
    144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
    177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
    215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

// convenience function to zero out a frame
static void zero_frame(frame_t *f) {
    memset(f, 0, sizeof(frame_t));
}

// pixels are written out in GBR order, so we use this pixel order everywhere internally.

// convenience macro to convert R, G, B to packed GBR
#define URGB_U32(r, g, b)  \
    ((((uint32_t) (r) << 8 ) | \
    ((uint32_t) (g) << 16 ) | \
    (uint32_t) (b)))

// write out a single pixel to the PIO
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// set the whole chain of pixels to a single color
static void put_all_pixels(uint32_t pixel_grb) {
    sleep_ms(10);
    for(int i = 0; i < PIXEL_COUNT ; i++) {
        put_pixel(pixel_grb);
    }
    sleep_ms(10);
}

// write out a frame, reshaping, gamma-correcting and (TODO) dithering as necessary.
static void put_frame(frame_t *f) {
    static uint32_t pixels[PIXEL_COUNT];

    for(int i = 0; i < FRAME_LEN ; i++) {
        pixels[frame_map[i]] = f->p[i];
    }

    for(int i = 0; i < PIXEL_COUNT ; i++) {
        uint8_t pp[4];

        pp[0] = pixel_gamma[(uint8_t) pixels[i]];
        pp[1] = pixel_gamma[(uint8_t) (pixels[i] >> 8)];
        pp[2] = pixel_gamma[(uint8_t) (pixels[i] >> 16)];
        pp[3] = pixel_gamma[(uint8_t) (pixels[i] >> 24)];

        uint32_t po;
        po = pp[0] | (pp[1] << 8) | (pp[2] << 16) | (pp[3] << 24);

        put_pixel(po);
    }

    printf("ending frame\n");
    sleep_ms(1);
}

// a quick test boot animation
static void boot_animation() {

    frame_t fr;
    int anim_len = 255;

    for(int i = 0; i < anim_len; i ++) {
        memset(&fr, URGB_U32(i, i, i), sizeof(frame_t));
        put_frame(&fr);

        sleep_ms(5);
    }
}

int main() {
    stdio_init_all();

    PIO pio = pio0;

    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &driver_program);

    driver_program_init(pio, sm, offset, LED_PIN, 800000, false);

    printf("booooot!\n");
    boot_animation();

    put_all_pixels(URGB_U32(0, 0, 0));

    // need an idle loop, so our USB connection stays alive for stdio
    while(1) {
        sleep_ms(500);
    }
}
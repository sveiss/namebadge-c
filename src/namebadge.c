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

// write out a frame, reshaping and (TODO) gamma-correcting/dithering as necessary.
static void put_frame(frame_t *f) {
    static uint32_t pixels[PIXEL_COUNT];

    for(int i = 0; i < FRAME_LEN ; i++) {
        pixels[frame_map[i]] = f->p[i];
    }

    for(int i = 0; i < PIXEL_COUNT ; i++) {
        put_pixel(pixels[i]);
    }

    printf("ending frame\n");
    sleep_ms(10);
}

// a quick test boot animation
static void boot_animation() {

    frame_t fr;
    int anim_len = 25;

    for(int i = 0; i < anim_len; i ++) {
        memset(&fr, URGB_U32(i*10, i*10, i*10), sizeof(frame_t));
        put_frame(&fr);

        sleep_ms(100);
        printf("frame: %d\n", i);
    }
}

int main() {
    stdio_init_all();

    PIO pio = pio0;

    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &driver_program);

    driver_program_init(pio, sm, offset, 0, 800000, false);

    boot_animation();

    // need an idle loop, so our USB connection stays alive for stdio
    while(1) {
        sleep_ms(500);
    }
}
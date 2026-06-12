/* bug-4008.c
   A bug in z80 code generation for additions.
 */

#include <testfwk.h>

// Based on code by "Under4Mhz" licensed under GPL 2.0 or later
#pragma disable_warning 85

#include <stdint.h>

void vdu_tiles_put( uint16_t tile, const uint8_t *pixels, const uint8_t *colours ) {

    static uint16_t i;
    ASSERT( tile == 15 + i++ * 256 );
}

///< Load pixel data for patch
void vdu_patch_load_impl( int8_t row, const uint8_t *pixels, const uint8_t *colours, uint8_t rows, uint16_t tile_offset ) {

    if ( row >= 24 )
        return;

    uint16_t segment_first = 0;
    uint16_t segments = 3;
    uint16_t tile = tile_offset;

    for(uint8_t segment = 0; segment < segments; segment++ ) {

        vdu_tiles_put( tile, pixels, colours );
        tile += 256;
    }
}

void
testBug(void) {

    vdu_patch_load_impl( 3, 0, 0, 0, 15 );
}


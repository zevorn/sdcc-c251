/* bug-4007.c
   TODO: Bug description
 */

#include <testfwk.h>

// Based on code by "Under4Mhz" licensed under GPL 2.0 or later
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define COLS 2
#define ROWS 4

typedef struct {

    uint8_t sprite;
    uint8_t colour : 4;

} Item;

typedef struct {

    int x;
    int y;

} maths_point;

Item blocks[ROWS][COLS];

Item *TableGetPoint( const maths_point *position ) { return &blocks[position->y][position->x]; }

uint8_t Count( const maths_point *position, const maths_point *direction ) {

    uint8_t count = 1;

    maths_point pos = *position;

    Item *block = TableGetPoint( &pos );
    pos.y += direction->y;

    while ( pos.y < ROWS ) {

        Item *next = TableGetPoint( &pos );

        bool matches = block->colour == next->colour;
        if ( !matches ) break;
        count++;

        pos.y += direction->y;
    }

    return count;
}

void
testBug(void) {

    maths_point position = { 1, 1 };
    maths_point direction = { 0, 1 };

    memset( &blocks, 0, sizeof(blocks));

    blocks[position.y][position.x].colour = 2;
    blocks[position.y + 1][position.x].colour = 2;

    uint8_t count = Count( &position, &direction );

    ASSERT( count == 2 );
}


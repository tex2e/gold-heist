// Gold Heist

#ifdef __APPLE__
#include <GLUT/glut.h>
#include "mac/include/GL/glpng.h"
#else
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glpng.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// -----------------------------------------------------------------------------
// --- Macros ---
// -----------------------------------------------------------------------------

#define SIZE 20             // map size
#define SPLITE_SIZE 30      // splite size (width/height)
#define MOVE_SPEED 10
#define START_MOVING 3

#define DOWN 0
#define LEFT 1
#define UP 2
#define RIGHT 3

#define MAP_IMG 16          // max map splite number
#define CHARACTOR_IMG 12    // max character splite number
#define ENEMY 7             // max enemy number

#define DECORATION_IMG 4    // max decoration image number


// -----------------------------------------------------------------------------
// --- Global Variables ---
// -----------------------------------------------------------------------------

int i, j;
int width = SIZE * SPLITE_SIZE;
int height = SIZE * SPLITE_SIZE;

// --- Game Flags ---

int is_moving_phase = 0; // 0..START_MOVING
int has_got_gold = 0;    // 0,1
int detected_player = 0; // 0,1
int did_gameover = 0;    // 0,1
int did_gamefinish = 0;  // 0,1

// --- Point ---

typedef struct Point {
    int x;
    int y;
} Point;

// --- Map ---

// 0.   : ground
// 1.  @: lighted ground
// 2.  #: wall
// 3.  C: lighted wall
// 4.  D: door
// 5.  d: lighted door
// 6.  K: key
// 7.  k: lighted key
// 8.  X: keyed door
// 9.  x: lighted keyed door
// 10. O: opened door
// 11. o: lighted opened door
// 12. S: start
// 13. s: lighted start
// 14. G: gold
// 15. g: lighted gold

char map[SIZE][SIZE];
char map1[SIZE][SIZE] = {
    "####################",
    "####################",
    "##       ####     ##",
    "##   #   D  X     ##",
    "##       ####   G ##",
    "#####D#######     ##",
    "###         ########",
    "### #######X########",
    "##          #     ##",
    "##          D   K ##",
    "##    ##    #     ##",
    "##          ########",
    "##          D     ##",
    "#####X########### ##",
    "##     D    ##    ##",
    "##    ##    ##    ##",
    "## S  ##    ##    ##",
    "##    ##          ##",
    "####################",
    "####################"
};
GLuint map_img[MAP_IMG];
pngInfo map_info[MAP_IMG];
Point start_pos = {4, 16};

// --- Enemy sight ---

char lighted_map[SIZE][SIZE] = {{0}};
char lighted_map_clear[SIZE][SIZE] = {{0}}; // use to clear array

// --- Lightness ---

char lightness_map[SIZE][SIZE] = {{0}};
char lightness_map_clear[SIZE][SIZE] = {{0}}; // use to clear array

// --- Player ---

//
//        {6,7,8}
//           ^
// {3,4,5} < p > {9,10,11}
//           V
//        {0,1,2}
//
// img_file := "img/character{{player.direction * 3 + player.state_img}}.png"

typedef struct Character {
    int type; // 0 == player, 1 >= enemy
    Point pos;
    int direction;   // 0..3
    int next_motion; // 0,1
    int state_img;   // 0..2
    GLuint img[CHARACTOR_IMG];
    pngInfo img_info[CHARACTOR_IMG];
} Character;

Character player = {0, {0, 0}, 0, 0, 0};

// --- Enemies ---

Character enemies[ENEMY];

// --- Other Decorations ---

GLuint decoration_img[DECORATION_IMG];
pngInfo decoration_info[DECORATION_IMG];


// -----------------------------------------------------------------------------
// --- Functions ---
// -----------------------------------------------------------------------------

void init_charactor(Character*, int, int, int);
void init();

void display();
void reshape(int, int);
void timer50(int);

void map_each(int (*f)(int, int, int, int));
int draw_elem(int, int, int, int);
void draw_map();
void draw_player(Character *);
void draw_enemy(Character *);
void put_sprite(int, int, int, pngInfo *);

int is_wall(int, int);
int is_wall_for_enemy(int, int);
int is_movable(Character*, int (*f)(int, int));
Point move(Character*, int, int (*f)(int, int));
int is_door(int, int);
void open_door(int, int);
int has_key(int, int);
void unlock_keyed_door(int, int);
int _unlock_keyed_door(int, int, int, int);
int is_gold(int, int);
void take_gold(int, int);
int is_finish(int, int);
void finish();

void light_on(int, int);
void light_off(int, int);
void update_lighted_map();
int light_on_off(int, int, int, int);
void apply_lighted_map();
void update_lightness_map();
int lightness_on_off(int i_x, int i_y, int x, int y);
void apply_lightness_map();
int is_gameover(int, int);
void gameover();

void Keyboard(unsigned char, int, int);
void SpecialKey(int, int, int);

// Game flow:
//
//       init() <----+
//         |         |
//         V         |
//      display()    |
//         |         |restart
//         V         |
//      waiting_keydown <------+
//      * Keyboard   |         |
//      * SpecialKey |         |
//                   |         |
//                   |move     |after_that
//                   V         |
//                animation ---+
//                * timer50
//                * display
//

int main(int argc, char **argv) {
    int i_enemy;
    char filepath[100];

    glutInit(&argc, argv);
    glutInitWindowSize(width, height);
    glutCreateWindow("Gold Heist");
    glutInitDisplayMode(GLUT_RGBA/* | GLUT_DOUBLE*/);
    glClearColor(1.0, 1.0, 1.0, 1.0);

    // Settings of enable to texture's alpha channel.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Load png images.
    map_img[0]  = pngBind("img/ground1.png",      PNG_NOMIPMAP, PNG_ALPHA, &map_info[0],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[1]  = pngBind("img/ground2.png",      PNG_NOMIPMAP, PNG_ALPHA, &map_info[1],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[2]  = pngBind("img/wall1.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[2],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[3]  = pngBind("img/wall2.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[3],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[4]  = pngBind("img/door1.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[4],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[5]  = pngBind("img/door2.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[5],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[6]  = pngBind("img/key1.png",         PNG_NOMIPMAP, PNG_ALPHA, &map_info[6],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[7]  = pngBind("img/key2.png",         PNG_NOMIPMAP, PNG_ALPHA, &map_info[7],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[8]  = pngBind("img/keyed-door1.png",  PNG_NOMIPMAP, PNG_ALPHA, &map_info[8],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[9]  = pngBind("img/keyed-door2.png",  PNG_NOMIPMAP, PNG_ALPHA, &map_info[9],  GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[10] = pngBind("img/opened-door1.png", PNG_NOMIPMAP, PNG_ALPHA, &map_info[10], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[11] = pngBind("img/opened-door2.png", PNG_NOMIPMAP, PNG_ALPHA, &map_info[11], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[12] = pngBind("img/start1.png",       PNG_NOMIPMAP, PNG_ALPHA, &map_info[12], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[13] = pngBind("img/start2.png",       PNG_NOMIPMAP, PNG_ALPHA, &map_info[13], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[14] = pngBind("img/gold1.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[14], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    map_img[15] = pngBind("img/gold2.png",        PNG_NOMIPMAP, PNG_ALPHA, &map_info[15], GL_CLAMP, GL_NEAREST, GL_NEAREST);

    decoration_img[0] = pngBind("img/gameover.png",   PNG_NOMIPMAP, PNG_ALPHA, &decoration_info[0], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    decoration_img[1] = pngBind("img/gameclear.png",  PNG_NOMIPMAP, PNG_ALPHA, &decoration_info[1], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    decoration_img[2] = pngBind("img/dark.png",       PNG_NOMIPMAP, PNG_ALPHA, &decoration_info[2], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    decoration_img[3] = pngBind("img/dark-enemy.png", PNG_NOMIPMAP, PNG_ALPHA, &decoration_info[3], GL_CLAMP, GL_NEAREST, GL_NEAREST);

    // player
    for (i = 0; i < CHARACTOR_IMG; i++) {
        sprintf(filepath, "img/character1/character%d.png", i);
        player.img[i] = pngBind(filepath, PNG_NOMIPMAP, PNG_ALPHA, &player.img_info[i], GL_CLAMP, GL_NEAREST, GL_NEAREST);
    }

    // enemies
    for (i_enemy = 0; i_enemy < ENEMY; i_enemy++) {
        for (i = 0; i < 4; i++) {
            sprintf(filepath, "img/enemy1/enemy%d.png", i);
            enemies[i_enemy].img[i] = pngBind(filepath, PNG_NOMIPMAP, PNG_ALPHA, &enemies[i_enemy].img_info[i], GL_CLAMP, GL_NEAREST, GL_NEAREST);
        }
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(50, timer50, 0);
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKey);

    init();
    glutMainLoop();

    return 0;
}

//
// Init game state such as flags, map data, player and enemies positions.
//
void init() {
    srand(time(NULL));

    // flags
    is_moving_phase = 0;
    has_got_gold = 0;
    detected_player = 0;
    did_gameover = 0;
    did_gamefinish = 0;

    // map
    memcpy(map, map1, sizeof(char) * SIZE * SIZE);
    memcpy(lightness_map, lightness_map_clear, sizeof(char) * SIZE * SIZE);

    // player
    init_charactor(&player, start_pos.x, start_pos.y, 0);
    player.state_img = 0;
    player.next_motion = 1;

    // enemy
    init_charactor(&enemies[0], 15,  3, 2);
    init_charactor(&enemies[1],  4,  2, 1);
    init_charactor(&enemies[2], 15,  8, 0);
    init_charactor(&enemies[3],  9, 10, 3);
    init_charactor(&enemies[4],  9, 12, 1);
    init_charactor(&enemies[5], 17, 16, 2);
    init_charactor(&enemies[6], 11, 15, 0);
}

//
// Init given character with given x, y position and direction.
//
void init_charactor(Character *character, int i_x, int i_y, int direction) {
    character->type = 1;
    character->pos.x = SPLITE_SIZE * i_x;
    character->pos.y = SPLITE_SIZE * i_y;
    character->direction = direction;
    character->next_motion = 1;
}

//
// Callback function called for to draw window.
//
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Change game map if event has occured.

    if (is_door(player.pos.x, player.pos.y)) {
        open_door(player.pos.x, player.pos.y);
    }

    if (has_key(player.pos.x, player.pos.y)) {
        unlock_keyed_door(player.pos.x, player.pos.y);
    }

    if (is_gold(player.pos.x, player.pos.y)) {
        take_gold(player.pos.x, player.pos.y);
    }

    // Update enemies' sight.
    update_lighted_map();
    apply_lighted_map();

    // Draw map.
    draw_map();

    // Draw player and enemies.
    for (i = 0; i < ENEMY; i++) draw_enemy(&enemies[i]);
    draw_player(&player);

    // Update player's sight.
    update_lightness_map(player.pos.x, player.pos.y);
    apply_lightness_map();

    // Check if gameover or gameclear.
    if (is_moving_phase == 0) {
        if (is_finish(player.pos.x, player.pos.y)) {
            finish();
        }

        if (is_gameover(player.pos.x, player.pos.y)) {
            gameover();
        }
    }

    glFlush();
    //glutSwapBuffers();
}

//
// Callback function called for when changed window size.
// It mainly transforms coordinate system using passed parameters w and h which
// indicates window width and height.
//
void reshape(int w, int h) {
    width  = w;
    height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glScaled(1, -1, 1);
    glTranslated(0, -h, 0);
}

//
// Callback function called for per 50ms.
// Its main role is to manage animation of both player and enemies.
//
void timer50(int value) {
    int i;
    if (is_moving_phase > 0) {
        is_moving_phase--;

        // Start moving!

        // player
        if (player.next_motion) {
            player.state_img = (player.state_img + 1) % 3;

            // move
            Point next = move(&player, MOVE_SPEED, is_wall);
            player.pos = next;
        }

        // enemies
        for (i = 0; i < ENEMY; i++) {
            // set correct direction to move
            if (is_moving_phase == START_MOVING - 1 && !is_movable(&enemies[i], is_wall_for_enemy)) {
                int new_direction;
                int can_move_up    = !is_wall_for_enemy(enemies[i].pos.x, enemies[i].pos.y - SPLITE_SIZE);
                int can_move_down  = !is_wall_for_enemy(enemies[i].pos.x, enemies[i].pos.y + SPLITE_SIZE);
                int can_move_left  = !is_wall_for_enemy(enemies[i].pos.x - SPLITE_SIZE, enemies[i].pos.y);
                int can_move_right = !is_wall_for_enemy(enemies[i].pos.x + SPLITE_SIZE, enemies[i].pos.y);
                int can_move[4] = {
                    can_move_down, can_move_left, can_move_up, can_move_right
                };
                int can_move_left_from_enemy  = can_move[(enemies[i].direction - 1 + 4) % 4];
                int can_move_right_from_enemy = can_move[(enemies[i].direction + 1) % 4];

                if (!can_move_down && !can_move_left && !can_move_up && !can_move_right) continue;

                if (can_move_left_from_enemy && can_move_right_from_enemy) {
                    if (rand() % 2 == 0) {
                        new_direction = (enemies[i].direction - 1 + 4) % 4;
                    } else {
                        new_direction = (enemies[i].direction + 1) % 4;
                    }
                } else if (can_move_left_from_enemy) {
                    new_direction = (enemies[i].direction - 1 + 4) % 4;
                } else if (can_move_right_from_enemy) {
                    new_direction = (enemies[i].direction + 1) % 4;
                } else {
                    new_direction = (enemies[i].direction + 2) % 4;
                }

                enemies[i].direction = new_direction;
            }

            // move
            Point next = move(&enemies[i], MOVE_SPEED, is_wall_for_enemy);
            enemies[i].pos = next;
        }

        glutPostRedisplay();
    }
    glutTimerFunc(50, timer50, 0);
}

//
// Calls the given function once for each index of element in map,
// passing that index as a parameter.
//
void map_each(int (*f)(int, int, int, int)) {
    int i, j, x, y;
    y = 0;
    for (j = 0; j < SIZE; j++) {
        x = 0;
        for (i = 0; i < SIZE; i++) {
            f(i, j, x, y);
            x += SPLITE_SIZE;
        }
        y += SPLITE_SIZE;
    }
}

//
// Draw map for each element.
//
int draw_elem(int i_x, int i_y, int x, int y) {
    char ch = map[i_y][i_x];
    switch (ch) {
        case ' ': put_sprite(map_img[0],  x, y, &map_info[0]); break;
        case '@': put_sprite(map_img[1],  x, y, &map_info[1]); break;
        case '#': put_sprite(map_img[2],  x, y, &map_info[2]); break;
        case 'C': put_sprite(map_img[3],  x, y, &map_info[3]); break;
        case 'D': put_sprite(map_img[4],  x, y, &map_info[4]); break;
        case 'd': put_sprite(map_img[5],  x, y, &map_info[5]); break;
        case 'K': put_sprite(map_img[6],  x, y, &map_info[6]); break;
        case 'k': put_sprite(map_img[7],  x, y, &map_info[7]); break;
        case 'X': put_sprite(map_img[8],  x, y, &map_info[8]); break;
        case 'x': put_sprite(map_img[9],  x, y, &map_info[9]); break;
        case 'O': put_sprite(map_img[10], x, y, &map_info[10]); break;
        case 'o': put_sprite(map_img[11], x, y, &map_info[11]); break;
        case 'S': put_sprite(map_img[12], x, y, &map_info[12]); break;
        case 's': put_sprite(map_img[13], x, y, &map_info[13]); break;
        case 'G': put_sprite(map_img[14], x, y, &map_info[14]); break;
        case 'g': put_sprite(map_img[15], x, y, &map_info[15]); break;
    }
    return 1;
}

//
// Draw map.
//
void draw_map() {
    map_each(draw_elem);
}

//
// Draw player sprite with its direction.
//
void draw_player(Character *character) {
    int img_id = character->direction * 3 + character->state_img;
    put_sprite(character->img[img_id], character->pos.x, character->pos.y, &character->img_info[img_id]);
}

void draw_enemy(Character *character) {
    int img_id = character->direction;
    put_sprite(character->img[img_id], character->pos.x, character->pos.y, &character->img_info[img_id]);
}

//
// Put given texture to given x, y position.
//
void put_sprite(int num, int x, int y, pngInfo *info) {
    int w, h;
    // Get texture's width and height.
    w = info->Width;
    h = info->Height;

    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, num);
    glColor4ub(255, 255, 255, 255);

    glBegin(GL_QUADS); // rectangle: width (w) by height (h)
        glTexCoord2i(0, 0);
        glVertex2i(x, y);
        glTexCoord2i(0, 1);
        glVertex2i(x, y + h);
        glTexCoord2i(1, 1);
        glVertex2i(x + w, y + h);
        glTexCoord2i(1, 0);
        glVertex2i(x + w, y);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

//
// Return true if a wall is laid on given coordinate.
// Besides walls, keyed doors are also regarded as wall.
//
int is_wall(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == '#' || map[i_y][i_x] == 'C' ||
            map[i_y][i_x] == 'X' || map[i_y][i_x] == 'x');
}

//
// Return true if given character is movable for its direction.
//
int is_movable(Character *character, int (*is_wall)(int, int)) {
    int x = character->pos.x;
    int y = character->pos.y;
    Point next = move(character, MOVE_SPEED * 3, is_wall);
    if (x == next.x && y == next.y) return 0;
    return 1;
}

//
// Return next position if movable.
//
Point move(Character *character, int speed, int (*is_wall)(int, int)) {
    int direction = character->direction;
    Point orig = character->pos;
    Point next;

    switch (direction) {
        case UP:
            next.x = orig.x;
            next.y = orig.y - speed;
            break;
        case DOWN:
            next.x = orig.x;
            next.y = orig.y + speed;
            break;
        case LEFT:
            next.x = orig.x - speed;
            next.y = orig.y;
            break;
        case RIGHT:
            next.x = orig.x + speed;
            next.y = orig.y;
            break;
    }
    if (is_wall(next.x, next.y)) return orig;
    return next;
}

//
// Return true if enemy can move forward.
//
int is_wall_for_enemy(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == '#' || map[i_y][i_x] == 'C' ||
            map[i_y][i_x] == 'X' || map[i_y][i_x] == 'x' ||
            map[i_y][i_x] == 'D' || map[i_y][i_x] == 'd' );
}

//
// Return true if a door is laid on given coordinate.
//
int is_door(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == 'D' || map[i_y][i_x] == 'd');
}

//
// Open door.
//
void open_door(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    if (map[i_y][i_x] == 'D') map[i_y][i_x] = 'O';
    if (map[i_y][i_x] == 'd') map[i_y][i_x] = 'o';
}

//
// Return true if has key.
//
int has_key(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == 'K' || map[i_y][i_x] == 'k');
}

//
// Unlock keyed door.
// Actually, it merely translate keyed door to ordinary door after pick up a key.
//
void unlock_keyed_door(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    if (map[i_y][i_x] == 'K') map[i_y][i_x] = ' ';
    if (map[i_y][i_x] == 'k') map[i_y][i_x] = '@';
    map_each(_unlock_keyed_door);
}

//
// Helper function for unlock_keyed_door()
//
int _unlock_keyed_door(int i_x, int i_y, int x, int y) {
    if (map[i_y][i_x] == 'X') map[i_y][i_x] = 'D';
    if (map[i_y][i_x] == 'x') map[i_y][i_x] = 'd';
    return 1;
}

//
// Return true if a gold is laid on given position.
//
int is_gold(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == 'G' || map[i_y][i_x] == 'g');
}

//
// Take the gold.
//
void take_gold(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    if (map[i_y][i_x] == 'G') map[i_y][i_x] = ' ';
    if (map[i_y][i_x] == 'g') map[i_y][i_x] = '@';
    has_got_gold = 1;
}

//
// Return true if the game is finish.
//
int is_finish(int x, int y) {
    int i_x, i_y;
    if (!has_got_gold) return 0;
    i_x = x / SPLITE_SIZE;
    i_y = y / SPLITE_SIZE;
    return (map[i_y][i_x] == 'S' || map[i_y][i_x] == 's');
}

//
// Finish this game.
// It puts a black rectangle and GAMECLEAR image.
//
void finish() {
    did_gamefinish = 1;
    // printf("finish!!\n");
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
        glVertex2f(0, height - 50);
        glVertex2f(0, height);
        glVertex2f(width, height);
        glVertex2f(width, height - 50);
    glEnd();
    put_sprite(decoration_img[1], 50, height - 40, &decoration_info[1]);
}

//
// Light up specified element of map.
//
void light_on(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    switch (map[i_y][i_x]) {
        case ' ': case '#': case 'D': case 'K': case 'X': case 'O': case 'S': case 'G':
            map[i_y][i_x] += ' ';
            break;
    }
}

//
// Light off specified element of map.
//
void light_off(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    switch (map[i_y][i_x]) {
        case '@': case 'C': case 'd': case 'k': case 'x': case 'o': case 's': case 'g':
            map[i_y][i_x] -= ' ';
            break;
    }
}

//
// Update lighted_map represented enemies' sight.
//
void update_lighted_map() {
    int i_enemy;
    int i, j;
    int enemy_sight = 3;
    // init array lighted_map with 0
    memcpy(lighted_map, lighted_map_clear, sizeof(char) * SIZE * SIZE);

    for (i_enemy = 0; i_enemy < ENEMY; i_enemy++) {
        int type = enemies[i_enemy].type;
        int direction = enemies[i_enemy].direction;
        int x = enemies[i_enemy].pos.x;
        int y = enemies[i_enemy].pos.y;
        int i_x = x / SPLITE_SIZE;
        int i_y = y / SPLITE_SIZE;
        int flag = 1;
        if (type == 1) {
            // Type 1 enemy's sight is like:
            //
            //     ooooo  o   o  oo oo    ooo    ooo   oooo
            //     ooooo  oo oo  ooxoo    ooo  oxooo  xoooo
            //      ooo    oxo    ooo    xoo    ooo    ooo
            //       E      E      E      E      E      E
            //
            // where 'E' is a enemy, 'o' is the enemy's sight, and 'x' is a wall.
            //
            switch (direction) {
                case UP:
                    flag = -1;
                    // break through
                case DOWN:
                    for (j = -1; j <= 1; j++) {
                        for (i = 1; i <= enemy_sight; i++) {
                            if (is_wall_for_enemy(x + j * SPLITE_SIZE, y + flag * i * SPLITE_SIZE)) {
                                lighted_map[i_y + flag * i][i_x + j] = 1;
                                break;
                            } else {
                                lighted_map[i_y + flag * i][i_x + j] = 1;
                            }
                        }
                    }
                    if (is_wall_for_enemy(x, y + flag * 1 * SPLITE_SIZE)) {
                        lighted_map[i_y + flag * 3][i_x + 1] = 0;
                        lighted_map[i_y + flag * 3][i_x - 1] = 0;
                    }
                    for (j = -2; j <= 2; j += 4) {
                        if (is_wall_for_enemy(x + (j/2) * SPLITE_SIZE, y + flag * 1 * SPLITE_SIZE)) continue;
                        lighted_map[i_y + flag * 2][i_x + j] = 1;
                        if (is_wall_for_enemy(x + (j/2) * SPLITE_SIZE, y + flag * 2 * SPLITE_SIZE) ||
                            is_wall_for_enemy(x +  j    * SPLITE_SIZE, y + flag * 2 * SPLITE_SIZE)) continue;
                        lighted_map[i_y + flag * 3][i_x + j] = 1;
                    }
                    break;
                case LEFT:
                    flag = -1;
                    // break through
                case RIGHT:
                    for (j = -1; j <= 1; j++) {
                        for (i = 1; i <= enemy_sight; i++) {
                            if (is_wall_for_enemy(x + flag * i * SPLITE_SIZE, y + j * SPLITE_SIZE)) {
                                lighted_map[i_y + j][i_x + flag * i] = 1;
                                break;
                            } else {
                                lighted_map[i_y + j][i_x + flag * i] = 1;
                            }
                        }
                    }
                    if (is_wall_for_enemy(x + flag * 1 * SPLITE_SIZE, y)) {
                        lighted_map[i_y + 1][i_x + flag * 3] = 0;
                        lighted_map[i_y - 1][i_x + flag * 3] = 0;
                    }
                    for (j = -2; j <= 2; j += 4) {
                        if (is_wall_for_enemy(x + flag * 1 * SPLITE_SIZE, y + (j/2) * SPLITE_SIZE)) continue;
                        lighted_map[i_y + j][i_x + flag * 2] = 1;
                        if (is_wall_for_enemy(x + flag * 2 * SPLITE_SIZE, y + (j/2) * SPLITE_SIZE) ||
                            is_wall_for_enemy(x + flag * 2 * SPLITE_SIZE, y +  j    * SPLITE_SIZE)) continue;
                        lighted_map[i_y + j][i_x + flag * 3] = 1;
                    }
                    break;
            }
        }
        // Perhaps another type of enemy is continues to follows...
        // if (type == 2) { ... }
    }
}

//
// Light on element of map when element of lighted_map is true.
// Otherwise light off.
//
int light_on_off(int i_x, int i_y, int x, int y) {
    if (lighted_map[i_y][i_x] == 1) {
        light_on(x, y);
    } else {
        light_off(x, y);
    }
    return 1;
}

//
// Apply lighted_map to map.
//
void apply_lighted_map() {
    map_each(light_on_off);
}

//
// Update lightness map represented player's sight.
//
void update_lightness_map(int x, int y) {
    int range = 4;
    int i, j;
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;

    // player
    for (i = -range; i <= range; i++) {
        for (j = -range; j <= range; j++) {
            int tmp_x = i_x + i;
            int tmp_y = i_y + j;
            if (tmp_x >= 0 && tmp_x < SIZE &&
                tmp_y >= 0 && tmp_y < SIZE) {
                // then
                lightness_map[tmp_y][tmp_x] = 1;
            }
        }
    }

    // For showing a question mark sprite, init all tiles except set 1 with 0,
    // and then if enemies' position is in darkness, set -1.
    //
    //  1: no darkness
    //  0: darkness
    // -1: enemy in darkness
    //
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (lightness_map[i][j] != 1) {
                lightness_map[i][j] = 0;
            }
        }
    }
    // enemies
    for (i = 0; i < ENEMY; i++) {
        i_x = enemies[i].pos.x / SPLITE_SIZE;
        i_y = enemies[i].pos.y / SPLITE_SIZE;
        if (lightness_map[i_y][i_x] == 0) {
            lightness_map[i_y][i_x] = -1;
        }
    }
}

//
// Put darkness sprite unless previously come to nearby tile.
//
int lightness_on_off(int i_x, int i_y, int x, int y) {
    if (lightness_map[i_y][i_x] == 0) {
        // put darkness
        put_sprite(decoration_img[2], x, y, &decoration_info[2]);
    }
    return 1;
}

//
// Put darkness sprite with question mark when enemy is among darkness.
//
int enemy_in_darkness(int i_x, int i_y, int x, int y) {
    if (lightness_map[i_y][i_x] == -1) {
        // put darkness enemy
        put_sprite(decoration_img[3], x, y, &decoration_info[3]);
    }
    return 1;
}

//
// Apply lightness map.
// First, apply lightness_map to display darkness sprite if needed.
// And then if enemies' position is in darkness, puts a question mark splite.
//
void apply_lightness_map() {
    map_each(lightness_on_off);
    map_each(enemy_in_darkness);
}

//
// Return true if gameover.
//
int is_gameover(int x, int y) {
    int i_x = x / SPLITE_SIZE;
    int i_y = y / SPLITE_SIZE;
    return (lighted_map[i_y][i_x] == 1);
}

//
// Do gameover process.
// It puts a white rectangle and GAMEOVER image.
//
void gameover() {
    did_gameover = 1;
    // printf("gameover!\n");
    glColor3f(0.2, 0.2, 0.2);
    glBegin(GL_QUADS);
        glVertex2f(0, height - 50);
        glVertex2f(0, height);
        glVertex2f(width, height);
        glVertex2f(width, height - 50);
    glEnd();
    put_sprite(decoration_img[0], 50, height - 50, &decoration_info[0]);
}


// --- Keyboard input ---

//
// Callback function called for when key is down.
// Close window when 'q' or ESC key is pressed.
// Restart game when 'r' key is down.
// Wait a turn when 'z' or SPACE key is down unless game is finished.
//
void Keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'q':
    case 27: // ESC
        // quit
        printf("byebye.\n");
        exit(0);
        break;
    case 'r':
        // restart
        init();
        glutPostRedisplay();
        break;
    case 'z':
    case ' ':
        if (did_gameover || did_gamefinish) break;
        if (is_moving_phase) break;
        player.next_motion = 0;
        is_moving_phase = START_MOVING;
        break;
    }
}

//
// Callback function called for when special key is down.
// If it gets arrow keys and player is movable for the direction,
// is_moving_phase flag will set to be true.
//
void SpecialKey(int key, int _a, int _b) {
    int direction;

    if (did_gameover || did_gamefinish) return;
    if (is_moving_phase) return;
    switch (key) {
        case GLUT_KEY_UP:    direction = UP;    break;
        case GLUT_KEY_DOWN:  direction = DOWN;  break;
        case GLUT_KEY_LEFT:  direction = LEFT;  break;
        case GLUT_KEY_RIGHT: direction = RIGHT; break;
        default: return;
    }

    // Set player direction.
    player.direction = direction;
    if (is_movable(&player, is_wall)) {
        player.next_motion = 1;
        is_moving_phase = START_MOVING;
    }
}

#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

// compile and link: gcc -o artillery_fool artillery_fool.c -lncurses -lm

const int MAX_ANGLE = 89;
const int MIN_ANGLE = 1;
const int MAX_POWDER = 100;
const int MIN_POWDER = 1;

struct Player {
    int y;              // curses coords. the larger, the further south.
    int x;
    int x_dir;          // determines the horizontal direction of travel.
    int angle;          // in degrees.
    int powder;
    char base[5];
    WINDOW* info_box;   // displays current gun settings.
};

void draw_terrain(WINDOW*);
void draw_player(WINDOW*, struct Player*);
void next_move(WINDOW*, struct Player*);
int fire(WINDOW*, struct Player*, char);

int main()
{
    int move = 2;           // it just works.
    int current_player = move%2;
    int both_alive = 1;
    char status_str[24];
    
    initscr();              // Initialize the curses library
    keypad(stdscr, TRUE);   // Enable keypad mode
    noecho();               // Do not echo the characters on the screen
    cbreak();               // Disable line buffering

    // scope the screen
    int screen_width, screen_height;
    getmaxyx(stdscr, screen_height, screen_width);

    // banner    
    mvaddstr(0, screen_width/2 - 26, "UP//DOWN: ADJUST POWDER\tLEFT/RIGHT: ADJUST ANGLE");
    // game status
    sprintf(status_str,"PLAYER %d TURN", current_player + 1);
    mvaddstr(1, screen_width/2 - 6, status_str);

    struct Player players[] = {
        {3, screen_width/6, 1, 1, 1, "XXXX"},
        {3, (screen_width/6) * 5, -1, 1, 1, "OOOO"}
    };

    // TODO: extract to 'factory' type func   
    players[0].info_box = newwin(3,20,1,1);
    mvwaddstr(players[0].info_box, 0, 1, "PLAYER 1");
    mvwaddstr(players[0].info_box, 1, 1, "angle:");
    mvwaddstr(players[0].info_box, 2, 1, "powder:");

    players[1].info_box = newwin(3, 20 , 1, screen_width - 14);
    mvwaddstr(players[1].info_box, 0, 1, "PLAYER 2");
    mvwaddstr(players[1].info_box, 1, 1, "angle:");
    mvwaddstr(players[1].info_box, 2, 1, "powder:");

    draw_terrain(stdscr);
    draw_player(stdscr, &players[0]);
    draw_player(stdscr, &players[1]);

    refresh();
    wrefresh(players[0].info_box);
    wrefresh(players[1].info_box);

    curs_set(0);

    while(both_alive) {
        //update status
        mvaddch(1, screen_width/2 + 1, (current_player + 1) + '0');
        next_move(players[current_player].info_box, &players[current_player]);
        int enemy = (move + 1)%2;
        both_alive = fire(stdscr, &players[current_player], players[enemy].base[0]);
        if(both_alive) {
            ++move;
            current_player = move%2;
        }
    }

    // update status
    sprintf(status_str,"PLAYER %d WINS", current_player + 1);
    mvaddstr(1, screen_width/2 - 6, status_str);

    getch();

    delwin(players[0].info_box);
    delwin(players[1].info_box);
    endwin();

    return 0;
}

void next_move(WINDOW* subscreen, struct Player *player) {
    int pressed = 0;
    char display_str[16];
    while(pressed != ' ') {
		pressed = getch();
		switch(pressed)
		{
			case KEY_UP:
                if(player->powder < MAX_POWDER){
                    ++player->powder;
                }   
                sprintf(display_str,"%d   ",player->powder);
				mvwaddstr(subscreen, 2, 9, display_str);
                break;
			case KEY_DOWN:
                if(player->powder > MIN_POWDER){
                    --player->powder;
                }
                sprintf(display_str,"%d    ",player->powder);
				mvwaddstr(subscreen, 2, 9, display_str);
                break;
            case KEY_LEFT:
                if(player->angle < MAX_ANGLE){
                    ++player->angle;
                }
                sprintf(display_str,"%d   ",player->angle);
				mvwaddstr(subscreen, 1, 9, display_str);
                break;
            case KEY_RIGHT:
                if(player->angle > MIN_ANGLE){
                    --player->angle;
                }
                sprintf(display_str,"%d   ",player->angle);
				mvwaddstr(subscreen, 1, 9, display_str);
                break;
			case ' ':
				break;
		}
        wrefresh(subscreen);
    }
}

int fire(WINDOW* screen, struct Player *player, char target) {
    int height, width;
    getmaxyx(screen, height, width);

    int alive = 1;

    float bullet_x = player->x;
    float bullet_y = player->y;
    float power = (float)(player->powder)/50.0;

    float in_rads = player->angle * (M_PI/180);
    float dx = power * cos(in_rads) * player->x_dir;
    float dy = power * sin(in_rads);

    while(bullet_y < height && bullet_x < width) {
        bullet_x += dx;
        bullet_y -= dy;
        dy -= .02;
        chtype ch = mvwinch(screen, (int)bullet_y, (int)bullet_x);
        mvwaddch(screen, (int)bullet_y, (int)bullet_x, '.');
        wrefresh(screen);
        if(ch == target) {
            alive = 0;
            break;
        }
        if(ch != ' ') {
            // mvaddch((int)bullet_y, (int)bullet_x, '*');
            break;
        }
        usleep(20000);
        mvaddch((int)bullet_y, (int)bullet_x, ' ');
    }
    mvaddch((int)bullet_y, (int)bullet_x, '*');
    return alive;
}

void draw_player(WINDOW* screen, struct Player *player){
    int height, width;
    getmaxyx(screen, height, width);
    for(int i = player->y; i <= height; ++i) {
        for(int j = player->x - 5; j <= player->x + 5; ++j){
            chtype ch = mvwinch(screen, i, j);
            if(ch != ' ') {
                player->y = i;
                i = height + 1;
                j = player->x + 11;
                break;
            }
        }
    }
    mvwaddstr(screen, player->y, player->x, player->base);
    mvwaddstr(screen, player->y + 1, player->x - 1, "|HHHH|");
    return;
}

void draw_terrain(WINDOW* screen) {
    int height, width;
    int direction = 0;
    getmaxyx(screen, height, width);
    int ground_y = height - 3;
    int slope_len = 0;                     // Returns a pseudo-random integer between 1 and 3.
    char slope_char = '_';
    int coin_toss = 0;                      // 0 heads, 1 tails.   
    for( int i = 0; i < width; ++i){
        slope_len = (rand() % 3) + 1;      // Returns a pseudo-random integer between 1 and 3.
        coin_toss = (rand() % 2);
        if(coin_toss == 0 && ground_y > ((height/2) - 2)) {
            direction = -1;
            if(slope_char == '\\') {
                    ground_y += 1;
            }
            slope_char = '/';
        }
        else {
            if(ground_y < (height - slope_len)){
                direction = 1;
                if((slope_char == '_') || (slope_char == '/')) {
                    ground_y -= 1;
                }
                slope_char = '\\';
            }
            else {
                direction = 0;
                if(slope_char == '/') {
                    ground_y += 1;
                }
                slope_char = '_';
            }
        }
        for(int j = i; j < slope_len + i; ++j) {
            ground_y += direction;
            mvwaddch(screen, ground_y, j, ("%c",slope_char));
        }
        i += slope_len;
    }
    return;
}



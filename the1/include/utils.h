#ifndef UTILS_H
#define UTILS_H

#include "message.h"
#include "bomber.h"

int gather_visible_objects(unsigned int x, unsigned int y,
                                    int obstacles[][3], int obstacle_count,
                                    int bomber_coordinates[][2], Bomber* bombers, int bomber_count,
                                    od visible_objects[],
                                    int map_width, int map_height,Bomb* bombs, int bomb_count);

int check_move(unsigned int x, unsigned int y, int obstacles[][3], int obstacle_count,
               int bomber_coordinates[][2], Bomber* bombers, int bomber_count, int map_width, int map_height, unsigned int new_x, unsigned new_y);

int is_bomb_at_location(unsigned int x, unsigned int y, Bomb bombs[], int bomb_count) ;

void handle_explosion(int bomb_index, Bomb* bombs, int bomb_count, int obstacles[][3], int obstacle_count,
                      Bomber* bombers, int bomber_count, struct pollfd* bomber_fds, int map_width, int map_height, int* bomber_alive_count, int bomber_coordinates[][2]);
#endif // UTILS_H

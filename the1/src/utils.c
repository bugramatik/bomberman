#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include "../include/message.h"
#include "../include/utils.h"
#include "../include/bomber.h"

int gather_visible_objects(unsigned int x, unsigned int y,
                                    int obstacles[][3], int obstacle_count,
                                    int bomber_coordinates[][2], Bomber* bombers, int bomber_count,
                                    od visible_objects[],
                                    int map_width, int map_height) {

    unsigned int object_count = 0;
    int dx[] = {0, 0, -1, 1}; // Up, Down, Left, Right
    int dy[] = {-1, 1, 0, 0}; // Up, Down, Left, Right

    for (int dir = 0; dir < 4; ++dir) {
        for (int step = 1; step <= 3; ++step) {
            int new_x = x + dx[dir] * step;
            int new_y = y + dy[dir] * step;
            int found = 0;

            // Check if the new position is out of bounds
            if (new_x < 0 || new_y < 0 || new_x >= map_width || new_y >= map_height) {
                break;
            }

            // Check for obstacles
            for (int i = 0; i < obstacle_count; ++i) {
                if (obstacles[i][2] != 0 && obstacles[i][0] == new_x && obstacles[i][1] == new_y) {
                    visible_objects[object_count].position.x = new_x;
                    visible_objects[object_count].position.y = new_y;
                    visible_objects[object_count].type = OBSTACLE;
                    ++object_count;
                    found = 1;
                    break;
                }
            }

            // If an obstacle is found, stop looking in this direction
            if (found) {
                break;
            }

            // Check for bombers
            for (int i = 0; i < bomber_count; ++i) {
                if (!bombers[i].is_killed && bomber_coordinates[i][0] == new_x && bomber_coordinates[i][1] == new_y) {
                    visible_objects[object_count].position.x = new_x;
                    visible_objects[object_count].position.y = new_y;
                    visible_objects[object_count].type = BOMBER;
                    ++object_count;
                    break;
                }
            }

            // Add code to check for bombs when their implementation is available
        }
    }

    return object_count;
}

int check_move(unsigned int x, unsigned int y, int obstacles[][3], int obstacle_count,
               int bomber_coordinates[][2] , Bomber* bombers, int bomber_count, int map_width, int map_height, unsigned int new_x, unsigned int new_y){
    /* Checks if move is valid returns 1 if it is valid */
    if (new_x < 0 || new_y < 0 || new_x >= map_width || new_y >= map_height) { // Checks is target inside map
        return 0;
    }
    if(abs(new_x - x) > 1 || abs(new_y - y) > 1){ // Checks target is one step away
        return 0;
    }
    if(new_x != x && new_y != y){ // Checks target is not diagonal
        return 0;
    }
    for (int i = 0 ; i < obstacle_count ; i++){ // Checks if there is any obstacle on target
        if(obstacles[i][2] != 0 && obstacles[i][0] == new_x && obstacles[i][1] == new_y){
            return 0;
        }

    }
    for (int i = 0 ; i < bomber_count ; i++){ // Checks if there is any bomber on target
        if(!bombers[i].is_killed && bomber_coordinates[i][0] == new_x && bomber_coordinates[i][1] == new_y){
            return 0;
        }

    }
    return 1;
}

int is_bomb_at_location(unsigned int x, unsigned int y, Bomb bombs[], int bomb_count) {
    for (int i = 0; i < bomb_count; ++i) {
        if (bombs[i].x == x && bombs[i].y == y) {
            return 1;
        }
    }
    return 0;
}

void handle_explosion(int bomb_index, Bomb* bombs, int bomb_count, int obstacles[][3], int obstacle_count,
                      Bomber* bombers, int bomber_count, struct pollfd* bomber_fds, int map_width, int map_height, int* bomber_alive_count, int bomber_coordinates[][2]) {
    Bomb exploded_bomb = bombs[bomb_index];
    int radius = exploded_bomb.explosion_radius;
    int dx[] = {0, 0, 0, -1, 1}; // Center, Up, Down, Left, Right
    int dy[] = {0, -1, 1, 0, 0}; // Center, Up, Down, Left, Right
    int killed_bomber_count_in_explosion = 0;
    int killed_bomber_indices_in_explosion[4] = {-1, -1, -1, -1};
    int distances_to_center[4] = {-1, -1, -1, -1};
    int equally_close_to_center_bomber_count = 1;

    for (int i = 0; i < 5; i++){

        for (int j = 1; j <= radius; j++){

            int new_x = exploded_bomb.x + dx[i] * j;
            int new_y = exploded_bomb.y + dy[i] * j;

            if (new_x < 0 || new_y < 0 || new_x >= map_width || new_y >= map_height){
                break;
            }

            for (int k = 0; k < obstacle_count; k++){
                if (obstacles[k][2] != 0 && obstacles[k][0] == new_x && obstacles[k][1] == new_y){
                    obstacles[k][2] -= (obstacles[k][2]!=-1 ? 1:0); // If obstacle is indestructible, don't decrease its health
                    break;
                }
            }

            for (int k = 0; k < bomber_count; k++){
                // There is no information about if bomber blocks explosion or not  in the HW pdf, so I implemented like they block explosion
                if (!bombers[k].is_killed && bomber_coordinates[k][0] == new_x && bomber_coordinates[k][1] == new_y){
                    bombers[k].is_killed = 1;
                    killed_bomber_indices_in_explosion[killed_bomber_count_in_explosion++] = k;
                    (*bomber_alive_count)--;
                    break;
                }
            }


        }
    }
    if (bomber_count - (*bomber_alive_count) == 0){
        if (killed_bomber_count_in_explosion == 1){// If there is only one bomber died, it wins
            bombers[killed_bomber_indices_in_explosion[0]].is_winner = 1;
            bombers[killed_bomber_indices_in_explosion[0]].is_killed = 0;
            (*bomber_alive_count)++;
        } else { // If more than one last bombers died at same time, bomber close to the center wins
            for (int i = 0; i < killed_bomber_count_in_explosion; i++){
                int distance = abs(killed_bomber_indices_in_explosion[i] - map_width/2) + abs(killed_bomber_indices_in_explosion[i] - map_height/2);
                distances_to_center[i] = distance;
            }

            int min_distance_index = -1;
            int min_distance = 1000000;
            for(int i = 0; i < 4 ; i++){
                if (distances_to_center[i] == min_distance){
                    equally_close_to_center_bomber_count++;
                }
                else if (distances_to_center[i] < min_distance){
                    equally_close_to_center_bomber_count = 1;
                    min_distance = distances_to_center[i];
                    min_distance_index = killed_bomber_indices_in_explosion[i];
                }
            }

            if (equally_close_to_center_bomber_count != 1){ // There are more than one bomber equally close to the center, randomly choose one of them
                int random_index = rand() % equally_close_to_center_bomber_count;
                for (int j = 0; j < 4; j++){
                    if (distances_to_center[j] == min_distance){
                        if (random_index == 0){
                            min_distance_index = killed_bomber_indices_in_explosion[j];
                            break;
                        }
                        random_index--;
                    }
                }
            }
            bombers[killed_bomber_indices_in_explosion[min_distance_index]].is_winner = 1;
            bombers[killed_bomber_indices_in_explosion[min_distance_index]].is_killed = 0;
            (*bomber_alive_count)++;
        }
    }
}

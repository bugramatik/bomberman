#ifndef BOMB_H
#define BOMB_H

#include <sys/types.h>

typedef struct {
    pid_t pid;
    int fd[2];
    int is_killed;
    int is_winner;
} Bomber;

typedef struct {
    pid_t pid;
    int fd[2];
    unsigned int x;
    unsigned int y;
    unsigned int explosion_radius;
    unsigned int explosion_interval;
} Bomb;

#endif // BOMB_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include "../include/message.h"

typedef struct {
    pid_t pid;
    int fd[2];
    int is_killed;
} BomberProcess;

int main(int argc, char *argv[]) {
    int map_width, map_height, obstacle_count, bomber_count;
    // Read input data
    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);

    int obstacles[obstacle_count][3];
    char** bomber_arguments[bomber_count+1];
    int bomber_coordinates[bomber_count][2];

    // Read obstacles
    for(int i = 0 ; i < obstacle_count ; i++ ){
        scanf("%d %d %d", &obstacles[i][0], &obstacles[i][1], &obstacles[i][2]);
    }

    // Read bombers
    for(int i = 0 ; i < bomber_count ; i++ ){
        int argument_number;
        scanf("%d %d %d", &bomber_coordinates[i][0], &bomber_coordinates[i][1], &argument_number);
        char** arguments = malloc(argument_number * sizeof(char*));
        for(int j = 0 ; j < argument_number ; j++ ){
            arguments[j] = malloc(256 * sizeof(char));
            scanf(" %s ", arguments[j]);
        }
        bomber_arguments[i] = arguments;
    }
    bomber_arguments[bomber_count] = NULL;

    BomberProcess bombers[bomber_count];

    // Create Pipes for Bombers
    for(int i = 0 ; i < bomber_count ; i++ ){

        if (PIPE(bombers[i].fd) == -1) {
            perror("socketpair");
            exit(EXIT_FAILURE);
        }
        bombers[i].is_killed = 0;
        bombers[i].pid = fork();

        if(bombers[i].pid == 0){
            /* Child Process */

            // Redirect the bomber stdin and stdout to the pipe
            close(bombers[i].fd[0]);
            dup2(bombers[i].fd[1], STDIN_FILENO);
            dup2(bombers[i].fd[1], STDOUT_FILENO);
            close(bombers[i].fd[1]);

            char bomber_executable_path[256];
            sprintf(bomber_executable_path, "./src/%s", bomber_arguments[i][0]);

            // Execute the bomber
            execv(bomber_executable_path, bomber_arguments[i]);

            // If execv fails, exit
            if (execv(bomber_executable_path, bomber_arguments[i]) == -1) {
                perror("execv");
                exit(EXIT_FAILURE);
            }



        } else {
            /*  Parent Process */
            close(bombers[i].fd[1]);
        }
    }

    // Create pollfd array
    struct pollfd fds[bomber_count];
    for(int i = 0 ; i < bomber_count ; i++ ){
        fds[i].fd = bombers[i].fd[0];
        fds[i].events = POLLIN;
    }
    
    int active_bomber_count = bomber_count;
    // Controller loop
    while(active_bomber_count > 0){
        printf("geldi\n");

        // Poll the bomber pipes to see if there's any input
        int timeout = 1;
        int ready_fds = poll(fds, bomber_count, timeout);

        if (ready_fds == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        //Iterate over the pollfd array
        for (int i = 0; i< bomber_count; i++){
            if (fds[i].revents & POLLIN){
                im incoming_message;
                read(fds[i].fd, &incoming_message, sizeof(im));
                ssize_t bytes_read = read(fds[i].fd, &incoming_message, sizeof(im));
                if (bytes_read < 0) {
                    perror("read error");
                    exit(EXIT_FAILURE);
                } else if (bytes_read == 0) {
                    // Nothing was read
                    printf("Nothing was read\n");
                } else {
                    printf("Incoming message type: %d\n", incoming_message.type);
                }
                printf(" Bakam %d\n \n\n" ,incoming_message.type);
                switch (incoming_message.type) {
                    case BOMBER_START:
                        printf("Received BOMBER_START message\n");
                        break;
                    case BOMBER_SEE:
                        break;
                    case BOMBER_MOVE:
                        break;
                    case BOMBER_PLANT:
                        break;
                    case BOMB_EXPLODE:
                        break;
                    default:
                        fprintf(stderr, "Unknown message type received\n");
                        break;
                }
            }
        }


    }




    // Free allocated memory
    for(int i = 0 ; i < bomber_count ; i++ ){
        for(int j = 0 ; bomber_arguments[i][j] != NULL ; j++ ){
            free(bomber_arguments[i][j]);
        }
        free(bomber_arguments[i]);
    }

    return 0;
}

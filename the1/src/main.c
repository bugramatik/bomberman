#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

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
            dup2(bombers[i].fd[1], STDIN_FILENO);
            dup2(bombers[i].fd[0], STDOUT_FILENO);
            close(bombers[i].fd[1]);
            close(bombers[i].fd[0]);

            char bomber_executable_path[256];
            sprintf(bomber_executable_path, "../execs/%s", bomber_arguments[i][0]);

            // Execute the bomber
            execv(bomber_executable_path, bomber_arguments[i]);

            // If execv fails, exit
            if (execv(bomber_executable_path, bomber_arguments[i]) == -1) {
                perror("execv");
                exit(EXIT_FAILURE);
            }


        } else {
            /*  Parent Process */
            1;
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

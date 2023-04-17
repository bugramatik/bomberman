#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include "../include/message.h"
#include "../include/utils.h"
#include "../include/bomber.h"
#include "../include/logging.h"

int main(int argc, char *argv[]) {

    int map_width, map_height, obstacle_count, bomber_count;
    int bomb_count = 0;

    // Read input data
    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);
    int obstacles[obstacle_count][3]; // x, y, life
    char** bomber_arguments[bomber_count+1]; // +1 for NULL
    int bomber_coordinates[bomber_count][2]; // x, y

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

    Bomber bombers[bomber_count];
    Bomb* bombs = NULL;

    // Create Pipes for Bombers
    for(int i = 0 ; i < bomber_count ; i++ ){

        if (PIPE(bombers[i].fd) == -1) {
            perror("socketpair");
            exit(EXIT_FAILURE);
        }
        bombers[i].is_killed = 0;
        bombers[i].is_winner = 0;
        bombers[i].sent_last_message = -1; // -1 : no last message, 0 : last message pending, 1: last message sent
        bombers[i].pid = fork();

        if(bombers[i].pid == 0){
            /* Child Process */

            // Redirect the bomber stdin and stdout to the pipe
            dup2(bombers[i].fd[1], STDIN_FILENO);
            dup2(bombers[i].fd[1], STDOUT_FILENO);

            close(bombers[i].fd[0]);
            close(bombers[i].fd[1]);


            char bomber_executable_path[256];
            sprintf(bomber_executable_path, "./%s", bomber_arguments[i][0]);

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



    //TODO: center 0,0 a cek

    // Creating bomber and bomb pollfd arrays
    struct pollfd* bomb_fds  = NULL;
    struct pollfd bomber_fds[bomber_count];
    for(int i = 0 ; i < bomber_count ; i++ ){
        bomber_fds[i].fd = bombers[i].fd[0];
        bomber_fds[i].events = POLLIN;
    }

    int active_bomber_count = bomber_count;
    int active_bomb_count = 0;
    int game_over = 0;
    // Controller loop
    while(active_bomber_count > 0) {

        if(active_bomb_count == 0 && game_over == 1) {
            break;
        }

        // Polling the bomber pipes to see if there's any input
        int timeout = 1;
        int ready_bomber_fds = poll(bomber_fds, bomber_count, timeout);
        int ready_fds_bombs = poll(bomb_fds, bomb_count, timeout);

        if (ready_bomber_fds == -1 || ready_fds_bombs == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        //Iterating over the bomb pollfd array
        for (int i = 0; i < bomb_count; i++) {
            if (bomb_fds[i].revents & POLLIN) {
                im incoming_message;
                ssize_t bytes_read = read(bomb_fds[i].fd, &incoming_message, sizeof(im));
                if (bytes_read < 0) {
                    perror("read error");
                    exit(EXIT_FAILURE);
                } else if (bytes_read == 0) {
                    // Nothing was read
                    
                } else {
                    //Message was read
                    imp in;
                    in.pid = bombs[i].pid;
                    in.m = &incoming_message;
                    print_output(&in, NULL, NULL, NULL);
                    if (incoming_message.type == BOMB_EXPLODE) {
                        handle_explosion(i, bombs, bomb_count, obstacles, obstacle_count, bombers, bomber_count,
                                         bomber_fds,
                                         map_width, map_height, &active_bomber_count, bomber_coordinates);
                        active_bomb_count--;
                    }
                }
            }
        }

        //Iterating over the bomber pollfd array
        for (int i = 0; i < bomber_count && !game_over; i++) {
            if (bomber_fds[i].revents & POLLIN) {
                im incoming_message;
                ssize_t bytes_read = read(bomber_fds[i].fd, &incoming_message, sizeof(im));
                if (bytes_read < 0) {
                    perror("read error");
                    exit(EXIT_FAILURE);
                } else if (bytes_read == 0) {
                    // Nothing was read
                } else {
                    // Message was read
                    imp in;
                    in.pid = bombers[i].pid;
                    in.m = &incoming_message;
                    print_output(&in, NULL, NULL, NULL);
                    if (bombers[i].sent_last_message == 0) { // Waiting for last message
                        om response_message;
                        if (bombers[i].is_winner == 1) {
                            response_message.type = BOMBER_WIN;
                            game_over = 1;
                        } else if (bombers[i].is_killed == 1) {
                            response_message.type = BOMBER_DIE;
                        }
                        send_message(bomber_fds[i].fd, &response_message);
                        bombers[i].sent_last_message = 1;
                        omp out;
                        out.pid = bombers[i].pid;
                        out.m = &response_message;
                        print_output(NULL, &out, NULL, NULL);

                        // Reap the child process
                        int status;
                        pid_t child_pid = waitpid(bombers[i].pid, &status, 0);
                        if (child_pid == -1) {
                            perror("waitpid");
                            exit(EXIT_FAILURE);
                        }
                        continue;
                    }
                    
                    
                    omp out;
                    out.pid = bombers[i].pid;
                    switch (incoming_message.type) {
                        case BOMBER_START:{
                            om location_message;
                            location_message.type = BOMBER_LOCATION;
                            location_message.data.new_position.x = bomber_coordinates[i][0];
                            location_message.data.new_position.y = bomber_coordinates[i][1];
                            send_message(bomber_fds[i].fd, &location_message);
                            out.m = &location_message;
                            print_output(NULL, &out, NULL, NULL);
                            break;}
                        case BOMBER_SEE:{
                            od visible_objects[25];
                            int object_count = gather_visible_objects(bomber_coordinates[i][0],
                                                                      bomber_coordinates[i][1],
                                                                      obstacles, obstacle_count, bomber_coordinates,
                                                                      bombers,

                                                                      bomber_count, visible_objects, map_width,
                                                                      map_height,bombs,bomb_count);
                            // Sending BOMBER_VISION message
                            om see_message;
                            see_message.type = BOMBER_VISION;
                            see_message.data.object_count = object_count;
                            send_message(bomber_fds[i].fd, &see_message);

                            // Sending visible objects
                            send_object_data(bomber_fds[i].fd, object_count, visible_objects);
                            out.m = &see_message;
                            print_output(NULL, &out, NULL, visible_objects);
                            break;}
                        case BOMBER_MOVE:{
                            unsigned int new_x = incoming_message.data.target_position.x;
                            unsigned int new_y = incoming_message.data.target_position.y;

                            int is_move_valid = check_move(bomber_coordinates[i][0], bomber_coordinates[i][1],
                                                           obstacles, obstacle_count,
                                                           bomber_coordinates, bombers, bomber_count, map_width,
                                                           map_height, new_x, new_y);
                            if (is_move_valid) {
                                bomber_coordinates[i][0] = new_x;
                                bomber_coordinates[i][1] = new_y;
                            }

                            // Sending BOMBER_LOCATION message
                            om move_location_message;
                            move_location_message.type = BOMBER_LOCATION;
                            move_location_message.data.new_position.x = bomber_coordinates[i][0];
                            move_location_message.data.new_position.y = bomber_coordinates[i][1];
                            send_message(bomber_fds[i].fd, &move_location_message);
                            out.m = &move_location_message;
                            print_output(NULL, &out, NULL, NULL);
                            break;}
                        case BOMBER_PLANT:{
                            unsigned int bomb_x = bomber_coordinates[i][0];
                            unsigned int bomb_y = bomber_coordinates[i][1];
                            unsigned int radius = incoming_message.data.bomb_info.radius;
                            long interval = incoming_message.data.bomb_info.interval;
                            int plant_success = 0;

                            if (!is_bomb_at_location(bomb_x, bomb_y, bombs, bomb_count)) {
                                plant_success = 1;
                                active_bomb_count++;
                                bombs = realloc(bombs, sizeof(Bomb) * (bomber_count + 1));
                                Bomb new_bomb;
                                new_bomb.x = bomb_x;
                                new_bomb.y = bomb_y;
                                new_bomb.explosion_radius = radius;
                                new_bomb.explosion_interval = interval;
                                new_bomb.exploded = 0;
                                if (PIPE(new_bomb.fd) == -1) {
                                    perror("socketpair");
                                    exit(EXIT_FAILURE);
                                }
                                new_bomb.pid = fork();

                                if (new_bomb.pid == 0) {
                                    // Child process
                                    char interval_str[64];
                                    sprintf(interval_str, "%u", new_bomb.explosion_interval);

                                    // Redirecting the bomb stdin and stdout to the pipe
                                    dup2(new_bomb.fd[1], STDIN_FILENO);
                                    dup2(new_bomb.fd[1], STDOUT_FILENO);

                                    close(new_bomb.fd[0]);
                                    close(new_bomb.fd[1]);

                                    execl("./bomb", "bomb", interval_str, (char *) NULL);

                                    // If execl fails, exit
                                    perror("execl");
                                    exit(EXIT_FAILURE);
                                } else {
                                    // Parent process
                                    close(new_bomb.fd[1]);
                                    bombs = realloc(bombs, (bomb_count + 1) * sizeof(Bomb));
                                    if (bombs == NULL) {
                                        perror("realloc");
                                        exit(EXIT_FAILURE);
                                    }
                                    bombs[bomb_count++] = new_bomb;

                                    // Filling the bomb pollfd array
                                    bomb_fds = realloc(bomb_fds, bomb_count * sizeof(struct pollfd));
                                    if (bomb_fds == NULL) {
                                        perror("realloc");
                                        exit(EXIT_FAILURE);
                                    }
                                    bomb_fds[bomb_count - 1].fd = new_bomb.fd[0];
                                    bomb_fds[bomb_count - 1].events = POLLIN;
                                }}

                            // Sending BOMBER_PLANT_RESULT message
                            om plant_result_message;
                            plant_result_message.type = BOMBER_PLANT_RESULT;
                            plant_result_message.data.planted = plant_success;
                            send_message(bomber_fds[i].fd, &plant_result_message);
                            out.m = &plant_result_message;
                            print_output(NULL, &out, NULL, NULL);
                            break;
                            }
                        default:
                            fprintf(stderr, "Unknown message type received\n");
                            break;
                    }
                }

            }
        }

        usleep(1000);
    }

    free(bomb_fds);

    // Free allocated memory
    for(int i = 0 ; i < bomber_count ; i++ ){
        for(int j = 0 ; bomber_arguments[i][j] != NULL ; j++ ){
            free(bomber_arguments[i][j]);
        }
        free(bomber_arguments[i]);
    }

    return 0;
}

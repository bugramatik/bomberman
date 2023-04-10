#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int map_width, map_height, obstacle_count, bomber_count;
    
    // Read input data
    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);

    int obstacles[obstacle_count][3];
    char** bomber_arguments[bomber_count];
    int bomber_coordinates[bomber_count][2];

    // Read obstacles
    for(int i = 0 ; i < obstacle_count ; i++ ){
        scanf("%d %d %d", &obstacles[i][0], &obstacles[i][1], &obstacles[i][2]);
    }

    // Read bombers
    for(int i = 0 ; i < bomber_count ; i++ ){
        int argument_number;
        scanf("%d %d %d", &bomber_coordinates[i][0], &bomber_coordinates[i][1], &argument_number);
        printf("%d %d %d\n", bomber_coordinates[i][0], bomber_coordinates[i][1], argument_number);
        char** arguments = malloc(argument_number * sizeof(char*));
        for(int j = 0 ; j < argument_number ; j++ ){
            arguments[j] = malloc(256 * sizeof(char));
            scanf(" %s ", arguments[j]);
        }
        bomber_arguments[i] = arguments;
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

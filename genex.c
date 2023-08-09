#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
- for each input string
  - 



*/



int main (int argc, char** inputs) {

    if (argc < 3) {
        printf("Error: at least 2 arguments required\n");
        printf("Usage: %s <input 1> <input 2> [... <input n>]\n", inputs[0]);
        return 1;
    }

    size_t* lengths = malloc(sizeof(size_t) * (argc - 1));
    size_t min_len = lengths[0];
    for (int i = 0; i < argc - 1; i++) {
        lengths[i] = strlen(inputs[i + 1]);
        printf("lengths[%d]: %d\n", i, (int) lengths[i]);
        fflush(stdout);
        if (lengths[i] < min_len) {
            min_len = lengths[i];
        }
    }
    int subset_index = argc - 2;

    size_t upper_bound = lengths[0];
    size_t lower_bound = 0;
    size_t subset_len;

    while (1) {
        printf("upper_bound: %d, lower_bound: %d\n", (int) upper_bound, (int) lower_bound);
        fflush(stdout);
        subset_len = (upper_bound + lower_bound) / 2;
        if (subset_len == lower_bound) {
            break;
        }
        // printf("lengths[0]: %d\n", (int) lengths[0]);
        // printf("subset_len: %d\n", (int) subset_len);
        fflush(stdout);
        for (int start_index_1 = 0; start_index_1 < lengths[0] - subset_len && subset_index > 0; start_index_1++) {
            printf("start_index_1: %d\n", start_index_1);
            printf("subset_len: %d\n", (int) subset_len);
            printf("subset_index: %d\n", subset_index);
            fflush(stdout);
            printf("lengths[sub_index]: %d\n", (int) lengths[subset_index]);
            fflush(stdout);
            printf("after\n");
            fflush(stdout);
            for (int start_index_2 = 0; start_index_2 < lengths[subset_index] - subset_len && subset_index > 0; start_index_2++) {
                printf("start_index_2: %d\n", start_index_2);
                if (memcmp(inputs[1] + start_index_1, inputs[subset_index] + start_index_2, subset_len) == 0) {
                    subset_index--;
                    start_index_2 = 0;
                }
            }
            if (subset_index == 0) {
                break;
            }
            subset_index = argc - 2;
        }
        if (subset_index == 1) {
            lower_bound = subset_len;
            subset_index = argc;
        } else {
            upper_bound = subset_len;
            subset_index = argc;
        }
    }

    printf("Longest common substring: %.*s\n", (int)subset_len, inputs[1]);


    return 0;
}
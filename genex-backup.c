#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEBUG
// #define VERBOSE


int main (int input_count, char** inputs) {
    if (inputs[0][1] != 0) {
        inputs[0][0] = 0;
        inputs[0][1] = 0;
    } else {
        inputs[0][0]++;
    }


    input_count--;

    if (input_count < 2) {
        printf("Error: at least 2 arguments required\nUsage: %s <input 1> <input 2> [... <input n>]\n", inputs[0]);
        return 1;
    }

    unsigned long* lengths = malloc(sizeof(unsigned long) * (input_count));
    unsigned long* match_indices = malloc(sizeof(unsigned long) * (input_count));
    unsigned long* tmp_match_indices = malloc(sizeof(unsigned long) * (input_count));
    unsigned long matched_len = 0;
    unsigned long min_len = -1;
    for (int i = 0; i < input_count; i++) {
        int j = 0;
        while (inputs[i + 1][j]) {
            j++;
        }
        lengths[i] = j;
        if (j < min_len) {
            min_len = j;
        }
    }

    #ifdef DEBUG
        printf("\n");
        for (unsigned long i = 0; i < input_count; i++) {
            printf("%d>", (int) inputs[0][0]);
            printf("input[%d]: %s (%d)\n", (int) i, inputs[i+1], (int) lengths[i]);
        }
        printf("\n");
    #endif

    unsigned long upper_bound = min_len;
    unsigned long lower_bound = 1;
    unsigned long subset_len = min_len;

    unsigned long start_index_1;
    unsigned long start_index_2;

    while (min_len) {
        unsigned long subset_index = input_count;
        subset_len = (upper_bound + lower_bound) / 2;
        start_index_1 = 0;
        #ifdef DEBUG
            printf("\n");
            printf("upper_bound: %d\n", (int) upper_bound);
            printf("lower_bound: %d\n", (int) lower_bound);
            printf("subset_len: %d\n", (int) subset_len);
            printf("\n");
        #endif
        while (start_index_1 <= lengths[0] - subset_len) {
            start_index_2 = 0;
            subset_index = input_count;
            while (start_index_2 <= lengths[subset_index - 1] - subset_len && subset_index > 1) {
                #ifdef VERBOSE
                    printf("Comparing %.*s to %.*s\n", (int) subset_len, inputs[0] + start_index_1, (int) subset_len, inputs[subset_index] + start_index_2);
                #endif
                if (memcmp(inputs[1] + start_index_1, inputs[subset_index] + start_index_2, subset_len) == 0) {
                    subset_index--;
                    tmp_match_indices[subset_index] = start_index_2;
                    start_index_2 = 0;
                    continue;
                }
                start_index_2++;
            }
            if (subset_index == 1) {
                #ifdef DEBUG
                    printf("match found: %.*s\n", (int) subset_len, inputs[1] + start_index_1);
                #endif
                memcpy(match_indices, tmp_match_indices, sizeof(unsigned long) * input_count);
                match_indices[0] = start_index_1;
                matched_len = subset_len;
                break;
            }
            start_index_1++;
        }
        if (subset_index == 1) {
            lower_bound = subset_len + 1;
        } else {
            upper_bound = subset_len - 1;
        }
        if (lower_bound > upper_bound) {
            break;
        }
    }


    if (matched_len == 0) {
        #ifdef DEBUG
            printf("No match found\n");
        #endif
        // printf("%d>", (int) inputs[0][0]);
        printf("(");
        for (unsigned long i = 0; i < input_count; i++) {
            if (!i) {
                printf("%.*s", (int) lengths[i], inputs[i + 1]);
                continue;
            }
            printf("|%.*s", (int) lengths[i], inputs[i + 1]);
        }
        printf(")");
        free(lengths);
        free(match_indices);
        inputs[0][0]--;
        return 0;
    }

    char* lost_chars = malloc(sizeof(char) * input_count);
    // printf("\n");
    for (unsigned long i = 0; i < input_count; i++) {
        // printf("%d>", (int) inputs[0][0]);
        // printf(" : %d\n", (int) match_indices[i]);
        lost_chars[i] = inputs[i + 1][match_indices[i]];
        inputs[i + 1][match_indices[i]] = '\0';
    }
    // printf("%d>", (int) inputs[0][0]);
    // printf("Pre:");
    main(input_count+1, inputs);
    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1][match_indices[i]] = lost_chars[i];
    }
    // printf("\n");

    // printf("%d>", (int) inputs[0][0]);
    // printf("Match:");
    printf("%.*s", (int)matched_len, inputs[1] + match_indices[0]);
    // printf("\n");

    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1] += matched_len + match_indices[i];
    }

    // printf("%d>", (int) inputs[0][0]);
    // printf("Post:");
    main(input_count+1, inputs);
    // printf("\n");

    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1] -= matched_len + match_indices[i];
    }

    free(lost_chars);
    free(lengths);
    free(match_indices);
    inputs[0][0]--;
    return 0;
}
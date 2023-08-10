#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int input_count, char** inputs) {
    if (--input_count < 2) {
        printf("Error: at least 2 arguments required\nUsage: %s <input 1> <input 2> [... <input n>]\n", inputs[0]);
        return 1;
    }

    unsigned long* lengths = calloc(input_count, sizeof(unsigned long));
    unsigned long* match_indices = malloc(sizeof(unsigned long) * (input_count));
    unsigned long* tmp_match_indices = malloc(sizeof(unsigned long) * (input_count));
    unsigned long matched_len = 0;
    unsigned long min_len = -1;
    for (int i = 0; i < input_count; i++) {
        while (inputs[i + 1][lengths[i]]) lengths[i]++;
        if (lengths[i] < min_len) min_len = lengths[i];
    }

    unsigned long upper_bound = min_len;
    unsigned long lower_bound = 1;
    unsigned long subset_len = min_len;
    unsigned long start_index_1, start_index_2;

    while (min_len) {
        unsigned long subset_index = input_count;
        subset_len = (upper_bound + lower_bound) / 2;
        start_index_1 = 0;
        while (start_index_1 <= lengths[0] - subset_len) {
            start_index_2 = 0;
            subset_index = input_count;
            while (start_index_2 <= lengths[subset_index - 1] - subset_len && subset_index > 1) {
                if (memcmp(inputs[1] + start_index_1, inputs[subset_index] + start_index_2, subset_len) == 0) {
                    subset_index--;
                    tmp_match_indices[subset_index] = start_index_2;
                    start_index_2 = 0;
                    continue;
                }
                start_index_2++;
            }
            if (subset_index == 1) {
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
        return 0;
    }

    char* lost_chars = malloc(sizeof(char) * input_count);
    for (unsigned long i = 0; i < input_count; i++) {
        lost_chars[i] = inputs[i + 1][match_indices[i]];
        inputs[i + 1][match_indices[i]] = '\0';
    }
    main(input_count+1, inputs);
    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1][match_indices[i]] = lost_chars[i];
    }

    printf("%.*s", (int)matched_len, inputs[1] + match_indices[0]);

    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1] += matched_len + match_indices[i];
    }
    main(input_count+1, inputs);
    for (unsigned long i = 0; i < input_count; i++) {
        inputs[i + 1] -= matched_len + match_indices[i];
    }

    free(lost_chars);
    free(lengths);
    free(match_indices);
    return 0;
}
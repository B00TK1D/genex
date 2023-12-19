#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct input_struct {
    unsigned long count;
    char** strings;
};

void print_options(struct input_struct input, unsigned long* lengths) {
    printf("(");
    for (int i = 0; i < input.count; i++) {
        if (i > 0) {
            printf("|");
        }
        printf("%s", input.strings[i]);
    }
    printf(")");
}

void print_escaped(char* str, unsigned long len) {
    for (unsigned long i = 0; i < len; i++) {
        printf("%c", str[i]);
    }
}

// Find the longest common substring among a series of input strings using a binary search for length
unsigned long longest_commong_substring(struct input_struct input, int min_len, unsigned long* lengths, unsigned long* match_indices) {
    unsigned long* tmp_match_indices = malloc(sizeof(unsigned long) * (input.count));
    unsigned long matched_len = 0;
    unsigned long upper_bound = min_len;
    unsigned long lower_bound = 1;
    unsigned long subset_len = min_len;
    unsigned long start_index_1, start_index_2;

    while (min_len) {
        unsigned long subset_index = input.count - 1;
        subset_len = (upper_bound + lower_bound) / 2;
        start_index_1 = 0;
        while (start_index_1 <= lengths[0] - subset_len) {
            start_index_2 = 0;
            subset_index = input.count - 1;
            while (start_index_2 <= lengths[subset_index] - subset_len && subset_index > 0) {
                if (memcmp(input.strings[0] + start_index_1, input.strings[subset_index] + start_index_2, subset_len) == 0) {
                    tmp_match_indices[subset_index] = start_index_2;
                    subset_index--;
                    start_index_2 = 0;
                    continue;
                }
                start_index_2++;
            }
            if (subset_index == 0) {
                memcpy(match_indices, tmp_match_indices, sizeof(unsigned long) * input.count);
                match_indices[0] = start_index_1;
                matched_len = subset_len;
                break;
            }
            start_index_1++;
        }
        if (subset_index == 0) {
            lower_bound = subset_len + 1;
        } else {
            upper_bound = subset_len - 1;
        }
        if (lower_bound > upper_bound) {
            break;
        }
    }
    free(tmp_match_indices);

    return matched_len;
}


// Process a set of inputs and print a regex that closely matches all of them
int process(struct input_struct input) {

    unsigned long min_len = -1;
    unsigned long* lengths = calloc(input.count, sizeof(unsigned long));
    for (int i = 0; i < input.count; i++) {
        while (input.strings[i][lengths[i]]) {
            lengths[i]++;
        }
        if (lengths[i] < min_len) {
            min_len = lengths[i];
        }
    }

    unsigned long* match_indices = malloc(sizeof(unsigned long) * (input.count));
    unsigned long matched_len = longest_commong_substring(input, min_len, lengths, match_indices);

    if (matched_len == 0) {
        print_options(input, lengths);
        free(lengths);
        free(match_indices);
        return 0;
    }

    char nonempty = 0;
    char* lost_chars = malloc(sizeof(char) * input.count);
    for (unsigned long i = 0; i < input.count; i++) {
        if (!nonempty && match_indices[i] > 0) {
            nonempty = 1;
        }
        lost_chars[i] = input.strings[i][match_indices[i]];
        input.strings[i][match_indices[i]] = '\0';
    }
    if (nonempty) {
        process(input);
    }
    for (unsigned long i = 0; i < input.count; i++) {
        input.strings[i][match_indices[i]] = lost_chars[i];
    }

    print_escaped(input.strings[0] + match_indices[0], matched_len);

    nonempty = 0;
    for (unsigned long i = 0; i < input.count; i++) {
        if (!nonempty && match_indices[i] + matched_len < lengths[i]) {
            nonempty = 1;
        }
        input.strings[i] += matched_len + match_indices[i];
    }
    if (nonempty) {
        process(input);
    }
    for (unsigned long i = 0; i < input.count; i++) {
        input.strings[i] -= matched_len + match_indices[i];
    }

    free(lost_chars);
    free(lengths);
    free(match_indices);
    return 0;
}


int main(int argc, char** argv) {
    char* input1 = "{'name': 'Sam Smith', 'age': 30, 'car': 'Chevy'}";
    char* input2 = "{'name': 'John Rogers', 'age': 25, 'car': 'Ford'}";
    struct input_struct input = {2, malloc(sizeof(char*) * 2)};
    input.strings[0] = malloc(sizeof(char) * (strlen(input1) + 1));
    input.strings[1] = malloc(sizeof(char) * (strlen(input2) + 1));
    strcpy(input.strings[0], input1);
    strcpy(input.strings[1], input2);
    process(input);
    return 0;
}
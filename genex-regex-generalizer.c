#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Define a struct to hold the input_count and inputs array
// (Helps with limiting function parameters to 4 or under, to avoid the overhead of passing arguments via the stack)
struct input_struct {
    int count;
    char** strings;
};

struct generalizer_struct {
    int count;
    regex_t* regexes;
    char** strings;
    int* lengths;
};

// Print a string to stdout, including regex special characters, escaping it if necessary.
void print_escaped(char* s, unsigned long len) {
    for (unsigned long i = 0; i < len; i++) {
        switch (s[i]) {
            case '\\':
            case '.':
            case '*':
            case '+':
            case '?':
            case '|':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '^':
            case '$':
                putchar('\\');
                putchar(s[i]);
                break;
            case '\n':
                putchar('\\');
                putchar('n');
                break;
            case '\t':
                putchar('\\');
                putchar('t');
                break;
            case '\r':
                putchar('\\');
                putchar('r');
                break;
            case '\f':
                putchar('\\');
                putchar('f');
                break;
            case '\v':
                putchar('\\');
                putchar('v');
                break;
            case '\a':
                putchar('\\');
                putchar('a');
                break;
            case '\b':
                putchar('\\');
                putchar('b');
                break;
            case '\0':
                putchar('\\');
                putchar('0');
                break;
            default:
                putchar(s[i]);
                break;
        }
    }
}

void write_escaped(char* s, unsigned long len, char* dest) {
    unsigned long dest_index = 0;
    for (unsigned long i = 0; i < len; i++) {
        switch (s[i]) {
            case '\\':
            case '.':
            case '*':
            case '+':
            case '?':
            case '|':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '^':
            case '$':
                dest[dest_index++] = '\\';
                dest[dest_index++] = s[i];
                break;
            case '\n':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'n';
                break;
            case '\t':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 't';
                break;
            case '\r':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'r';
                break;
            case '\f':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'f';
                break;
            case '\v':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'v';
                break;
            case '\a':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'a';
                break;
            case '\b':
                dest[dest_index++] = '\\';
                dest[dest_index++] = 'b';
                break;
            case '\0':
                dest[dest_index++] = '\\';
                dest[dest_index++] = '0';
                break;
            default:
                dest[dest_index++] = s[i];
                break;
        }
    }
}

// Print a series of options that a variable might have (in regex-compatible format), generalizing the regex if generalizers are provided
void print_options(struct input_struct input, unsigned long* lengths, struct generalizer_struct generalizers) {
    if (!input.count) {
        return;
    }
    char** output_strings = malloc(sizeof(char*) * input.count);
    unsigned long* output_lengths = malloc(sizeof(unsigned long) * input.count);
    unsigned long output_count = 0;
    unsigned long match_index = 0;
    int match_result;

    putchar('(');
    //print_escaped(input.strings[0], lengths[0]);
    for (unsigned long i = 0; i < input.count; i++) {
        // For each generalizer regex, check if there are any matches in the current string
        // If so, print_escaped up until the match, then print the unescaped regex string, then print after the match (or until the next match)
        // If not, print the entire string
        match_index = 0;
        output_strings[output_count] = malloc(sizeof(char) * lengths[i] * 8);
        for (int j = 0; j < generalizers.count; j++) {
            regmatch_t match;
            match_result = regexec(generalizers.regexes + j, input.strings[i], 1, &match, 0);
            while (match_result != REG_NOMATCH) {
                if (match.rm_so) {
                    write_escaped(input.strings[i] + match_index, match.rm_so, output_strings[output_count] + output_lengths[output_count]);
                    output_lengths[output_count] += match.rm_so;
                    //print_escaped(input.strings[i] + match_index, match_index - match.rm_so);
                }
                memcpy(output_strings[output_count] + output_lengths[output_count], generalizers.strings[j], generalizers.lengths[j]);
                output_lengths[output_count] += generalizers.lengths[j];
                // for (unsigned long k = 0; k < generalizers.lengths[j]; k++) {
                //     output_strings[output_count][k] = generalizers.strings[j][k];
                //     //putchar(generalizers.strings[j][k]);
                // }
                match_index += match.rm_eo;
                match_result = regexec(generalizers.regexes + j, input.strings[i] + match_index, 1, &match, 0);
            }
            //print_escaped(input.strings[i] + match_index, lengths[i] - match_index);
            write_escaped(input.strings[i] + match_index, lengths[i] - match_index, output_strings[output_count] + output_lengths[output_count]);
            output_lengths[output_count] += lengths[i] - match_index;
            if (output_lengths[output_count] > lengths[i] * 4) {
                output_strings[output_count] = realloc(output_strings[output_count], sizeof(char) * lengths[i] * 32);
            }
        }
        output_count++;
        // print_escaped(input.strings[i], lengths[i]);
    }
    for (unsigned long i = 0; i < output_count; i++) {
        if (i) {
            putchar('|');
        }
        for (unsigned long j = 0; j < output_lengths[i]; j++) {
            putchar(output_strings[i][j]);
        }
        free(output_strings[i]);
    }
    putchar(')');

    free(output_lengths);
    free(output_strings);
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
int process(struct input_struct input, struct generalizer_struct generalizers) {

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
        print_options(input, lengths, generalizers);
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
        process(input, generalizers);
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
        process(input, generalizers);
    }
    for (unsigned long i = 0; i < input.count; i++) {
        input.strings[i] -= matched_len + match_indices[i];
    }

    free(lost_chars);
    free(lengths);
    free(match_indices);
    return 0;
}

int main (int input_count, char** inputs) {

    if (input_count < 2) {
        fprintf(stderr, "Usage: %s [-gr] [file...]\n", inputs[0]);
        exit(EXIT_FAILURE);
    }

    struct generalizer_struct generalizers = {0, malloc((input_count / 2) * sizeof(regex_t)), malloc((input_count / 2) * sizeof(char)), malloc((input_count / 2) * sizeof(int))};

    int opt;
    int arg_count = 1;
    while ((opt = getopt(input_count, inputs, "g:d:")) != -1) {
        switch (opt) {
        case 'g':
            arg_count += 2;
            if (regcomp(&(generalizers.regexes[generalizers.count]), optarg, REG_EXTENDED) != 0) {
                fprintf(stderr, "Error: Failed to compile provided generalizer regex '%s'\n", optarg);
                exit(EXIT_FAILURE);
            }
            generalizers.strings[generalizers.count] = optarg;
            generalizers.lengths[generalizers.count] = strlen(optarg);
            generalizers.count++;
            break;
        case 'd':
            arg_count += 2;
            printf("Reading from directory %s\n", optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-gr] [file...]\n", inputs[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (input_count - arg_count < 1) {
        fprintf(stderr, "Error: No input provided - provide input strings or use -d to read files from a directory.\n");
        exit(EXIT_FAILURE);
    }

    char stdout_buf[8192];
    setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));

    struct input_struct input = {input_count - arg_count, inputs + arg_count};

    process(input, generalizers);

    free(generalizers.regexes);
    exit(EXIT_SUCCESS);
}
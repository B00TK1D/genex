#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

// Define a struct to hold the input_count and inputs array
// (Helps with limiting function parameters to 4 or under, to avoid the overhead of passing arguments via the stack)
struct input_struct {
    int count;
    char** strings;
};

char strict = 0;
char verystrict = 0;

// Help message
void print_help(char* bin_name) {
    printf("Usage: %s [-sSd:f:] [inputs...]\n", bin_name);
    printf("  -s: Strict mode - do not attempt to expand charsets based on statistical analysis and common charsets\n");
    printf("  -S: Very strict mode - do not merge variables from different inputs into combined charsets\n");
    printf("  -d <directory>: Read inputs from a directory (every file in directory is read as its own input)\n");
    printf("  -f <file>: Read inputs from a file (every line in file is read as its own input)\n\n");
}


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


void print_number(unsigned long n) {
    if (n/10) {
        print_number(n/10);
    }
    putchar('0' + (n%10));
}


// Print a regex range expression (i.e. {3,5}), given a min and max value
void print_range(unsigned long min, unsigned long max) {
    if (min == 0) {
        if (max == 1) {
            putchar('?');
            return;
        }
        putchar('{');
        putchar('0');
        putchar(',');
        print_number(max);
        putchar('}');
        return;
    }
    if (min == max) {
        putchar('{');
        print_number(min);
        putchar('}');
        return;
    }
    putchar('{');
    print_number(min);
    putchar(',');
    print_number(max);
    putchar('}');
}


// Print a series of options that a variable might have (in regex-compatible format)
void print_options(struct input_struct input, unsigned long* lengths) {
    if (!input.count) {
        return;
    }

    if (verystrict) {
        putchar('(');
        print_escaped(input.strings[0], lengths[0]);
        for (unsigned long i = 1; i < input.count; i++) {
            putchar('|');
            print_escaped(input.strings[i], lengths[i]);
        }
        putchar(')');
        return;
    }

    char chars_present[256] = {0};
    unsigned long long sample_size = 0;
    unsigned long min_len = lengths[0];
    unsigned long max_len = lengths[0];

    for (unsigned long i = 0; i < input.count; i++) {
        for (unsigned long j = 0; j < lengths[i]; j++) {
            chars_present[input.strings[i][j]] = 1;
        }
        sample_size += lengths[i];
        if (lengths[i] < min_len) {
            min_len = lengths[i];
        }
        if (lengths[i] > max_len) {
            max_len = lengths[i];
        }
    }

    putchar('(');
    putchar('[');
    unsigned char i = 0;
    while(1) {
        if (chars_present[i]) {
            print_escaped((char*)&i, 1);
        }
        if (++i == 0) {
            break;
        }
    }
    putchar(']');
    print_range(min_len, max_len);
    putchar(')');
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


int main (int argc, char** argv) {

    
            DIR *dir = opendir("test");
}
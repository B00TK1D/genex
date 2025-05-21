#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

// Define a struct to hold the input_count and inputs array
// (Helps with limiting function parameters to 4 or under, to avoid the overhead of passing arguments via the stack)
struct input_struct {
    unsigned long count;
    char** strings;
    unsigned long* lengths;
};

struct processing_struct {
  struct input_struct input;
  unsigned long min_len;
  unsigned long max_len;

  unsigned long* tmp_indices;   // [input.count]
  unsigned long** lcs_indices;  // [min_len][input.count]
  unsigned long* lcs_options;   // [input.count]
  unsigned long lcs_count;
  unsigned long lcs_len;
  unsigned long lcs_index;

  unsigned long** match_indices_list;   // [input.count][length[i]]
  unsigned long* match_counts;          // [input.count]
  unsigned long* tmp_match_indices;     // [max_len]
  unsigned long* current_match_indices; // [input.count]

  char** lost_chars;  // [input.count][min_len]
  unsigned long depth;
};

char strict = 0;
char verystrict = 0;
char quiet = 0;

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
    if (quiet) {
        return;
    }
    for (unsigned long i = 0; i < len; i++) {
        switch (s[i]) {
            case '\\':
            case '/':
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
    if (quiet) {
        return;
    }
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
    char in_range = 0;
    while(1) {
        if (in_range) {
            if (!chars_present[i]) {
                putchar('-');
                i--;
                print_escaped((char*) &i, 1);
                i++;
                in_range = 0;
            }
        } else if (chars_present[i]) {
            print_escaped((char*)&i, 1);
            if (i < 255 && chars_present[i + 1]) {
              in_range = 1;
            }
        }
        if (++i == 0) {
            break;
        }
    }
    putchar(']');
    print_range(min_len, max_len);
    putchar(')');
}


#define STATIC_ALLOC_SIZE 1048576

void* static_alloc_buf[STATIC_ALLOC_SIZE];
void* global_memory = static_alloc_buf;
unsigned long global_size = 0;
unsigned long global_used = 0;

void stack_init(unsigned long size) {
  if (size > STATIC_ALLOC_SIZE) {
    global_memory = malloc(size);
  }
  global_size = size;
  global_used = 0;
}

void* stack_alloc(unsigned long size) {
  global_used += size;
  return (void*) (global_memory + (global_used - size));
  printf("Alloc to %lu\n", global_used);
  fflush(stdout);
}

void stack_free(unsigned long size) {
  global_used -= size;
  printf("Free to %lu\n", global_used);
  fflush(stdout);
}



// Find the longest common substrings among a series of input strings using a binary search for length
unsigned long longest_common_substrings(struct processing_struct data) {
    unsigned long lcs_len = 0;
    unsigned long min_len = data.min_len;
    unsigned long upper_bound = min_len;
    unsigned long lower_bound = 1;
    unsigned long tmp_lcs_len = min_len;
    unsigned long start_index_1, start_index_2;

    while (min_len) {
        // Binary search subset length
        unsigned long subset_index = data.input.count - 1;
        tmp_lcs_len = (upper_bound + lower_bound) / 2;
        start_index_1 = 0;
        data.lcs_count = 0;
        while (start_index_1 <= data.input.lengths[0] - tmp_lcs_len) {
            // Don't duplicate lcs searches
            start_index_2 = 0;
            for (unsigned long i = 0; i < data.lcs_count && start_index_2 == 0; i++) {
                if (memcmp(data.input.strings[0] + start_index_1, data.input.strings[0] + data.lcs_options[i], tmp_lcs_len) == 0) {
                    start_index_2 = 1;
                }
            }
            if (start_index_2) {
                start_index_1++;
                continue;
            }
            subset_index = data.input.count - 1;
            while (start_index_2 <= data.input.lengths[subset_index] - tmp_lcs_len && subset_index > 0) {
                if (memcmp(data.input.strings[0] + start_index_1, data.input.strings[subset_index] + start_index_2, tmp_lcs_len) == 0) {
                    subset_index--;
                    start_index_2 = 0;
                    continue;
                }
                start_index_2++;
            }
            if (subset_index == 0) {
                data.lcs_options[(data.lcs_count)++] = start_index_1;
                lcs_len = tmp_lcs_len;
            }
            start_index_1++;
        }
        if (subset_index == 0) {
            lower_bound = tmp_lcs_len + 1;
        } else {
            upper_bound = tmp_lcs_len - 1;
        }
        if (lower_bound > upper_bound) {
            break;
        }
    }

    data.lcs_len = lcs_len;
    return lcs_len;
}


// Minimize the distance between the substrings
unsigned long minimize_distance(struct processing_struct data) {

    unsigned long tmp_match_count = 0;
    for (unsigned long i = 0; i < data.input.count; i++) {
        // Find all occurances of the substring in the string
        for (unsigned long j = 0; j <= data.input.lengths[i] - data.lcs_len; j++) {
            if (memcmp(data.input.strings[i] + j, data.input.strings[0] + data.lcs_index, data.lcs_len) == 0) {
                data.tmp_match_indices[tmp_match_count++] = j;
            }
        }
        // Allocate the match_indices_list array
        data.match_indices_list[i] = stack_alloc(sizeof(unsigned long) * tmp_match_count);
        // Copy the matches into the match_indices_list array
        memcpy(data.match_indices_list[i], data.tmp_match_indices, sizeof(unsigned long) * tmp_match_count);

        data.current_match_indices[i] = data.tmp_match_indices[tmp_match_count - 1];
        data.match_counts[i] = tmp_match_count;
        tmp_match_count = 0;
    }

    unsigned long min_distance = ULONG_MAX;
    unsigned long current_min;
    unsigned long current_max;
    unsigned long current_max_index;
    while (1) {
        current_min = ULONG_MAX;
        current_max = 0;
        current_max_index = 0;

        for (unsigned long i = 0; i < data.input.count; i++) {
            if (data.current_match_indices[i] < current_min) {
                current_min = data.current_match_indices[i];
            }
            if (data.current_match_indices[i] > current_max) {
                current_max = data.current_match_indices[i];
            }
            if (data.current_match_indices[i] > data.current_match_indices[current_max_index]) {
                current_max_index = i;
            }
        }

        if (current_max - current_min < min_distance) {
            min_distance = current_max - current_min;
            memcpy(data.tmp_indices, data.current_match_indices, sizeof(unsigned long) * data.input.count);
        }

        if (data.match_counts[current_max_index] == 0) {
            return min_distance;
        }

        data.current_match_indices[current_max_index] = data.match_indices_list[current_max_index][--data.match_counts[current_max_index]];
    }

    return 0;
}

struct processing_struct pre_process(struct input_struct input) {
    struct processing_struct data = {
        .input = input,
        .max_len = 0,
        .min_len = -1,
        .depth = 0,
    };

    for (int i = 0; i < input.count; i++) {
        if (data.input.lengths[i] < data.min_len) {
            data.min_len = data.input.lengths[i];
        }
        if (data.input.lengths[i] > data.max_len) {
            data.max_len = data.input.lengths[i];
        }
    }


    data.tmp_indices = malloc(sizeof(unsigned long) * input.count);
    data.lcs_indices = malloc(sizeof(unsigned long*) * data.min_len);
    data.lcs_options = malloc(sizeof(unsigned long) * input.count);

    data.lost_chars = malloc(sizeof(char*) * input.count);

    data.match_indices_list = malloc(sizeof(unsigned long) * input.count);
    data.match_counts = malloc(sizeof(unsigned long) * input.count);
    data.tmp_match_indices = malloc(sizeof(unsigned long) * data.max_len);
    data.current_match_indices = malloc(sizeof(unsigned long) * input.count);

    for (int i = 0; i < data.min_len; i++) {
        data.lcs_indices[i] = malloc(sizeof(unsigned long) * input.count);
    }

    for (int i = 0; i < input.count; i++) {
        data.match_indices_list[i] = malloc(sizeof(unsigned long) * data.input.lengths[i]);
        data.lost_chars[i] = malloc(sizeof(char) * data.min_len);
    }

    return data;
}

// Process a set of inputs and print a regex that closely matches all of them
int process(struct processing_struct data) {

    unsigned long min_len = -1;
    unsigned long max_len = 0;
    for (int i = 0; i < data.input.count; i++) {
        if (data.input.lengths[i] < data.min_len) {
            data.min_len = data.input.lengths[i];
        }
        if (data.input.lengths[i] > data.max_len) {
            data.max_len = data.input.lengths[i];
        }
    }

    longest_common_substrings(data);

    unsigned long min_distance = ULONG_MAX;
    for (unsigned long i = 0; i < data.lcs_count; i++) {
        unsigned long temp_distance = minimize_distance(data);
        if (temp_distance < min_distance) {
            min_distance = temp_distance;
            memcpy(data.lcs_indices[data.depth], data.tmp_indices, sizeof(unsigned long) * data.input.count);
        }
    }

    if (data.lcs_len == 0) {
        print_options(data.input, data.input.lengths);
        return 0;
    }

    char nonempty = 0;
    for (unsigned long i = 0; i < data.input.count; i++) {
        if (!nonempty && data.lcs_indices[i] > 0) {
            nonempty = 1;
        }
        data.lost_chars[data.depth][i] = data.input.strings[i][data.lcs_indices[data.depth][i]];
        data.input.strings[i][data.lcs_indices[data.depth][i]] = '\0';
    }
    if (nonempty) {
        data.depth++;
        process(data);
        data.depth--;
    }
    for (unsigned long i = 0; i < data.input.count; i++) {
        data.input.strings[i][data.lcs_indices[data.depth][i]] = data.lost_chars[data.depth][i];
    }

    print_escaped(data.input.strings[0] + data.lcs_indices[data.depth][0], data.lcs_len);

    nonempty = 0;
    for (unsigned long i = 0; i < data.input.count; i++) {
        if (!nonempty && data.lcs_indices[data.depth][i] + data.lcs_len < data.input.lengths[i]) {
            nonempty = 1;
        }
        data.input.strings[i] += data.lcs_len + data.lcs_indices[data.depth][i];
    }
    if (nonempty) {
        data.depth++;
        process(data);
        data.depth--;
    }
    for (unsigned long i = 0; i < data.input.count; i++) {
        data.input.strings[i] -= data.lcs_len + data.lcs_indices[data.depth][i];
    }

    return 0;
}


int main (int argc, char** argv) {

    int opt;
    int arg_count = 1;
    char source_type = 0;
    char* source = NULL;
    while ((opt = getopt(argc, argv, "hqsSd:f:")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        case 'q':
            arg_count++;
            quiet = 1;
            break;
        case 's':
            arg_count++;
            strict = 1;
            break;
        case 'S':
            arg_count++;
            verystrict = 1;
            break;
        case 'd':
            arg_count += 2;
            if (source_type) {
                fprintf(stderr, "Error: Only one source can be specified\n");
                exit(EXIT_FAILURE);
            }
            source_type = 1;
            source = optarg;
            break;
        case 'f':
            arg_count += 2;
            if (source_type) {
                fprintf(stderr, "Error: Only one source can be specified\n");
                exit(EXIT_FAILURE);
            }
            source_type = 2;
            source = optarg;
            break;
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    char stdout_buf[8192];
    setvbuf(stdout, stdout_buf, _IOFBF, sizeof(stdout_buf));

    char** inputs;
    unsigned long input_count = 0;
    switch (source_type) {
        case 1: {
            // List all the files in the directory, and read the contents of each one into an entry in inputs
            DIR *dir = opendir(source);
            if (!dir) {
                fprintf(stderr, "Error: Could not open directory %s\n", source);
                exit(EXIT_FAILURE);
            }
            // First, count the number of files in the directory
            struct dirent* entry;
            while ((entry = readdir(dir))) {
                if (entry->d_type == DT_REG) {
                    input_count++;
                }
            }
            // Then, allocate the inputs array
            inputs = malloc(sizeof(char*) * input_count);
            // Then, read the contents of each file into an entry in inputs
            rewinddir(dir);
            unsigned long i = 0;
            while ((entry = readdir(dir))) {
                if (entry->d_type == DT_REG) {
                    char* filename = malloc(sizeof(char) * FILENAME_MAX);
                    snprintf(filename, FILENAME_MAX, "%s/%s", source, entry->d_name);
                    FILE* file = fopen(filename, "r");
                    if (!file) {
                        fprintf(stderr, "Error: Could not open file %s\n", filename);
                        exit(EXIT_FAILURE);
                    }
                    fseek(file, 0, SEEK_END);
                    unsigned long file_size = ftell(file);
                    rewind(file);
                    inputs[i] = malloc(sizeof(char) * (file_size + 1));
                    fread(inputs[i], sizeof(char), file_size, file);
                    inputs[i][file_size] = '\0';
                    fclose(file);
                    free(filename);
                    i++;
                }
            }
            break;
        }
        case 2: {
            // read each line of the file into an entry in inputs
            // First, open the file
            FILE* file = fopen(source, "r");
            if (!file) {
                fprintf(stderr, "Error: Could not open file %s\n", source);
                exit(EXIT_FAILURE);
            }
            // Then, count the number of lines in the file
            char c;
            while ((c = fgetc(file)) != EOF) {
                if (c == '\n') {
                    input_count++;
                }
            }
            // Then, allocate the inputs array
            inputs = malloc(sizeof(char*) * input_count);
            // Then, read each line into an entry in inputs
            rewind(file);
            unsigned long i = 0;
            unsigned long line_start = 0;
            // Use strtok
            char* line = NULL;
            size_t line_len = 0;
            while (getline(&line, &line_len, file) != -1) {
                inputs[i] = malloc(sizeof(char) * (line_len + 1));
                strcpy(inputs[i], line);
                inputs[i][line_len] = '\0';
                i++;
            }
            fclose(file);
            break;
        }
        default: {
            if (argc - arg_count < 1) {
                fprintf(stderr, "Error: No input provided - provide inputs strings as arguments, or specify -d or -f to read from a directory or a file\n");
                exit(EXIT_FAILURE);
            }
            inputs = argv + arg_count;
            input_count = argc - arg_count;
            break;
        }
    }

    struct input_struct input = {
      .count = input_count,
      .strings = inputs,
      .lengths = malloc(sizeof(unsigned long) * input_count),
    };

    for (int i = 0; i < input.count; i++) {
        input.lengths[i] = 0;
        while (input.strings[i][input.lengths[i]]) {
            input.lengths[i]++;
        }
    }

    for (int i = 0; i < 1000000; i++) {
        process(input);
    }
    if (!quiet) {
        putchar('\n');
    }
    printf("DEBUG: Allocated %lu bytes\n", global_used);

    exit(EXIT_SUCCESS);
}

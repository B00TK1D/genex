#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Memory pool structure - optimized for cache alignment
typedef struct {
  char *buffer;
  unsigned long size;
  unsigned long used;
} memory_pool;

// Initialize a memory pool
static inline void pool_init(memory_pool *pool, unsigned long initial_size) {
  pool->buffer = malloc(initial_size);
  if (!pool->buffer) {
    fprintf(stderr, "Error: Failed to allocate memory pool\n");
    exit(EXIT_FAILURE);
  }
  pool->size = initial_size;
  pool->used = 0;
}

// Allocate from pool - inline for speed
static inline void *pool_alloc(memory_pool *pool, unsigned long size) {
  // Align to 8 bytes
  unsigned long aligned_size = (size + 7) & ~7UL;

  if (__builtin_expect(pool->used + aligned_size > pool->size, 0)) {
    // Need to grow the pool (cold path)
    unsigned long new_size = pool->size * 2;
    while (new_size < pool->used + aligned_size) {
      new_size *= 2;
    }

    char *new_buffer = realloc(pool->buffer, new_size);
    if (!new_buffer) {
      fprintf(stderr, "Error: Failed to grow memory pool\n");
      exit(EXIT_FAILURE);
    }

    pool->buffer = new_buffer;
    pool->size = new_size;
  }

  void *ptr = pool->buffer + pool->used;
  pool->used += aligned_size;
  return ptr;
}

// Free last allocation (stack-like) - inline for speed
static inline void pool_free_last(memory_pool *pool, unsigned long size) {
  unsigned long aligned_size = (size + 7) & ~7UL;
  pool->used -= aligned_size;
}

// Reset pool to beginning - inline for speed
static inline void pool_reset(memory_pool *pool) { pool->used = 0; }

// Destroy pool
static inline void pool_destroy(memory_pool *pool) {
  free(pool->buffer);
  pool->buffer = NULL;
  pool->size = 0;
  pool->used = 0;
}

// Optimized bytes struct
typedef struct {
  unsigned long len;
  char *contents;
} bytes;

typedef struct {
  const unsigned int count;
  unsigned long min_len;
  unsigned long max_len;
  bytes *values;
} input_buffers;

typedef struct {
  const unsigned int input_count;
  unsigned long const_count;
  bytes *constants;
  bytes **variables;
} output_buffers;

// Print bytes - optimized to avoid temporary allocation for small strings
static void print_bytes(bytes b) {
  if (b.contents == NULL) {
    return;
  }

  // For small strings, use stack buffer
  if (b.len < 4096) {
    char tmpContents[4096];
    memcpy(tmpContents, b.contents, b.len);
    tmpContents[b.len] = 0;
    fprintf(stderr, "%s", tmpContents);
  } else {
    // For large strings, allocate temporarily
    memory_pool tmp_pool;
    pool_init(&tmp_pool, b.len + 1);
    char *tmpContents = pool_alloc(&tmp_pool, b.len + 1);
    memcpy(tmpContents, b.contents, b.len);
    tmpContents[b.len] = 0;
    fprintf(stderr, "%s", tmpContents);
    pool_destroy(&tmp_pool);
  }
}

static void print_output(output_buffers *output) {
  const unsigned int input_count = output->input_count;
  const unsigned int const_count = output->const_count;

  for (unsigned long const_i = 0; const_i <= const_count; const_i++) {
    fprintf(stderr, "(");
    for (unsigned int input_i = 0; input_i < input_count; input_i++) {
      if (input_i > 0) {
        fprintf(stderr, "|");
      }
      print_bytes(output->variables[const_i][input_i]);
    }
    fprintf(stderr, ")");
    print_bytes(output->constants[const_i]);
  }
  fprintf(stderr, "\n");
}

static void print_input(input_buffers *input) {
  const unsigned int input_count = input->count;
  const bytes *values = input->values;

  for (int i = 0; i < input_count; i++) {
    fprintf(stderr, "%d: ", i);
    print_bytes(values[i]);
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}

static inline void add_variables(input_buffers *input, output_buffers *output) {
  const unsigned int input_count = input->count;
  const unsigned long const_count = output->const_count;
  const bytes *input_values = input->values;
  bytes **output_values = output->variables;

  // Unroll for common cases
  bytes *out = output_values[const_count];
  for (unsigned int input_i = 0; input_i < input_count; input_i++) {
    out[input_i].len = input_values[input_i].len;
    out[input_i].contents = input_values[input_i].contents;
  }
}

static inline void add_constant(input_buffers *input, output_buffers *output,
                                unsigned long match_index,
                                unsigned long matched_len) {

  bytes *con = &output->constants[output->const_count];
  con->len = matched_len;
  con->contents = input->values[0].contents + match_index;
  output->const_count++;
}

// Optimized memcmp that uses native word size comparisons
static inline int fast_memcmp(const char *s1, const char *s2, unsigned long n) {
  if (n <= 4) {
    for (unsigned long i = 0; i < n; i++) {
      if (s1[i] != s2[i])
        return 1;
    }
    return 0;
  }

  if (n <= 8) {
    // Use 64-bit comparison
    uint64_t v1, v2;
    memcpy(&v1, s1, 8);
    memcpy(&v2, s2, 8);
    return v1 != v2;
  }

  if (n <= 16) {
    // Use two 64-bit comparisons with overlap
    uint64_t v1a, v2a, v1b, v2b;
    memcpy(&v1a, s1, 8);
    memcpy(&v2a, s2, 8);
    memcpy(&v1b, s1 + n - 8, 8);
    memcpy(&v2b, s2 + n - 8, 8);
    return (v1a != v2a) || (v1b != v2b);
  }

  if (n <= 32) {
    // Use four 64-bit comparisons with overlap for 17-32 bytes
    uint64_t v1a, v2a, v1b, v2b, v1c, v2c, v1d, v2d;
    memcpy(&v1a, s1, 8);
    memcpy(&v2a, s2, 8);
    memcpy(&v1b, s1 + 8, 8);
    memcpy(&v2b, s2 + 8, 8);
    memcpy(&v1c, s1 + n - 16, 8);
    memcpy(&v2c, s2 + n - 16, 8);
    memcpy(&v1d, s1 + n - 8, 8);
    memcpy(&v2d, s2 + n - 8, 8);
    return (v1a != v2a) || (v1b != v2b) || (v1c != v2c) || (v1d != v2d);
  }

  return memcmp(s1, s2, n);
}

// Simple string search with first-character heuristic
static inline unsigned long simple_search(const char *haystack, unsigned long haystack_len,
                                          const char *needle, unsigned long needle_len) {
  if (needle_len > haystack_len) return haystack_len + 1;

  unsigned long max_pos = haystack_len - needle_len;
  char first = needle[0];

  for (unsigned long pos = 0; pos <= max_pos; pos++) {
    if (haystack[pos] == first) {
      if (needle_len == 1 || fast_memcmp(needle + 1, haystack + pos + 1, needle_len - 1) == 0) {
        return pos;
      }
    }
  }

  return haystack_len + 1;
}

// Boyer-Moore-Horspool string search for longer patterns
static inline unsigned long bmh_search(const char *haystack, unsigned long haystack_len,
                                       const char *needle, unsigned long needle_len) {
  if (needle_len > haystack_len) return haystack_len + 1;

  // For very short needles, simple search is faster
  if (needle_len <= 3) {
    return simple_search(haystack, haystack_len, needle, needle_len);
  }

  // Build bad character table
  unsigned long bad_char[256];
  for (int i = 0; i < 256; i++) {
    bad_char[i] = needle_len;
  }
  for (unsigned long i = 0; i < needle_len - 1; i++) {
    bad_char[(unsigned char)needle[i]] = needle_len - 1 - i;
  }

  unsigned long pos = 0;
  unsigned long max_pos = haystack_len - needle_len;

  while (pos <= max_pos) {
    if (fast_memcmp(needle, haystack + pos, needle_len) == 0) {
      return pos;
    }
    pos += bad_char[(unsigned char)haystack[pos + needle_len - 1]];
  }

  return haystack_len + 1;
}

// Find the longest common substring - heavily optimized
static unsigned long longest_commong_substring(input_buffers input,
                                               unsigned long *match_indices,
                                               memory_pool *pool) {

  const unsigned int input_count = input.count;
  const unsigned long min_len = input.min_len;

  if (min_len == 0)
    return 0;

  const unsigned long input_len_0 = input.values[0].len;
  const char *input_contents_0 = input.values[0].contents;
  const bytes *input_values = input.values;

  unsigned long *tmp_match_indices =
      pool_alloc(pool, sizeof(unsigned long) * input_count);

  unsigned long matched_len = 0;
  unsigned long upper_bound = min_len;
  unsigned long lower_bound = 1;

  // Optimize: start with larger initial guess based on heuristic
  unsigned long initial_guess = min_len / 4;
  if (initial_guess > 16) {
    lower_bound = initial_guess;
  }

  while (lower_bound <= upper_bound) {
    unsigned long subset_len = (upper_bound + lower_bound) / 2;
    unsigned long max_start = input_len_0 - subset_len;
    int found = 0;

    // Adaptive stride: larger for longer patterns
    unsigned long stride = (subset_len > 32) ? (subset_len / 2) :
                          (subset_len > 16) ? (subset_len / 4) : 1;

    // Outer loop over first string
    for (unsigned long start_index_1 = 0; start_index_1 <= max_start;
         start_index_1 += stride) {
      const char *needle = input_contents_0 + start_index_1;
      unsigned int subset_index = input_count - 1;

      // Check all other strings
      while (subset_index > 0) {
        const bytes *val = &input_values[subset_index];
        const char *haystack = val->contents;
        const unsigned long haystack_len = val->len;

        // Use Boyer-Moore-Horspool for longer patterns, simple search for short
        unsigned long pos;
        if (subset_len > 8) {
          pos = bmh_search(haystack, haystack_len, needle, subset_len);
        } else {
          pos = simple_search(haystack, haystack_len, needle, subset_len);
        }

        if (pos > haystack_len - subset_len) {
          break;
        }

        tmp_match_indices[subset_index] = pos;
        subset_index--;
      }

      if (subset_index == 0) {
        // Found match in all strings
        memcpy(match_indices, tmp_match_indices,
               sizeof(unsigned long) * input_count);
        match_indices[0] = start_index_1;
        matched_len = subset_len;
        found = 1;
        break;
      }
    }

    // If stride search didn't find anything, do fine-grained search
    if (!found && stride > 1) {
      unsigned long search_start = (matched_len > 0) ?
        (match_indices[0] > stride ? match_indices[0] - stride : 0) : 0;
      unsigned long search_end = (matched_len > 0) ?
        (match_indices[0] + stride < max_start ? match_indices[0] + stride : max_start) : max_start;

      for (unsigned long start_index_1 = search_start; start_index_1 <= search_end;
           start_index_1++) {
        const char *needle = input_contents_0 + start_index_1;
        unsigned int subset_index = input_count - 1;

        while (subset_index > 0) {
          const bytes *val = &input_values[subset_index];
          const char *haystack = val->contents;
          const unsigned long haystack_len = val->len;
          unsigned long max_pos = haystack_len - subset_len;

          unsigned long pos = 0;
          int found_in_string = 0;

          while (pos <= max_pos) {
            if (fast_memcmp(needle, haystack + pos, subset_len) == 0) {
              tmp_match_indices[subset_index] = pos;
              found_in_string = 1;
              break;
            }
            pos++;
          }

          if (!found_in_string) {
            break;
          }
          subset_index--;
        }

        if (subset_index == 0) {
          memcpy(match_indices, tmp_match_indices,
                 sizeof(unsigned long) * input_count);
          match_indices[0] = start_index_1;
          matched_len = subset_len;
          found = 1;
          break;
        }
      }
    }

    if (found) {
      lower_bound = subset_len + 1;
    } else {
      upper_bound = subset_len - 1;
    }
  }

  pool_free_last(pool, sizeof(unsigned long) * input_count);
  return matched_len;
}

// Minimize the distance between the substrings - optimized
static void minimize_distance(input_buffers *input,
                              unsigned long *match_indices,
                              unsigned long matched_len, memory_pool *pool) {

  const unsigned int input_count = input->count;
  const char *input_contents_0 = input->values[0].contents;
  const char *pattern = input_contents_0 + match_indices[0];

  // Allocate all buffers at once for better cache locality
  unsigned long *match_counts =
      pool_alloc(pool, sizeof(unsigned long) * input_count);
  unsigned long *current_match_indices =
      pool_alloc(pool, sizeof(unsigned long) * input_count);
  unsigned long *best_match_indices =
      pool_alloc(pool, sizeof(unsigned long) * input_count);

  // Pre-allocate space for all match lists
  unsigned long total_matches = 0;
  unsigned long match_offsets[input_count];

  // First pass: count matches using optimized search with first-char heuristic
  char first_char = pattern[0];
  for (unsigned int i = 0; i < input_count; i++) {
    match_offsets[i] = total_matches;
    const bytes *val = &input->values[i];
    unsigned long count = 0;

    // Use first-character heuristic for counting
    for (unsigned long j = 0; j <= val->len - matched_len; j++) {
      if (val->contents[j] == first_char &&
          fast_memcmp(val->contents + j, pattern, matched_len) == 0) {
        count++;
      }
    }

    match_counts[i] = count;
    total_matches += count;
  }

  // Early exit if only one match per string
  int single_match = 1;
  for (unsigned int i = 0; i < input_count; i++) {
    if (match_counts[i] != 1) {
      single_match = 0;
      break;
    }
  }

  if (single_match) {
    pool_free_last(pool, sizeof(unsigned long) * input_count); // best
    pool_free_last(pool, sizeof(unsigned long) * input_count); // current
    pool_free_last(pool, sizeof(unsigned long) * input_count); // counts
    return;
  }

  // Allocate single contiguous buffer for all matches
  unsigned long *all_matches =
      pool_alloc(pool, sizeof(unsigned long) * total_matches);
  unsigned long **match_indices_list =
      pool_alloc(pool, sizeof(unsigned long *) * input_count);

  // Second pass: store matches
  for (unsigned int i = 0; i < input_count; i++) {
    match_indices_list[i] = all_matches + match_offsets[i];
    const bytes *val = &input->values[i];
    unsigned long count = 0;

    for (unsigned long j = 0; j <= val->len - matched_len; j++) {
      if (val->contents[j] == first_char &&
          fast_memcmp(val->contents + j, pattern, matched_len) == 0) {
        match_indices_list[i][count++] = j;
      }
    }

    // Initialize to last match
    current_match_indices[i] = match_indices_list[i][match_counts[i] - 1];
  }

  memcpy(best_match_indices, current_match_indices,
         sizeof(unsigned long) * input_count);

  unsigned long min_distance = ULONG_MAX;

  // Optimize the distance minimization loop
  while (1) {
    unsigned long current_min = ULONG_MAX;
    unsigned long current_max = 0;
    unsigned int current_max_index = 0;

    // Find min and max in one pass
    for (unsigned int i = 0; i < input_count; i++) {
      unsigned long val = current_match_indices[i];
      if (val < current_min)
        current_min = val;
      if (val > current_max) {
        current_max = val;
        current_max_index = i;
      }
    }

    unsigned long distance = current_max - current_min;
    if (distance < min_distance) {
      min_distance = distance;
      memcpy(best_match_indices, current_match_indices,
             sizeof(unsigned long) * input_count);
    }

    if (match_counts[current_max_index] == 0) {
      break;
    }

    current_match_indices[current_max_index] =
        match_indices_list[current_max_index]
                          [--match_counts[current_max_index]];
  }

  memcpy(match_indices, best_match_indices,
         sizeof(unsigned long) * input_count);

  // Free in reverse order
  pool_free_last(pool, sizeof(unsigned long *) * input_count);
  pool_free_last(pool, sizeof(unsigned long) * total_matches);
  pool_free_last(pool, sizeof(unsigned long) * input_count); // best
  pool_free_last(pool, sizeof(unsigned long) * input_count); // current
  pool_free_last(pool, sizeof(unsigned long) * input_count); // counts
}

// Process function - optimized with better memory management
static void process(input_buffers *input, output_buffers *output,
                    memory_pool *pool) {

  const unsigned int input_count = input->count;
  const unsigned long saved_min_len = input->min_len;

  unsigned long *match_indices =
      pool_alloc(pool, sizeof(unsigned long) * input_count);
  unsigned long *saved_lengths =
      pool_alloc(pool, sizeof(unsigned long) * input_count);

  const unsigned long matched_len =
      longest_commong_substring(*input, match_indices, pool);

  if (matched_len == 0) {
    add_variables(input, output);
    pool_free_last(pool, sizeof(unsigned long) * input_count * 2);
    return;
  }

  minimize_distance(input, match_indices, matched_len, pool);

  // Save lengths
  bytes *values = input->values;
  for (unsigned int i = 0; i < input_count; i++) {
    saved_lengths[i] = values[i].len;
  }

  // Process prefix
  char nonempty = 0;
  for (unsigned int i = 0; i < input_count; i++) {
    unsigned long idx = match_indices[i];
    if (idx > 0)
      nonempty = 1;
    values[i].len = idx;
    if (idx < input->min_len) {
      input->min_len = idx;
    }
  }

  if (nonempty) {
    process(input, output, pool);
  }

  // Restore
  for (unsigned int i = 0; i < input_count; i++) {
    values[i].len = saved_lengths[i];
  }
  input->min_len = saved_min_len;

  add_constant(input, output, match_indices[0], matched_len);

  // Process suffix
  nonempty = 0;
  for (unsigned int i = 0; i < input_count; i++) {
    unsigned long offset = matched_len + match_indices[i];
    if (offset < values[i].len)
      nonempty = 1;
    values[i].contents += offset;
    unsigned long new_len = values[i].len - offset;
    if (new_len < input->min_len) {
      input->min_len = new_len;
    }
    values[i].len = new_len;
  }

  if (nonempty) {
    process(input, output, pool);
  }

  // Restore
  for (unsigned int i = 0; i < input_count; i++) {
    values[i].contents -= matched_len + match_indices[i];
  }
  input->min_len = saved_min_len;

  pool_free_last(pool, sizeof(unsigned long) * input_count * 2);
}

int main(int argc, char **argv) {

  char **inputs;
  unsigned long input_count = 0;
  if (argc < 2) {
    fprintf(stderr, "Error: No input provided\n");
    exit(EXIT_FAILURE);
  }
  inputs = argv + 1;
  input_count = argc - 1;

  const unsigned long test_count = 10000;

  // Pre-calculate input lengths once
  unsigned long *input_lengths = malloc(sizeof(unsigned long) * input_count);
  unsigned long max_len = 0;
  unsigned long min_len = ULONG_MAX;

  for (unsigned int i = 0; i < input_count; i++) {
    unsigned long len = strlen(inputs[i]);
    input_lengths[i] = len;
    if (len < min_len)
      min_len = len;
    if (len > max_len)
      max_len = len;
  }

  // Initialize memory pool with larger size for better performance
  memory_pool pool;
  pool_init(&pool, 1 * 1024 * 1024);

  for (unsigned long i = 0; i < test_count; i++) {
    // Reset pool for each iteration
    pool_reset(&pool);

    // Allocate structures for this iteration
    bytes *input_values = pool_alloc(&pool, sizeof(bytes) * input_count);
    bytes *output_constants = pool_alloc(&pool, sizeof(bytes) * min_len);
    bytes **output_vars = pool_alloc(&pool, sizeof(bytes *) * min_len);
    bytes *var_list = pool_alloc(&pool, sizeof(bytes) * input_count * min_len);

    for (unsigned int j = 0; j < min_len; j++) {
      output_vars[j] = var_list + j * input_count;
    }

    // Initialize input values
    for (unsigned int j = 0; j < input_count; j++) {
      input_values[j].len = input_lengths[j];
      input_values[j].contents = inputs[j];
    }

    input_buffers input = {input_count, min_len, max_len, input_values};
    output_buffers output = {input_count, 0, output_constants, output_vars};

    process(&input, &output, &pool);

    if (i == test_count - 1) {
      print_output(&output);
    }
  }

  free(input_lengths);
  pool_destroy(&pool);
  exit(EXIT_SUCCESS);
}

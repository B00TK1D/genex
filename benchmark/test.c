#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#define MAX_FILES 100
#define MAX_FILE_CONTENT 1000

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    char *directory = argv[1];
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("Error opening directory");
        return 1;
    }

    struct dirent *entry;
    char *file_contents[MAX_FILES];
    int num_files = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Only process regular files
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);

            printf("Processing file: %s\n", file_path);

            FILE *file = fopen(file_path, "r");
            if (!file) {
                perror("Error opening file");
                closedir(dir);
                return 1;
            }

            char *content = (char *)malloc(MAX_FILE_CONTENT * sizeof(char));
            if (!content) {
                perror("Memory allocation error");
                fclose(file);
                closedir(dir);
                return 1;
            }

            size_t bytes_read = fread(content, sizeof(char), MAX_FILE_CONTENT, file);
            if (bytes_read < 0) {
                perror("Error reading file");
                free(content);
                fclose(file);
                closedir(dir);
                return 1;
            }
            content[bytes_read] = '\0';

            file_contents[num_files] = content;
            num_files++;

            fclose(file);
        }
    }

    closedir(dir);

    // Now you can use the file_contents array
    for (int i = 0; i < num_files; i++) {
        printf("File %d content:\n%s\n", i + 1, file_contents[i]);
        free(file_contents[i]);
    }

    return 0;
}
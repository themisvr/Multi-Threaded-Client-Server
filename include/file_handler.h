#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>


// true if file exists
bool file_exists(const char *path) __attribute__((nonnull (1)));

// true if given path is directory (in unix-like systems everythings is being treated as file)
bool file_is_dir(const char *path)  __attribute__((nonnull (1)));

// returns the size of the given file/directory
off_t file_size(const char *path)  __attribute__((nonnull (1)));

// returns the number of files/directories (type) in a directory
// in case of directories we exclude the ./ and ../ directories
uint32_t n_files_in_directory(const char *path, const char *type)  __attribute__((nonnull (1, 2)));

// returns an array of names for each subdirectory for a given directory
char **subdir_names(const char *dirpath, size_t n_dirs) __attribute__ ((nonnull (1)));


#endif // FILE_HANDLER_H
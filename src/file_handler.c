#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include "file_handler.h"


bool file_exists(const char *path) {
	struct stat info;
	return (stat(path, &info) == 0);
}


bool file_is_dir(const char *path) {
	struct stat info;
	if (stat(path, &info) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	return 	((info.st_mode & S_IFMT) == S_IFDIR);
}


off_t file_size(const char *file) {
	struct stat info;
	if (stat(file, &info) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}
	return info.st_size;
}


uint32_t n_files_in_directory(const char *dirpath, const char *type) {
	struct dirent *dir;
	DIR *dirp;
	uint32_t n_files = 0, n_dirs = 0;
	
	if ((dirp = opendir(dirpath)) == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}
	while ((dir = readdir(dirp)) != NULL) {
		if (dir->d_type == DT_REG) {
			++n_files;
		}
		if (dir->d_type == DT_DIR) {
			++n_dirs;
		}
	}
	if (closedir(dirp) < 0) {
		perror("closedir");
		exit(EXIT_FAILURE);
	}
	return ((strncmp("file", type, strlen("file")) == 0) ? n_files : (n_dirs - 2));
}


char **subdir_names(const char *dirpath, size_t n_dirs) {
	struct dirent *dir;
	DIR *dirp;

	// we assume that maximum name size for the subdirectories is 20 bytes
	char **subdirs = malloc(n_dirs * sizeof(char *));
	if (!subdirs) {
		fprintf(stderr, "Error: Could not allocate enough memory for subdirs' names\n");
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i != n_dirs; ++i) {
		subdirs[i] = malloc(50 * sizeof(char));
		if (!subdirs[i]) {
			fprintf(stderr, "Error: Could not allocate enough memory for subdirs' names\n");
			exit(EXIT_FAILURE);
		}
		memset(subdirs[i], '\0', 50);
	}
	if ((dirp = opendir(dirpath)) == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}
	size_t ith_index = 0;
	while ((dir = readdir(dirp)) != NULL) {
		if ((strncmp(dir->d_name, ".", strlen(dir->d_name)) == 0) || (strncmp(dir->d_name, "..", strlen(dir->d_name)) == 0)) {
			continue;
		}
		/* add their parent directory so to create the path that we will pass to the processes */
		char *subdir_path = (char *) malloc(strlen(dirpath) + strlen(dir->d_name) + strlen("/") + 1);
		if (!subdir_path) {
			fprintf(stderr, "Error: Could not allocate enough memory for subdir_path\n");
			exit(EXIT_FAILURE);
		}
		memset(subdir_path, '\0', strlen(dirpath) + strlen(dir->d_name) + strlen("/") + 1);
		strncpy(subdir_path, dirpath, strlen(dirpath));
		strncat(subdir_path, "/", strlen("/"));		
		strncat(subdir_path, dir->d_name, strlen(dir->d_name));
		subdir_path[strlen(subdir_path)] = '\0';
		strncpy(subdirs[ith_index], subdir_path, strlen(subdir_path));
		++ith_index;
		free(subdir_path);
	}
	if (closedir(dirp) < 0) {
		perror("closedir");
		exit(EXIT_FAILURE);
	}
	return subdirs;
}

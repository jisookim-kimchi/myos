#ifndef PATHPARSER_H
#define PATHPARSER_H

#define MYOS_MAX_PATH_LENGTH 108

#include "../string/string.h"
#include "../memory/memory.h"
#include "../status.h"
#include "../memory/heap/kernel_heap.h"

typedef struct path_root
{
    int drive_number;
    struct path_part *first;
} path_root_t;

typedef struct path_part
{
    char *part;
    struct path_part *next;
} path_part_t;


//0:/test.txt 면 0: root_node , test.txt는 next node 
path_root_t *parse_path(const char *path, const char *cur_dir_path);
void release_path_parts(path_root_t *root);
#endif
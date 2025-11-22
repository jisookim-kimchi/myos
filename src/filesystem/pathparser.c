#include "pathparser.h"

static int path_valid_format(const char *filename)
{
    if (!filename)
        return -MYOS_INVALID_ARG;
    int len = ft_strlen(filename);
    if (len < 3)
        return -MYOS_INVALID_ARG;

    if (!ft_isdigit(filename[0]))
        return -MYOS_INVALID_ARG;
    
    if (filename[1] != ':')
        return -MYOS_INVALID_ARG;
    if (filename[2] != '/')
        return -MYOS_INVALID_ARG;

    return 1;
    // because we are looking for something like this 5:/
    // return (len >= 3 && ft_isdigit(filename[0]) && filename[1] == ':' && filename[2] == '/');
}

static int get_drive_path(const char **path)
{
    if(!path_valid_format(*path))
    {
        return -MYOS_BAD_PATH;
    }

    int drive_number = ft_to_numeric_digit((*path)[0]);
    *path += 3; // skip "X:/"

    return drive_number;
}

static path_root_t* create_root(int drive_num)
{
    path_root_t* root = kernel_zero_alloc(sizeof(path_root_t));
    if (!root)
    {
        return NULL;
    }
    root->drive_number = drive_num;
    root->first = NULL;
    return root;
}

static const char* get_path_part(const char **path)
{
    // char *result = kernel_zero_alloc(MYOS_MAX_PATH_LENGTH);
    // int i = 0;
    // while (**path != '/' && **path != '\0' && i < MYOS_MAX_PATH_LENGTH - 1)
    // {
    //     result[i] = **path;
    //     (*path)++;
    //     i++;
    // }
    // result[i] = '\0';
    // if (**path == '/')
    // {
    //     (*path)++;
    // }
    // if(i == 0)
    // {
    //     kernel_free(result);
    //     result = NULL;
    // }
    // return result;
    const char *start = *path;
    int len = 0;

    while (start[len] != '/' && start[len] != '\0')
        len++;

    if (len == 0)
    {
        if (start[len] == '/')
            (*path)++;
        return NULL;
    }

    char *result = kernel_zero_alloc(len + 1);
    if (!result)
        return NULL;

    for (int i = 0; i < len; i++)
        result[i] = start[i];

    result[len] = '\0';

    *path += len;
    if (**path == '/')
        (*path)++;

    return result;
}

path_part_t* parse_path_parts(path_part_t *last, const char **path)
{
    const char *path_part_str = get_path_part(path);
    if (!path_part_str)
    {
        return NULL;
    }

    path_part_t *new_part = kernel_zero_alloc(sizeof(path_part_t));
    if (!new_part)
    {
        kernel_free((void*)path_part_str);
        return NULL;
    }
    new_part->part =  (char*)path_part_str;
    new_part->next = NULL;

    if (last)
    {
        last->next = new_part;
    }

    return new_part;
}

void release_path_parts(path_root_t *root)
{
    path_part_t *current = root->first;
    while (current)
    {
        path_part_t *next = current->next;
        kernel_free(current->part);
        kernel_free(current);
        current = next;
    }
    kernel_free(root);
}

path_root_t *parse_path(const char *path, const char *cur_dir_path)
{
    int res = 0;
    const char *temp_path = path;
    path_root_t *root = NULL;

    if(ft_strlen(path) > MYOS_MAX_PATH_LENGTH)
    {
        goto out;
    }

    res = get_drive_path(&temp_path);
    if (res < 0)
    {
        goto out;
    }

    root = create_root(res);
    if (!root)
    {
        goto out;
    }

    path_part_t* first = parse_path_parts(NULL, &temp_path);
    if (!first)
    {
        release_path_parts(root);
        root = NULL;
        goto out;
    }

    root->first = first;

    path_part_t *last = first;
    while (1)
    {
        path_part_t *next = parse_path_parts(last, &temp_path);
        if (!next)
            break;
        last = next;
    }

out:
    return root;
}
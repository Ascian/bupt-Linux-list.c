#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

#define SIZE_MAX LONG_MAX             // The maximum size of a file
#define SIZE_MIN 0                    // The minimum size of a file
#define TIME_MAX LONG_MAX             // The maximum time limit
#define ERROR_MESSAGE_LENGTH PATH_MAX // The maximum length of an error message

#define TRUE 1
#define FALSE 0

typedef char Status;

// Error type
typedef enum Error
{
    SUCCESS,
    OPTION_INVALID,
    ARGUMENT_INVALID,
    ARGUMENT_MISSING,
    ARGUMENT_TOO_LARGE,
    PATH_TOO_LONG,
    PATH_INVALID,
    PATH_UNACCESSIBLE,
} Error;

long min_size = SIZE_MIN;                     // -l option
long max_size = SIZE_MAX;                     // -h option
long time_limit = TIME_MAX;                   // -m option
char is_all = 0;                              // -a option
char is_recursive = 0;                        // -r option
char path_amount = 1;                         // amount of path argument
char **path;                                  // path argument to list
char error_message[ERROR_MESSAGE_LENGTH + 1]; // store error message

// Analyze the arguments
// param:
//     argc - the amount of arguments
//     argv - the arguments
void CmdArgumentAnalyze(const int argc, const char **const argv);

// List files within the given paths
void FileList();

int main(int argc, char **argv)
{
    CmdArgumentAnalyze(argc, argv);

    FileList();

    return 0;
}

// Print the error message
// param:
//    error_str - the string to store the error message
//    error - the error code
void ErrorPrint(char *error_str, const Error error)
{
    switch (error)
    {
    case OPTION_INVALID:
        sprintf(error_str, "ls: invalid option '%s'", error_message);
        break;
    }
} // TODO

// Check if the error is not SUCCESS and print the error message
// param:
//     error - the error code
inline void ErrorCheck(const Error error)
{
    if (error != SUCCESS)
    {
        char error_str[2 * ERROR_MESSAGE_LENGTH + 1];
        ErrorPrint(error_str, error);
        fprintf(stderr, "%s", error_str);
        exit(EXIT_FAILURE);
    }
}

// check the argument is a option
// param:
//     x - the argument to check
#define IS_OPTION(x) ((x)[0] == '-' && (x)[1] != '\0')

// transform the argument to long type
// param:
//     val - the variable to store the result
//     argv - the argument to transform
#define PARSE_ARGUMENT(val, argv)                 \
    do                                            \
    {                                             \
        if ((argv) == NULL)                       \
        {                                         \
            return ARGUMENT_MISSING;              \
        }                                         \
        char *end_pointer;                        \
        (val) = strtol((argv), &end_pointer, 10); \
        if (*end_pointer != '\0' || (val) < 0)    \
        {                                         \
            strcpy(error_message, (argv));        \
            return ARGUMENT_INVALID;              \
        }                                         \
        else if ((val) == LONG_MAX)               \
        {                                         \
            strcpy(error_message, (argv));        \
            return ARGUMENT_TOO_LARGE;            \
        }                                         \
    } while (0)

// Analyze the options
// param:
//     index - the index of the argument to analyze
//     argc - the amount of arguments
//     argv - the arguments
// return:
//     the error code
const Error CmdOptionAnalyze(int *index, const int argc, const char **const argv)
{
    for (; *index < argc && IS_OPTION(argc[*index]); (*index)++)
    {
        switch (argv[*index][1])
        {
        case 'a':
            is_all = 1;
            break;
        case 'r':
            is_recursive = 1;
            break;
        case 'l':
            (*index)++;
            PARSE_ARGUMENT(min_size, argv[*index]);
            break;
        case 'h':
            (*index)++;
            PARSE_ARGUMENT(max_size, argv[*index]);
            break;
        case 'm':
            (*index)++;
            PARSE_ARGUMENT(time_limit, argv[*index]);
            break;
        case '-':
            return SUCCESS;
        default:
            strcpy(error_message, argv[*index]);
            return OPTION_INVALID;
        }
    }
    return SUCCESS;
}

// Analyze the paths
// param:
//     index - the index of the argument to analyze
//     argc - the amount of arguments
//     argv - the arguments
// return:
//     the error code
const Error CmdPathAnalyze(int *index, const int argc, const char **const argv)
{
    if (*index == argc)
    {
        // No path argument
        path_amount = 1;
        path = (char **)malloc(sizeof(char *));
        path[0] = ".";
    }
    else
    {
        path_amount = argc - *index;
        path = argv + *index;
        for (int i = 0; i < path_amount; i++)
        {
            if (strlen(path[i]) > PATH_MAX)
            {
                strncpy(error_message, path[i], ERROR_MESSAGE_LENGTH);
                error_message[ERROR_MESSAGE_LENGTH] = '\0';
                return PATH_TOO_LONG;
            }
        }
    }
    return SUCCESS;
}

// Analyze the arguments
void CmdArgumentAnalyze(const int argc, const char **const argv)
{
    int i;
    // Analyze options
    ErrorCheck(CmdOptionAnalyze(&i, argc, argv));
    // Analyze paths
    ErrorCheck(CmdPathAnalyze(&i, argc, argv));
    return SUCCESS;
}

// Path information
typedef struct Path
{
    struct stat st;
    char *path_str;
    struct Path *next;
} Path;

// Classify the paths into files and dictionaries
// param:
//    file_paths - the header of the file path list
//    dir_paths - the header of the directory path list
// return:
//    the error code
const Error PathClassify(Path *file_paths, Path *dir_paths)
{
    for (int i = 0; i < path_amount; i++)
    {
        Path *path = (Path *)malloc(sizeof(Path));
        path->path_str = path[i];
        if (stat(path->path_str, &path->st) == -1)
        {
            strcpy(error_message, path->path_str);
            return PATH_INVALID;
        }
        path->next = NULL;
        // if the path is a directory, add it to the directory list
        // otherwise add it to the file list
        if (S_ISDIR(path->st.st_mode))
        {
            dir_paths->next = path;
            dir_paths = path;
        }
        else
        {
            dir_paths->next = path;
            dir_paths = path;
        }
    }
    return SUCCESS;
}

// transform the day to second
#define DAY_TO_SECOND(day) ((day)*3600)

// Check the file is valid to print
// param:
//     st - the file information
// return:
//     the file is valid or not
inline const Status FileCheck(const struct stat st)
{
    time_t current_time;
    time(&current_time);
    return current_time - st.st_mtime <= DAY_TO_SECOND(time_limit) &&
           st.st_size <= max_size && st.st_size >= min_size
}

// List file paths
// param:
//     file_paths - the header of the file path list
// return:
//     the error code
const Error FilePathList(const Path *file_paths)
{
    file_paths = file_paths->next;
    while (file_paths != NULL)
    {
        if (FileCheck(file_paths->st))
        {
            printf("%s ", file_paths->path_str);
        }
        file_paths = file_paths->next;
    }
    return SUCCESS;
}

#define DICT_PATH_LIST(dir_statement, file_statement)                         \
    do                                                                        \
    {                                                                         \
        DIR dir * = opendir(path);                                            \
        /* Check the dictionary is accessible */                              \
        if (!dir)                                                             \
        {                                                                     \
            strcpy(error_message, path);                                      \
            return PATH_UNACCESSIBLE;                                         \
        }                                                                     \
                                                                              \
        struct dirent *dp;                                                    \
        dp = readdir(dir);                                                    \
        while (dp != NULL)                                                    \
        {                                                                     \
            /* Ignore the file starts with '.' if the option -a is not set */ \
            if (dp->d_name[0] == '.' && !is_all)                              \
            {                                                                 \
                continue;                                                     \
            }                                                                 \
            if (dp->d_type == DT_DIR)                                         \
            {                                                                 \
                dir_statement                                                 \
            }                                                                 \
            else                                                              \
            {                                                                 \
                struct stat st;                                               \
                stat(subpath, &st);                                           \
                if (FileCheck(st))                                            \
                {                                                             \
                    file_statement                                            \
                }                                                             \
            }                                                                 \
            dp = readdir(dir);                                                \
        }                                                                     \
        closedir(dir);                                                        \
        return SUCCESS;                                                       \
    } while (0)

// Recursively list all the files under the dictionary
// param:
//     dic_path - the dictionary path
// return:
//     the error code
const Error DictPathListRecursive(const char *const path)
{
    DICT_PATH_LIST(
        {
            printf("%s\n", subpath);
            DictPathRecursive(subpath);
        },
        {
            printf("%s\n", subpath);
        })
}

// List all the files under the dictionary
// param:
//     dic_path - the dictionary path
// return:
//     the error code
const Error DictPathListNoRecursive(const char *const path)
{
    DICT_PATH_LIST(
        {
            printf("%.*s ", dp->d_namlen, dp->d_name);
        },
        {
            printf("%.*s ", dp->d_namlen, dp->d_name);
        })
}

#define DIR_LIST(dic_function)                         \
    do                                                 \
    {                                                  \
        Path *dir_path = dir_paths->next;              \
        while (dir_path != NULL)                       \
        {                                              \
            printf("%s:\n", dir_path->path_str);       \
            ErrorCheck(dict_function(dir_path->next)); \
            dir_path = dir_path->next;                 \
            printf("\n\n");                            \
        }                                              \
    } while (0)

// List files
void FileList()
{
    // Classify the paths into files and dictionaries
    Path *file_paths = (Path *)malloc(sizeof(Path));
    Path *dir_paths = (Path *)malloc(sizeof(Path));
    file_paths->next = NULL;
    dir_paths->next = NULL;
    ErrorCheck(PathClassify(file_paths, dir_paths));

    if (file_paths->next != NULL)
    {
        ErrorCheck(FilePathList(file_paths));
        printf("\n\n");
    }

    if (dir_paths->next != NULL)
    {
        if (path_amount == 1)
        {
            if (is_recursive)
            {
                ErrorCheck(DictPathListRecursive(dir_paths->next));
            }
            else
            {
                ErrorCheck(DictPathListNoRecursive(dir_paths->next));
            }
        }
        else
        {
            if (is_recursive)
            {
                DIR_LIST(DictPathListRecursive);
            }
            else
            {
                DIR_LIST(DictPathListNoRecursive);
            }
        }
    }
}
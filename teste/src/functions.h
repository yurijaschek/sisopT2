/*****************************************************************************
 *  Instituto de Informatica - Universidade Federal do Rio Grande do Sul     *
 *  INF01142 - Sistemas Operacionais I N                                     *
 *  Test shell for SisOp1's Task 2: T2FS                                     *
 *                                                                           *
 *  Author: Yuri Jaschek <yuri.jaschek@inf.ufrgs.br>                         *
 *                       <yurijaschek@gmail.com>                             *
 *                                                                           *
 *****************************************************************************/

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <map>
#include <string>
#include <vector>

// Recipe for a function definition
#define DEFINE_FUNCTION(ret, nam, ...) ret nam(__VA_ARGS__)

// To define a function name for each funcion code
#define FUNC_NAME(id) f_##id // FN_EXIT's function name becomes f_FN_EXIT

// For declaring terminal functions providing only its code
#define DECL_FUNC(id) \
    DEFINE_FUNCTION(int, FUNC_NAME(id), std::vector<std::string> args)

// For easier declaration of the mapping between codes and terminal functions
#define ADD_TO_MAP(id, usg, dsc) {id, {FUNC_NAME(id), usg, dsc}}

#ifndef MAX_PATH_SIZE
    #define MAX_PATH_SIZE 1024
#endif

enum functions // Functions in the terminal
{
    FN_ABOUT,
    FN_CD,
    FN_CLOSE,
    FN_CMD,
    FN_CMP,
    FN_CP,
    FN_CREATE,
    FN_EXIT,
    FN_FORMAT,
    FN_FSCP,
    FN_LN,
    FN_LS,
    FN_MAN,
    FN_MKDIR,
    FN_OPEN,
    FN_PWD,
    FN_READ,
    FN_RM,
    FN_RMDIR,
    FN_SEEK,
    FN_SETVAR,
    FN_TRUNC,
    FN_WHO,
    FN_WRITE,
};

// Declarations of the terminal functions by its code
DECL_FUNC(FN_ABOUT);
DECL_FUNC(FN_CD);
DECL_FUNC(FN_CLOSE);
DECL_FUNC(FN_CMD);
DECL_FUNC(FN_CMP);
DECL_FUNC(FN_CP);
DECL_FUNC(FN_CREATE);
DECL_FUNC(FN_EXIT);
DECL_FUNC(FN_FORMAT);
DECL_FUNC(FN_FSCP);
DECL_FUNC(FN_LN);
DECL_FUNC(FN_LS);
DECL_FUNC(FN_MAN);
DECL_FUNC(FN_MKDIR);
DECL_FUNC(FN_OPEN);
DECL_FUNC(FN_PWD);
DECL_FUNC(FN_READ);
DECL_FUNC(FN_RM);
DECL_FUNC(FN_RMDIR);
DECL_FUNC(FN_SEEK);
DECL_FUNC(FN_SETVAR);
DECL_FUNC(FN_TRUNC);
DECL_FUNC(FN_WHO);
DECL_FUNC(FN_WRITE);

// Struct to hold terminal function information
struct Function
{
    int (*fn)(std::vector<std::string>); // Function pointer
    std::string usage; // Usage string
    std::string desc; // Description string
};

// List of terminal functions
const std::map<int,Function> fn_lst =
{
    ADD_TO_MAP(FN_ABOUT,  "%s",
                          "Display information about this shell"),
    ADD_TO_MAP(FN_CD,     "%s directory",
                          "Change the current working directory\n"),
    ADD_TO_MAP(FN_CLOSE,  "%s handle",
                          "Close an opened file, given its handle"),
    ADD_TO_MAP(FN_CMD,    "%s",
                          "Display all avaiable commands in this shell"),
    ADD_TO_MAP(FN_CMP,    "%s file1 file2",
                          "Compare two files"),
    ADD_TO_MAP(FN_CP,     "%s src dst",
                          "Copy a file from source to destiny"),
    ADD_TO_MAP(FN_CREATE, "%s file",
                          "Create a new file"),
    ADD_TO_MAP(FN_EXIT,   "%s",
                          "Exit this shell"),
    ADD_TO_MAP(FN_FORMAT, "%s number",
                          "Format the partition 0 using number sectors per block\n" \
                          "Note that \"t2fs_disk.dat\" must be in the same directory as this shell"),
    ADD_TO_MAP(FN_FSCP,   "%s {-f | -t} file1 file2",
                          "Copy a file between filesystems\n" \
                          "-f copies existing file1 in T2FS to file2 in HostFS\n" \
                          "-t copies existing file1 in HostFS to file2 in T2FS"),
    ADD_TO_MAP(FN_LN,     "%s link file",
                          "Create a symbolic link to a file"),
    ADD_TO_MAP(FN_LS,     "%s [directory]",
                          "List directory contents\n" \
                          "If no directory is given, cwd is assumed"),
    ADD_TO_MAP(FN_MAN,    "%s [command]",
                          "Display manual information for commands"),
    ADD_TO_MAP(FN_MKDIR,  "%s directory",
                          "Create a new directory"),
    ADD_TO_MAP(FN_OPEN,   "%s file",
                          "Open an existing file"),
    ADD_TO_MAP(FN_PWD,    "%s",
                          "Display current working directory"),
    ADD_TO_MAP(FN_READ,   "%s handle bytes",
                          "Read size bytes from a file, given its handle"),
    ADD_TO_MAP(FN_RM,     "%s file",
                          "Delete an existing file"),
    ADD_TO_MAP(FN_RMDIR,  "%s directory",
                          "Delete an empty directory"),
    ADD_TO_MAP(FN_SEEK,   "%s handle offset",
                          "Change current pointer to offset in an opened file, given its handle\n" \
                          "An offset of -1 puts the current pointer at EOF (end of file)"),
    ADD_TO_MAP(FN_SETVAR, "%s variable",
                          "Set the variable to the value returned by the previous command\n" \
                          "To refer to a variable set, use a dollar sign before its name"),
    ADD_TO_MAP(FN_TRUNC,  "%s handle",
                          "Truncate an opened file at its current pointer, given its handle\n" \
                          "Truncation deletes all data from (and including) the current pointer"),
    ADD_TO_MAP(FN_WHO,    "%s",
                          "Display the creators of the T2FS library being used"),
    ADD_TO_MAP(FN_WRITE,  "%s handle {-b | -t} [...]",
                          "Write to an opened file, given its handle\n" \
                          "Using -b, bytes are written, passed as decimal numbers from 0 to 255\n" \
                          "Using -t, text is written"),
};

// Mapping between terminal commands and terminal function codes
const std::map<std::string,int> cmd_lst =
{
    {"about", FN_ABOUT},
    {"cd", FN_CD}, {"chdir", FN_CD},
    {"close", FN_CLOSE},
    {"cmd", FN_CMD},
    {"cmp", FN_CMP}, {"diff", FN_CMP},
    {"cp", FN_CP}, {"copy", FN_CP},
    {"create", FN_CREATE},
    {"exit", FN_EXIT}, {"quit", FN_EXIT}, {"q", FN_EXIT},
    {"format", FN_FORMAT}, {"fmt", FN_FORMAT},
    {"fscp", FN_FSCP},
    {"ln", FN_LN}, {"link", FN_LN},
    {"ls", FN_LS}, {"dir", FN_LS},
    {"man", FN_MAN}, {"help", FN_MAN},
    {"mkdir", FN_MKDIR},
    {"open", FN_OPEN},
    {"pwd", FN_PWD}, {"cwd", FN_PWD},
    {"read", FN_READ},
    {"rm", FN_RM}, {"del", FN_RM},
    {"rmdir", FN_RMDIR},
    {"seek", FN_SEEK},
    {"setvar", FN_SETVAR},
    {"trunc", FN_TRUNC},
    {"who", FN_WHO}, {"id", FN_WHO},
    {"write", FN_WRITE},
};

#endif // FUNCTIONS_H

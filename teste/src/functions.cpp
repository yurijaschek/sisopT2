/*****************************************************************************
 *  Instituto de Informatica - Universidade Federal do Rio Grande do Sul     *
 *  INF01142 - Sistemas Operacionais I N                                     *
 *  Test shell for SisOp1's Task 2: T2FS                                     *
 *                                                                           *
 *  Author: Yuri Jaschek <yuri.jaschek@inf.ufrgs.br>                         *
 *                       <yurijaschek@gmail.com>                             *
 *                                                                           *
 *****************************************************************************/

/*
 *
 */

#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "functions.h"
#include "shell.h"
extern "C"
{
    #include "t2fs.h"
}

using namespace std;


/********************************
 *  Terminal functions helpers  *
 ********************************/

/*-----------------------------------------------------------------------------
Funct:  Prints the usage of the given Function, using the string as caller.
Input:  fn  -> Function whose Usage is to be printed
        cmd -> The way the function was called
Return: The function always returns -1.
-----------------------------------------------------------------------------*/
int printUsage(string cmd)
{
    const Function *fn = findFunction(cmd);
    if(!fn)
        return -1;
    printf("Usage: ");
    printf(fn->usage.c_str(), cmd.c_str());
    printf("\n");
    return -1;
}


/*-----------------------------------------------------------------------------
Funct:  Converts a string to an int, returning if it was possible and putting
            the result in ans.
Input:  num -> String to be converted to int
        ans -> Location to put the answer (number of the conversion)
Return: On success, the function returns 0. Otherwise, -1.
-----------------------------------------------------------------------------*/
int stringToInt(string num, int *ans)
{
    try
    {
        *ans = stoi(num);
    }
    catch(const invalid_argument&)
    {
        return setError(-1, "%s: not an integer", num.c_str());
    }
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Creates or opens a file/dir with given path, retrieving its handle.
Input:  path -> Path to the file/dir to be created or opened
        dir  -> Boolean to decide if it is a regular file or directory
        crt  -> Boolean to decide if the file is to be created or opened
Return: On success, the function returns the file handle (> 0).
        On failure, the function returns a negative number.
-----------------------------------------------------------------------------*/
int getHandle(string path, bool dir, bool crt)
{
    char buffer[MAX_PATH_SIZE];
    strncpy(buffer, path.c_str(), sizeof(buffer));
    int handle = dir ? (crt ? mkdir2(buffer) : opendir2(buffer))
                     : (crt ? create2(buffer) : open2(buffer));
    if(handle < 0)
        return setError(handle, "%s: %s %s", path.c_str(),
                        dir ? "directory" : "regular file",
                        crt ? "could not be created" : "does not exist");
    return handle;
}


/*-----------------------------------------------------------------------------
Funct:  Closes a file/dir with given handle.
Input:  handle -> Handle of the file/dir to be closed
        dir    -> Boolean to decide if it is a regular file or directory
Return: On success, the function returns 0.
        On failure, the function returns != 0.
-----------------------------------------------------------------------------*/
int closeFile(int handle, bool dir)
{
    int res = dir ? closedir2(handle) : close2(handle);
    if(res != 0)
        return setError(res, "could not close %s with handle %d",
                        dir ? "dir" : "file", handle);
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Deletes a file/dir with given path.
Input:  path -> Path to the file/dir to be deleted
        dir  -> Boolean to decide if it is a regular file or directory
Return: On success, the function returns 0.
        On failure, the function returns != 0.
-----------------------------------------------------------------------------*/
int deleteFile(string path, bool dir)
{
    char buffer[MAX_PATH_SIZE];
    strncpy(buffer, path.c_str(), sizeof(buffer));
    int res = dir ? rmdir2(buffer) : delete2(buffer);
    if(res != 0)
        return setError(res, "%s: %s could not be deleted",
                        path.c_str(), dir ? "directory" : "file");
    return 0;
}


/*-----------------------------------------------------------------------------
Funct:  Read len bytes from a file, given its handle, and puts it in buffer.
Input:  handle -> Handle of the file to be read from
        buffer -> Buffer to store the data read
        len    -> Number of bytes to read
Return: On success, the function returns the number of bytes read.
        On failure, the function returns a negative number.
-----------------------------------------------------------------------------*/
int readBytes(int handle, char *buffer, int len)
{
    int res = read2(handle, buffer, len);
    if(res < 0)
        return setError(res, "error while trying to read %d byte%s from file "
                        "handle %d", len, len == 1 ? "" : "s", handle);
    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Write len bytes from the buffer to a file, given its handle.
Input:  handle -> Handle of the file to be written to
        buffer -> Buffer that holds the data to be written
        len    -> Number of bytes to write
Return: On success, the function returns the number of bytes written.
        On failure, the function returns a negative number.
-----------------------------------------------------------------------------*/
int writeBytes(int handle, char *buffer, int len)
{
    int res = write2(handle, buffer, len);
    if(res < 0)
        return setError(res, "error while trying to write %d byte%s to file"
                             " handle %d", len, len == 1 ? "" : "s", handle);
    return res;
}


/*-----------------------------------------------------------------------------
Funct:  Prints data on screen, hexadecimal + ASCII.
Input:  buffer  -> Buffer that holds the data to be printed
        len     -> Data bytes to be printed
        counter -> Start of numbering
-----------------------------------------------------------------------------*/
void hexDump(char *buffer, int len, int counter)
{
    for(int i=0; i<len; i+=16)
    {
        printf("%08x", i+counter);
        for(int j=0; j<16; j++)
        {
            if(j%8 == 0)
                printf(" ");
            if(i+j < len)
                printf(" %02x", (unsigned char)buffer[i+j]);
            else
                printf("   ");
        }
        printf("  |");
        for(int j=0; j<16; j++)
            printf("%c", printable(i+j < len ? buffer[i+j] : ' '));
        printf("|\n");
    }
}


/************************************
 *  Terminal functions definitions  *
 ************************************/

/*-----------------------------------------------------------------------------
Funct:  Each function performs a task at the terminal.
Input:  args -> Vector of strings, arguments to the function
Return: Unless noted, a return value of 0 means the command went well.
        Otherwise, if it has been set, the error message contains more
            information about the error.
-----------------------------------------------------------------------------*/


DECL_FUNC(FN_ABOUT)
{
    if(args.size() != 1)
        return printUsage(args[0]);
    printf("Shell made by Yuri Jaschek for SisOp1's *famous* T2FS\n");
    return 0;
}

DECL_FUNC(FN_CD)
{
    if(args.size() > 2)
        return printUsage(args[0]);
    string dir;
    if(args.size() == 2)
        dir = args[1];
    else
        dir = "/";
    char buffer[MAX_PATH_SIZE];
    strncpy(buffer, dir.c_str(), sizeof(buffer));
    if(chdir2(buffer) != 0)
        return setError(-1, "%s: no such directory", dir.c_str());
    int res = getcwd2(buffer, sizeof(buffer));
    if(res != 0)
        return setError(res, "%s: couldn't change to that directory",
                        dir.c_str());
    else
        cwd_path = buffer;
    return 0;
}

DECL_FUNC(FN_CLOSE)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    int handle = 0;
    if(stringToInt(args[1], &handle) != 0)
        return setError(-1, "invalid handle: ");
    return closeFile(handle, false);
}

DECL_FUNC(FN_CMD)
{
    if(args.size() != 1)
        return printUsage(args[0]);
    int W = 79; // Width of the terminal (in characters)
    if(interactive)
    {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        W = ws.ws_col;
    }
    int N = cmd_lst.size(); // Number of commands to print
    int longest = 0;
    vector<string> names;
    for(auto it = cmd_lst.begin(); it != cmd_lst.end(); it++)
    {
        names.push_back(it->first);
        longest = max(longest, (int)it->first.size());
    }
    int n_col = (W+2) / (longest+2);
    n_col = max(1, n_col);
    int n_lin = 1 + (N-1) / n_col;
    for(int i=0; i<n_lin; i++)
    {
        printf("%-*s", longest, names[i].c_str());
        for(int j=i+n_lin; j<N; j+=n_lin)
            printf("  %-*s", longest, names[j].c_str());
        printf("\n");
    }
    return 0;
}

DECL_FUNC(FN_CMP)
{
    if(args.size() != 3)
        return printUsage(args[0]);
    int handle1 = getHandle(args[1], false, false);
    if(handle1 < 0)
        return setError(handle1, "");
    int handle2 = getHandle(args[2], false, false);
    if(handle2 < 0)
    {
        string aux = error_msg;
        closeFile(handle1, false);
        error_msg = aux;
        return setError(handle2, "");
    }
    int bytes1, bytes2;
    char buffer1[1024], buffer2[1024];
    int ans = 0;
    int read = 0;
    while(ans == 0)
    {
        bytes1 = readBytes(handle1, buffer1, sizeof(buffer1));
        if(bytes1 < 0)
        {
            ans = bytes1;
            break;
        }
        bytes2 = readBytes(handle2, buffer2, sizeof(buffer2));
        if(bytes2 < 0)
        {
            ans = bytes2;
            break;
        }
        if(bytes1 != bytes2)
            ans = 2;
        else if(bytes1 == 0)
            break; // Files are equal!
        else for(int i=0; i<bytes1; i++)
        {
            if(buffer1[i] != buffer2[i])
            {
                ans = 1;
                break;
            }
            read++;
        }
    }
    string aux = error_msg;
    closeFile(handle1, false);
    closeFile(handle2, false);
    error_msg = aux;
    if(ans == 1)
        printf("%s %s differ: byte %d", args[1].c_str(),
                        args[2].c_str(), read+1);
    if(ans == 2)
        printf("%s %s differ in size", args[1].c_str(),
                                       args[2].c_str());
    return ans;
}

DECL_FUNC(FN_CP)
{
    if(args.size() != 3)
        return printUsage(args[0]);
    int src = getHandle(args[1], false, false);
    if(src < 0)
        return setError(src, "");
    int dst = getHandle(args[2], false, true);
    if(dst < 0)
    {
        string aux = error_msg;
        closeFile(src, false);
        error_msg = aux;
        return setError(dst, "");
    }
    char buffer[1024];
    int ans = 0;
    for(;;)
    {
        int rb = readBytes(src, buffer, sizeof(buffer));
        if(rb <= 0)
        {
            ans = rb;
            break;
        }
        int wb = writeBytes(dst, buffer, rb);
        if(wb <= 0)
        {
            ans = wb;
            break;
        }
        if(rb != wb)
        {
            ans = setError(-1, "I/O error");
            break;
        }
    }
    string aux = error_msg;
    closeFile(src, false);
    closeFile(dst, false);
    error_msg = aux;
    return ans;
}


/*-----------------------------------------------------------------------------
Return: If successful, the handle is returned (> 0).
        Otherwise, an error is returned (< 0).
-----------------------------------------------------------------------------*/
DECL_FUNC(FN_CREATE)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    int handle = getHandle(args[1], false, true);
    if(handle < 0)
        return setError(handle, "");
    printf("File %s created with handle %d\n", args[1].c_str(), handle);
    return handle;
}

DECL_FUNC(FN_EXIT)
{
    (void)args;
    return 0;
}

DECL_FUNC(FN_FORMAT)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    int num_sectors = 0;
    if(stringToInt(args[1], &num_sectors) != 0)
        return setError(-1, "invalid num_sectors: ");
    int ans = format2(num_sectors);
    if(ans != 0)
        return setError(ans, "could not format t2fs_disk.dat");
    return ans;
}

DECL_FUNC(FN_FSCP)
{
    // TODO: Implement
    (void)args; return 0;
}

DECL_FUNC(FN_LN)
{
    if(args.size() != 3)
        return printUsage(args[0]);
    char buffer_l[MAX_PATH_SIZE], buffer_f[MAX_PATH_SIZE];
    strncpy(buffer_l, args[1].c_str(), sizeof(buffer_l));
    strncpy(buffer_f, args[2].c_str(), sizeof(buffer_f));
    int res = ln2(buffer_l, buffer_f);
    if(res != 0)
        return setError(res, "could not create soft link from %s to %s",
                        args[1].c_str(), args[2].c_str());
    return 0;
}

DECL_FUNC(FN_LS)
{
    if(args.size() > 2)
        return printUsage(args[0]);
    string dir;
    if(args.size() == 2)
        dir = args[1];
    else
        dir = ".";
    int handle = getHandle(dir, true, false);
    if(handle < 0)
        return setError(handle, "");
    DIRENT2 entry;
    vector<DIRENT2> entries;
    unsigned int max_size = 0;
    while(readdir2(handle, &entry) == 0)
    {
        max_size = max(max_size, entry.fileSize);
        entries.push_back(entry);
    }
    max_size = 1 + log10(max_size);
    for(int i=0; i<(int)entries.size(); i++)
    {
        char t = entries[i].fileType;
        t = t == FILETYPE_DIRECTORY ? 'd'
                                    : (t == FILETYPE_SYMLINK ? 'l' : '-');
        printf("%c  %*d  ", t, max_size, entries[i].fileSize);
        printf("%s\n", entries[i].name);
    }
    return closeFile(handle, true);
}

DECL_FUNC(FN_MAN)
{
    if(args.size() == 1)
    {
        printf("To view the list of commands, enter \"cmd\"\n");
        printf("To get help on a specific command, enter \"%s command\"\n",
               args[0].c_str());
        printf("If you haven't already done so, please remember to \"format\" the t2fs_disk.dat\n");
        return 0;
    }
    if(args.size() != 2)
        return printUsage(args[0]);
    const Function *fn = findFunction(args[1]);
    if(!fn)
        return -1;
    printUsage(args[1]);
    printf("%s\n", fn->desc.c_str());
    return 0;
}


/*-----------------------------------------------------------------------------
Return: If successful, the handle is returned (> 0).
        Otherwise, an error is returned (< 0).
-----------------------------------------------------------------------------*/
DECL_FUNC(FN_MKDIR)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    return getHandle(args[1], true, true);
}

DECL_FUNC(FN_MV)
{
    // TODO: Implement
    (void)args; return 0;
}


/*-----------------------------------------------------------------------------
Return: If successful, the handle is returned (> 0).
        Otherwise, an error is returned (< 0).
-----------------------------------------------------------------------------*/
DECL_FUNC(FN_OPEN)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    int handle = getHandle(args[1], false, false);
    if(handle < 0)
        return setError(handle, "");
    printf("File %s opened with handle %d\n", args[1].c_str(), handle);
    return handle;
}

DECL_FUNC(FN_PWD)
{
    if(args.size() != 1)
        return printUsage(args[0]);
    char buffer[MAX_PATH_SIZE];
    int res = getcwd2(buffer, sizeof(buffer));
    if(res != 0)
        return setError(res, "could not retrieve current directory");
    printf("%s\n", buffer);
    return 0;
}

DECL_FUNC(FN_READ)
{
    if(args.size() != 3)
        return printUsage(args[0]);
    int handle = 0, bytes = 0;
    if(stringToInt(args[1], &handle) != 0)
        return setError(-1, "invalid handle: ");
    if(stringToInt(args[2], &bytes) != 0 || bytes < 0)
        return setError(-1, "invalid # of bytes%s", (bytes < 0 ? "" : ": "));
    char buffer[1024];
    int read = 0;
    int res;
    for(;;)
    {
        if(read >= bytes)
        {
            res = 0;
            break;
        }
        res = readBytes(handle, buffer+read%1024, 1);
        if(res != 1) break;
        if(++read % 1024 == 0)
            hexDump(buffer, 1024, read-1024);
    }
    hexDump(buffer, read%1024, read - read%1024);
    if(read == 0)
        printf("No bytes read from file handle %d\n", handle);
    else
        printf("%d byte%s read from file handle %d\n", read, read == 1 ? "" : "s", handle);
    return res;
}

DECL_FUNC(FN_RM)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    return deleteFile(args[1], false);
}

DECL_FUNC(FN_RMDIR)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    return deleteFile(args[1], true);
}

DECL_FUNC(FN_SEEK)
{
    if(args.size() != 3)
        return printUsage(args[0]);
    int handle = 0, offset = 0;
    if(stringToInt(args[1], &handle) != 0)
        return setError(-1, "invalid handle: ");
    if(stringToInt(args[2], &offset) != 0)
        return setError(-1, "invalid offset: ");
    int res = seek2(handle, offset);
    if(res != 0)
        return setError(res, "could not change offset of file handle %d to %d", handle, offset);
    return 0;
}

DECL_FUNC(FN_TRUNC)
{
    if(args.size() != 2)
        return printUsage(args[0]);
    int handle = 0;
    if(stringToInt(args[1], &handle) != 0)
        return setError(-1, "invalid handle: ");
    int res = truncate2(handle);
    if(res != 0)
        return setError(res, "could not truncate file with handle %d", handle);
    return 0;
}

DECL_FUNC(FN_WHO)
{
    if(args.size() != 1)
        return printUsage(args[0]);
    char buffer[MAX_PATH_SIZE];
    int res = identify2(buffer, sizeof(buffer));
    if(res != 0)
        return setError(res, "could not retrieve information");
    printf("%s\n", buffer);
    if(buffer[strlen(buffer)-1] != '\n')
        printf("\n");
    return 0;
}

DECL_FUNC(FN_WRITE)
{
    if(args.size() < 3)
        return printUsage(args[0]);
    int handle = 0;
    if(stringToInt(args[1], &handle) != 0)
        return setError(-1, "invalid handle: ");
    vector<char> data;
    if(args[2] == "-b")
    {
        for(int i=3; i<(int)args.size(); i++)
        {
            int ch = 0;
            if(stringToInt(args[i], &ch) != 0) continue;
            data.push_back((char)ch);
        }
        error_msg = "";
    }
    else if(args[2] == "-t")
    {
        for(int i=3; i<(int)args.size(); i++)
        {
            if(i != 3)
                data.push_back(' ');
            for(int j=0; j<(int)args[i].size(); j++)
                data.push_back(args[i][j]);
        }
    }
    else return setError(-1, "%s: invalid option", args[2].c_str());
    int res = 0;
    if(data.size() > 0)
        res = writeBytes(handle, &data[0], data.size());
    if(res == 0)
        printf("No bytes written to file handle %d\n", handle);
    else if(res > 0)
        printf("%d byte%s written to file handle %d\n", res, res == 1 ? "" : "s", handle);
    return res;
}

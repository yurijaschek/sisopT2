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

Help links to terminal raw input and output:

https://viewsourcecode.org/snaptoken/kilo/index.html
http://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html

 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "functions.h"
#include "shell.h"

using namespace std;

enum keys
{
    KEY_NONE = 0,
    KEY_TAB = 9,
    KEY_ENTER = 13,
    KEY_ESC = 27,
    KEY_BACKSPACE = 127,
    // Escaped sequences
    KEY_DELETE = 256, // Arbitrarily chosen (> 255)
    KEY_HOME,
    KEY_END,
    KEY_CTRL_DEL,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CTRL_LEFT,
    KEY_CTRL_RIGHT,
};

static inline int KEY_CTRL(const char k)
{
    return k & 0x1F;
}

char printable(char ch)
{
    return isprint(ch) ? ch : '.';
}


/********************************
 *  Error and system functions  *
 ********************************/

static void die(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

static struct termios initial_termios; // To save initial terminal state

/*-----------------------------------------------------------------------------
Funct:  Disable raw mode in the terminal.
-----------------------------------------------------------------------------*/
static void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial_termios);
}


/*-----------------------------------------------------------------------------
Funct:  Enable raw mode in the terminal.
        This function should be called at the beginning of the program, if
            reading raw input from the user is desirable.
        This function also tags disableRawMode to be executed when the program
            finishes, via atexit call.
-----------------------------------------------------------------------------*/
static void enableRawMode()
{
    // Get current terminal properties
    if(tcgetattr(STDIN_FILENO, &initial_termios) == -1)
        die("tcgetattr");

    // To restore the terminal after we're done
    atexit(disableRawMode);

    // Turn off buffering, echo and key processing
    struct termios raw = initial_termios;
    raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                     | INLCR | IGNCR | ICRNL | IXON);
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;

    // Block until 1 key comes in
    raw.c_cc[VMIN] = 1;
    // Doesn't really matter, since we'll wait for 1 key
    raw.c_cc[VTIME] = 0;

    // Turn on new attributes
    if(tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
        die("tcsetattr");
}


/*********************
 *  Input functions  *
 *********************/

/*-----------------------------------------------------------------------------
Funct:  Translate a terminal escaped code character sequence to an int code.
Input:  buffer -> A null-terminated string containing an escaped sequence.
Return: The corresponding int code, according to 'keys' enum.
        If a sequence is not supported, KEY_NONE will be returned.
-----------------------------------------------------------------------------*/
static int getEscSequence(unsigned char *buffer)
{
    (void)buffer;
    return KEY_NONE;
}


/*-----------------------------------------------------------------------------
Funct:  Wait for a keyboard key press and return it.
Return: If an error occurred or the key press couldn't be identified, KEY_NONE
            is returned.
        If an escaped sequence was read, it is returned, according to 'keys'
            enum, having a value > 255.
        Else, the ASCII character read is returned.
-----------------------------------------------------------------------------*/
static int getKeyPress()
{
    const int sz = 8; // Max sequence is probably 6
    unsigned char buffer[sz] = {};
    // Discard previous key strokes
    if(tcflush(STDIN_FILENO, TCIFLUSH) == -1)
        die("tcflush");
    // Wait until characters are read as input
    int res = read(STDIN_FILENO, buffer, sz-1);

    if(res == -1) // Error while trying to read or waiting for input
        die("read");

    // Can only happen if enableRawMode() sets struct termios' c_cc[VMIN] as 0
    if(res == 0)
        return KEY_NONE;

    int ans;
    if(buffer[0] == '\e') // Escape sequence
        ans = getEscSequence(buffer);
    else
        ans = min((int)buffer[0], 127);

    return ans;
}


/********************
 *  Core functions  *
 ********************/

/*-----------------------------------------------------------------------------
Funct:  Prompt for getting the user command to be executed.
Return: A string that contains the text typed by the user in the terminal.
        Does not contain the ending '\n' character.
-----------------------------------------------------------------------------*/
static string getCommandLine()
{
    static vector<string> history; // Terminal commands history
    // To preserve history entries, since they can be modified in-place
    map<int,string> original;
    history.push_back(""); // New entry for this prompt
    const int len = history.size();
    int hist_idx = len-1, word_idx = 0;
    bool done = false;
    while(!done)
    {
        string str_aux;
        fflush(stdout);
        int ch = getKeyPress();
        bool regchar = ch >= ' ' && ch < 128;
        if(regchar)
        {
            str_aux = history[hist_idx];
            history[hist_idx].insert(word_idx++, 1, ch);
            printf("%c", ch);
        }
        else switch(ch)
        {
            case KEY_ENTER:
                done = true;
                break;
            default:;
        }

        if(str_aux != "") // History was modified
        {
            // Only if it's not already saved
            if(original.find(hist_idx) == original.end())
                original[hist_idx] = str_aux;
        }
    }

    string ans = history[hist_idx];
    for(auto it=original.begin(); it!=original.end(); it++)
        history[it->first] = it->second;
    history[len-1] = ans;
    if(history[len-1] == "" || (len > 1 && history[len-1] == history[len-2]))
        history.pop_back();
    printf("\n");
    return ans;
}


/*-----------------------------------------------------------------------------
Funct:  Parse a given string, returning its tokens in a vector of strings.
Input:  line -> Line to be parsed.
Return: A vector of strings, corresponding to the tokens in the line.
        If a parsing error is encountered, an empty vector is returned and
            error_msg is set appropriately.
-----------------------------------------------------------------------------*/
static vector<string> parseCommandLine(string line)
{
    vector<string> ans;
    stringstream ss(line);
    string arg;
    while(ss >> arg)
        ans.push_back(arg);
    return ans;
}


bool interactive;
char user_info[] = "user";
char host_info[] = "t2fs";
string cwd_path = "/";
string error_msg;
int exit_status = 0;


int setError(int err, const char *fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    error_msg = buffer + error_msg;
    return err;
}


const struct Function *findFunction(string cmd)
{
    auto cmd_it = cmd_lst.find(cmd);
    if(cmd_it == cmd_lst.end())
    {
        setError(0, "%s: command not found", cmd.c_str());
        return NULL;
    }
    auto fn_it = fn_lst.find(cmd_it->second);
    if(fn_it == fn_lst.end())
    {
        setError(0, "%s: function not implemented or structure uninitialized",
                 cmd.c_str());
        return NULL;
    }
    return &fn_it->second;
}


/************
 *  Shell!  *
 ************/

/*-----------------------------------------------------------------------------
Funct:  Entry point for the operating system.
        If the program was given a regular file as stdin, the shell won't be
            interactive, simply reading lines of command from stdin until EOF,
            printing its results to stdout.
        Otherwise, the shell will be interactive, being the user able to type
            in commands, modify them and reenter them by browsing through the
            command history, among possibly other functionalities.
-----------------------------------------------------------------------------*/
int main()
{
    interactive = isatty(STDIN_FILENO);
    if(interactive)
    {
        enableRawMode(); // For advanced user input
        printf("Welcome to T2FS shell!\n");
//        FUNC_NAME(FN_MAN)({"man"}); // Call the shell's manual
    }

    for(;;)
    {
        string cmd_line;
        error_msg = ""; // To clear errors of previous activity
        if(interactive)
        {
            printf("%s@%s:", user_info, host_info);
            printf("%s$ ", cwd_path.c_str());
            cmd_line = getCommandLine();
        }
        else // "Script" shell
        {
            if(!getline(cin, cmd_line))
                break;
        }

        vector<string> args = parseCommandLine(cmd_line);
        if(error_msg != "") // Parsing error
        {
            printf("shell: %s\n", error_msg.c_str());
            continue;
        }
        if(args.size() == 0) // Empty line entered
            continue;

        const Function *fn = findFunction(args[0]);
        if(!fn) // Invalid command or function
        {
            printf("%s\n", error_msg.c_str());
            continue;
        }

        exit_status = fn->fn(args); // Call the function
        if(exit_status != 0 && error_msg != "") // Error
        {
            printf("%s returned status %d: %s\n", args[0].c_str(), exit_status,
                                                  error_msg.c_str());
        }

        if(fn->fn == FUNC_NAME(FN_EXIT))
            break;
    }
    return 0;
}

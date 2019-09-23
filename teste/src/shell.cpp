/*****************************************************************************
 *  Instituto de Informatica - Universidade Federal do Rio Grande do Sul     *
 *  INF01142 - Sistemas Operacionais I N                                     *
 *  Test shell for SisOp1's Task 2: T2FS                                     *
 *                                                                           *
 *  Author: Yuri Jaschek <yuri.jaschek@inf.ufrgs.br>                         *
 *                       <yurijaschek@gmail.com>                             *
 *                                                                           *
 *****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
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
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
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
static int getEscSequence(char *buffer)
{
    // for(char *ch=buffer; *ch; ch++)
    //     fprintf(stderr, "%d\n", *ch);

    if(strcmp(buffer, "\e[A") == 0)
        return KEY_UP;
    if(strcmp(buffer, "\e[B") == 0)
        return KEY_DOWN;
    if(strcmp(buffer, "\e[C") == 0)
        return KEY_RIGHT;
    if(strcmp(buffer, "\e[D") == 0)
        return KEY_LEFT;
    if(strcmp(buffer, "\e[3~") == 0)
        return KEY_DELETE;
    return KEY_NONE; // No known escape sequence
}


/*-----------------------------------------------------------------------------
Funct:  Wait for a keyboard key press and return it.
Input:  processEsc -> If this function should process escaped sequences or not
        buffer     -> The buffer in which the esc sequence should be returned
                      If processEsc is true, then this should be valid
Return: If an error occurred or the key press couldn't be identified if
            processEsc is true, KEY_NONE is returned.
        If an escaped sequence was read and processEsc is true, the sequence
            is returned, according to 'keys' enum, having a value > 255.
        Otherwise, if processEsc is false, the sequence is copied to the given
            buffer.
        Else, the ASCII character read is returned.
-----------------------------------------------------------------------------*/
static int getKeyPress(bool processEsc, char *sequence)
{
    const int sz = 16; // Max sequence is probably 6
    char buffer[sz] = {};
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
    if(buffer[0] != '\e')
        ans = max('\0', buffer[0]);
    // Escaped sequence now
    else if(processEsc)
        ans = getEscSequence(buffer);
    else
    {
        ans = KEY_NONE;
        strcpy(sequence, buffer);
    }
    return ans;
}


/*-----------------------------------------------------------------------------
Funct:  Query the terminal for the current cursor position.
Input:  row -> Pointer to return the cursor row position
        col -> Pointer to return the cursor column position
-----------------------------------------------------------------------------*/
static void getCursorPosition(int *row, int *col)
{
    char buffer[16];
    *row = 0; *col = 0;
    printf("\e[6n");
    fflush(stdout); // For response to be available immediately
    getKeyPress(false, buffer);
    // buffer should now have the cursor position in format "\e[n;mR", where
    // 'n' and 'm' are the row and column, respectively
    sscanf(buffer, "\e[%d;%dR", row, col);
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
    /*
     *  TODO
     *  - Deal with terminal resizing
     *  - Deal with input string longer than the terminal screen
     */

    static vector<string> history; // Terminal commands history
    // To preserve history entries, since they can be modified in-place
    map<int,string> original;
    if(history.size() == 128) // Just keep the last 128 commands
        history.erase(history.begin());
    history.push_back(""); // New entry for this prompt
    const int len = history.size();
    int hist_idx = len-1, word_idx = 0;
    bool done = false;

    int row, col; // Beginning of editable text
    getCursorPosition(&row, &col);

    while(!done)
    {
        string str_aux;
        int ch = getKeyPress(true, NULL);
        if(ch >= ' ' && ch < 127) // Regular printable character
        {
            str_aux = history[hist_idx];
            history[hist_idx].insert(word_idx++, 1, ch);
        }
        else switch(ch)
        {
            case KEY_ENTER:
                done = true;
                break;
            case KEY_RIGHT:
                word_idx = min(word_idx+1, (int)history[hist_idx].size());
                break;
            case KEY_LEFT:
                word_idx = max(0, word_idx-1);
                break;
            case KEY_UP:
                if(hist_idx != 0)
                {
                    hist_idx--;
                    word_idx = history[hist_idx].size();
                }
                break;
            case KEY_DOWN:
                if(hist_idx != len-1)
                {
                    hist_idx++;
                    word_idx = history[hist_idx].size();
                }
                break;
            case KEY_BACKSPACE:
                if(word_idx > 0)
                {
                    str_aux = history[hist_idx];
                    history[hist_idx].erase(--word_idx, 1);
                }
                break;
            case KEY_DELETE:
                if(word_idx < (int)history[hist_idx].size())
                {
                    str_aux = history[hist_idx];
                    history[hist_idx].erase(word_idx, 1);
                }
                break;
            default:;
        }

        if(str_aux != "") // History was modified
        {
            // Only if it wasn't saved before
            if(original.find(hist_idx) == original.end())
                original[hist_idx] = str_aux;
        }

        // Get terminal dimensions
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        // Write output
        printf("\e[%d;%dH", row, col); // Move cursor
        int num_char = col + history[hist_idx].size();
        int num_lines = (num_char-1)/ ws.ws_col;
        if(row + num_lines > ws.ws_row)
            row = ws.ws_row - num_lines;
        printf("%s ", history[hist_idx].c_str());
        printf("\e[J"); // Clear terminal from cursor to end of screen

        // Position cursor
        printf("\e[%d;%dH", row, 1); // Move cursor
        int cursor_down = (col+word_idx-1) / ws.ws_col;
        if(cursor_down > 0)
            printf("\e[%dB", cursor_down); // Move cursor down
        int cursor_right = (col+word_idx-1) % ws.ws_col;
        if(cursor_right)
            printf("\e[%dC", cursor_right); // Move cursor forward

        // Update output
        fflush(stdout);
    }

    string ans = history[hist_idx];
    for(auto it=original.begin(); it!=original.end(); it++)
        history[it->first] = it->second;
    history[len-1] = ans; // In case the user repeated a command
    if(history[len-1] == "" || (len > 1 && history[len-1] == history[len-2]))
        history.pop_back(); // No empty commands or repetition in sequence
    printf("\n");
    fflush(stdout); // So user
    return ans;
}


bool interactive;
char user_info[] = "user";
char host_info[] = "t2fs";
string cwd_path = "/";
string error_msg;
int last_status = 0;
map<string,int> variables;


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
    {
        if(arg[0] == '$' && arg.size() > 1)
        {
            auto it = variables.find(arg.substr(1));
            if(it != variables.end())
                ans.push_back(to_string(it->second));
        }
        else
            ans.push_back(arg);
    }
    return ans;
}


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
        FUNC_NAME(FN_MAN)({"man"}); // Call the shell's manual
    }

    for(;;)
    {
        string cmd_line;
        error_msg = ""; // To clear errors of previous activity
        if(interactive)
        {
            printf("%s@%s:", user_info, host_info);
            printf("%s$ ", cwd_path.c_str());
            fflush(stdout);
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

        int exit_status = fn->fn(args); // Call the function
        if(exit_status != 0 && error_msg != "") // Error
        {
            printf("%s returned status %d: %s\n", args[0].c_str(), exit_status,
                                                  error_msg.c_str());
        }
        last_status = exit_status;

        if(fn->fn == FUNC_NAME(FN_EXIT))
            break;
    }
    return 0;
}

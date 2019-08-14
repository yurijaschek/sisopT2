/*****************************************************************************
 *  Instituto de Informatica - Universidade Federal do Rio Grande do Sul     *
 *  INF01142 - Sistemas Operacionais I N                                     *
 *  Test shell for SisOp1's Task 2: T2FS                                     *
 *                                                                           *
 *  Author: Yuri Jaschek <yuri.jaschek@inf.ufrgs.br>                         *
 *                       <yurijaschek@gmail.com>                             *
 *                                                                           *
 *****************************************************************************/

#ifndef SHELL_H
#define SHELL_H

#include <string>

struct Function;

char printable(char ch);
int setError(int err, const char *fmt, ...);
const struct Function *findFunction(std::string cmd);

extern bool interactive;
extern char user_info[];
extern char host_info[];
extern std::string cwd_path;
extern std::string error_msg;
extern int exit_status;

#endif // SHELL_H

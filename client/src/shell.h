#ifndef _SHELL_H
#define _SHELL_H

void interactive_shell(void);
void exec_cmd(int argc, char **argv);
void exec_cmd_wait(int argc, char **argv);

void msh_exec_cmd(int argc, char **argv);
#endif

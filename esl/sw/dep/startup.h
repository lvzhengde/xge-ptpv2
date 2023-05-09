#ifndef _STARTUP_H__
#define _STARTUP_H__

#include "datatypes.h"

void catch_signals(int sig);

void do_signal_close(PtpClock * ptpClock);

void do_signal_sighup(RunTimeOpts * rtOpts);

void check_signals(RunTimeOpts * rtOpts, PtpClock * ptpClock);

#ifdef RUNTIME_DEBUG
void enable_runtime_debug(void );

void disable_runtime_debug(void );
#endif

int lockfile(int fd);

int daemon_already_running(void);

int pgrep_matches(char *name);

int query_shell(char *command, char *answer, int answer_size);

int check_parallel_daemons(string name, int expected, int strict, RunTimeOpts * rtOpts);

int logToFile(RunTimeOpts * rtOpts);

int recordToFile(RunTimeOpts * rtOpts);

void ptpdShutdown(PtpClock * ptpClock);

void dump_command_line_parameters(int argc, char **argv);

void display_short_help(string error);

PtpClock *ptpdStartup(int argc, char **argv, Integer16 * ret, RunTimeOpts * rtOpts);

#endif // _STARTUP_H__


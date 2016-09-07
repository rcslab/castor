
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include <histedit.h>

#include <castor/Common/runtime.h>
#include <castor/Common/cli.h>

#define MAX_LEN  64
#define MAX_ARGS 5

static char *curPrompt = "> ";
static EditLine *el = NULL;
static History *hist;
static HistEvent ev;

void CLI_Quit(int argc, char *argv[])
{
    exit(0);
}

void
CLI_Help(int argc, char *argv[])
{
   printf("Help:\n");
   printf("quit		Quit the command line.\n");
   printf("help		Display this message.\n");
   printf("step		Step one or more events.\n");
   printf("dump		Dump queues.\n");
}

void
CLI_Step(int argc, char *argv[])
{
    int result;
    int count = 1;

    if (argc == 2) {
	count = atoi(argv[1]);
    }

    for (int i = 0; i < count; i++) {
	result = QueueOne();
	if (result <= 0) {
	    printf("Reached end of replay log\n");
	    break;
	}
    }
}

void
CLI_Dump(int argc, char *argv[])
{
    DumpLog();
}

void
CLI_Process(const char *line)
{
    int argc;
    const char **argv;

    Tokenizer *tok = tok_init("");
    tok_str(tok, line, &argc, &argv);

    if (argv[0] == NULL) {
	// Ignore
    } else if (strcmp(argv[0], "quit") == 0) {
	CLI_Quit(argc, (char **)&argv);
    } else if (strcmp(argv[0], "help") == 0) {
	CLI_Help(argc, (char **)&argv);
    } else if (strcmp(argv[0], "step") == 0) {
	CLI_Step(argc, (char **)&argv);
    } else if (strcmp(argv[0], "dump") == 0) {
	CLI_Dump(argc, (char **)&argv);
    } else if (strcmp(argv[0], "") != 0) {
	printf("Unknown command %s\n", argv[0]);
    }

    tok_end(tok);
}

void CLI_Loop()
{
    int len = MAX_LEN;
    const char *inputLine;

    printf("Interactive Replay...\n");

    while (true) {
	inputLine = (char *)el_gets(el, &len);
	if (len > 0) {
	    history(hist, &ev, H_ENTER, inputLine);
	}

	CLI_Process(inputLine);
    }
}

static char *
elprompt(EditLine *el)
{
   return curPrompt;
}

void
CLI_Start()
{
   el = el_init("", stdin, stdout, stderr);
   hist = history_init();
   history(hist, &ev, H_SETSIZE, 100);
   el_set(el, EL_EDITOR, "emacs");
   el_set(el, EL_HIST, history, hist);
   el_set(el, EL_PROMPT, elprompt);

   CLI_Loop();

   history_end(hist);
   el_end(el);
}


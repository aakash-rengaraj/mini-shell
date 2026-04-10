/*
 * ============================================================
 *  Mini Shell / Command Interpreter
 *  BACSE104 – Structured and Object-Oriented Programming
 *  C Programming Track Micro-Project
 *  MADE BY AAKASH R | 25BYB0020
 * ============================================================
 *
 * 
 *  Concepts demonstrated:
 *    - Arrays & strings
 *    - Functions
 *    - Structures
 *    - Pointers
 *    - Dynamic memory allocation (malloc / realloc / free)
 *    - File handling  (history log)
 *    - Function pointers (command dispatch table)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>   /* chdir, getcwd */

/* ── Constants ─────────────────────────────────────────── */
#define MAX_INPUT      256
#define MAX_ARGS       32
#define HISTORY_FILE   "shell_history.log"
#define PROMPT         "mysh> "

/* ── Structure: one history entry ──────────────────────── */
typedef struct {
    int    index;
    char   timestamp[32];
    char   command[MAX_INPUT];
} HistoryEntry;

/* ── Structure: command dispatch record ────────────────── */
typedef struct {
    const char *name;
    int (*handler)(char **args, int argc);   /* function pointer */
} Command;

/* ── Dynamic history buffer ─────────────────────────────── */
static HistoryEntry *g_history  = NULL;
static int           g_hist_cnt = 0;
static int           g_hist_cap = 0;

/* ── Forward declarations ───────────────────────────────── */
int cmd_echo   (char **args, int argc);
int cmd_cd     (char **args, int argc);
int cmd_pwd    (char **args, int argc);
int cmd_history(char **args, int argc);
int cmd_clear  (char **args, int argc);
int cmd_help   (char **args, int argc);
int cmd_exit   (char **args, int argc);

/* ── Command table (uses function pointers) ─────────────── */
static Command g_commands[] = {
    { "echo",    cmd_echo    },
    { "cd",      cmd_cd      },
    { "pwd",     cmd_pwd     },
    { "history", cmd_history },
    { "clear",   cmd_clear   },
    { "help",    cmd_help    },
    { "exit",    cmd_exit    },
    { NULL, NULL }
};

/* ═══════════════════════════════════════════════════════════
 *  Utility – get current timestamp string
 * ═══════════════════════════════════════════════════════════ */
static void get_timestamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* ═══════════════════════════════════════════════════════════
 *  History – add entry to dynamic array & log to file
 * ═══════════════════════════════════════════════════════════ */
static void history_add(const char *cmd_line) {
    /* Grow buffer if needed (dynamic memory allocation) */
    if (g_hist_cnt >= g_hist_cap) {
        int new_cap = (g_hist_cap == 0) ? 8 : g_hist_cap * 2;
        HistoryEntry *tmp = (HistoryEntry *)realloc(
                                g_history,
                                new_cap * sizeof(HistoryEntry));
        if (!tmp) {
            fprintf(stderr, "mysh: memory allocation failed\n");
            return;
        }
        g_history  = tmp;
        g_hist_cap = new_cap;
    }

    HistoryEntry *e = &g_history[g_hist_cnt];
    e->index = g_hist_cnt + 1;
    get_timestamp(e->timestamp, sizeof(e->timestamp));
    strncpy(e->command, cmd_line, MAX_INPUT - 1);
    e->command[MAX_INPUT - 1] = '\0';
    g_hist_cnt++;

    /* Append to log file (file handling) */
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (fp) {
        fprintf(fp, "[%d] %s  %s\n", e->index, e->timestamp, e->command);
        fclose(fp);
    }
}

/* ═══════════════════════════════════════════════════════════
 *  Parser – split input line into tokens (argv-style)
 *  Returns argc; fills args[] with pointers into buf (in-place)
 * ═══════════════════════════════════════════════════════════ */
static int parse_input(char *buf, char **args, int max_args) {
    int argc = 0;
    char *token = strtok(buf, " \t\n\r");
    while (token && argc < max_args - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n\r");
    }
    args[argc] = NULL;
    return argc;
}

/* ═══════════════════════════════════════════════════════════
 *  Dispatcher – look up command name and call handler
 * ═══════════════════════════════════════════════════════════ */
static int dispatch(char **args, int argc) {
    if (argc == 0) return 0;

    for (int i = 0; g_commands[i].name != NULL; i++) {
        if (strcmp(args[0], g_commands[i].name) == 0) {
            return g_commands[i].handler(args, argc);  /* function pointer call */
        }
    }

    fprintf(stderr, "mysh: '%s': command not found. Type 'help' for list.\n",
            args[0]);
    return 1;
}

/* ═══════════════════════════════════════════════════════════
 *  Command handlers
 * ═══════════════════════════════════════════════════════════ */

/* echo – print arguments */
int cmd_echo(char **args, int argc) {
    for (int i = 1; i < argc; i++) {
        printf("%s", args[i]);
        if (i < argc - 1) putchar(' ');
    }
    putchar('\n');
    return 0;
}

/* cd – change directory */
int cmd_cd(char **args, int argc) {
    if (argc < 2) {
        fprintf(stderr, "mysh: cd: missing argument\n");
        return 1;
    }
    if (chdir(args[1]) != 0) {          /* POSIX chdir */
        perror("mysh: cd");
        return 1;
    }
    return 0;
}

/* pwd – print working directory */
int cmd_pwd(char **args, int argc) {
    (void)args; (void)argc;
    char buf[1024];
    if (getcwd(buf, sizeof(buf))) {
        printf("%s\n", buf);
    } else {
        perror("mysh: pwd");
        return 1;
    }
    return 0;
}

/* history – display in-memory list and mention log file */
int cmd_history(char **args, int argc) {
    (void)args; (void)argc;
    if (g_hist_cnt == 0) {
        printf("No commands in history.\n");
        return 0;
    }
    printf("\n  %-4s  %-20s  %s\n", "#", "Timestamp", "Command");
    printf("  %-4s  %-20s  %s\n",
           "----", "--------------------", "-------");
    for (int i = 0; i < g_hist_cnt; i++) {
        printf("  %-4d  %-20s  %s\n",
               g_history[i].index,
               g_history[i].timestamp,
               g_history[i].command);
    }
    printf("\n  (Full log saved to: %s)\n\n", HISTORY_FILE);
    return 0;
}

/* clear – clear terminal */
int cmd_clear(char **args, int argc) {
    (void)args; (void)argc;
    printf("\033[2J\033[H");   /* ANSI escape */
    return 0;
}

/* help – list available commands */
int cmd_help(char **args, int argc) {
    (void)args; (void)argc;
    printf("\n  Mini Shell – Built-in Commands\n");
    printf("  ══════════════════════════════\n");
    printf("  echo  <text>   Print text to the terminal\n");
    printf("  cd    <path>   Change current directory\n");
    printf("  pwd            Print working directory\n");
    printf("  history        Show command history\n");
    printf("  clear          Clear the screen\n");
    printf("  help           Show this help message\n");
    printf("  exit           Quit the shell\n\n");
    return 0;
}

/* exit – free memory, close shell */
int cmd_exit(char **args, int argc) {
    (void)args; (void)argc;
    printf("Goodbye!\n");
    free(g_history);     /* release dynamic memory */
    g_history = NULL;
    exit(0);
}

/* ═══════════════════════════════════════════════════════════
 *  Main – REPL loop
 * ═══════════════════════════════════════════════════════════ */
int main(void) {
    char  input[MAX_INPUT];
    char  buf[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("╔══════════════════════════════════════╗\n");
    printf("║   Mini Shell  –  BACSE104 Project    ║\n");
    printf("║   Type 'help' for available commands ║\n");
    printf("║    Made by AAKASH R | 25BYB0020      ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    while (1) {
        printf(PROMPT);
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            /* EOF (Ctrl-D) */
            printf("\n");
            break;
        }

        /* Remove trailing newline */
        input[strcspn(input, "\n")] = '\0';

        /* Skip empty input */
        if (strlen(input) == 0) continue;

        /* Save to history before parsing (parse mutates the string) */
        history_add(input);

        /* Copy for parsing (strtok modifies the buffer) */
        strncpy(buf, input, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        int argc = parse_input(buf, args, MAX_ARGS);
        dispatch(args, argc);
    }

    free(g_history);
    return 0;
}

#include "shell.h"
#include "shell_commands.h"
#include "uart.h"
#include "keyboard.h"
#include "malloc.h"
#include "strings.h"
#include "pi.h"
#include "printf.h"

#define LINE_LEN 80

static int (*shell_printf)(const char * format, ...);

static char *strndup(const char *src, int n);
static int isspace(char ch);
static int tokenize(const char *line, char *tokens[],  int max);

static const command_t commands[] = {
    {"help", "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo", "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the Raspberry Pi back to the bootloader", cmd_reboot},
    {"peek", "[address] print contents of memory at address", cmd_peek},
    {"poke", "[address] [value] store value at address", cmd_poke},
};

int cmd_echo(int argc, const char *argv[]) 
{
    for (int i = 1; i < argc; ++i) 
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) 
{
    // no additional args provided, print all commands
    if (argc == 1) {
        for (int i = 0; i < (sizeof(commands) / sizeof(command_t)); i++) {
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
        }
        return 0;
    }

    const char *command = argv[1];
    for (int i = 0; i < (sizeof(commands) / sizeof(command_t)); i++) {
        if (strcmp(command, commands[i].name) == 0) {
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
            return 0;
        }
    }

    // specified invalid command
    shell_printf("error: no such command '%s'.\n", command);
    return 1;
}

int cmd_reboot(int argc, const char* argv[])
{
    shell_printf("System rebooting. See ya back at the bootloader!");
    shell_printf("\n%c",0x04);
    pi_reboot();
    return 0;
}

int cmd_peek(int argc, const char *argv[]) 
{
    if (argc == 1) {
        shell_printf("error: peek requires 1 argument [address]\n");
        return 1;
    }
    
    char *endptr = 0;
    const char *address_as_str = argv[1];
    int address_str_len = strlen(address_as_str);
    unsigned int address = strtonum(address_as_str, (const char **) &endptr);

    // invalid address format
    if (endptr != address_as_str + address_str_len) {
        shell_printf("error: peek cannot convert '%s'.\n", address_as_str);
        return 1;
    }

    // if address is not 4-byte aligned
    if (address % (sizeof(int)) != 0) {
        shell_printf("error: peek address must be 4-byte aligned\n");
        return 1;
    }

    // otherwise is a valid address
    shell_printf("%p: %08x\n", address, *(int *)address);
    return 0;
}

int cmd_poke(int argc, const char *argv[]) 
{
    if (argc < 3) {
        shell_printf("error: poke expects 2 arguments [address] [value]\n");
        return 1;
    }

    char *address_endptr = 0;
    const char *address_as_str = argv[1];
    int address_str_len = strlen(address_as_str);
    char *value_endptr = 0;
    const char *value_as_str = argv[2];
    int value_str_len = strlen(value_as_str);

    unsigned int address = strtonum(address_as_str, (const char **) &address_endptr);
    unsigned int value = strtonum(value_as_str, (const char **) &value_endptr);

    // invalid address format
    if (address_endptr != address_as_str + address_str_len) {
        shell_printf("error: poke cannot convert '%s'.\n", address_as_str);
        return 1;
    }

    // if address is not 4-byte aligned
    if (address % (sizeof(int)) != 0) {
        shell_printf("error: poke address must be 4-byte aligned\n");
        return 1;
    }

    // invalid value format
    if (value_endptr != value_as_str + value_str_len) {
        shell_printf("error: poke cannot convert '%s'.\n", value_as_str);
        return 1;
    }

    *(int *)address = value;
    shell_printf("%p: %08x\n", address, *(int *)address);
    return 0;
}

void shell_init(formatted_fn_t print_fn)
{
    shell_printf = print_fn;
}

void shell_bell(void)
{
    uart_putchar('\a');
}

// I diverged from assign5 since I used uart instead of shell_printf
void shell_readline(char buf[], int bufsize)
{
    int len = 0;
    while (len < bufsize - 1) {
        unsigned char char_read = keyboard_read_next();
        // don't print out non-characters
        if (char_read >= 0x90) continue;
        if (char_read == '\n') {
            shell_printf("%c", char_read);
            break;
        }
        // delete a character
        if (char_read == '\b') {
            // don't delete nonexistent character
            if (len == 0) {
                shell_bell();
                continue;
            } else {
                shell_printf("%c", char_read);
                shell_printf("%c", ' ');
                len -= 1;
            }
        } else {
            buf[len] = char_read;
            len++;
        }
        shell_printf("%c", char_read);
    }
    buf[len] = '\0';
}

// helper function for tokenize
static char *strndup(const char *src, int n)
{
    if (n >= strlen(src)) {
        n = strlen(src);
    }
    char *cpy = malloc(n + 1);
    int i;
    for (i = 0; i < n; i++) {
        cpy[i] = src[i];
    }
    cpy[i] = '\0';
    return cpy;
}

// helper function for tokenize
static int isspace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n';
}

static int tokenize(const char *line, char *tokens[],  int max)
{
    int ntokens = 0;

    while (*line != '\0' && ntokens < max) {
        while (isspace(*line)) line++;
        if (*line == '\0') break;
        const char *start = line;
        while (*line != '\0' && !isspace(*line)) line++;
        int nchars = line - start;
        tokens[ntokens++] = strndup(start, nchars);
    }
    return ntokens;
}

int shell_evaluate(const char *line)
{
    int len = strlen(line);
    // if line is empty do nothing
    if (len == 0) return 0;

    char *tokens[LINE_LEN];
    // will use malloc to store token strings
    int ntokens = tokenize(line, tokens, LINE_LEN);

    char *command = tokens[0];
    int command_not_found = 1;
    int status = 0;
    for (int i = 0; i < (sizeof(commands) / sizeof(command_t)); i++) {
        if (strcmp(command, commands[i].name) == 0) {
            command_not_found = 0;
            status = commands[i].fn(ntokens, (const char **) tokens);
            break;
        }
    }

    if (command_not_found) {
        shell_printf("error: no such command '%s'.\n", command);
        return 1;
    }

    // must free memory when done with tokens
    for (int i = 0; i < ntokens; i++) {
        free(tokens[i]);
    }
    
    return status;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1) 
    {
        char line[LINE_LEN];

        shell_printf("Pi> ");
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
    }
}

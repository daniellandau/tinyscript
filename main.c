#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "tinyscript.h"

#ifdef __propeller__
#include <propeller.h>
#define ARENA_SIZE 2048
#else
#define ARENA_SIZE 8192
#define MAX_SCRIPT_SIZE 100000
#endif

int inchar() {
    return getchar();
}
void outchar(int c) {
    putchar(c);
}

#ifdef MAX_SCRIPT_SIZE
char script[MAX_SCRIPT_SIZE];

void
runscript(const char *filename)
{
    FILE *f = fopen(filename, "r");
    int r;
    if (!f) {
        perror(filename);
        return;
    }
    r=fread(script, 1, MAX_SCRIPT_SIZE, f);
    fclose(f);
    if (r <= 0) {
        fprintf(stderr, "File read error on %s\n", filename);
        return;
    }
    script[r] = 0;
    r = TinyScript_Run(script, 0, 1);
    if (r != 0) {
        printf("script error %d\n", r);
    }
    exit(r);
}
#endif

#ifdef __propeller__
static Val getcnt_fn()
{
#ifdef CNT
    return CNT;
#else
    return _cnt();
#endif
}
static Val waitcnt_fn(Val when)
{
    waitcnt(when);
    return when;
}
static Val pinout_fn(Val pin, Val onoff)
{
    unsigned mask = 1<<pin;
    DIRA |= mask;
    if (onoff) {
        OUTA |= mask;
    } else {
        OUTA &= ~mask;
    }
    return OUTA;
}
static Val pinin_fn(Val pin)
{
    unsigned mask=1<<pin;
    DIRA &= ~mask;
    return (INA & mask) ? 1 : 0;
}

#else
// compute a function of two variables
// used for testing scripts
static Val testfunc(Val x, Val y)
{
    return x*x + y*y;
}
#endif

struct def {
    const char *name;
    intptr_t val;
    int nargs;
} funcdefs[] = {
#ifdef __propeller__
    { "getcnt",    (intptr_t)getcnt_fn, 0 },
    { "waitcnt",   (intptr_t)waitcnt_fn, 1 },
    { "pinout",    (intptr_t)pinout_fn,  2 },
    { "pinin",     (intptr_t)pinin_fn, 1 },
#else
    { "dsqr",      (intptr_t)testfunc, 2 },
#endif
    { NULL, 0 }
};

void
REPL()
{
    int r;
    read_history("tinyscript_history");

    for(;;) {
#ifdef __FLEXC__
      char buf[128];
      printf("ts> ");
      gets(buf);
#else
      char *buf = readline("ts> ");
      if (!buf)
        break;
      add_history(buf);
      write_history("tinyscript_history");
#endif        
        r = TinyScript_Run(buf, 1, 1);
        if (r != 0) {
            printf("error %d\n", r);
        }
#ifndef __FLEXC__
        free(buf);
#endif
    }
}

char memarena[ARENA_SIZE];

int
main(int argc, char **argv)
{
    int err;
    int i;
    
    err = TinyScript_Init(memarena, sizeof(memarena));
    for (i = 0; funcdefs[i].name; i++) {
        err |= TinyScript_Define(funcdefs[i].name, CFUNC(funcdefs[i].nargs), funcdefs[i].val);
    }
    if (err != 0) {
        printf("Initialization of interpreter failed!\n");
        return 1;
    }
#ifdef __propeller__
    REPL();
#else
    if (argc > 2) {
        printf("Usage: tinyscript [file]\n");
    }
    if (argv[1]) {
        runscript(argv[1]);
    } else {
        REPL();
    }
#endif
    return 0;
}


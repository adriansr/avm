#ifndef ARGS_H_INCLUDED
#define ARGS_H_INCLUDED

struct Args
{
    const char *exeName,
               *inputName,
               *outputName;
};

typedef struct Args Args;

int parse_args(Args *args, int argc, const char *argv[]);

#endif // ARGS_H_INCLUDED


#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"
#include "aux.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

extern struct cmd *parsed_pipe;

void exec_cmd(struct cmd *c);

#endif  // EXEC_H

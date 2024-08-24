/**
 * @file imgfscmd.c
 * @brief imgFS command line interpreter for imgFS core commands.
 *
 * Image Filesystem Command Line Tool
 *
 * @author Mia Primorac
 */

#include "imgfs.h"
#include "imgfscmd_functions.h"
#include "util.h"   // for _unused

#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

#define NAME_SIZE 6
#define CMDS_SIZE 6

typedef int (*command)(int argc, char* argv[]);

struct command_mapping {
    char const str[NAME_SIZE+1];
    command cmd;
} command_mapping;

const struct command_mapping commands[] = {{"list", do_list_cmd}, {"create", do_create_cmd},
    {"help", help}, {"delete", do_delete_cmd}, {"read", do_read_cmd}, {"insert", do_insert_cmd}
};


/*******************************************************************************
 * MAIN
 */
int main(int argc, char* argv[])
{
    int ret = 0;
    command r = 0;

    VIPS_INIT(argv[0]);

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {
        argc--; argv++; // skips command call name
        ret = ERR_INVALID_COMMAND;

        // look up for the associated command call of the command name entered by the user
        for (int i = 0; i < CMDS_SIZE; ++i) {
            if (!strcmp(commands[i].str, argv[0])) {
                argc--; argv++; // skips command name
                ret = commands[i].cmd(argc, argv);
                break;
            }
        }
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERR_MSG(ret));
        help();
    }

    vips_shutdown();

    return ret;
}

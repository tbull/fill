/*
 *  fill - fill a file full of zeroes
 *
 *  (C) Copyright 2002-2010 by T-Bull <tbull@tbull.org>
 *
 */


#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "version.h"


#define  private static
#define  public

#define  print(s)           fputs(s, stdout)
#define  fprint(stream, s)  fputs(s, stream)


#if defined(__CYGWIN__) && !defined(_WIN32)
#define _WIN32
#endif


#define FILEBUF_SIZE       (1024*1024)


#define ERR_OK                       0  /* ERR_OK  *must*  be 0, this is assumed throughout the prog */
#define ERR_OTHER                  253
#define ERR_BAD_COMMAND_LINE       254
#define ERR_OUT_OF_MEM             255


typedef struct __arguments {
    int append1;            /* -a */
    int append2;            /* -A */
    int force;              /* -f */
} arguments;


typedef uint64_t filesize_t;




private const char * usagestring = "Usage:\n\
    fill [-aAf] [-s size] file\n\
    fill {-h | -V}\n\
\n\
    Fills a file full of zeros, either until running out of diskspace \n\
    or for so many bytes as specified by the -s option.\n\
\n\
    -a      [NYI]   append, write given number of bytes\n\
    -A      [NYI]   append, write until target size is reached\n\
    -f      [NYI]   force\n\
    -s      The file written is going to be size bytes large.\n\
            You may use K, M, G, T, P binary prefixes (1024-based).\n\
    -V      Give version information.\n\
";

private const char * versionstring = "\
fill version " PROGRAM_VERSION "\n\
(C) Copyright 2002-2010 by T-Bull <tbull@tbull.org>\n"   /* \
Project homepage: <http://calocybe.dyndns.org/software/fill>\n" */
#ifdef __CYGWIN__
"Deploying Cygwin UNIX emulation layer <http://cygwin.com/>\n"
#endif
;




private const char * progname;
private int exit_status;
private arguments args;
private void * filebuf;
private FILE * f;






/*
 *
 *  Returns the size value if all in rock, 0 if value too large or any other error.
 */
private filesize_t parse_size(char * sizestring) {
    filesize_t value;
    filesize_t multiplier;
    char * end;


    errno = 0;
    value = strtoll(sizestring, &end, 10);
    if (errno != 0 || end == sizestring) {
        fprintf(stderr, "%s: %s: size cannot be parsed: %s\n", progname, sizestring, strerror(errno));
        return 0;
    }

    while (*end == ' ') end++;
    if (*end == '\0') return value;

    multiplier = 1;
    switch (*end) {
        case 'k':
        case 'K':
            multiplier = 1024; break;

        case 'm':
        case 'M':
            multiplier = 1024 * 1024; break;

        case 'g':
        case 'G':
            multiplier = 1024 * 1024 * 1024; break;

        case 't':
        case 'T':
            multiplier = (filesize_t) 1024 * 1024 * 1024 * 1024; break;

        case 'p':
        case 'P':
            multiplier = (filesize_t) 1024 * 1024 * 1024 * 1024 * 1024; break;

        case '\0':
            break;

        default:
            fprintf(stderr, "%s: %s: unknown postfix: %c\n", progname, sizestring, *end);
            return 0;
    }

    value *= multiplier;

    return value;
}





    /*  int global_cleanup(int err);
     *
     *  Closes f, if it is open. Frees filebuf, if allocated.
     *  Returns err.
     */
    private int global_cleanup(int err) {
        if (f) {
            fclose(f);
            f = NULL;
        }

        if (filebuf) {
            free(filebuf);
            filebuf = NULL;
        }

        return err;
    }


    private void bad_command_line_exit(const char * msg, ...) {
        va_list vargs;
        const char * defmsg = "Bad command line!";


        global_cleanup(0);


        va_start(vargs, msg);

        if (!msg) msg = defmsg;
        fprint(stderr, progname); fprint(stderr, ": ");
        vfprintf(stderr, msg, vargs);
        fprintf(stderr, "\nType '%s -h' for help.\n", progname);

        va_end(vargs);

        exit(exit_status = ERR_BAD_COMMAND_LINE);
    }


    private void out_of_mem_exit(void) {
        global_cleanup(0);
        fprint(stderr, "\nOut of memory!\n");
        exit(exit_status = ERR_OUT_OF_MEM);
    }




public int main(int argc, char ** argv) {
    int opt;
    filesize_t size, bytes_written;
    bool flood;
    size_t count;


    /* init globals */
    progname = argv[0];
    filebuf = NULL;
    f = NULL;
    exit_status = ERR_OK;

    /* set default options */
    memset(&args, 0, sizeof(args));
    size = 0; flood = true;

    /* parse command line options */
    opterr = 0;
    while ((opt = getopt(argc, argv, ":aAfs:hV")) != -1) {
        switch (opt) {
            case 'h':
                print(usagestring);
                return 0;

            case 'V':
                print(versionstring);
                return 0;

            case 'a':
                args.append1 = 1;
                fprint(stderr, "Warning: -a is not supported yet.\n");
                break;

            case 'A':
                args.append2 = 1;
                fprint(stderr, "Warning: -A is not supported yet.\n");
                break;

            case 'f':
                args.force = 1;
                fprint(stderr, "Warning: -f is not supported yet.\n");
                break;

            case 's':
                flood = false;
                size = parse_size(optarg);
                if (!size) return ERR_OTHER;
                break;

            case ':':
                bad_command_line_exit("The option -%c requires an argument!", optopt);

                break;

            case '?':
            default:
                bad_command_line_exit("Unknown option: -%c", optopt);
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1) bad_command_line_exit("Exactly one filename required.");
//    if (size == 0) bad_command_line_exit("-s option is required.");


    /* suck mem */
    filebuf = malloc(FILEBUF_SIZE);
    if (!filebuf) out_of_mem_exit();

    memset(filebuf, 0, FILEBUF_SIZE);


    /* work */
    bytes_written = 0;

    f = fopen(*argv, "wb");
    if (!f) {
        fprintf(stderr, "%s: Could not open %s for writing: %s\n", progname, *argv, strerror(errno));
    } else {
        if (flood) {
            printf("Writing file %s as full as I can ... ", *argv); fflush(stdout);
        } else {
            printf("Writing file %s with %llu bytes ... ", *argv, size); fflush(stdout);
        }

        /* write full FILEBUF_SIZE chunks as long as filesize to be written permits this (or in flood mode) */
        while (flood || size > (filesize_t) FILEBUF_SIZE) {
            count = fwrite(filebuf, 1, FILEBUF_SIZE, f);
            bytes_written += count; size -= (filesize_t) FILEBUF_SIZE;

            if (count < FILEBUF_SIZE) {
                /* has not written as much as requested */
                if (flood) {
                    /* this *has* to occur eventually in flood mode, so this is expected behaviour */
                    if (errno == ENOSPC || errno == EDQUOT) {
                        /* No space left on device -- we're done */
                        printf("\nWrote %llu bytes to %s. Done.\n", bytes_written, *argv);
                        return global_cleanup(0);
                    }
                }

                /* this is a real error in non-flood mode or if errno was != expected */
                fprintf(stderr, "%s: Could not write %s: %s\n", progname, *argv, strerror(errno));
                return global_cleanup(ERR_OTHER);
            }
        }

        /* write the last chunk (smaller than FILEBUF_SIZE) to make up for the requested size */
        if (!flood && size > 0) {
            count = fwrite(filebuf, 1, (size_t) size, f);
            bytes_written += count;
            if (count < (size_t) size) {
                fprintf(stderr, "%s: Could not write %s: %s\n", progname, *argv, strerror(errno));
                return global_cleanup(ERR_OTHER);
            }
        }

        fclose(f); f = NULL;
        print("Done.\n");
    }


    return global_cleanup(0);
}

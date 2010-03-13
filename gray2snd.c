/* vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab tw=72:
 *
 * gray2snd -- Convert raw grayscale images to audio.
 * Copyright (C) 2003  Casey Marshall <rsdio@metastatic.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 *
 *    Free Software Foundation, Inc.,
 *    59 Temple Place, Suite 330,
 *    Boston, MA  02111-1307
 *    USA
 *
 * --------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sndfile.h>
#include <popt.h>

#include "config.h"
#include "audio.h"
#include "image.h"

#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_DURATION      100

void help(const char *);
void usage(poptContext, const char *, char *);
void version(const char *);
int get_format(char *, int *);
int get_type(char *, int *);

/* Show verbose output. */
int verbose = 0;

int main(int argc, const char **argv)
{
    audio_options opts = {
        0, DEFAULT_SAMPLE_RATE, 0.0, -1.0, 1.0, DEFAULT_DURATION, 0, 0, 0, 0
    };
    unsigned char *image = NULL;
    int format = SF_FORMAT_WAV;
    int order  = SF_ENDIAN_FILE;
    int type   = SF_FORMAT_PCM_16;
    const char *in_file, *out_file;
    char *arg;
    struct poptOption popts[] = {
        { "big-endian",        'b', POPT_ARG_NONE,   NULL,             'b' },
        { "cpu-endian",        'c', POPT_ARG_NONE,   NULL,             'c' },
        { "duration",          'd', POPT_ARG_INT,    &(opts.duration),  0  },
        { "format",            'f', POPT_ARG_STRING, &arg,             'f' },
        { "fourier",           'F', POPT_ARG_NONE,   NULL,             'F' },
        { "gain",              'g', POPT_ARG_DOUBLE, &(opts.gain),      0  },
        { "help",              'h', POPT_ARG_NONE,   NULL,             'h' },
        { "little-endian",     'l', POPT_ARG_NONE,   NULL,             'l' },
        { "logarithmic",       'L', POPT_ARG_NONE,   NULL,             'L' },
        { "maximum-frequency", 'M', POPT_ARG_DOUBLE, &(opts.max_freq),  0  },
        { "minimum-frequency", 'm', POPT_ARG_DOUBLE, &(opts.min_freq),  0  },
        { "sample-rate",       'r', POPT_ARG_INT,    &(opts.sample_rate),0 },
        { "sample-type",       't', POPT_ARG_STRING, &arg,             't' },
        { "size",              's', POPT_ARG_STRING, &arg,             's' },
        { "verbose",           'v', POPT_ARG_NONE,   NULL,             'v' },
        { "version",           'V', POPT_ARG_NONE,   NULL,             'V' },
        POPT_TABLEEND
    };
    poptContext popt_ctx = poptGetContext(NULL, argc, argv, popts, 0);
    int c;

    poptSetOtherOptionHelp(popt_ctx, "in-file out-file");

    if (argc < 2) {
        usage(popt_ctx, argv[0], "insufficient number of arguments.");
    }

    while ((c = poptGetNextOpt(popt_ctx)) >= 0) switch (c) {
        case 'b':
            order = SF_ENDIAN_BIG;
            break;
        case 'c':
            order = SF_ENDIAN_CPU;
            break;
        case 'f':
            if (!get_format(arg, &format)) {
                usage(popt_ctx, argv[0], "unknown format");
            }
            break;
        case 'F':
            opts.fourier = 1;
            break;
        case 'l':
            order = SF_ENDIAN_LITTLE;
            break;
        case 'h':
            help(argv[0]);
            exit(0);
        case 'L':
            opts.logarithmic = 1;
            break;
        case 's':
            opts.img_width = atoi(arg);
            arg = strpbrk(arg, "x");
            if (!arg) {
                usage(popt_ctx, argv[0], "invalid size");
            }
            opts.img_height = atoi(arg+1);
            break;
        case 't':
            if (!get_type(arg, &type)) {
                usage(popt_ctx, argv[0], "unknown sample type");
            }
            break;
        case 'v':
            verbose++;
            break;
        case 'V':
            version(argv[0]);
            exit(0);
    }

    if (c < -1) {
        fprintf(stderr, "%s: %s\n",
            poptBadOption(popt_ctx, POPT_BADOPTION_NOALIAS), poptStrerror(c));
        exit(1);
    }

    in_file  = poptGetArg(popt_ctx);
    out_file = poptGetArg(popt_ctx);
    if (in_file == NULL || out_file == NULL) {
        usage(popt_ctx, argv[0], "insufficient number of arguments.");
    }

    if (opts.img_width < 1 || opts.img_height < 1) {
        usage(popt_ctx, argv[0], "bad or missing image dimensions.");
    }

    if (type == SF_FORMAT_PCM_U8 &&
        !(format == SF_FORMAT_WAV || format == SF_FORMAT_RAW))
    {
        usage(popt_ctx, argv[0], "U8 can only be used with RAW or WAV.");
    }

    if (opts.duration < 1) {
        usage(popt_ctx, argv[0], "duration must be positive.");
    }

    if (opts.sample_rate < 1) {
        usage(popt_ctx, argv[0], "sample rate must be positive.");
    }

    if (opts.max_freq == -1.0) {
        opts.max_freq = opts.sample_rate / 2.0;
    }

    if (opts.max_freq <= opts.min_freq || opts.min_freq < 0) {
        usage(popt_ctx, argv[0], "improper frequency range.");
    }

    if (opts.gain <= 0.0) {
        usage(popt_ctx, argv[0], "gain must be positive.");
    }

    poptFreeContext(popt_ctx);

    image = fetch_image(in_file, opts.img_width, opts.img_height);
    if (!image) {
        fprintf(stderr, "%s: %s: %s\n", argv[0], in_file,
            errno ? strerror(errno) : "image file too short.");
        exit(1);
    }

    opts.format = format | order | type;

    if (render_audio(out_file, image, opts)) {
        fprintf(stderr, "%s: error writing audio: %s\n",
            argv[0], strerror(errno));
        exit(1);
    }

    exit(0);
}

/*
 * Print help message.
 */
void help(const char *name)
{
    printf(
"usage: %s [options] --size=WxH in-file out-file\n\n"
"Input options:\n"
"  -s, --size=WxH              Specify input image size.\n\n"
"Output options:\n"
"  -b, --big-endian            Write big-endian audio samples.\n"
"  -l, --little-endian         Write little-endian audio samples.\n"
"  -c, --cpu-endian            Write audio samples in the host's native byte\n"
"                              order. If no byte order is specified the file\n"
"                              format's default type is used.\n"
"  -d, --duration=LEN          Duration of each column, is samples.\n"
"                              (default: 100)\n"
"  -g, --gain=GAIN             Set output gain. (default: 1)\n"
"  -f, --format=FORMAT         The format of the output file (default WAV).\n"
"                              Allowed formats: AU AIFF RAW WAV.\n"
"  -F, --fourier               Use the inverse discrete Fourier transform\n"
"                              instead of simple summations.\n"
"  -L, --logarithmic           Map frequecnies logarithmically instead of\n"
"                              linearly.\n"
"  -m, --minimum-frequency=Hz  Minimum frequency to generate in Hz.\n"
"                              (default: 0)\n"
"  -M, --maximum-frequency=Hz  Maximum frequency to generate (Default:\n"
"                              SAMPLE_RATE/2)\n"
"  -r, --sample-rate=RATE      Sample rate. (default: 44100)\n"
"  -t, --sample-type=TYPE      Sample type (default signed 16-bit ints).\n"
"                              Allowed values: U8 S8 16 24 32 FLOAT DOUBLE.\n"
"  -v, --verbose               Print extra information during execution.\n\n"
"Other options:\n"
"  -h, --help                  Show this help message.\n"
"  -V, --version               Display version information and exit.\n",
        name);
}

/*
 * Print a usage/error message.
 */
void usage(poptContext popt_ctx, const char *name, char* error)
{
    if (error)
        fprintf(stderr, "%s: %s\n", name, error);
    poptPrintUsage(popt_ctx, stderr, 0);
    exit(1);
}

/*
 * Print a version message.
 */
void version(const char *name)
{
    printf("%s: version %s\n", name, PACKAGE_VERSION);
    printf("Copyright (C) 2003 Casey Marshall <rsdio@metastatic.org>\n\n");
    printf(
"This program is free software; you can use and/or modify this program\n\
under the terms of the GNU General Public License. There is NO WARRANTY;\n\
not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
See the source for details.\n");
}

/*
 * Store the appropriate format constant given a format string.
 */
int get_format(char *fmt, int *result)
{
    if (strcasecmp(fmt, "WAV") == 0) {
        *result = SF_FORMAT_WAV;
    } else if (strcasecmp(fmt, "AIFF") == 0) {
        *result = SF_FORMAT_AIFF;
    } else if (strcasecmp(fmt, "AU") == 0) {
        *result = SF_FORMAT_AU;
    } else if (strcasecmp(fmt, "RAW") == 0) {
        *result = SF_FORMAT_RAW;
    } else {
        return 0;
    }
    return 1;
}

/*
 * Store the appropriate type constant given a type string.
 */
int get_type(char *type, int *result)
{
    if (strcasecmp(type, "S8") == 0) {
        *result = SF_FORMAT_PCM_S8;
    } else if (strcasecmp(type, "U8") == 0) {
        *result = SF_FORMAT_PCM_U8;
    } else if (strcmp(type, "16") == 0) {
        *result = SF_FORMAT_PCM_16;
    } else if (strcmp(type, "24") == 0) {
        *result = SF_FORMAT_PCM_24;
    } else if (strcmp(type, "32") == 0) {
        *result = SF_FORMAT_PCM_32;
    } else if (strcasecmp(type, "FLOAT") == 0) {
        *result = SF_FORMAT_FLOAT;
    } else if (strcasecmp(type, "DOUBLE") == 0) {
        *result = SF_FORMAT_DOUBLE;
    } else {
        return 0;
    }
    return 1;
}

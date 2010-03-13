/* vim:set softtabstop=4 shiftwidth=4 tabstop=4 expandtab tw=72:
 *
 * audio -- render pixel data to sound waves.
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
#include <math.h>
#include <limits.h>
#include <sndfile.h>

#include "config.h"

#ifdef HAVE_FFTW_PREFIX
#  include <drfftw.h>
#else
#  include <rfftw.h>
#endif

#include "audio.h"

/*
 * Grab pixel (x, y) from img with width w, return the proportional
 * brightness of that pixel (between 0 and 1).
 */
#define PIXEL(img, x, y, w) ((*(img + (y * w) + x)) / 255.0)

/*
 * Render an eight bit grayscale pixmap to an audio file, where the
 * pitch of the sound is derived from the intensity of the pixels up and
 * down each column (thus, the top left pixel affects the highest pitch
 * in the first time interval, and the bottom right affects the lowest
 * pitch in the last time interval).
 *
 * The wave is written to the supplied pathname, according to the
 * specified options.
 *
 *   char *fname         The file to write to.
 *   unsigned char *img  The image data.
 *   audio_options opts  The run-time options.
 *
 *   Returns 0 on success, nonzero otherwise.
 */
int render_audio(char *fname, unsigned char *img, audio_options opts)
{
    SF_INFO sfinfo;
    SNDFILE *sndfile;
    double waves[opts.duration];
    double freq_table[opts.img_height];
    /* Frequency increment, Hertz. */
    double df = 0.0;
    /* Time increment, seconds. */
    double dt = (1.0 / opts.sample_rate);
    double T = 0;
    int x, y, t;
    fftw_real fft_in[opts.sample_rate], fft_out[opts.sample_rate];
    rfftw_plan fft_plan;
    extern int verbose;

    sfinfo.frames = opts.img_width * opts.duration;
    sfinfo.samplerate = opts.sample_rate;
    sfinfo.channels = 1;
    sfinfo.format = opts.format;

    if (!sf_format_check(&sfinfo)) {
        return -1;
    }
    sndfile = sf_open(fname, SFM_WRITE, &sfinfo);
    if (!sndfile) {
        return -1;
    }

    if (opts.logarithmic) {
        df = (opts.max_freq - opts.min_freq);
        for (x = opts.img_height-1; x >= 0; x--) {
            freq_table[x] = df + opts.min_freq;
            if (!opts.fourier)  /* Scale to radians. */
                freq_table[x] *= 2.0 * M_PI;
            df /= 2.0;
        }
    } else {
        df = (opts.max_freq - opts.min_freq) / (double) (opts.img_height-1);
        for (x = 0; x < opts.img_height; x++) {
            freq_table[x] = df * x + opts.min_freq;
            if (!opts.fourier)  /* Scale to radians. */
                freq_table[x] *= 2.0 * M_PI;
        }
    }
    if (verbose >= 2) {
        for (x = 0; x < opts.img_height; x++)
            fprintf(stderr, "frequency [%d] = %.32f Hz\n", x,
                opts.fourier ? freq_table[x] : freq_table[x] / (2*M_PI));
    }

    if (verbose) {
        fprintf(stderr, "Writing audio:   0%%");
    }

    /* Generate a waveform for each vertical pixel. */
    if (opts.fourier) {
        fft_plan = rfftw_create_plan(opts.sample_rate, FFTW_COMPLEX_TO_REAL,
                                      FFTW_ESTIMATE);
        for (x = 0; x < opts.img_width; x++) {
            memset(fft_in, 0, sizeof(fft_in));
            for (y = 0; y < opts.img_height; y++) {
                fftw_real pixel = PIXEL(img, x, y, opts.img_width) * 255.0;
                fft_in[(int) freq_table[opts.img_height-1-y]]
                    += pixel;
                fft_in[opts.sample_rate-(int)freq_table[opts.img_height-1-y]]
                    += pixel;
            }
            rfftw_one(fft_plan, fft_in, fft_out);
            for (y = 0; y < opts.sample_rate; y++) {
                fft_out[y] = fft_out[y] / (opts.sample_rate/2) * opts.gain;
            }
#           if FFTW_ENABLE_FLOAT
                sf_writef_float(sndfile, fft_out, opts.duration);
#           else
                sf_writef_double(sndfile, fft_out, opts.duration);
#           endif
            if (verbose) {
                fprintf(stderr, "\b\b\b\b%3d%%", (100*(x+1)) / opts.img_width);
            }
        }
    } else {
        for (x = 0; x < opts.img_width; x++) {
            for (t = 0; t < opts.duration; t++) {
                waves[t] = 0.0;
                for (y = 0; y < opts.img_height; y++) {
                    waves[t] += PIXEL(img, x, y, opts.img_width)
                              * sin(freq_table[opts.img_height-y] * T);
                }
                waves[t] = (waves[t] / opts.img_height) * opts.gain;
                T += dt;
            }
            /* Write out the waveform. */
            sf_writef_double(sndfile, waves, opts.duration);
            if (verbose) {
                fprintf(stderr, "\b\b\b\b%3d%%", (100*(x+1)) / opts.img_width);
            }
        }
    }
    if (verbose) fputc('\n', stderr);

    return sf_close(sndfile);
}

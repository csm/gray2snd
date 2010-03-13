#ifndef __AUDIO_H__
#define __AUDIO_H__

typedef struct {
    unsigned format;
    int sample_rate;
    double min_freq;
    double max_freq;
    double gain;
    unsigned duration;
    unsigned img_width   : 15;
    unsigned logarithmic :  1;
    unsigned img_height  : 15;
    unsigned fourier     :  1;
} audio_options;

int render(char *, unsigned char *, audio_options);

#endif /* __AUDIO_H__ */

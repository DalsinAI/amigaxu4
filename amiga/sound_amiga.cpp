/*
 * sound_amiga.cpp: xu4's sound service on AmigaOS without SDL_mixer.
 *
 * Upstream's sound_sdl.cpp needs SDL_mixer 1.2, which GFOpenGPU's kit does
 * not have. Here the sound effects (Ogg Vorbis, sound/) are decoded once,
 * the first time each is played, by stb_vorbis into 16-bit samples at
 * 22050 Hz, and one effects channel (as upstream: FX_CHANNEL) is mixed into
 * SDL's own audio. Music (MIDI and Impulse Tracker files) is not played:
 * the music calls keep upstream's on/off and volume bookkeeping only.
 *
 * The AmiCradle Team. GPL 2 or later, like xu4.
 */

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "sound.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "settings.h"

extern "C" int stb_vorbis_decode_memory(const unsigned char *mem, int len,
                                        int *channels, int *sample_rate,
                                        short **output);

extern int u4_SDL_InitSubSystem(Uint32 flags);
extern void u4_SDL_QuitSubSystem(Uint32 flags);

#define OUT_RATE 22050

struct Clip {
    Sint16 *data;   /* mono, OUT_RATE */
    int len;        /* samples */
};

static bool audioFunctional = false;
static bool musicEnabled = false;
static std::vector<Clip> clips;

/* The one effects channel, used by the audio callback under SDL_LockAudio. */
static const Clip *fxClip = NULL;
static int fxPos = 0;
static int fxEnd = 0;     /* samples to play before stopping */
static bool fxLoop = false;
static int fxVolume = 256; /* 0..256 */

static void audio_callback(void *, Uint8 *stream, int len) {
    Sint16 *out = (Sint16 *)stream;
    int frames = len / 4; /* 16-bit stereo */
    memset(stream, 0, len);
    if (!fxClip)
        return;
    for (int i = 0; i < frames; i++) {
        if (fxPos >= fxEnd) {
            fxClip = NULL;
            break;
        }
        int idx = fxPos % fxClip->len;
        Sint16 s = (Sint16)((fxClip->data[idx] * fxVolume) >> 8);
        out[i * 2] = s;
        out[i * 2 + 1] = s;
        fxPos++;
    }
}

int soundInit(void) {
    ASSERT(!audioFunctional, "soundInit called more than once");

    if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
        errorWarning("unable to init SDL audio subsystem: %s", SDL_GetError());
        return 0;
    }

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = OUT_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    if (SDL_OpenAudio(&want, &have) < 0) {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        return 0;
    }
    if (have.format != AUDIO_S16SYS || have.channels != 2 || have.freq != OUT_RATE) {
        fprintf(stderr, "Audio opened in a format this port can't mix; no sound\n");
        SDL_CloseAudio();
        return 0;
    }
    audioFunctional = true;
    clips.resize(SOUND_MAX);
    for (int i = 0; i < SOUND_MAX; i++) {
        clips[i].data = NULL;
        clips[i].len = 0;
    }
    musicEnabled = settings.musicVol;
    soundSetVolume(settings.soundVol);
    SDL_PauseAudio(0);
    return 1;
}

void soundDelete(void) {
    if (!audioFunctional)
        return;
    SDL_CloseAudio();
    for (size_t i = 0; i < clips.size(); i++)
        free(clips[i].data);
    clips.clear();
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioFunctional = false;
}

/* Decodes the effect's Ogg file into mono samples at OUT_RATE. */
static bool sound_load(Sound sound) {
    Clip &c = clips[sound];
    if (c.data)
        return true;
    const char *pathname = configService->soundFile(sound);
    if (!pathname)
        return false;
    FILE *f = fopen(pathname, "rb");
    if (!f) {
        errorWarning("Unable to open sound file %s", pathname);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char *)malloc(size > 0 ? size : 1);
    if (!buf || fread(buf, 1, size, f) != (size_t)size) {
        fclose(f);
        free(buf);
        return false;
    }
    fclose(f);

    int channels = 0, rate = 0;
    short *pcm = NULL;
    int frames = stb_vorbis_decode_memory(buf, (int)size, &channels, &rate, &pcm);
    free(buf);
    if (frames <= 0 || !pcm || channels < 1 || rate <= 0) {
        free(pcm);
        errorWarning("Unable to decode sound file %s", pathname);
        return false;
    }

    /* Down to mono, then to OUT_RATE (each output sample the mean of the
     * input samples it covers; the files are 44100 Hz). */
    int outLen = (int)((long long)frames * OUT_RATE / rate);
    if (outLen < 1)
        outLen = 1;
    Sint16 *data = (Sint16 *)malloc(outLen * sizeof(Sint16));
    if (!data) {
        free(pcm);
        return false;
    }
    for (int o = 0; o < outLen; o++) {
        int a = (int)((long long)o * rate / OUT_RATE);
        int b = (int)((long long)(o + 1) * rate / OUT_RATE);
        if (b <= a)
            b = a + 1;
        if (b > frames)
            b = frames;
        long sum = 0;
        int n = 0;
        for (int i = a; i < b; i++)
            for (int ch = 0; ch < channels; ch++, n++)
                sum += pcm[i * channels + ch];
        data[o] = (Sint16)(n ? sum / n : 0);
    }
    free(pcm);
    c.data = data;
    c.len = outLen;
    return true;
}

void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    if (!audioFunctional || !settings.soundVol)
        return;
    if (!sound_load(sound))
        return;

    SDL_LockAudio();
    if (!onlyOnce || !fxClip) {
        fxClip = &clips[sound];
        fxPos = 0;
        /* As Mix_PlayChannelTimed: a duration in ms loops the effect for
         * that long; -1 plays it once. */
        if (specificDurationInTicks == -1) {
            fxLoop = false;
            fxEnd = fxClip->len;
        } else {
            fxLoop = true;
            fxEnd = (int)((long long)specificDurationInTicks * OUT_RATE / 1000);
        }
    }
    SDL_UnlockAudio();
}

void soundStop() {
    if (!audioFunctional)
        return;
    SDL_LockAudio();
    fxClip = NULL;
    SDL_UnlockAudio();
}

void soundSetVolume(int volume) {
    if (!audioFunctional)
        return;
    SDL_LockAudio();
    fxVolume = volume * 256 / MAX_VOLUME;
    SDL_UnlockAudio();
}

int soundVolumeDec() {
    if (settings.soundVol > 0)
        soundSetVolume(--settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);
}

int soundVolumeInc() {
    if (settings.soundVol < MAX_VOLUME)
        soundSetVolume(++settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);
}

/* Music: not played on the Amiga (see the top of this file). */
void musicPlay(int) {}
void musicPlayLocale() {}
void musicStop() {}
void musicFadeOut(int) {}
void musicFadeIn(int, bool) {}
void musicSetVolume(int) {}

int musicVolumeDec() {
    if (settings.musicVol > 0)
        --settings.musicVol;
    return (settings.musicVol * 100 / MAX_VOLUME);
}

int musicVolumeInc() {
    if (settings.musicVol < MAX_VOLUME)
        ++settings.musicVol;
    return (settings.musicVol * 100 / MAX_VOLUME);
}

bool musicToggle() {
    if (!audioFunctional)
        return false;
    musicEnabled = !musicEnabled;
    return musicEnabled;
}

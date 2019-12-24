#ifndef OSC_H__
#define OSC_H__

#define SIZEL(a) (sizeof(a[0]))
#define HEARABLE_MAX_FREQ (20000)
#define HEARABLE_MIN_FREQ (20)
#define HEARABLE_FREQ_NUM ( MAX_FREQ - MIN_FREQ )
#define MAX_AMP (1.0)
#define ZERO_PHASE (0)
#define ZERO_PHASE_BIAS (0)
#define A_FIRST_OCTAVE_FREQ (410)

typedef struct {
	float *wave_table;
	float amp;
	jack_port_t *amp_mod_port;

	jack_nframes_t sample_rate;

	unsigned int freq;
	jack_port_t *freq_mod_port;

	int current_phase;
	jack_port_t *phase_bias_mod_port;
	int phase_bias;

	jack_client_t *client;
	jack_port_t *output_port;
	char name[BUFSIZ];
} Oscillator ;

int oscillate(jack_nframes_t nframes, Oscillator *osc); /* Oscillate for "nframes" frames. */
int oscsetamp(Oscillator *osc, float amp); /* Set amplitude.*/
int oscsetfreq(Oscillator *osc, float freq); /* Set frequency. */
int oscsetcurphase(Oscillator *osc, int current_phase); /* Set current phase. */
int oscsetphasebias(Oscillator *osc, int phase_bias); /* Set phase bias. */
int oscsetname(Oscillator *osc, char *name); /* Set name. */
Oscillator *mkosc(jack_client_t *client, char *name); /* Create standard oscillator. */
void mksinarr(float arr[], size_t siz); /* Sine shape. */
void mksawarr(float arr[], size_t siz); /* Saw shape. */
void mkpulsearr(float arr[], size_t siz);

#endif /* OSC_H__ */
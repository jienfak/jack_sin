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
	jack_port_t *phase_mod_port;
	int phase_bias;

	jack_client_t *client;
	jack_port_t *output_port;
	char name[64];
} Oscillator ;

int oscillate(jack_nframes_t nframes, Oscillator *osc); /* Oscillate for "nframes" frames. */
Oscillator *osc_new(jack_client_t *client, char *name);
void osc_make_sin(Oscillator *osc);
void osc_make_saw(Oscillator *osc);
void osc_make_pulse(Oscillator *osc);
#endif /* OSC_H__ */
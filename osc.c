/* Simple oscillator abstraction module. */

#include <math.h>
#include <jack/jack.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "osc.h"
#ifndef M_PI
	#define M_PI  (3.14159265)
#endif
static int analog2int(float analog, int max){
	return (int)((float)max*analog) ;
}
int oscillate(jack_nframes_t nframes, Oscillator *osc){
	/* Oscillate for "nframes" frames. */
#ifdef DBG
	jack_nframes_t current_frames;
	jack_time_t current_usecs, next_usecs;
	float period_usecs;
	int cycle_times = jack_get_cycle_times( osc->client,
	                                    &current_frames,
	                                    &current_usecs,
	                                    &next_usecs,
	                                    &period_usecs ) ;
	int delta_usecs = next_usecs - current_usecs ;
	fprintf(stderr, "[DBG:%s] :\tcycle_times    = %d\n"
	       "\tnframes        = %u ;\n"
	       "\tcurrent_frames = %u ;\n"
	       "\tcurrent_usecs  = %u ;\n"
	       "\tnext_usecs     = %u ;\n"
	       "\tperiod_usecs   = %f ;\n"
	       "\tdelta_usecs    = %u ;\n",
	       osc->name, cycle_times,
	       nframes,
	       current_frames,
	       current_usecs,
	       next_usecs,
	       period_usecs,
	       delta_usecs);
#endif /* DBG */
	jack_default_audio_sample_t *out, *amp_mod, *freq_mod, *phase_mod;
	out = jack_port_get_buffer (osc->output_port, nframes) ;
	amp_mod = jack_port_get_buffer(osc->amp_mod_port, nframes) ;
	freq_mod = jack_port_get_buffer(osc->freq_mod_port, nframes) ;
	phase_mod = jack_port_get_buffer(osc->phase_mod_port, nframes) ;
	
	for( int i=0 ; i<nframes ; ++i ){ /* Processing N frames of output buffer. */
		/* Modulating phase. */
		int real_phase =  (osc->current_phase+osc->phase_bias+analog2int(phase_mod[i], osc->sample_rate)) % osc->sample_rate ;

		out[i] = osc->wave_table[real_phase] * osc->amp * (amp_mod[i] ? amp_mod[i] : 1. ) ; /* Modulating amplitude. */

		int analog_freq_mod_val = analog2int(freq_mod[i], osc->sample_rate) ;
		
		osc->current_phase +=  osc->freq+analog_freq_mod_val ; /* Modulating frequency. */
		osc->current_phase %= osc->sample_rate ;		
		
	}
	return 0 ;
}

int osc_set_amp(Oscillator *osc, float amp){
	osc->amp = amp ;
	return 0 ;
}

int osc_set_freq(Oscillator *osc, int freq){
	osc->freq = freq ;
	return 0 ;
}

int osc_set_phase_bias(Oscillator *osc, int phase_bias){
	osc->phase_bias = phase_bias ;
	return 0 ;
}



int osc_set_name(Oscillator *osc, char *name){
	strncpy(osc->name, name, sizeof(osc->name));
	return 0 ;
}

static char *strcpycat(char *buf, char *s1, char *s2){
	strcpy(buf, s1);
	strcat(buf, s2);
	return buf ;
}
static jack_port_t *make_in_port(Oscillator *osc, char *suffix){
	char buf[BUFSIZ];
	return jack_port_register(osc->client, strcpycat(buf,
		osc->name, suffix),
		JACK_DEFAULT_AUDIO_TYPE,
		JackPortIsInput, 0) ;
}

Oscillator *osc_new(jack_client_t *client, char *name){
	/* Returns pointer to new oscillator structure with default values.. */
	/* Values. */
	Oscillator *osc = malloc(sizeof(Oscillator)) ;
	osc->client = client ;
	osc->sample_rate = jack_get_sample_rate(client) ;
	osc->wave_table = malloc(SIZEL(osc->wave_table)*osc->sample_rate) ;
	osc->freq = A_FIRST_OCTAVE_FREQ ;
	osc->current_phase =  ZERO_PHASE ;
	osc->phase_bias = ZERO_PHASE_BIAS ;
	osc->amp =  0.1 ;
	osc_set_name(osc, name);

	/* Ports creating. */
	osc->output_port = jack_port_register (osc->client, osc->name,
	                    JACK_DEFAULT_AUDIO_TYPE,
	                    JackPortIsOutput, 0) ;
	osc->freq_mod_port = make_in_port(osc, "_freq_mod") ;
	osc->phase_mod_port = make_in_port(osc, "_phase_mod") ;
	osc->amp_mod_port = make_in_port(osc, "_amp_mod") ;

	return osc ;
}

void osc_make_sin(Oscillator *osc){
	int siz = osc->sample_rate ;
	float *buf = osc->wave_table ;
	for( int i=0 ; i<siz ; ++i ){
		buf[i] = (float) sin( ((double)i/(double)siz) * M_PI * 2. ) ;
	}
}

void osc_make_saw(Oscillator *osc){
	int siz = osc->sample_rate ;
	float *buf = osc->wave_table ;
	for( int i=0 ; i<siz ; ++i ){
		buf[i] = (double)i/siz ;
	}
}

void osc_make_pulse(Oscillator *osc){
	int siz = osc->sample_rate ;
	float *buf = osc->wave_table ;
	int half_siz = siz / 2 ;
	for( int i=0 ; i<siz ; ++i ){
		buf[i]  = (float)((int)i/half_siz) ;
	}
}
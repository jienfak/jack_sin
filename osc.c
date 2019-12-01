/* Simple oscillator abstraction module. */
#include "osc.h"
#include <math.h>

int oscillate(jack_nframes_t nframes, Oscillator *osc){
	/* Oscillate for "nframes" frames. */
	jack_nframes_t current_frames;
	jack_time_t current_usecs, next_usecs;
	float period_usecs;
	int cycle_times = jack_get_cycle_times( osc->client,
	                                    &current_frames,
	                                    &current_usecs,
	                                    &next_usecs,
	                                    &period_usecs ) ;
	int delta_usecs = next_usecs - current_usecs ;
#ifdef DBG
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
	jack_default_audio_sample_t *out;
	out = (jack_default_audio_sample_t *)jack_port_get_buffer (osc->output_port, nframes);
	for( int i=0 ; i<nframes ; ++i ){ /* Processing N frames of output buffer(I thought I will use delta-t ore something). */
		int real_phase =  (osc->current_phase+osc->phase_bias) % osc->sample_rate ;
		out[i] = osc->wave_table[real_phase] * osc->amp ;
		osc->current_phase = osc->current_phase + osc->freq ;
		if( osc->current_phase >= osc->sample_rate + osc->phase_bias )
			osc->current_phase -= osc->sample_rate ;
	}
	return 0 ;
}

int oscsetamp(Oscillator *osc, float amp){
	osc->amp = amp ;
	return 0 ;
}

int oscsetfreq(Oscillator *osc, float freq){
	osc->freq = freq ;
	return 0 ;
}

int oscsetcurphase(Oscillator *osc, int current_phase){
	osc->current_phase = current_phase % osc->sample_rate ;
	return 0 ;
}

int oscsetphasebias(Oscillator *osc, int phase_bias){
	osc->phase_bias = phase_bias ;
	return 0 ;
}

int oscsetname(Oscillator *osc, char *name){
	osc->name = name ;
	return 0 ;
}

Oscillator *mkosc(jack_client_t *client, char *name){
	/* Returns pointer to new oscillator structure. */
	Oscillator *osc = malloc(SIZEL(osc)) ;
	osc->client = client ;
	osc->sample_rate = jack_get_sample_rate(osc->client) ;
	osc->wave_table = malloc(SIZEL(osc->wave_table)*osc->sample_rate) ;
	oscsetfreq(osc, A_FIRST_OCTAVE_FREQ);
	oscsetcurphase(osc, ZERO_PHASE);
	oscsetphasebias(osc, ZERO_PHASE_BIAS);
	oscsetamp(osc, ZERO_AMP);
	oscsetname(osc, name);
	/* Ports creation. */
	osc->output_port = jack_port_register (osc->client, "out",
	                    JACK_DEFAULT_AUDIO_TYPE,
	                    JackPortIsOutput, 0) ;
	return osc ;
}

void mksinarr(float arr[], size_t siz){
	for( int i=0 ; i<siz ; i++ ){
		arr[i] = (float) sin( ((double)i/(double)siz) * M_PI * 2. ) ;
	}
}

void mksawarr(float arr[], size_t siz){
	for( int i=0 ; i<siz ; ++i ){
		arr[i] = (double)i/siz ;
	}
}

void mkpulsearr(float arr[], size_t siz){
	int half_siz = siz/2 ;
	for( int i=0 ; i<siz ; ++i ){
		arr[i] = (float)((int)i/half_siz) ;
	}
}
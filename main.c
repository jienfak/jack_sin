/** @file simple_client.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define SIZEL(a) (sizeof(a[0]))
#define HEARABLE_MAX_FREQ (20000)
#define HEARABLE_MIN_FREQ (20)
#define HEARABLE_FREQ_NUM ( MAX_FREQ - MIN_FREQ )
#define ZERO_AMP (1.0)
#define ZERO_PHASE (0)
#define ZERO_PHASE_BIAS (0)
#define A_FIRST_OCTAVE_FREQ (410)

typedef struct {
	float *wave_table;
	float amp;
	jack_nframes_t sample_rate;
	unsigned int freq;
	int current_phase;
	int phase_bias;

	jack_client_t *client;
	jack_port_t *output_port;
	char *name;
} Oscillator ;
/*
static void signal_handler(int sig)
{
	jack_client_close(client);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);

	}
*/
/*
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

int oscillate(jack_nframes_t nframes, Oscillator *osc){
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
	/* Creates oscillator in 'osc' pointer. */
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

int process(jack_nframes_t nframes, void *arg){
	Oscillator *osc = (Oscillator *)arg ;
	return oscillate(nframes, osc) ;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown (void *arg){
	exit(1);
}

void usage(char **argv0){
	exit(1);
}

int main(int argc, char *argv[]){
	const char **ports;
	const char *client_name;
	const char *server_name = NULL;
	float *wave_table;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	jack_client_t *client;
	Oscillator *osc;
	int i;

	/*if( argc<3 ){
		usage(argv);
	}*/

	if( argc >= 2 ){/* Client name specified? */
		client_name = argv[1];
		if( argc >= 3 ){/* Server name specified? */
			server_name = argv[2];
			int my_option = JackNullOption | JackServerName;
			options = (jack_options_t)my_option;
		}
	}else{ /* Use basename of argv[0]. */
		client_name = strrchr(argv[0], '/');
		if (client_name == 0) {
			client_name = argv[0];
		} else {
			client_name++;
		}
	}


	//client_name = argv[1] ;

	/* Open a client connection to the JACK server. */
	client = jack_client_open(client_name, options, &status, server_name) ;
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			            "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "%s: unable to connect to JACK server\n", argv[0]);
		}
		exit (1);
	}if( status & JackServerStarted ){
		fprintf (stderr, "JACK server started\n");
	}if(status & JackNameNotUnique){
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}
	osc = mkosc(client, "01") ;
	mkpulsearr(osc->wave_table, osc->sample_rate);

	/* Tell the JACK server to call `process()' whenever
	 * there is work to be done. */

	jack_set_process_callback(osc->client, process, osc);

	/* Tell the JACK server to call `jack_shutdown()' if
	 * it ever shuts down, either entirely, or if it
	 * just decides to stop calling us. */

	jack_on_shutdown(osc->client, jack_shutdown, 0);


	if ((osc->output_port == NULL) ) {
		fprintf(stderr, "%s: no more JACK ports available.\n", argv[0]);
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate(osc->client)) {
		fprintf (stderr, "%s: cannot activate client.\n", argv[0]);
		exit(1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from it. */

	/*ports = jack_get_ports (osc.client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}*/

	/*if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}*/

	/*if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}*/

	/*free (ports);*/
    
	/* install a signal handler to properly quits jack client */
#ifdef WIN32
	/*signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);
#else
	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);*/
#endif

	/* Keep running until the Ctrl+C. */
	while (1) {
	#ifdef WIN32
		Sleep(1000);
	#else
		usleep(20000);
	#endif
	}

	jack_client_close (osc->client);
	exit (0);
}

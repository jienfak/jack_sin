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

#define MAX_FREQ (20000)
#define MIN_FREQ (20)
#define FREQ_NUM ( MAX_FREQ - MIN_FREQ )

typedef struct {
	float *wave_table;
	jack_nframes_t sample_rate;
	unsigned int freq;
	int current_phase;
	int phase_bias;
	int phase_step;
	jack_port_t *output_port;
	jack_client_t *client;
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

int process(jack_nframes_t nframes, void *arg){
	Oscillator *osc = (Oscillator *)arg ;
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
	fprintf(stderr, "[DBG] :\tcycle_times    = %d\n"
	       "\tnframes        = %u ;\n"
	       "\tcurrent_frames = %u ;\n"
	       "\tcurrent_usecs  = %u ;\n"
	       "\tnext_usecs     = %u ;\n"
	       "\tperiod_usecs   = %f ;\n"
	       "\tdelta_usecs    = %u ;\n",
	       cycle_times,
	       nframes,
	       current_frames,
	       current_usecs,
	       next_usecs,
	       period_usecs,
	       delta_usecs);
#endif /* DBG */
	jack_default_audio_sample_t *out;
	int i;
	out = (jack_default_audio_sample_t *)jack_port_get_buffer (osc->output_port, nframes);
	for( i=0 ; i<nframes ; ++i ){ /* Processing N frames of output buffer(I thought I will use delta-t ore something). */
		out[i] = osc->wave_table[osc->current_phase] ;
		osc->current_phase += osc->phase_step * osc->freq ;
		if( osc->current_phase >= osc->sample_rate ) osc->current_phase -= osc->sample_rate ;
	}
	return 0 ;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main(int argc, char *argv[]){
	const char **ports;
	const char *client_name;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	Oscillator osc;
	int i;

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

	/* Open a client connection to the JACK server. */
	osc.client = jack_client_open (client_name, options, &status, server_name);
	if (osc.client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "%s: unable to connect to JACK server\n", argv[0]);
		}
		exit (1);
	}if( status & JackServerStarted ){
		fprintf (stderr, "JACK server started\n");
	}if(status & JackNameNotUnique){
		client_name = jack_get_client_name(osc.client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}

	osc.sample_rate = jack_get_sample_rate(osc.client) ;
	osc.wave_table = malloc(sizeof(osc.wave_table[0]) * osc.sample_rate ) ;
	osc.current_phase = 0 ;
	osc.phase_bias = 0 ;
	osc.phase_step = 1 ;
	osc.freq = 410 ;

	for( i=0; i<osc.sample_rate; i++ ){
		osc.wave_table[i] = 0.1 * (float) sin( ((double)i/(double)osc.sample_rate) * M_PI * 2. );
	}


	/* Tell the JACK server to call `process()' whenever
	 * there is work to be done. */

	jack_set_process_callback(osc.client, process, &osc);

	/* Tell the JACK server to call `jack_shutdown()' if
	 * it ever shuts down, either entirely, or if it
	 * just decides to stop calling us. */

	jack_on_shutdown(osc.client, jack_shutdown, 0);

	/* Ports creation. */
	osc.output_port = jack_port_register (osc.client, "out",
	                    JACK_DEFAULT_AUDIO_TYPE,
	                    JackPortIsOutput, 0) ;

	if ((osc.output_port == NULL) ) {
		fprintf(stderr, "%s: no more JACK ports available.\n", argv[0]);
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (osc.client)) {
		fprintf (stderr, "%s: cannot activate client.\n", argv[0]);
		exit (1);
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
		++osc.freq;
	#endif
	}

	jack_client_close (osc.client);
	exit (0);
}

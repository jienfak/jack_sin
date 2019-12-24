
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <jack/jack.h>
#include "osc.h"

#define MAX_OSCILLATORS_AMOUNT 64
typedef struct {
	Oscillator **oscs;
	int oscs_amt;
} Arg ;

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





int process(jack_nframes_t nframes, void *out_arg){
	Arg *arg = (Arg *)out_arg ;
	for( int i=0 ; i < arg->oscs_amt ; ++i ){
		oscillate(nframes, arg->oscs[i]);
	}
	return 0 ;
}

/*
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
	Oscillator *osc, *osc1;
	Arg *arg = malloc(sizeof(Arg)) ;
	Oscillator **oscs = (Oscillator **)malloc(sizeof(Oscillator *)*MAX_OSCILLATORS_AMOUNT) ;
	arg->oscs = oscs ;
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
	mksinarr(osc->wave_table, osc->sample_rate);
	osc1 = mkosc(client, "02") ;
	mksinarr(osc1->wave_table, osc->sample_rate);

	oscs[0] = osc; oscs[1] = osc1 ;	
	oscsetfreq(osc1, 210);

	arg->oscs_amt = 2 ;
	

	/* Tell the JACK server to call `process()' whenever
		there is work to be done. */

	jack_set_process_callback(osc->client, process, arg);

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

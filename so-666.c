#include <stdio.h>	
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>
#include "so-666.h"
int done;

#define NUMNOTES 80
#define BASENOTE 21

double *strings[NUMNOTES];
unsigned int stringpos[NUMNOTES];
unsigned int stringlength[NUMNOTES];
double stringcutoff[NUMNOTES];
int status[NUMNOTES];

unsigned int samplerate;
double lpval, lplast;
double hpval, hplast;
double fcutoff, freso, ffeedback;
unsigned int feedback, cutoff, resonance, volume;


double dist( double in )
{
	double out;
	
	out = tanh( in );
	
	return out;
}

int runSO_666( LV2_Handle *arg, uint32_t frames )
{
	jack_default_audio_sample_t *outbuffer;
	int i, note;
	double  sample, damp;

	outbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( outport, nframes );

	for( i=0; i<nframes; i++ )
	{
		sample = (((double)rand()/(double)RAND_MAX)*2.0-1.0)*0.001;

		for( note=0; note<NUMNOTES; note++ )
		{
			damp = stringcutoff[note];

			if( stringpos[note] > 0 )
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
								 strings[note][stringpos[note]-1]*(1.0-damp);
			else
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
								 strings[note][stringlength[note]-1]*(1.0-damp);

			strings[note][stringpos[note]] = dist( strings[note][stringpos[note]] ) * 0.99;

			sample += strings[note][stringpos[note]];
		}

		hpval += (sample-hplast) * 0.0001;
		hplast += hpval;
		hpval *= 0.96;
		sample -= hplast;

		lpval += (sample-lplast) * fcutoff * (1.0-tanh(lplast)*tanh(lplast)*0.9);
		lplast += lpval;
		lpval *= freso;
		sample = lplast;
		
		for( note=0; note<NUMNOTES; note++ )
		{
			if( status[note] > 0 )
			{
				strings[note][stringpos[note]] += sample*ffeedback;
			}

			if( fabs( strings[note][stringpos[note]] ) <= 0.0001 )
				strings[note][stringpos[note]] = 0.0;

			stringpos[note]++;
			if( stringpos[note] >= stringlength[note] ) stringpos[note] = 0;
		}

		outbuffer[i] = dist( sample ) * (volume/127.0);
	}
	
	return 0;
}

int main( int argc, char *argv[] )
{
	jack_client_t *jackClient;

	snd_seq_t *seqport;
	struct pollfd *pfd;
	int npfd;
	snd_seq_event_t *midievent;
	int channel, midiport;
	
	int note, length, i;
	double freq;

	puts( "SO-666 v.1.0 by 50m30n3 2009" );

	if( argc > 1 )
		channel = atoi( argv[1] );
	else
		channel = 0;

	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );


	puts( "Connecting to Jack Audio Server" );
	
	jackClient = jack_client_open( "SO-666", JackNoStartServer, NULL );
	if( jackClient == NULL )
	{
		fputs( "Cannot connect to Jack Server\n", stderr );
		return 1;
	}

	jack_on_shutdown( jackClient, jack_shutdown, 0 );

	outport = jack_port_register( jackClient, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsTerminal, 0 );

	jack_set_process_callback( jackClient, process, 0 );

	samplerate = jack_get_sample_rate( jackClient );


	puts( "Initializing synth parameters" );

	feedback = 32;
	cutoff = 64;
	resonance = 64;
	volume = 100;

	fcutoff = pow( (cutoff+50.0)/200.0, 5.0 );
	freso = resonance/127.0;
	ffeedback = 0.01+pow( feedback/127.0, 4.0)*0.9;

	for( note=0; note<NUMNOTES; note++ )
	{
		freq = 440.0*pow( 2.0, (note+BASENOTE-69) / 12.0 );
		//stringcutoff[note] = ( freq * 16.0 ) / (double)samplerate;
		stringcutoff[note] = 0.9;
		length = (double)samplerate / freq;
		stringlength[note] = length;
		strings[note] = malloc( length * sizeof( double ) );
		if( strings[note] == NULL )
		{
			fputs( "Error allocating memory\n", stderr );
			return 1;
		}
		
		for( i=0; i<length; i++ )
		{
			strings[note][i] = 0.0;
		}
		stringpos[note] = 0;
		status[note] = 0;
	}

	jack_activate( jackClient );


	printf( "Listening on MIDI channel %i\n", channel );

	if( snd_seq_open( &seqport, "default", SND_SEQ_OPEN_INPUT, 0 ) < 0 )
	{
		fputs( "Cannot connect to ALSA sequencer\n", stderr );
		return 1;
	}

	snd_seq_set_client_name( seqport, "SO-666" );

	midiport = snd_seq_create_simple_port( seqport, "input",
		SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
		SND_SEQ_PORT_TYPE_APPLICATION );

	if( midiport < 0 )
	{
		fputs( "Cannot create ALSA sequencer port\n", stderr );
		return 1;
	}

	npfd = snd_seq_poll_descriptors_count( seqport, POLLIN );
	pfd = (struct pollfd *)malloc( npfd * sizeof( struct pollfd ) );
	snd_seq_poll_descriptors( seqport, pfd, npfd, POLLIN );


	done = 0;

	while( ! done )
	{
		if( poll( pfd, npfd, 100000 ) > 0 )
		{
			do
			{
				snd_seq_event_input( seqport, &midievent );
				
				if( ( midievent->type == SND_SEQ_EVENT_NOTEON ) && ( midievent->data.note.channel == channel ) )
				{
					note = midievent->data.note.note;
					if( ( note >= BASENOTE ) && ( note < BASENOTE+NUMNOTES ) )
					{
						note -= BASENOTE;

						status[note] = 1;
					}
				}
				else if( ( midievent->type == SND_SEQ_EVENT_NOTEOFF ) && ( midievent->data.note.channel == channel ) )
				{
					note = midievent->data.note.note;
					if( ( note >= BASENOTE ) && ( note < BASENOTE+NUMNOTES ) )
					{
						note -= BASENOTE;
						status[note] = 0;
					}
				}
				else if( ( midievent->type == SND_SEQ_EVENT_CONTROLLER ) && ( midievent->data.control.channel == channel ) )
				{
					if( midievent->data.control.param == 74 )
					{
						cutoff = midievent->data.control.value;
						fcutoff = pow( (cutoff+50.0)/200.0, 5.0 );
						printf( "Cutoff: %i     \r", cutoff );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 71 )
					{
						resonance = midievent->data.control.value;
						freso = resonance/127.0;
						printf( "Resonance: %i     \r", resonance );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 7 )
					{
						volume = midievent->data.control.value;
						printf( "Volume: %i     \r", volume );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 1 )
					{
						feedback = midievent->data.control.value;
						ffeedback = 0.01+pow( feedback/127.0, 4.0)*0.9;
						printf( "Feedback: %i    \r", feedback );
						fflush( stdout );
					}
				}
				
				snd_seq_free_event( midievent );
			}
			while( snd_seq_event_input_pending( seqport, 0 ) > 0 );
		}
	}

	return 0;
}

void cleanupSO_666(LV2_Handle instance) {
	
	free( pfd );
	snd_seq_delete_port( seqport, midiport );
	snd_seq_close( seqport );
	
	jack_deactivate( jackClient );

	puts( "Freeing data" );
	for( note=0; note<NUMNOTES; note++ )
	{
		free( strings[note] );
	}

	jack_port_unregister( jackClient, outport );
	jack_client_close( jackClient );

}

static LV2_Descriptor so-666-Descriptor= {
	.URI="urn:50m30n3:plugins:so-666",
	.instantiate=NULL,
	.connect_port=NULL,
	.activate=NULL,
	.run=runSO_666,
	.deactivate=NULL,
	.cleanup=cleanupSO_666,
	.extension_data=NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    switch(index) {
        case 0:
            return so-666-descriptor;
        default:
            return NULL;
    }
}


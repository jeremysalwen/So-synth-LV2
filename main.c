/*
*    This file is part of SO-KL5.
*
*    SO-KL5 is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    SO-KL5 is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with SO-KL5.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#include <jack/jack.h>

#include <alsa/asoundlib.h>

int done;

#define NUMNOTES 80
#define BASENOTE 21

double *strings[NUMNOTES];
unsigned int stringpos[NUMNOTES];
unsigned int stringlength[NUMNOTES];
double stringcutoff[NUMNOTES];
int status[NUMNOTES];

unsigned int samplerate;
double hpval, hplast;
double lpval, lplast;
double fcutoff, freso, ssustain, sattack;
unsigned int sustain, cutoff, resonance, attack, volume;

jack_port_t *outport;

void sig_exit( int sig )
{
	puts( "Got SINGINT or SIGTERM, shutting down" );
	done = 1;
}

void jack_shutdown( void *arg )
{
	puts( "Jack kicked us, were boned" );
	exit( 1 );
}

int process( jack_nframes_t nframes, void *arg )
{
	jack_default_audio_sample_t *outbuffer;
	int i, note;
	double damp;
	double sample;

	outbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( outport, nframes );

	for( i=0; i<nframes; i++ )
	{
		sample = 0.0;

		for( note=0; note<NUMNOTES; note++ )
		{
			damp = stringcutoff[note];

			if( stringpos[note] > 0 )
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
								 strings[note][stringpos[note]-1]*(1.0-damp);
			else
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
								 strings[note][stringlength[note]-1]*(1.0-damp);

			damp = ((double)note/(double)NUMNOTES)*0.009999;

			if( status[note] == 0 )
				strings[note][stringpos[note]] *= 0.8+ssustain*0.19+damp;
			else
				strings[note][stringpos[note]] *= 0.99+damp;

			sample += strings[note][stringpos[note]];
		}

		hpval += (sample-hplast) * 0.00001;
		hplast += hpval;
		hpval *= 0.96;
		sample -= hplast;

		for( note=0; note<NUMNOTES; note++ )
		{
			damp = 1.0-((double)note/(double)NUMNOTES);
			strings[note][stringpos[note]] += sample*damp*0.001;

			if( fabs( strings[note][stringpos[note]] ) <= 0.00001 )
				strings[note][stringpos[note]] = 0.0;

			stringpos[note]++;
			if( stringpos[note] >= stringlength[note] ) stringpos[note] = 0;
		}

		lpval += (sample-lplast) * fcutoff;
		lplast += lpval;
		lpval *= freso;
		sample = lplast;
		
		outbuffer[i] = sample * (volume/127.0);
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
	
	int note, length, i, j, minpos;
	double freq, avg, vol, scale, min;
	double *tempstring;

	puts( "SO-KL5 v.1.1 by 50m30n3 2010" );

	if( argc > 1 )
		channel = atoi( argv[1] );
	else
		channel = 0;

	signal( SIGINT, sig_exit );
	signal( SIGTERM, sig_exit );


	puts( "Connecting to Jack Audio Server" );
	
	jackClient = jack_client_open( "SO-KL5", JackNoStartServer, NULL );
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

	sustain = 0;
	cutoff = 64;
	resonance = 100;
	attack = 64;
	volume = 100;

	fcutoff = (cutoff+5.0)/400.0;
	sattack = (attack+5.0)/800.0;
	freso = (resonance/160.0)*(1.0-fcutoff);
	ssustain = 0.6+pow( sustain/127.0, 0.4)*0.4;

	for( note=0; note<NUMNOTES; note++ )
	{
		freq = 440.0*pow( 2.0, (note+BASENOTE-69) / 12.0 );
		stringcutoff[note] = 0.3 + pow( (double)note / (double)NUMNOTES, 0.5 ) * 0.65;
		length = round( (double)samplerate / freq );
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

	freq = 440.0*pow( 2.0, (BASENOTE-69) / 12.0 );
	length = (double)samplerate / freq;
	tempstring = malloc( length * sizeof( double ) );
	if( tempstring == NULL )
	{
		fputs( "Error allocating memory\n", stderr );
		return 1;
	}


	jack_activate( jackClient );


	printf( "Listening on MIDI channel %i\n", channel );

	if( snd_seq_open( &seqport, "default", SND_SEQ_OPEN_INPUT, 0 ) < 0 )
	{
		fputs( "Cannot connect to ALSA sequencer\n", stderr );
		return 1;
	}

	snd_seq_set_client_name( seqport, "SO-KL5" );

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

						for( i=0; i<stringlength[note]; i++ )
						{
							tempstring[i] = ((double)rand()/(double)RAND_MAX)*2.0-1.0;
						}

						freq = stringcutoff[note] * 0.25 + midievent->data.note.velocity/127.0 * 0.2 + sattack + 0.1;

						for( j=0; j<30; j++ )
						{
							tempstring[0] = tempstring[0]*freq + tempstring[stringlength[note]-1]*(1.0-freq);
							for( i=1; i<stringlength[note]; i++ )
							{
								tempstring[i] = tempstring[i]*freq + tempstring[(i-1)%stringlength[note]]*(1.0-freq);
							}
						}

						avg = 0.0;

						for( i=0; i<stringlength[note]; i++ )
						{
							avg += tempstring[i];
						}

						avg /= stringlength[note];

						scale = 0.0;

						for( i=0; i<stringlength[note]; i++ )
						{
							tempstring[i] -= avg;
							if( fabs( tempstring[i] ) > scale )
								scale = fabs( tempstring[i] );
						}

						min = 10.0;
						minpos = 0;

						for( i=0; i<stringlength[note]; i++ )
						{
							tempstring[i] /= scale;
							if( fabs( tempstring[i] ) + fabs( tempstring[i] - tempstring[i-1] ) * 5.0 < min )
							{
								min = fabs( tempstring[i] ) + fabs( tempstring[i] - tempstring[i-1] ) * 5.0;
								minpos = i;
							}
						}

						vol = midievent->data.note.velocity/256.0;

						for( i=0; i<stringlength[note]; i++ )
						{
							strings[note][(stringpos[note]+i)%stringlength[note]] += tempstring[(i+minpos)%stringlength[note]]*vol;
						}
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
						fcutoff = (cutoff+5.0)/400.0;
						printf( "Cutoff: %i     \r", cutoff );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 71 )
					{
						resonance = midievent->data.control.value;
						freso = (resonance/160.0)*(1.0-fcutoff);
						printf( "Resonance: %i     \r", resonance );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 73 )
					{
						attack = midievent->data.control.value;
						sattack = (attack+5.0)/800.0;
						printf( "Attack: %i     \r", attack );
						fflush( stdout );
					}
					else if( midievent->data.control.param == 7 )
					{
						volume = midievent->data.control.value;
						printf( "Volume: %i     \r", volume );
						fflush( stdout );
					}
					else if( ( midievent->data.control.param == 64 ) || ( midievent->data.control.param == 1 ) )
					{
						sustain = midievent->data.control.value;
						ssustain = 0.6+pow( sustain/127.0, 0.4)*0.4;
						printf( "Sustain: %i    \r", sustain );
						fflush( stdout );
					}
				}
				
				snd_seq_free_event( midievent );
			}
			while( snd_seq_event_input_pending( seqport, 0 ) > 0 );
		}
	}

	free( pfd );
	snd_seq_delete_port( seqport, midiport );
	snd_seq_close( seqport );
	
	jack_deactivate( jackClient );

	puts( "Freeing data" );
	for( note=0; note<NUMNOTES; note++ )
	{
		free( strings[note] );
	}
	free( tempstring );

	jack_port_unregister( jackClient, outport );
	jack_client_close( jackClient );

	return 0;
}


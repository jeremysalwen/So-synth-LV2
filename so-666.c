
#include "so-666.h"
int done;



double dist( double in )
{
	double out;

	out = tanh( in );

	return out;
}

static void runSO_666( LV2_Handle arg, uint32_t nframes )
{
	so_666* so=(so_666*)arg;
	float* outbuffer=so->output;

	int i, note;
	double  sample, damp;

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

}

int main( int argc, char *argv[] )
{


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
static LV2_Handle instantiateSO_666(const LV2_Descriptor *descriptor,double s_rate, const char *path,const LV2_Feature * const* features) {
	so_666* so=malloc(sizeof(so_666));
	LV2_URI_Map_Feature *map_feature;
	const LV2_Feature * const *  i;
	for (i = features; *i; i++) {
		if (!strcmp((*i)->URI, "http://lv2plug.in/ns/ext/uri-map")) {
		            map_feature = (*i)->data;
		            so->midi_event_id = map_feature->uri_to_id(map_feature->callback_data,"http://lv2plug.in/ns/ext/event","http://lv2plug.in/ns/ext/midi#MidiEvent");
		} else if (!strcmp((*i)->URI, "http://lv2plug.in/ns/ext/event")) {
		so->event_ref = (*i)->data;
	}
	
	int npfd;
	int channel, midiport;

	int note, length, i;
	double freq;

	puts( "SO-666 v.1.0 by 50m30n3 2009" );

	channel = 0;

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

	return so;
}
			
static void cleanupSO_666(LV2_Handle instance) {

	puts( "Freeing data" );
	int note;
	for(note=0; note<NUMNOTES; note++ )
	{
		free( strings[note] );
	}

}

static LV2_Descriptor so_666_Descriptor= {
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
			return &so_666_Descriptor;
		default:
			return NULL;
	}
}


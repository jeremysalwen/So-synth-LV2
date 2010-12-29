#include "so-666.h"

double dist( double in )
{
	double out;

	out = tanh( in );

	return out;
}

static void runSO_666( LV2_Handle arg, uint32_t nframes )
{
	so_666* so=(so_666*)arg;
	lv2_event_begin(&so->in_iterator,so->MidiIn);
	double **strings=so->strings;
	unsigned int* stringpos=so->stringpos;
	unsigned int* stringlength=so->stringlength;
	float* outbuffer=so->output;
	int * status=so->status;

	int i, note;
	double  sample, damp;

for( i=0; i<nframes; i++ )
{
	while(lv2_event_is_valid(&so->in_iterator)) {
		LV2_Event* event=lv2_event_get(&so->in_iterator);
		if(event->frames>iframes) {
			break;
		}
		if(event->type==so->midi_event_id) {
			const uint8_t* mididata=((unit8_t*)event)+sizeof(LV2_Event);
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
			lv2_event_increment(&so->in_iterator);
			}
		sample = (((double)rand()/(double)RAND_MAX)*2.0-1.0)*0.001;

		for( note=0; note<NUMNOTES; note++ )
		{
			damp = so->stringcutoff[note];

			if( stringpos[note] > 0 )
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
				strings[note][stringpos[note]-1]*(1.0-damp);
			else
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
				strings[note][stringlength[note]-1]*(1.0-damp);

			strings[note][stringpos[note]] = dist( strings[note][stringpos[note]] ) * 0.99;

			sample += strings[note][stringpos[note]];
		}

		so->hpval += (sample-so->hplast) * 0.0001;
		so->hplast += so->hpval;
		so->hpval *= 0.96;
		sample -= so->hplast;

		so->lpval += (sample-so->lplast) * so->fcutoff * (1.0-tanh(so->lplast)*tanh(so->lplast)*0.9);
		so->lplast +=so-> lpval;
		so->lpval *= so->freso;
		sample = so->lplast;

		for( note=0; note<NUMNOTES; note++ )
		{
			if( status[note] > 0 )
			{
				strings[note][stringpos[note]] += sample*so->ffeedback;
			}

			if( fabs( strings[note][stringpos[note]] ) <= 0.0001 )
				strings[note][stringpos[note]] = 0.0;

			stringpos[note]++;
			if( stringpos[note] >= stringlength[note] ) stringpos[note] = 0;
		}

		outbuffer[i] = dist( sample ) * (so->volume/127.0);
	}

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
		//so->stringcutoff[note] = ( freq * 16.0 ) / (double)samplerate;
		so->stringcutoff[note] = 0.9;
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
	.instantiate=InstantiateSO_666,
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


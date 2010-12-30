#include "so-666.h"

double dist( double in )
{
	double out;
	out = tanh( in );
	return out;
}

static void runSO_666( LV2_Handle arg, uint32_t nframes ) {
	so_666* so=(so_666*)arg;
	lv2_event_begin(&so->in_iterator,so->MidiIn);
	double **strings=so->strings;
	unsigned int* stringpos=so->stringpos;
	unsigned int* stringlength=so->stringlength;
	float* outbuffer=so->output;
	int * status=so->status;

	int i, note;
	double  sample, damp;

	for( i=0; i<nframes; i++ ) {
		while(lv2_event_is_valid(&so->in_iterator)) {
			uint8_t* data;
			LV2_Event* event= lv2_event_get(&so->in_iterator,&data);
			if (event->type == 0) {
				so->event_ref->lv2_event_unref(so->event_ref->callback_data, event);
			} else if(event->type==so->midi_event_id) {
				if(event->frames > i) {
					break;
				} else{
					const midi_event* evt=(midi_event*)data;
					if(evt->channel==so->channel) {
						if( evt->command==MIDI_NOTEON) 	{
							note = evt->info1;
							if( ( note >= BASENOTE ) && ( note < BASENOTE+NUMNOTES ) ) {
								note -= BASENOTE;
								status[note] = 1;
							}
						}
						else if(  evt->command==MIDI_NOTEOFF )	{
							note = evt->info1;
							if( ( note >= BASENOTE ) && ( note < BASENOTE+NUMNOTES ) ) {
								note -= BASENOTE;
								status[note] = 0;
							}
						}
						else if( evt->command==MIDI_CONTROL )	{
							if( evt->info1 == 74 )	{
								unsigned int cutoff =evt->info2;
								so->fcutoff = pow( (cutoff+50.0)/200.0, 5.0 );
								printf( "Cutoff: %i     \r", cutoff );
								fflush( stdout );
							}
							else if( evt->info1 == 71 )	{
								unsigned int resonance = evt->info2;
								so->freso = resonance/127.0;
								printf( "Resonance: %i     \r", resonance );
								fflush( stdout );
							}
							else if( evt->info1 == 7 )	{
								so->volume = evt->info2;
								printf( "Volume: %i     \r", so->volume );
								fflush( stdout );
							}
							else if( evt->info1== 1 ) {
								unsigned int feedback =evt->info2;
								so->ffeedback = 0.01+pow( feedback/127.0, 4.0)*0.9;
								printf( "Feedback: %i    \r", feedback );
								fflush( stdout );
							}
						}
					}
				}
			}
			lv2_event_increment(&so->in_iterator);
		}
		sample = (((double)rand()/(double)RAND_MAX)*2.0-1.0)*0.001;

		for( note=0; note<NUMNOTES; note++ ) {
			damp = so->stringcutoff[note];

			if( stringpos[note] > 0 ) {
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
					strings[note][stringpos[note]-1]*(1.0-damp);
			} else {
				strings[note][stringpos[note]] = strings[note][stringpos[note]]*damp +
					strings[note][stringlength[note]-1]*(1.0-damp);
			}
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

		for( note=0; note<NUMNOTES; note++ ) {
			if( status[note] > 0 ) {
				strings[note][stringpos[note]] += sample*so->ffeedback;
			}

			if( fabs( strings[note][stringpos[note]] ) <= 0.0001 ) {
				strings[note][stringpos[note]] = 0.0;
			}
			stringpos[note]++;
			if( stringpos[note] >= stringlength[note] ) {
				stringpos[note] = 0;
			}
		}

		outbuffer[i] = dist( sample ) * (so->volume/127.0);
	}
}

LV2_Handle instantiateSO_666(const LV2_Descriptor *descriptor,double s_rate, const char *path,const LV2_Feature * const* features) {
	so_666* so=malloc(sizeof(so_666));
	LV2_URI_Map_Feature *map_feature;
	const LV2_Feature * const *  ft;
	for (ft = features; *ft; ft++) {
		if (!strcmp((*ft)->URI, "http://lv2plug.in/ns/ext/uri-map")) {
			map_feature = (*ft)->data;
			so->midi_event_id = map_feature->uri_to_id(
			                                           map_feature->callback_data,
			                                           "http://lv2plug.in/ns/ext/event",
			                                           "http://lv2plug.in/ns/ext/midi#MidiEvent");
		                                           } else if (!strcmp((*ft)->URI, "http://lv2plug.in/ns/ext/event")) {
		                                                              so->event_ref = (*ft)->data;
												                                                                 }
	}

	puts( "SO-666 v.1.0 by 50m30n3 2009" );

	so->channel = 0;

	puts( "Initializing synth parameters" );
	unsigned int feedback,cutoff,resonance;
	feedback = 32;
	cutoff = 64;
	resonance = 64;
	so->volume = 100;
	so->fcutoff = pow( (cutoff+50.0)/200.0, 5.0 );
	so->freso = resonance/127.0;
	so->ffeedback = 0.01+pow( feedback/127.0, 4.0)*0.9;
	
	so->lplast=0;
	so->lpval=0;
	so->hplast=0;
	so->hpval=0;
	
	int note;
	for( note=0; note<NUMNOTES; note++ ) {
		double freq = 440.0*pow( 2.0, (note+BASENOTE-69) / 12.0 );
		//so->stringcutoff[note] = ( freq * 16.0 ) / (double)s_rate;
		so->stringcutoff[note] = 0.9;
		int length = (double)s_rate /freq;
		so->stringlength[note] = length;
		so->strings[note] = malloc( length * sizeof( double ) );
		if( so->strings[note] == NULL )	{
			fputs( "Error allocating memory\n", stderr );
			return 0;
		}
		unsigned int i;
		for( i=0; i<(length); i++ )
		{
			so->strings[note][i] = 0.0;
		}
		so->stringpos[note] = 0;
		so->status[note] = 0;
	}

	return so;
}
void cleanupSO_666(LV2_Handle instance) {
	so_666* so=(so_666*)instance;
	puts( "Freeing data" );
	int note;
	for(note=0; note<NUMNOTES; note++ )
	{
		free( so->strings[note] );
	}
	free(so);
}

void connectPortSO_666(LV2_Handle instance, uint32_t port, void *data_location) {
	so_666* so=(so_666*) instance;
	switch(port) {
		case PORT_OUTPUT:
			so->output=data_location;
			break;
		case PORT_MIDI:
			so->MidiIn=data_location;
			break;
		default:
			fputs("Warning, unconnected port!",stderr);
	}
}

static LV2_Descriptor so_666_Descriptor= {
	.URI="urn:50m30n3:plugins:SO-666",
	.instantiate=instantiateSO_666,
	.connect_port=connectPortSO_666,
	.activate=NULL,
	.run=runSO_666,
	.deactivate=NULL,
	.cleanup=cleanupSO_666,
	.extension_data=NULL,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
	switch(index) {
		case 0:
			return &so_666_Descriptor;
		default:
			return NULL;
	}
}


#include "so-666.h"

static float dist( float in )
{
	float out;
	out = tanhf( in );
	return out;
}

inline float feedback_scale(float input) {
	return 0.01+powf( input, 4.0)*0.9;
}
inline float cutoff_scale(float input) {
	return powf( input, 5.0 );
}
void runSO_666( LV2_Handle arg, uint32_t nframes ) {
	
	so_666* so=(so_666*)arg;
	lv2_event_begin(&so->in_iterator,so->MidiIn);
	float **strings=so->strings;
	unsigned int* stringpos=so->stringpos;
	unsigned int* stringlength=so->stringlength;
	float* outbuffer=so->output;
	int * status=so->status;

	int i, note;
	float  sample, damp;

	if(*so->controlmode_p>0) {
		so->ffeedback=feedback_scale(*so->feedback_p);
		so->fcutoff=cutoff_scale(*so->cutoff_p);
		so->freso=*so->resonance_p;
		so->volume=*so->volume_p;
	}
	
	for( i=0; i<nframes; i++ ) {
		while(lv2_event_is_valid(&so->in_iterator)) {
			uint8_t* data;
			LV2_Event* event= lv2_event_get(&so->in_iterator,&data);
			if (event->type == 0) {
				so->event_ref->lv2_event_unref(so->event_ref->callback_data, event);
			} else if(event->type==so->midi_event_id) {
				if(event->frames > i) {
					break;
				} else if(*so->commandmode_p<=0) {
					const uint8_t* evt=(uint8_t*)data;
					if((evt[0]&MIDI_CHANNELMASK)==(int) (*so->channel_p)) {
						if((evt[0]&MIDI_COMMANDMASK)==MIDI_NOTEON) 	{
							note = evt[1];
							if( (note >= BASENOTE) && ( note < BASENOTE+NUMNOTES ) ) {
								note -= BASENOTE;
								status[note] = 1;
							}
						}
						else if((evt[0]&MIDI_COMMANDMASK)==MIDI_NOTEOFF )	{
							note = evt[1];
							if( ( note >= BASENOTE ) && ( note < BASENOTE+NUMNOTES ) ) {
								note -= BASENOTE;
								status[note] = 0;
							}
						}
						else if((evt[0]&MIDI_COMMANDMASK)==MIDI_CONTROL )	{
							if( evt[1] == 74 )	{
								unsigned int cutoff =evt[2];
								so->fcutoff = cutoff_scale((cutoff+50.0)/200.0);
							}
							else if( evt[1]== 71 )	{
								unsigned int resonance = evt[2];
								so->freso = resonance/127.0;
							}
							else if( evt[1] == 7 )	{
								so->volume = evt[2];
							}
							else if( evt[1]== 1 ) {
								unsigned int feedback =evt[2];
								so->ffeedback = feedback_scale (feedback/127.0);
							}
						}
					}
				}
			}
			lv2_event_increment(&so->in_iterator);
		}
		sample = (((float)rand()/(float)RAND_MAX)*2.0-1.0)*0.001;

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

	unsigned int feedback,cutoff,resonance;
	feedback = 32;
	cutoff = 64;
	resonance = 64;
	so->volume = 100;
	so->fcutoff = powf( (cutoff+50.0)/200.0, 5.0 );
	so->freso = resonance/127.0;
	so->ffeedback = 0.01+powf( feedback/127.0, 4.0)*0.9;
	
	so->lplast=0;
	so->lpval=0;
	so->hplast=0;
	so->hpval=0;
	
	int note;
	for( note=0; note<NUMNOTES; note++ ) {
		float freq = 440.0*powf( 2.0, (note+BASENOTE-69) / 12.0 );
		//so->stringcutoff[note] = ( freq * 16.0 ) / (float)s_rate;
		so->stringcutoff[note] = 0.9;
		int length = (float)s_rate /freq;
		so->stringlength[note] = length;
		so->strings[note] = malloc( length * sizeof( float ) );
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
		case PORT_CONTROLMODE:
			so->controlmode_p=data_location;
			break;
		case PORT_FEEDBACK:
			so->feedback_p=data_location;
			break;
		case PORT_RESONANCE:
			so->resonance_p=data_location;
			break;
		case PORT_CUTOFF:
			so->cutoff_p=data_location;
			break;
		case PORT_VOLUME:
			so->volume_p=data_location;
			break;
		case PORT_CHANNEL:
			so->channel_p=data_location;
			break;
		default:
			fputs("Warning, unconnected port!\n",stderr);
	}
}

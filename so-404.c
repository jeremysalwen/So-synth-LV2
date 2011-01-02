#include "so-404.h"

inline float midi_to_hz(int note) {
	return 440.0*powf( 2.0, (note-69) / 12.0 );
}

void runSO_404( LV2_Handle arg, uint32_t nframes ) {
	so_404* so=(so_404*)arg;
	lv2_event_begin(&so->in_iterator,so->MidiIn);
	
	float* outbuffer=so->output;

	if(*so->controlmode_p >0) {
		so->cutoff=*so->cutoff_p;
		so->portamento=*so->portamento_p;
		so->release=*so->release_p;
		so->volume=*so->volume_p;
		so->envmod=*so->envmod_p;
		so->resonance=*so->resonance_p;
	}
	
	int i;
	for( i=0; i<nframes; i++ ) {
		while(lv2_event_is_valid(&so->in_iterator)) {
			uint8_t* data;
			LV2_Event* event= lv2_event_get(&so->in_iterator,&data);
			if (event->type == 0) {
				so->event_ref->lv2_event_unref(so->event_ref->callback_data, event);
			} else if(event->type==so->midi_event_id) {
				if(event->frames > i) {
					break;
				} else  if(*so->controlmode_p<=0) {
					const uint8_t* evt=(uint8_t*)data;
					if((evt[0]&MIDI_CHANNELMASK)==(int) (*so->channel_p)) {
						if((evt[0]&MIDI_COMMANDMASK)==MIDI_NOTEON) 	{
							unsigned int note = evt[1];
							so->tfreq=midi_to_hz(note);
							if( so->noteson == 0 )
							{
								so->freq = so->tfreq;
							}
							so->amp = ((float)evt[2])/127.0;
							so->cdelay = 0;
							so->noteson += 1;
						}
						else if((evt[0]&MIDI_COMMANDMASK)==MIDI_NOTEOFF )	{
							so->noteson -= 1;
						}
						else if((evt[0]&MIDI_COMMANDMASK)==MIDI_CONTROL )	{
							unsigned int command_val=evt[2];
							switch(evt[1]) {
								case 74:
									so->cutoff =command_val;
									break;
								case 65:
									so->portamento = command_val;
									break;
								case 72:
									so->release = command_val;
									break;
								case 7:
									so->volume = command_val;
									break;
								case 79:
									so->envmod = command_val;
									break;
								case 71:
									so->resonance = command_val;
									break;
							}
						}
					}
				}
			}
			lv2_event_increment(&so->in_iterator);
		}
		if( so->cdelay <= 0 )
		{
			so->freq = ((so->portamento/127.0)*0.9)*so->freq + (1.0-((so->portamento/127.0)*0.9))*so->tfreq;
			so->amp *= 0.8+(so->release/127.0)/5.1;
			so->fcutoff = powf(so->cutoff/127.0,5.0)+so->amp*so->amp*powf(so->envmod/128.0,2.0);
			if( so->fcutoff > 1.0 ) {
				so->fcutoff = 1.0;
			}
			so->fcutoff = sin(so->fcutoff*M_PI/2.0);
			so->freso = powf(so->resonance/127.0,0.25);
			so->cdelay = so->samplerate/100;
		}
		so->cdelay--;
		
		float max = so->samplerate / so->freq;
		float sample = (so->phase/max)*(so->phase/max)-0.25;
		so->phase++;
		if( so->phase >= max )
		so->phase -= max;
		
		sample *= so->amp;

		so->fpos += so->fspeed;
		so->fspeed *= so->freso;
		so->fspeed += (sample-so->fpos)*so->fcutoff;
		sample = so->fpos;

		sample = sample*0.5+so->lastsample*0.5;
		so->lastsample = sample;

		outbuffer[i] = sample * (so->volume/127.0);
	}
}

LV2_Handle instantiateSO_404(const LV2_Descriptor *descriptor,double s_rate, const char *path,const LV2_Feature * const* features) {
	so_404* so=malloc(sizeof(so_404));
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

	puts( "SO-404 v.1.0 by 50m30n3 2009" );
		
	so->phase = 0.0;
	so->freq = 440.0;
	so->tfreq = 440.0;
	so->amp = 0.0;
	so->fcutoff = 0.0;
	so->fspeed = 0.0;
	so->fpos = 0.0;
	so->lastsample = 0.0;
	so->noteson = 0;
	so->cdelay = s_rate/100;
	so->samplerate=s_rate;
	
	so->release = 100;
	so->cutoff = 50;
	so->envmod = 80;
	so->resonance = 100;
	so->volume = 100;
	so->portamento = 64;
	
	return so;
}
void cleanupSO_404(LV2_Handle instance) {
	free(instance);
}

void connectPortSO_404(LV2_Handle instance, uint32_t port, void *data_location) {
	so_404* so=(so_404*) instance;
	switch(port) {
		case PORT_404_OUTPUT:
			so->output=data_location;
			break;
		case PORT_404_MIDI:
			so->MidiIn=data_location;
			break;
		case PORT_404_CONTROLMODE:
			so->controlmode_p=data_location;
			break;
		case PORT_404_VOLUME:
			so->volume_p=data_location;
			break;
		case PORT_404_CUTOFF:
			so->cutoff_p=data_location;
			break;
		case PORT_404_RESONANCE:
			so->resonance_p=data_location;
			break;
		case PORT_404_ENVELOPE:
			so->envmod_p=data_location;
			break;
		case PORT_404_PORTAMENTO:
			so->portamento_p=data_location;
			break;
		case PORT_404_RELEASE:
			so->release_p=data_location;
			break;
		case PORT_404_CHANNEL:
			so->channel_p=data_location;
			break;
		default:
			fputs("Warning, unconnected port!\n",stderr);
	}
}

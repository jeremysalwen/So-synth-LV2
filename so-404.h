/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * 
 * aubiolv2 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * aubiolv2 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <lv2.h>
#include <string.h>
#include "event-helpers.h"
#include "uri-map.h"
#include <stdio.h>	
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define NUMNOTES 80
#define BASENOTE 21

#define MIDI_COMMANDMASK 0xF0
#define MIDI_CHANNELMASK 0x0F

#define MIDI_NOTEON 0x90
#define MIDI_NOTEOFF 0x80
#define MIDI_CONTROL 0xB0

enum PORTS {
	PORT_OUTPUT=0,
	PORT_MIDI,
	PORT_CONTROLMODE,
	PORT_VOLUME,
	PORT_CUTOFF,
	PORT_RESONANCE,
	PORT_ENVELOPE,
	PORT_PORTAMENTO,
	PORT_RELEASE,
	PORT_CHANNEL
};

void runSO_404( LV2_Handle arg, uint32_t nframes );
LV2_Handle instantiateSO_404(const LV2_Descriptor *descriptor,double s_rate, const char *path,const LV2_Feature * const* features);
void cleanupSO_404(LV2_Handle instance);
void connectPortSO_404(LV2_Handle instance, uint32_t port, void *data_location);

static LV2_Descriptor so_404_Descriptor= {
	.URI="urn:50m30n3:plugins:SO-404",
	.instantiate=instantiateSO_404,
	.connect_port=connectPortSO_404,
	.activate=NULL,
	.run=runSO_404,
	.deactivate=NULL,
	.cleanup=cleanupSO_404,
	.extension_data=NULL,
};

typedef struct so_404_t {
	float* output;
	LV2_Event_Buffer *MidiIn;
	LV2_Event_Iterator in_iterator;

	LV2_Event_Feature* event_ref;
	int midi_event_id;
	
	float* controlmode_p;
	float* cutoff_p;
	float* portamento_p;
	float* release_p;
	float* volume_p;
	float* envmod_p;
	float* resonance_p;
		
	float freq, tfreq;
	unsigned int cdelay;
	
	unsigned int cutoff;
	unsigned int resonance;
	unsigned int volume;
	unsigned int portamento;
	unsigned int release;
	unsigned int envmod;

	float phase;
	float amp;
	float lastsample;
	
	float fcutoff;
	float fspeed;
	float fpos;
	float freso;
	
	int noteson;

} so_404;
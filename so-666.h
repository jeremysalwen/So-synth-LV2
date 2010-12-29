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

#define MIDI_NOTEON 0x9
#define MIDI_NOTEOFF 0x8
#define MIDI_CONTROL 0xB
LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index);
typedef struct midi_event_t {
	unsigned int command:4;
	unsigned int channel:4;
	unsigned char info1;
	unsigned char info2;
} midi_event;

typedef struct so_666_t {
	float* output;
	LV2_Event_Buffer *MidiIn;
	LV2_Event_Iterator in_iterator;

	LV2_Event_Feature* event_ref;
	int midi_event_id;

	double *strings[NUMNOTES];
	unsigned int stringpos[NUMNOTES];
	unsigned int stringlength[NUMNOTES];
	double stringcutoff[NUMNOTES];
	int status[NUMNOTES];

	unsigned int volume;
	double samplerate;
	
	double lpval, lplast;
	double hpval, hplast;
	double fcutoff, freso, ffeedback;

	
	int npfd;
	int channel, midiport;

	int note, length, i;
	double freq;
} so_666;
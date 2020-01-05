/*
 * so-kl5.c
 *
 * Copyright (C) 2011 - Jeremy Salwen
 * Copyright (C) 2010 - 50m30n3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "so-kl5.h"

inline float sustain_scale(float input) { return 0.6 + powf(input, 0.4) * 0.4; }
void runSO_kl5(LV2_Handle arg, uint32_t nframes) {
  so_kl5* so = (so_kl5*)arg;
  lv2_event_begin(&so->in_iterator, so->MidiIn);

  float** strings = so->strings;
  unsigned int* stringpos = so->stringpos;
  unsigned int* stringlength = so->stringlength;
  float* tempstring = so->tempstring;
  float* stringcutoff = so->stringcutoff;
  float* outbuffer = so->output;
  int* status = so->status;

  int i, note;
  float sample, damp;

  if (*so->controlmode_p > 0) {
    so->volume = *so->volume_p;
    so->fcutoff = *so->cutoff_p;
    so->sattack = *so->attack_p;
    so->freso = (*so->resonance_p) * (1.0 - (*so->cutoff_p));
    so->ssustain = sustain_scale(*so->sustain_p);
  }

  for (i = 0; i < nframes; i++) {
    while (lv2_event_is_valid(&so->in_iterator)) {
      uint8_t* data;
      LV2_Event* event = lv2_event_get(&so->in_iterator, &data);
      if (event->type == 0) {
        so->event_ref->lv2_event_unref(so->event_ref->callback_data, event);
      } else if (event->type == so->midi_event_id) {
        if (event->frames > i) {
          break;
        } else {
          const uint8_t* evt = (uint8_t*)data;
          if ((evt[0] & MIDI_CHANNELMASK) == (int)(*so->channel_p)) {
            if ((evt[0] & MIDI_COMMANDMASK) == MIDI_NOTEON) {
              note = evt[1];
              if ((note >= BASENOTE) && (note < BASENOTE + NUMNOTES)) {
                note -= BASENOTE;

                status[note] = 1;

                int j;
                for (j = 0; j < stringlength[note]; j++) {
                  tempstring[j] = ((float)rand() / (float)RAND_MAX) * 2.0 - 1.0;
                }
                float velocity = evt[2];
                float freq = stringcutoff[note] * 0.25 +
                             velocity / 127.0 * 0.2 + so->sattack + 0.1;

                for (j = 0; j < 30; j++) {
                  tempstring[0] =
                      tempstring[0] * freq +
                      tempstring[stringlength[note] - 1] * (1.0 - freq);
                  int k;
                  for (k = 1; k < stringlength[note]; k++) {
                    tempstring[k] =
                        tempstring[k] * freq +
                        tempstring[(k - 1) % stringlength[note]] * (1.0 - freq);
                  }
                }

                float avg = 0.0;

                for (j = 0; j < stringlength[note]; j++) {
                  avg += tempstring[j];
                }

                avg /= stringlength[note];

                float scale = 0.0;

                for (j = 0; j < stringlength[note]; j++) {
                  tempstring[j] -= avg;
                  if (fabs(tempstring[j]) > scale) scale = fabs(tempstring[j]);
                }

                float min = 10.0;
                int minpos = 0;

                for (j = 0; j < stringlength[note]; j++) {
                  tempstring[j] /= scale;
                  if (fabs(tempstring[j]) +
                          fabs(tempstring[j] - tempstring[j - 1]) * 5.0 <
                      min) {
                    min = fabs(tempstring[j]) +
                          fabs(tempstring[j] - tempstring[j - 1]) * 5.0;
                    minpos = j;
                  }
                }

                float vol = velocity / 256.0;

                for (j = 0; j < stringlength[note]; j++) {
                  strings[note][(stringpos[note] + j) % stringlength[note]] +=
                      tempstring[(j + minpos) % stringlength[note]] * vol;
                }
              }
            } else if ((evt[0] & MIDI_COMMANDMASK) == MIDI_NOTEOFF) {
              note = evt[1];
              if ((note >= BASENOTE) && (note < BASENOTE + NUMNOTES)) {
                note -= BASENOTE;
                status[note] = 0;
              }
            } else if ((*so->controlmode_p <= 0) &&
                       (evt[0] & MIDI_COMMANDMASK) == MIDI_CONTROL) {
              if (evt[1] == 74) {
                unsigned int cutoff = evt[2];
                so->fcutoff = (cutoff + 5.0) / 400.0;
              } else if (evt[1] == 71) {
                unsigned int resonance = evt[2];
                so->freso = (resonance / 140.0) * (1.0 - so->fcutoff);
              } else if (evt[1] == 73) {
                unsigned int attack = evt[2];
                so->sattack = (attack + 5.0) / 800.0;
              } else if (evt[1] == 7) {
                so->volume = evt[2];
              } else if (evt[1] == 1 || evt[1] == 64) {
                unsigned int sustain = evt[2];
                so->ssustain = sustain_scale(sustain / 127.0);
              }
            }
          }
        }
      }
      lv2_event_increment(&so->in_iterator);
    }
    sample = 0.0;

    for (note = 0; note < NUMNOTES; note++) {
      damp = stringcutoff[note];

      if (stringpos[note] > 0)
        strings[note][stringpos[note]] =
            strings[note][stringpos[note]] * damp +
            strings[note][stringpos[note] - 1] * (1.0 - damp);
      else
        strings[note][stringpos[note]] =
            strings[note][stringpos[note]] * damp +
            strings[note][stringlength[note] - 1] * (1.0 - damp);

      damp = ((float)note / (float)NUMNOTES) * 0.0049995 + 0.0049995;

      if (status[note] == 0)
        strings[note][stringpos[note]] *= 0.8 + so->ssustain * 0.19 + damp;
      else
        strings[note][stringpos[note]] *= 0.99 + damp;

      sample += strings[note][stringpos[note]];
    }

    for (note = 0; note < NUMNOTES; note++) {
      if (fabs(strings[note][stringpos[note]]) <= 0.00001)
        strings[note][stringpos[note]] = 0.0;

      stringpos[note]++;
      if (stringpos[note] >= stringlength[note]) stringpos[note] = 0;
    }

    so->lpval += (sample - so->lplast) * so->fcutoff;
    so->lplast += so->lpval;
    so->lpval *= so->freso;
    sample = so->lplast;

    outbuffer[i] = sample * (so->volume / 127.0);
  }
}

LV2_Handle instantiateSO_kl5(const LV2_Descriptor* descriptor, double s_rate,
                             const char* path,
                             const LV2_Feature* const* features) {
  so_kl5* so = malloc(sizeof(so_kl5));
  LV2_URI_Map_Feature* map_feature;
  const LV2_Feature* const* ft;
  for (ft = features; *ft; ft++) {
    if (!strcmp((*ft)->URI, "http://lv2plug.in/ns/ext/uri-map")) {
      map_feature = (*ft)->data;
      so->midi_event_id = map_feature->uri_to_id(
          map_feature->callback_data, "http://lv2plug.in/ns/ext/event",
          "http://lv2plug.in/ns/ext/midi#MidiEvent");
    } else if (!strcmp((*ft)->URI, "http://lv2plug.in/ns/ext/event")) {
      so->event_ref = (*ft)->data;
    }
  }

  puts("SO-666 v.1.0 by 50m30n3 2009");

  unsigned int cutoff, resonance, sustain, attack;
  sustain = 0;
  cutoff = 64;
  resonance = 100;
  attack = 4;
  so->volume = 100;

  so->lplast = 0;
  so->lpval = 0;

  so->fcutoff = (cutoff + 5.0) / 400.0;
  so->sattack = (attack + 5.0) / 800.0;
  so->freso = (resonance / 160.0) * (1.0 - so->fcutoff);
  so->ssustain = 0.6 + powf(sustain / 127.0, 0.4);

  int note;
  for (note = 0; note < NUMNOTES; note++) {
    float freq = 440.0 * powf(2.0, (note + BASENOTE - 69) / 12.0);
    so->stringcutoff[note] =
        0.5 + pow((double)note / (double)NUMNOTES, 0.5) * 0.45;
    int length = round((float)s_rate / freq);
    so->stringlength[note] = length;
    so->strings[note] = malloc(length * sizeof(float));
    if (so->strings[note] == NULL) {
      fputs("Error allocating memory\n", stderr);
      return 0;
    }
    int i;
    for (i = 0; i < length; i++) {
      so->strings[note][i] = 0.0;
    }
    so->stringpos[note] = 0;
    so->status[note] = 0;
  }

  float freq = 440.0 * powf(2.0, (BASENOTE - 69) / 12.0);
  float length = (float)s_rate / freq;
  so->tempstring = malloc(length * sizeof(float));
  if (so->tempstring == NULL) {
    fputs("Error allocating memory\n", stderr);
    return 0;
  }

  return so;
}
void cleanupSO_kl5(LV2_Handle instance) {
  so_kl5* so = (so_kl5*)instance;
  free(so->tempstring);
  int note;
  for (note = 0; note < NUMNOTES; note++) {
    free(so->strings[note]);
  }
  free(so);
}

void connectPortSO_kl5(LV2_Handle instance, uint32_t port,
                       void* data_location) {
  so_kl5* so = (so_kl5*)instance;
  switch (port) {
    case KL5_PORT_OUTPUT:
      so->output = data_location;
      break;
    case KL5_PORT_MIDI:
      so->MidiIn = data_location;
      break;
    case KL5_PORT_CONTROLMODE:
      so->controlmode_p = data_location;
      break;
    case KL5_PORT_VOLUME:
      so->volume_p = data_location;
      break;
    case KL5_PORT_SUSTAIN:
      so->sustain_p = data_location;
      break;
    case KL5_PORT_CUTOFF:
      so->cutoff_p = data_location;
      break;
    case KL5_PORT_RESONANCE:
      so->resonance_p = data_location;
      break;
    case KL5_PORT_ATTACK:
      so->attack_p = data_location;
      break;
    case KL5_PORT_CHANNEL:
      so->channel_p = data_location;
      break;
    default:
      fputs("Warning, unconnected port!\n", stderr);
  }
}

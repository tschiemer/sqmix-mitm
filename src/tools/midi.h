/**
* sqmix-mitm
* Copyright (C) 2025  Philip Tschiemer
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU Affero General Public License as published by
        * the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
        * but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SQMIX_MITM_MIDI_H
#define SQMIX_MITM_MIDI_H


enum MidiType {
    NoteOff         = 0x80,
    NoteOn          = 0x90,
    ControlChange   = 0xb0,
    ProgramChange   = 0xc0
};

enum MidiMmcType {
    Stop            = 0x01,
    Play            = 0x02,
    FastForward     = 0x04,
    Rewind          = 0x05,
    Record          = 0x06,
    Pause           = 0x09
};

#endif //SQMIX_MITM_MIDI_H

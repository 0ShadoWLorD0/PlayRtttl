/*
 * PlayRttl.cpp
 * Plays RTTTL melodies / ringtones from FLASH or RAM.
 * Includes a non blocking version and a name output function.
 *
 *  Copyright (C) 2018  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *     Based on the RTTTL.pde example code written by Brett Hagman
 *     http://www.roguerobotics.com/
 *     bhagman@roguerobotics.com
 *
 *     The example melodies may have copyrights you have to respect.
 *
 *  This file is part of PlayRttl https://github.com/ArminJo/PlayRttl.
 *
 *  PlayRttl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#include "PlayRtttl.h"

//uncomment next line to see debug output on serial
//#define DEBUG

struct playRtttlState sPlayRtttlState;

int notes[] = { 0,
NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7 };

#define OCTAVE_OFFSET 0
#define isdigit(n) (n >= '0' && n <= '9')

/*
 * Blocking versions
 */
void playRtttlBlocking(uint8_t aTonePin, char *aRTTTLArrayPtr) {
    startPlayRtttl(aTonePin, aRTTTLArrayPtr, NULL);
    while (checkForRtttlToneUpdate()) {
        delay(1);
    }
}

void playRtttlBlockingPGM(uint8_t aTonePin, const char *aRTTTLArrayPtrPGM) {
    startPlayRtttlPGM(aTonePin, aRTTTLArrayPtrPGM, NULL);
    while (checkForRtttlToneUpdate()) {
        delay(1);
    }
}

/*
 * Non blocking version for RTTTL Data in FLASH. Ie. you must call checkForRtttlToneUpdate() in your loop.
 */
void startPlayRtttlPGM(uint8_t aTonePin, const char * aRTTTLArrayPtrPGM, void (*aOnComplete)()) {
    sPlayRtttlState.IsPGMMemory = true;
    sPlayRtttlState.OnComplete = aOnComplete;
    sPlayRtttlState.TonePin = aTonePin;
    int tNumber;

    char tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);

    /*
     * Skip name and :
     */
    while (tPGMChar != ':') {
        aRTTTLArrayPtrPGM++;
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
    }
    aRTTTLArrayPtrPGM++;
    tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);

    /*
     * Read song info with format: d=N(N),o=N,b=NNN:
     */

    /*
     * get default duration
     */
    tNumber = DEFAULT_DURATION;
    if (tPGMChar == 'd') {
        aRTTTLArrayPtrPGM++;
        aRTTTLArrayPtrPGM++;              // skip "d="
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
        tNumber = 0;
        while (isdigit(tPGMChar)) {
            tNumber = (tNumber * 10) + (tPGMChar - '0');
            aRTTTLArrayPtrPGM++;
            tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
        }
        if (tNumber == 0) {
            tNumber = DEFAULT_DURATION;
        }
        aRTTTLArrayPtrPGM++;                   // skip comma
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
    }
    sPlayRtttlState.DefaultDuration = tNumber;

    /*
     * get default octave
     */
    tNumber = DEFAULT_OCTAVE;
    if (tPGMChar == 'o') {
        aRTTTLArrayPtrPGM++;
        aRTTTLArrayPtrPGM++;              // skip "o="
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
        aRTTTLArrayPtrPGM++;
        tNumber = tPGMChar - '0';
        if (tNumber < 3 && tNumber > 7) {
            tNumber = DEFAULT_OCTAVE;
        }

        aRTTTLArrayPtrPGM++;                   // skip comma
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
    }
    sPlayRtttlState.DefaultOctave = tNumber;

    // get BPM
    tNumber = DEFAULT_BPM;
    if (tPGMChar == 'b') {
        aRTTTLArrayPtrPGM++;
        aRTTTLArrayPtrPGM++;              // skip "b="
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
        tNumber = 0;
        while (isdigit(tPGMChar)) {
            tNumber = (tNumber * 10) + (tPGMChar - '0');
            aRTTTLArrayPtrPGM++;
            tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
        }
        if (tNumber == 0) {
            tNumber = DEFAULT_BPM;
        }
        aRTTTLArrayPtrPGM++;                   // skip colon
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM);
    }
    // BPM usually expresses the number of quarter notes per minute
    sPlayRtttlState.TimeForWholeNoteMillis = (60 * 1000L / tNumber) * 4;

#ifdef DEBUG
    Serial.print("DefaultDuration=");
    Serial.print (sPlayRtttlState.DefaultDuration);
    Serial.print(" DefaultOctave=");
    Serial.print (sPlayRtttlState.DefaultOctave);
    Serial.print(" BPM=");
    Serial.println(tNumber);
#endif

    sPlayRtttlState.MillisOfNextAction = 0;
    sPlayRtttlState.NextTonePointer = aRTTTLArrayPtrPGM;
    sPlayRtttlState.IsStopped = false;

    /*
     * Play first tone
     */
    checkForRtttlToneUpdate();
}

/*
 * Version for RTTTL Data in RAM. Ie. you must call checkForRtttlToneUpdate() in your loop.
 * Since we do not need all the pgm_read_byte_near() calls this version is more simple and maybe better to understand.
 */
void startPlayRtttl(uint8_t aTonePin, char * aRTTTLArrayPtr, void (*aOnComplete)()) {
    sPlayRtttlState.IsPGMMemory = false;
    sPlayRtttlState.OnComplete = aOnComplete;
    sPlayRtttlState.TonePin = aTonePin;
    int tNumber;
    /*
     * Skip name and :
     */
    while (*aRTTTLArrayPtr != ':') {
        aRTTTLArrayPtr++;
    }
    aRTTTLArrayPtr++;

    /*
     * Read song info with format: d=N,o=N,b=NNN:
     */

    /*
     * get default duration
     */
    tNumber = DEFAULT_DURATION;
    if (*aRTTTLArrayPtr == 'd') {
        aRTTTLArrayPtr++;
        aRTTTLArrayPtr++;              // skip "d="
        tNumber = 0;
        while (isdigit(*aRTTTLArrayPtr)) {
            tNumber = (tNumber * 10) + (*aRTTTLArrayPtr++ - '0');
        }
        if (tNumber == 0) {
            tNumber = DEFAULT_DURATION;
        }
        aRTTTLArrayPtr++;                   // skip comma
    }
    sPlayRtttlState.DefaultDuration = tNumber;

    /*
     * get default octave
     */
    tNumber = DEFAULT_OCTAVE;
    if (*aRTTTLArrayPtr == 'o') {
        aRTTTLArrayPtr++;
        aRTTTLArrayPtr++;              // skip "o="
        tNumber = *aRTTTLArrayPtr++ - '0';
        if (tNumber < 3 && tNumber > 7) {
            tNumber = DEFAULT_OCTAVE;
        }
        aRTTTLArrayPtr++;                   // skip comma
    }
    sPlayRtttlState.DefaultOctave = tNumber;

    // get BPM
    tNumber = DEFAULT_BPM;
    if (*aRTTTLArrayPtr == 'b') {
        aRTTTLArrayPtr++;
        aRTTTLArrayPtr++;              // skip "b="
        tNumber = 0;
        while (isdigit(*aRTTTLArrayPtr)) {
            tNumber = (tNumber * 10) + (*aRTTTLArrayPtr++ - '0');
        }
        if (tNumber == 0) {
            tNumber = DEFAULT_BPM;
        }
        aRTTTLArrayPtr++;                   // skip colon
    }
    // BPM usually expresses the number of quarter notes per minute
    sPlayRtttlState.TimeForWholeNoteMillis = (60 * 1000L / tNumber) * 4;

#ifdef DEBUG
    Serial.print("DefaultDuration=");
    Serial.print (sPlayRtttlState.DefaultDuration);
    Serial.print(" DefaultOctave=");
    Serial.print (sPlayRtttlState.DefaultOctave);
    Serial.print(" BPM=");
    Serial.println(tNumber);
#endif

    sPlayRtttlState.MillisOfNextAction = 0;
    sPlayRtttlState.NextTonePointer = aRTTTLArrayPtr;
    sPlayRtttlState.IsStopped = false;

    /*
     * Play first tone
     */
    checkForRtttlToneUpdate();
}

void stopPlayRtttl(void) {
    noTone(sPlayRtttlState.TonePin);
    sPlayRtttlState.IsStopped = true;
}

/*
 * Returns true if tone is playing, false if tone has ended or stopped
 */
bool checkForRtttlToneUpdate(void) {

    if (sPlayRtttlState.IsStopped) {
        return false;
    }

    long tMillis = millis();
    if (sPlayRtttlState.MillisOfNextAction <= tMillis) {
        noTone(sPlayRtttlState.TonePin);
        const char * tRTTTLArrayPtr = sPlayRtttlState.NextTonePointer;

        char tChar;
        if (sPlayRtttlState.IsPGMMemory) {
            tChar = pgm_read_byte_near(tRTTTLArrayPtr);
        } else {
            tChar = *tRTTTLArrayPtr;
        }

        /*
         * Check if end of string reached
         */
        if (tChar == '\0') {
            sPlayRtttlState.IsStopped = true;
            if (sPlayRtttlState.OnComplete != NULL) {
                sPlayRtttlState.OnComplete();
            }
            return false;
        }

        int tNumber;
        long tDuration;
        uint8_t tNote;
        uint8_t tScale;

        // first, get note duration, if available
        tNumber = 0;
        while (isdigit(tChar)) {
            tNumber = (tNumber * 10) + (tChar - '0');
            tRTTTLArrayPtr++;
            if (sPlayRtttlState.IsPGMMemory) {
                tChar = pgm_read_byte_near(tRTTTLArrayPtr);
            } else {
                tChar = *tRTTTLArrayPtr;
            }
        }

        if (tNumber != 0) {
            tDuration = sPlayRtttlState.TimeForWholeNoteMillis / tNumber;
        } else {
            tDuration = sPlayRtttlState.TimeForWholeNoteMillis / sPlayRtttlState.DefaultDuration; // we will need to check if we are a dotted note after
        }

        // now get the note
        tNote = 0;

        switch (tChar) {
        case 'c':
            tNote = 1;
            break;
        case 'd':
            tNote = 3;
            break;
        case 'e':
            tNote = 5;
            break;
        case 'f':
            tNote = 6;
            break;
        case 'g':
            tNote = 8;
            break;
        case 'a':
            tNote = 10;
            break;
        case 'b':
            tNote = 12;
            break;
        case 'p':
        default:
            tNote = 0;
        }

        tRTTTLArrayPtr++;
        if (sPlayRtttlState.IsPGMMemory) {
            tChar = pgm_read_byte_near(tRTTTLArrayPtr);
        } else {
            tChar = *tRTTTLArrayPtr;
        }

        // now, get optional '#' sharp
        if (tChar == '#') {
            tNote++;
            tRTTTLArrayPtr++;
            if (sPlayRtttlState.IsPGMMemory) {
                tChar = pgm_read_byte_near(tRTTTLArrayPtr);
            } else {
                tChar = *tRTTTLArrayPtr;
            }
        }

        // now, get optional '.' dotted note
        if (tChar == '.') {
            tDuration += tDuration / 2;
            tRTTTLArrayPtr++;
            if (sPlayRtttlState.IsPGMMemory) {
                tChar = pgm_read_byte_near(tRTTTLArrayPtr);
            } else {
                tChar = *tRTTTLArrayPtr;
            }
        }

        // now, get scale
        if (isdigit(tChar)) {
            tScale = tChar - '0';
            tRTTTLArrayPtr++;
            if (sPlayRtttlState.IsPGMMemory) {
                tChar = pgm_read_byte_near(tRTTTLArrayPtr);
            } else {
                tChar = *tRTTTLArrayPtr;
            }
        } else {
            tScale = sPlayRtttlState.DefaultOctave;
        }

        tScale += OCTAVE_OFFSET;

        if (tChar == ',') {
            tRTTTLArrayPtr++;       // skip comma for next note (or we may be at the end)
        }

        /*
         * now play the note
         */
        if (tNote > 0) {
#ifdef DEBUG
            Serial.print("Playing: ");
            Serial.print(tScale, 10);
            Serial.print(' ');
            Serial.print(tNote, 10);
            Serial.print(" (");
            Serial.print(notes[(tScale - 4) * 12 + tNote], 10);
            Serial.print(") ");
            Serial.println(tDuration, 10);
#endif
            tone(sPlayRtttlState.TonePin, notes[(tScale - 4) * 12 + tNote]);
        }
#ifdef DEBUG
        else {
            Serial.print("Pausing: ");
            Serial.println(tDuration, 10);
        }
#endif
        sPlayRtttlState.MillisOfNextAction = tMillis + tDuration;
        sPlayRtttlState.NextTonePointer = tRTTTLArrayPtr;
    }
    return true;
}

void getRtttlNamePGM(const char *aRTTTLArrayPtrPGM, char * aBuffer, uint8_t aBuffersize) {
    char tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM++);
    while (tPGMChar != ':' && aBuffersize > 1) {
        *aBuffer++ = tPGMChar;
        aBuffersize--;
        tPGMChar = pgm_read_byte_near(aRTTTLArrayPtrPGM++);
    }
    *aBuffer = '\0';
}

void getRtttlName(char *aRTTTLArrayPtr, char * aBuffer, uint8_t aBuffersize) {
    char tChar = *aRTTTLArrayPtr++;
    while (tChar != ':' && aBuffersize > 1) {
        *aBuffer++ = tChar;
        aBuffersize--;
        tChar = *aRTTTLArrayPtr++;
    }
    *aBuffer = '\0';
}

/*
 * Prints text "Now playing: Song xy"
 */
void printNamePGM(const char *aRTTTLArrayPtrPGM) {
    char StringBuffer[16];
    Serial.print(F("Now playing: "));
    getRtttlNamePGM(aRTTTLArrayPtrPGM, StringBuffer, sizeof(StringBuffer));
    Serial.println(StringBuffer);
}

void printName(char *aRTTTLArrayPtr) {
    char StringBuffer[16];
    Serial.print(F("Now playing: "));
    getRtttlNamePGM(aRTTTLArrayPtr, StringBuffer, sizeof(StringBuffer));
    Serial.println(StringBuffer);
}

/*
 * Plays one of the examples
 */
void playRandomRtttlBlocking(uint8_t aTonePin) {
    uint8_t tRandomIndex = random(0, 21);
    printNamePGM(RTTTLsongs1[tRandomIndex]);
    playRtttlBlockingPGM(aTonePin, RTTTLsongs1[tRandomIndex]);
}

/*
 * Plays one of the examples non blocking. Ie. you must call checkForRtttlToneUpdate() in your loop.
 */
void startPlayRandomRtttl(uint8_t aTonePin, void (*aOnComplete)()) {
    uint8_t tRandomIndex = random(0, 21);
    startPlayRtttlPGM(aTonePin, RTTTLsongs1[tRandomIndex], aOnComplete);
    printNamePGM(RTTTLsongs1[tRandomIndex]);
}

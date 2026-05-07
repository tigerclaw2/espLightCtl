#include "light.h"
#include <TelnetPrint.h>
#include <TaskSchedulerDeclarations.h>

extern Task tFader;
extern void diyload(int num);

LightController Light;

// ==============================================================================
// LEGACY WRAPPERS
// ==============================================================================
void lightWatcher() { Light.watcher(); }
void faderCallback() { Light.fader(); }
void awrite(int mode) { Light.awrite(mode); }
void notifade(int ldiy, int tick) { Light.notifade(ldiy, tick); }

// ==============================================================================
// CLASS IMPLEMENTATION: CHANNEL MANAGEMENT & SETTERS
// ==============================================================================

void LightController::addCh(int id, int pin, int calib) {
    Channel ch;
    ch.id = id;
    ch.pin = pin;
    ch.calib = calib;
    channels.push_back(ch);
}

size_t LightController::getChCount() const {
    return channels.size();
}

void LightController::setBri(const int* values, size_t size) {
    // Only update up to the number of channels we actually have
    size_t limit = (size < channels.size()) ? size : channels.size();
    for (size_t i = 0; i < limit; i++) {
        channels[i].targetbr = values[i];
    }
}

void LightController::getBri(int* outValues, size_t maxSize) const {
    size_t limit = (maxSize < channels.size()) ? maxSize : channels.size();
    for (size_t i = 0; i < limit; i++) {
        outValues[i] = channels[i].targetbr;
    }
}

int LightController::getBriSingle(int id) const {
    for (const auto& ch : channels) {
        if (ch.id == id) return ch.targetbr;
    }
    return 0;
}

void LightController::setBriSingle(int id, int value) {
    for (auto& ch : channels) {
        if (ch.id == id) {
            ch.targetbr = value;
            break;
        }
    }
}

void LightController::setFadeSpeed(double speed) {
    nms = speed;
}

// ==============================================================================
// CLASS IMPLEMENTATION: CORE LOGIC
// ==============================================================================

void LightController::watcher() {
    bool newCommandDetected = false;
    long maxDelta = 0;

    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].targetbr != channels[i].lastSeenTarget) {
            newCommandDetected = true;
            channels[i].lastSeenTarget = channels[i].targetbr;
            TelnetPrint.printf("New command detected at index %d: %d\n", i, channels[i].targetbr);
        }

        if (channels[i].targetbr != channels[i].currentbr) {
            long diff = abs(channels[i].targetbr - channels[i].currentbr);
            if (diff > maxDelta) {
                maxDelta = diff;
                TelnetPrint.printf ("New maxDelta detected: %ld at index %d\n", maxDelta, i);
            }
        }
    }

    if (!newCommandDetected && tFader.isEnabled()) {
        TelnetPrint.println("No new command detected; fading will continue.");
        return;
    }
    
    if (!newCommandDetected && maxDelta == 0) {
        return;
    }

    if (tFader.isEnabled()) {
        TelnetPrint.println("Retargeting fade...");
        tFader.disable(); 
    }

    for (size_t i = 0; i < channels.size(); i++) {
        channels[i].startbr = channels[i].currentbr;
        TelnetPrint.printf("Starting condition for channel %d: %d\n", i, channels[i].startbr);
    }

    if (maxDelta == 0) {
        TelnetPrint.println("maxDelta is zero; exiting to prevent errors.");
        return; 
    }

    long calculatedInterval = 0;
    long timePerStep = nms / maxDelta; 
    TelnetPrint.printf("Calculated timePerStep: %ld\n", timePerStep);

    if (timePerStep < MIN_INTERVAL) {
        calculatedInterval = MIN_INTERVAL;
        fadeTotalSteps = nms / MIN_INTERVAL;
        fadeTotalSteps = fadeTotalSteps < 1 ? 1 : fadeTotalSteps;
        TelnetPrint.println("Mode: Speed Priority");
    } else {
        calculatedInterval = timePerStep;
        fadeTotalSteps = maxDelta;
        TelnetPrint.println("Mode: Precision Priority");
    }

    fadeCurrentStep = 0;
    tFader.setInterval(calculatedInterval);
    tFader.setIterations(fadeTotalSteps);
    tFader.enable(); 
    TelnetPrint.printf("Fade launched with interval: %ld ms and total steps: %ld\n", calculatedInterval, fadeTotalSteps);
}

void LightController::fader() {
    fadeCurrentStep++; 
    TelnetPrint.printf("[%ld]Fader: fadeStep: %ld\n", millis(), fadeCurrentStep);
    bool updateHardware = false;

    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].currentbr == channels[i].targetbr) continue;

        long delta = channels[i].targetbr - channels[i].startbr;
        int newVal = channels[i].startbr + ((delta * fadeCurrentStep) / fadeTotalSteps);

        if (channels[i].currentbr != newVal) {
            channels[i].currentbr = newVal;
            updateHardware = true;
        }
    }

    if (updateHardware) {
        awrite(anw);
    }
}

void LightController::awrite(int mode) {
    analogWriteResolution(anw);
    
    // Safety check in case we fire awrite before adding channels
    if (channels.empty()) return; 

    int masterCurrent = channels[0].currentbr; // Virtual channel 0

    // Loop starting from 1 since 0 is the virtual general brightness channel
    for (size_t i = 1; i < channels.size(); i++) {
        // EXACT math you requested, dynamically applied to the hardware pins
        analogWrite(channels[i].pin, map(map(channels[i].currentbr, 0, 256, 0, channels[i].calib), 0, 256, 0, masterCurrent));
    }
}

void LightController::notifade(int ldiy, int tick) {  
    if (channels.empty()) return;

    // Use a standard vector instead of a Variable Length Array (C++ compliance)
    std::vector<int> noti(channels.size());
    int done = 0;
    
    for (size_t i = 0; i < channels.size(); i++) {
        noti[i] = channels[i].targetbr;
    }
    
    TelnetPrint.println(ldiy);
    TelnetPrint.println(tick);
    diyload(ldiy);
    
    while (done <= channels.size()) { // Replaces done <= chnr + 1
        for (size_t i = 0; i < channels.size(); i++) {
            done++;
            if (channels[0].currentbr != 0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if (channels[i].targetbr > channels[i].currentbr) {
                channels[i].currentbr++;
                done = 0;
            }
            if (channels[i].targetbr < channels[i].currentbr) {
                channels[i].currentbr--;
            }
            awrite(anw);
        }
    }
    
    for (size_t i = 0; i < channels.size(); i++) {
        channels[i].targetbr = noti[i];
    }
    done = 0;
    
    while (done <= channels.size()) {
        for (size_t i = 0; i < channels.size(); i++) {
            done++;
            if (channels[0].currentbr != 0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if (channels[i].targetbr > channels[i].currentbr) {
                channels[i].currentbr++;
                done = 0;
            }
            if (channels[i].targetbr < channels[i].currentbr) {
                channels[i].currentbr--;
            }
            awrite(anw);
        }
    }
    TelnetPrint.println("finished notifade");
}
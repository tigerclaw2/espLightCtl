#include "light.h"
//#include <TelnetPrint.h>
#include <TaskSchedulerDeclarations.h>

extern Task tFader;
extern void diyload(int num);

LightController Light;

void faderCallback() {
    Light.fader();
}
void awrite(int mode) {
    Light.awrite(mode);
}
void notifade(int ldiy, int tick) {
    Light.notifade(ldiy, tick);
}


bool LightController::addCh(uint8_t id, uint8_t pin, uint8_t calib, bool highdef) {
    if (pin > -1 && (pin < 6 || (pin > 11 && pin <= NUM_DIGITAL_PINS))) {
        Channel ch;
        ch.id = id;
        ch.pin = pin;
        ch.calib = calib;
        if (highdef) {
            ch.currentbr=255;
        }
        
        pinMode(ch.id, OUTPUT);
        
        channels.push_back(ch);
        return 0;
    }
    return 1;
}

bool LightController::begin(bool highdef) {
    if(highdef) {
    channels = { Channel{0, 255, 0, 255, 0, 0} };
    } else {
    channels = { Channel{0, 255, 0, 1, 0, 1} }; //1 as currentbr will effectively force a refresh since it is different than target
    }
    return 0;
}
void LightController::end(bool block) {
    // Kill the background fader task immediately
    tFader.disable();

    if(block){
        channels[0].targetbr=0;
        while (channels[0].currentbr != 0) {
            channels[0].currentbr--;
            awrite();
            delay(8);
        }

    }

    for (size_t i = 1; i < channels.size(); i++) {
        analogWrite(channels[i].pin, 0); 
    }
    channels.clear(); 
    std::vector<Channel>().swap(channels); 

    // Reset internal state variables to default
    fadeTotalSteps = 0;
    fadeCurrentStep = 0;
    nms = 1000;
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
    recalculateFade();
}

void LightController::getBri(int* outValues, size_t maxSize) const {
    size_t limit = (maxSize < channels.size()) ? maxSize : channels.size();
    for (size_t i = 0; i < limit; i++) {
        outValues[i] = channels[i].targetbr;
    }
}

uint8_t LightController::getBriSingle(int id) const {
    for (const auto& ch : channels) {
        if (ch.id == id) return ch.targetbr;
    }
    return 0;
}

void LightController::setBriSingle(int id, int value) {
    for (auto& ch : channels) {
        if (ch.id == id) {
            ch.targetbr = value;
            recalculateFade();
            break;
        }
    }
}

void LightController::setFadeSpeed(double speed) {
    if(speed > 249){
        nms = speed;
    }
}

void LightController::recalculateFade() {
    if (channels.empty()) return;

    long maxDelta = 0;

    for (size_t i = 0; i < channels.size(); i++) {
        long diff = abs(channels[i].targetbr - channels[i].currentbr);
        if (diff > maxDelta) {
            maxDelta = diff;
        }
    }

    if (maxDelta == 0) return;

    //TelnetPrint.println("New command detected, retargeting fade...");

    for (size_t i = 0; i < channels.size(); i++) {
        channels[i].startbr = channels[i].currentbr;
    }

    long calculatedInterval = 0;
    long timePerStep = nms / maxDelta;

    if (timePerStep < MIN_INTERVAL) {
        calculatedInterval = MIN_INTERVAL;
        fadeTotalSteps = nms / MIN_INTERVAL;
        fadeTotalSteps = fadeTotalSteps < 1 ? 1 : fadeTotalSteps;
    } else {
        calculatedInterval = timePerStep;
        fadeTotalSteps = maxDelta;
    }

    fadeCurrentStep = 0;
    //TelnetPrint.printf("Interval: %ld | steps: %ld\n", calculatedInterval, fadeTotalSteps);
    tFader.setInterval(calculatedInterval);
    tFader.setIterations(fadeTotalSteps);

    // Using restart() instead of enable() guarantees the scheduler resets its internal clock
    tFader.enable();
}

void LightController::fader() {
    fadeCurrentStep++;
    //TelnetPrint.printf("[%ld]Fader: fadeStep: %ld\n", millis(), fadeCurrentStep);
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

    // Safety check in case we fire awrite before adding channels
    if (channels.empty()) return;

    int masterCurrent = channels[0].currentbr; // Virtual channel 0

    // Loop starting from 1 since 0 is the virtual general brightness channel
    for (size_t i = 1; i < channels.size(); i++) {
        //since max range is 2^16 we can now simply multiply each channel with the master as both are 8bit
        analogWrite(channels[i].pin, (((channels[i].currentbr * channels[i].calib) / 255) * masterCurrent) >> 1);
        //analogWrite(channels[i].pin, map(channels[i].currentbr, 0, 255, 0, channels[i].calib) * masterCurrent);
        //analogWrite(channels[i].pin, map(map(channels[i].currentbr, 0, 256, 0, channels[i].calib), 0, 256, 0, masterCurrent));
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

    // TelnetPrint.println(ldiy);
    // TelnetPrint.println(tick);
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
    // TelnetPrint.println("finished notifade");
}

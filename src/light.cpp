#include "light.h"
#include <TelnetPrint.h>
#include <TaskSchedulerDeclarations.h>

extern int chnr;
extern int targetbr[6];
extern int lastSeenTarget[6];
extern int currentbr[6];
extern int startbr[6];
extern int calib[6];
extern int chout1, chout2, chout3, chout4, chout5;
extern int anw;
extern int atx;
extern double nms;
extern long fadeTotalSteps, fadeCurrentStep;
extern Task tFader;

// External function from main.cpp
extern void diyload(int num);

void lightWatcher() {
    bool newCommandDetected = false;
    long maxDelta = 0;
    //TelnetPrint.println("Watcher Callback started.");

    // 1. Check if the USER changed the targets
    for (int i = 0; i <= chnr; i++) {
        // Compare against LAST COMMAND processed, not current brightness
        if (targetbr[i] != lastSeenTarget[i]) {
            newCommandDetected = true;
            lastSeenTarget[i] = targetbr[i];
            TelnetPrint.printf("New command detected at index %d: %d", i, targetbr[i]);
        }

        // Calculate delta based on current physical reality
        if (targetbr[i] != currentbr[i]) {
            long diff = abs(targetbr[i] - currentbr[i]);
            if (diff > maxDelta) {
                maxDelta = diff;
                TelnetPrint.printf ("New maxDelta detected: %ld at index %d ", maxDelta, i);
            }
        }
    }

    // Check for conditions to return
    if (!newCommandDetected && tFader.isEnabled()) {
        TelnetPrint.println("No new command detected; fading will continue.");
        return;
    }
    
    if (!newCommandDetected && maxDelta == 0) {
        //TelnetPrint.println("No new command and maxDelta is 0. Exiting.");
        return;
    }

    // --- If we get here, restart the fade ---
    if (tFader.isEnabled()) {
        TelnetPrint.println("Retargeting fade...");
        tFader.disable(); 
    }

    // 2. Snapshot Starting Conditions
    for (int i = 0; i <= chnr; i++) {
        startbr[i] = currentbr[i];
        TelnetPrint.printf("Starting condition for channel %d: %d\n", i, startbr[i]);
    }

    // 3. Calculate Strategy
    if (maxDelta == 0) {
        TelnetPrint.println("maxDelta is zero; exiting to prevent errors.");
        return; 
    }

    long calculatedInterval = 0;
    long timePerStep = nms / maxDelta; 
    TelnetPrint.printf("Calculated timePerStep: %ld\n", timePerStep);

    if (timePerStep < MIN_INTERVAL) {
        // Fast Fade
        calculatedInterval = MIN_INTERVAL;
        fadeTotalSteps = nms / MIN_INTERVAL;
        fadeTotalSteps = fadeTotalSteps < 1 ? 1 : fadeTotalSteps;
        TelnetPrint.println("Mode: Speed Priority");
    } else {
        // Slow Fade
        calculatedInterval = timePerStep;
        fadeTotalSteps = maxDelta;
        TelnetPrint.println("Mode: Precision Priority");
    }

    // 4. Launch
    fadeCurrentStep = 0;
    tFader.setInterval(calculatedInterval);
    tFader.setIterations(fadeTotalSteps);
    tFader.enable(); 
    TelnetPrint.printf("Fade launched with interval: %ld ms and total steps: %ld\n", calculatedInterval, fadeTotalSteps);
}

void faderCallback() {
    fadeCurrentStep++; 

    bool updateHardware = false;

    for (int i = 0; i <= chnr; i++) {
        // Skip if channel is already at target
        if (currentbr[i] == targetbr[i]) continue;

        long delta = targetbr[i] - startbr[i];
        int newVal = startbr[i] + ((delta * fadeCurrentStep) / fadeTotalSteps);

        if (currentbr[i] != newVal) {
            currentbr[i] = newVal;
            updateHardware = true;
        }
    }

    if (updateHardware) {
        awrite(anw);
    }
}

void awrite(int mode) {
    // analog write lol

    analogWriteResolution(anw);
    analogWrite(chout1, map(map(currentbr[1], 0, 256, 0, calib[1]), 0, 256, 0, currentbr[0]));
    analogWrite(chout2, map(map(currentbr[2], 0, 256, 0, calib[2]), 0, 256, 0, currentbr[0]));
    analogWrite(chout3, map(map(currentbr[3], 0, 256, 0, calib[3]), 0, 256, 0, currentbr[0]));
    analogWrite(chout4, map(map(currentbr[4], 0, 256, 0, calib[4]), 0, 256, 0, currentbr[0]));
    analogWrite(chout5, map(map(currentbr[5], 0, 256, 0, calib[5]), 0, 256, 0, currentbr[0]));
}

void notifade(int ldiy, int tick) {  // blocking function

    int noti[chnr], done = 0;
    for (int i = 0; i <= chnr; i++) {
        noti[i] = targetbr[i];
    }
    TelnetPrint.println(ldiy);
    TelnetPrint.println(tick);
    diyload(ldiy);
    while (done <= chnr + 1) {
        // if(micros() - lastfade >= 2) {
        for (int i = 0; i <= chnr; i++) {
            done++;
            if (currentbr[0] != 0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if (targetbr[i] > currentbr[i]) {
                currentbr[i]++;
                done = 0;
            }
            if (targetbr[i] < currentbr[i]) {
                currentbr[i]--;
            }
            awrite(anw);
        }
        //     lastfade=micros();
        // }
    }
    for (int i = 0; i <= chnr; i++) {
        targetbr[i] = noti[i];
    }
    done = 0;
    while (done <= chnr + 1) {
        // if(micros() - lastfade >= 2) {
        for (int i = 0; i <= chnr; i++) {
            done++;
            if (currentbr[0] != 0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if (targetbr[i] > currentbr[i]) {
                currentbr[i]++;
                done = 0;
            }
            if (targetbr[i] < currentbr[i]) {
                currentbr[i]--;
            }
            awrite(anw);
        }
        //     lastfade=micros();
        // }
    }
    TelnetPrint.println("finished notifade");
}


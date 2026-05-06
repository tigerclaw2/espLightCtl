#ifndef LED_SCRIPT_H
#define LED_SCRIPT_H

#include <Arduino.h>
#include <FS.h>

struct scriptinfo {
    File file;
    int meta_chnr;
    int pbegin;
    int loopbegin;
    int loopcount;
    bool running = false;
    String path;
    int savedbr[6]; // Replaces lastbr for script state restoration
};

// Expose the active script pointer so main.cpp can check if it's running
extern scriptinfo* activeScript;

void scriptBegin(String path);
void scriptEnd();
void scriptRunner();

#endif // LED_SCRIPT_H
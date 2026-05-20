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
    String path;
    int* savedbr;
    //int savedbr[6];
};

extern scriptinfo* activeScript;

int scriptBegin(String path);
int scriptEnd();
void scriptRunner();

#endif // LED_SCRIPT_H
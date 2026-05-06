#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>

#define MIN_INTERVAL 10 // Minimum interval in milliseconds for fading steps

void awrite(int mode = 0);
void notifade(int ldiy = 9, int tick = 20);
void lightWatcher();
void faderCallback();

#endif // LIGHT_H
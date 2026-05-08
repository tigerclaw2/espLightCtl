#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>
#include <vector>

#define MIN_INTERVAL 10 // Minimum interval in milliseconds for fading steps

extern int atx;
extern int anw;

// We bundle all the variables that used to be parallel arrays into a single Channel struct
struct Channel {
    int id;
    int pin;
    int calib;
    int currentbr = 0;
    int targetbr = 0;
    int startbr = 0;
};

class LightController {
private:
    // A single dynamic vector holding all our channel data
    std::vector<Channel> channels;

    long fadeTotalSteps = 0;
    long fadeCurrentStep = 0;
    double nms = 1000;
    
    void recalculateFade();

public:
    LightController() = default;

    // --- Channel Management ---
    // Make sure you add channel 0 first as your virtual brightness channel!
    void addCh(int id, int pin, int calib);
    size_t getChCount() const;

    // --- Setters & Getters (Standard C-Array Approach) ---
    // Pass a pointer to your array and tell it how many elements it has
    void setBri(const int* values, size_t size);
    void getBri(int* outValues, size_t maxSize) const;

    // --- Setters & Getters (Variadic / Unlimited Arguments) ---
    // This allows: Light.setBri(255, 128, 64, 200);
    template <typename... Args>
    void setBri(Args... args) {
        // Unpack the arguments into a temporary C-array and pass it to the standard function
        int values[] = { static_cast<int>(args)... };
        setBri(values, sizeof...(args));
    }

    int getBriSingle(int id) const;
    void setBriSingle(int id, int value);

    void setFadeSpeed(double speed);

    //void watcher();
    void fader();
    void awrite(int mode = 0);
    void notifade(int ldiy = 9, int tick = 20);
};

extern LightController Light;

// --- Legacy Wrappers ---
//void lightWatcher();
void faderCallback();
void awrite(int mode = 0);
void notifade(int ldiy = 9, int tick = 20);

#endif // LIGHT_H
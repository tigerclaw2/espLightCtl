#ifndef LIGHT_H
#define LIGHT_H

#include <Arduino.h>
#include <vector>

#define MIN_INTERVAL 10 // Minimum interval in milliseconds for fading steps

extern int atx;
extern int anw;

struct Channel {
    uint8_t id;
    uint8_t pin;
    uint8_t calib;
    uint8_t currentbr = 0;
    uint8_t targetbr = 0;
    uint8_t startbr = 0;
};

class LightController {
    private:

        //std::vector<Channel> channels = { Channel{0, -1, 0, 0, 0, 0} };   //this now supposedly initializes the master automatically.
        std::vector<Channel> channels;

        long fadeTotalSteps = 0;
        long fadeCurrentStep = 0;
        double nms = 1000;

        void recalculateFade();

    public:
        LightController() = default;

        // --- Channel Management ---
        bool addCh(uint8_t id, uint8_t pin, uint8_t calib, bool highdef=false);
        bool begin(bool highdef=false);
        void end(bool block=true);
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

        uint8_t getBriSingle(int id) const;
        void setBriSingle(int id, int value);

        void setFadeSpeed(double speed);

        //void watcher();
        void fader();
        void awrite(int mode = 0);
        void notifade(int ldiy = 9, int tick = 20);
};

extern LightController Light;

void faderCallback();
void awrite(int mode = 0);
void notifade(int ldiy = 9, int tick = 20);

#endif // LIGHT_H

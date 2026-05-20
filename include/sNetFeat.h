#ifndef SNETFEAT_H
#define SNETFEAT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <vector>
#include <ArduinoJson.h>
#include "FS.h"

enum ActionType {
    ACT_NONE,
    ACT_SCRIPT,
    ACT_DIY,
    ACT_POWER
};

struct TriggerAction {
    ActionType type = ACT_NONE;
    int num_val = -1;       
    String str_val = "";    
};

struct TrackedDevice {
    IPAddress ip;
    uint8_t mac[6];
    String hostname;
    bool has_mac;

    // Execution Rules
    TriggerAction on_found;
    TriggerAction on_lost;

    // Timings
    unsigned int ping_interval;
    unsigned long last_seen;
    unsigned long last_ping;

    // State
    bool is_present;
};

// Discovery Struct (For the Web API)
struct DiscoveredDevice {
    IPAddress ip;
    uint8_t mac[6];
    String hostname;
};


// Global exports for main
extern std::vector<TrackedDevice> tracked_devices;
extern WiFiUDP Udp;                           
extern unsigned int localUdpPort;     
extern std::vector<DiscoveredDevice> discovered_devices;
extern bool is_sweeping;   

// Core System Functions
void loadSnfConfig();
void performArpSweep();
void trackDevicesTask(); 
void activateDiscoveryWindow();

#endif // SNETFEAT_H
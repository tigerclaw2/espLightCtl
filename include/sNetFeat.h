#ifndef SNETFEAT_H
#define SNETFEAT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <vector>
#include <ArduinoJson.h>
#include "AsyncPing.h"
#include "FS.h"


#define MAX_DHCP_PER_TICK 10    //max leases to process per task loop run

enum ActionType {
    ACT_NONE,
    ACT_SCRIPT,
    ACT_DIY,
    ACT_POWER
};

enum PingType {
    PING_NONE = 0,    // ARP scan or dhcp lease only (very long discovery times, no resource usage)
    PING_ARP  = 1,    // ARP table query (long discovery times, not very reliable, lower resource usage)
    PING_ICMP = 2,    // AsyncPing (fast, reliable, low resource usage)
    PING_HTTP = 3     // HTTP healthcheck (slower, more reliable, high resource usage)
};
struct TriggerAction {
    ActionType type = ACT_NONE;
    int num_val = -1;       
    String str_val = "";    
};

struct TrackedDevice {
    uint32_t ip;
    //IPAddress ip;
    uint8_t mac[6];
    String hostname;
    
    uint8_t flags = 0;
    static const uint8_t FLAG_USER_IP       = 1 << 0; // 00000001
    static const uint8_t FLAG_USER_MAC      = 1 << 1; // 00000010
    static const uint8_t FLAG_USER_HOSTNAME = 1 << 2; // 00000100
    //static const uint8_t FLAG_USER_    = 1 << 3; // 00001000

    // getters and setters for json variables
    inline bool user_ip() const       { return flags & FLAG_USER_IP; }
    inline bool user_mac() const      { return flags & FLAG_USER_MAC; }
    inline bool user_hostname() const { return flags & FLAG_USER_HOSTNAME; }

    inline void user_ip(bool val)       { val ? flags |= FLAG_USER_IP       : flags &= ~FLAG_USER_IP; }
    inline void user_mac(bool val)      { val ? flags |= FLAG_USER_MAC      : flags &= ~FLAG_USER_MAC; }
    inline void user_hostname(bool val) { val ? flags |= FLAG_USER_HOSTNAME : flags &= ~FLAG_USER_HOSTNAME; }
    
    inline int user_fcount() const  { return __builtin_popcount(flags); }

    // Execution Rules
    TriggerAction on_found;
    TriggerAction on_lost;

    // Pings
    PingType ping_type;
    unsigned int ping_interval;
    unsigned long last_seen;
    unsigned long last_ping;

    // State
    bool is_present;
    AsyncPing* icmp_pinger = nullptr;
};

// Discovery Struct (For the Web API)
struct DiscoveredDevice {
    uint32_t ip;
    //IPAddress ip;
    uint8_t mac[6];
    String hostname;
};

//WiFiUDP Udp;

//uint16_t localUdpPort = 67;

// Global exports for main
extern std::vector<TrackedDevice> tracked_devices;                       
extern std::vector<DiscoveredDevice> discovered_devices;
extern bool is_scanning;   

// Core System Functions
void snfLoadConf(const char* cnfp);
void snfDoARPscan();
void snfMain(); 
void snfActivateDiscoveryWindow();
void checkDevice(DiscoveredDevice device);

#endif // SNETFEAT_H
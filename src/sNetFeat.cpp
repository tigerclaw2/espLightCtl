#include "sNetFeat.h"
#include <TelnetPrint.h>
#include "ledScript.h"
#include "light.h"

extern "C" {
#include <lwip/etharp.h>
#include <lwip/netif.h>
#include <lwip/ip_addr.h>
}

std::vector<TrackedDevice> tracked_devices;
WiFiUDP Udp;
unsigned int localUdpPort = 67;

bool is_sweeping = false;
uint8_t sweep_ip_octet = 1;

std::vector<DiscoveredDevice> discovered_devices;
unsigned long last_discovery_request = 0;
const unsigned long DISCOVERY_WINDOW_MS = 30000; // 30 seconds


bool macsMatch(const uint8_t* mac1, const uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

void parseMacString(const char* macStr, uint8_t* macArr) {
    sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &macArr[0], &macArr[1], &macArr[2], &macArr[3], &macArr[4], &macArr[5]);
}

// Parses "d5", "p0", or "s/path" into the enum struct directly from the JSON pointer
TriggerAction parseAction(const char* val) {
    TriggerAction action;
    if (!val || val[0] == '\0') return action; 

    if (val[0] == 's') {
        action.type = ACT_SCRIPT;
        action.str_val = String(val + 1); 
    } 
    else if (val[0] == 'd') {
        action.type = ACT_DIY;
        action.num_val = atoi(val + 1);
    } 
    else if (val[0] == 'p') {
        action.type = ACT_POWER;
        action.num_val = atoi(val + 1);
    }
    return action;
}

// --- 2. Action Execution Handlers ---

void handleDeviceFound(TrackedDevice& dev) {
    TelnetPrint.printf("[NET-IFTTT] Device Found! IP: %s\n", dev.ip.toString().c_str());
    
    switch (dev.on_found.type) {
        case ACT_SCRIPT:
            TelnetPrint.printf("  -> Trigger Script: %s\n", dev.on_found.str_val.c_str());
            // [ENTRY POINT] - Found Script Handler
            scriptBegin(dev.on_found.str_val);
            break;
            
        case ACT_DIY:
            TelnetPrint.printf("  -> Trigger DIY: %d\n", dev.on_found.num_val);
            // [ENTRY POINT] - Found DIY Handler
            //diyload(dev.on_found.num_val);
            break;
            
        case ACT_POWER:
            TelnetPrint.printf("  -> Trigger Power: %d\n", dev.on_found.num_val);
            // [ENTRY POINT] - Found Power Handler
            Light.setBriSingle(0, dev.on_found.num_val == 1 ? 255 : 0);
            break;
            
        case ACT_NONE:
        default:
            break;
    }
}

void handleDeviceLost(TrackedDevice& dev) {
    TelnetPrint.printf("[NET-IFTTT] Device Lost! IP: %s\n", dev.ip.toString().c_str());
    
    switch (dev.on_lost.type) {
        case ACT_SCRIPT:
            TelnetPrint.printf("  -> Trigger Script: %s\n", dev.on_lost.str_val.c_str());
            // [ENTRY POINT] - Lost Script Handler
            scriptEnd();
            break;
            
        case ACT_DIY:
            TelnetPrint.printf("  -> Trigger DIY: %d\n", dev.on_lost.num_val);
            // [ENTRY POINT] - Lost DIY Handler
            break;
            
        case ACT_POWER:
            TelnetPrint.printf("  -> Trigger Power: %d\n", dev.on_lost.num_val);
            // [ENTRY POINT] - Lost Power Handler
            Light.setBriSingle(0, dev.on_found.num_val == 1 ? 0 : 255);
            break;
            
        case ACT_NONE:
        default:
            break;
    }
}

// --- 3. Core System Logic ---

void loadSnfConfig() {
    tracked_devices.clear();
    tracked_devices.shrink_to_fit(); 

    File file = SPIFFS.open("/conf/snf.json", "r");
    if (!file) {
        TelnetPrint.println("[NET] /conf/snf.json not found. Tracking disabled.");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        TelnetPrint.printf("[NET] Failed to parse snf.json: %s\n", error.c_str());
        return;
    }

    for (JsonObject item : doc.as<JsonArray>()) {
        TrackedDevice dev;
        
        // Dynamic Identifiers
        if (item.containsKey("i")) dev.ip.fromString(item["i"].as<String>());
        else dev.ip = IPAddress(0, 0, 0, 0);

        if (item.containsKey("m")) {
            parseMacString(item["m"].as<const char*>(), dev.mac);
            dev.has_mac = true;
        } else {
            memset(dev.mac, 0, 6);
            dev.has_mac = false;
        }

        dev.hostname = item["h"] | "";

        // Rules
        if (item.containsKey("f")) dev.on_found = parseAction(item["f"]);
        if (item.containsKey("l")) dev.on_lost = parseAction(item["l"]);

        // Timings & State
        dev.ping_interval = item["p"] | 5000; 
        dev.is_present = false;
        dev.last_seen = 0;
        dev.last_ping = 0;

        tracked_devices.push_back(dev);
    }
    TelnetPrint.printf("[NET] Loaded %d rules from snf.json\n", tracked_devices.size());
}

void activateDiscoveryWindow() {
    last_discovery_request = millis();
}

void addDiscoveredDevice(IPAddress ip, const uint8_t* mac, String host) {
    for (auto& dev : discovered_devices) {
        if (macsMatch(dev.mac, mac)) {
            // Update missing info if we found it
            if (dev.ip == IPAddress(0,0,0,0) && ip[0] != 0) dev.ip = ip;
            if (dev.hostname == "" && host != "") dev.hostname = host;
            return; 
        }
    }
    // It's a new device, add it to the list
    DiscoveredDevice newDev;
    newDev.ip = ip;
    memcpy(newDev.mac, mac, 6);
    newDev.hostname = host;
    discovered_devices.push_back(newDev);
}

void performArpSweep() {
    if (WiFi.status() != WL_CONNECTED || tracked_devices.empty()) return;
    is_sweeping = true;
    sweep_ip_octet = 1;
    TelnetPrint.println("[NET] Starting ARP sweep...");
}

void trackDevicesTask() {
    if (WiFi.status() != WL_CONNECTED) return; // Exit if not connected (No need to check tracked_devices.empty() here so discovery still works)

    unsigned long current_time = millis();
    bool discovery_active = (current_time - last_discovery_request < DISCOVERY_WINDOW_MS);

    // --- A. DHCP Snooping ---
    int packetSize = Udp.parsePacket();
    if (packetSize >= 240) { 
        uint8_t buffer[300]; 
        int len = Udp.read(buffer, sizeof(buffer));

        if (buffer[236] == 0x63 && buffer[237] == 0x82 && buffer[238] == 0x53 && buffer[239] == 0x63) {
            uint8_t packet_mac[6];
            memcpy(packet_mac, &buffer[28], 6);
            IPAddress packet_ip(buffer[16], buffer[17], buffer[18], buffer[19]); 
            
            String packet_host = "";
            int idx = 240;
            while (idx < len && buffer[idx] != 255) {
                if (buffer[idx] == 0) { idx++; continue; }
                uint8_t tag = buffer[idx];
                uint8_t opt_len = buffer[idx + 1];
                if (tag == 12) {
                    for (int i = 0; i < opt_len; i++) packet_host += (char)buffer[idx + 2 + i];
                }
                idx += 2 + opt_len;
            }

            // 1. ALWAYS run the logic for our tracked JSON devices
            for (auto& dev : tracked_devices) {
                bool matched = false;
                if (dev.has_mac && macsMatch(dev.mac, packet_mac)) matched = true;
                else if (dev.ip != IPAddress(0,0,0,0) && dev.ip == packet_ip) matched = true;
                else if (dev.hostname.length() > 0 && dev.hostname.equalsIgnoreCase(packet_host)) matched = true;

                if (matched) {
                    if (!dev.has_mac) { memcpy(dev.mac, packet_mac, 6); dev.has_mac = true; }
                    if (dev.ip == IPAddress(0,0,0,0) && packet_ip[0] != 0) dev.ip = packet_ip;

                    dev.last_seen = current_time;
                    if (!dev.is_present) {
                        dev.is_present = true;
                        handleDeviceFound(dev);
                    }
                }
            }

            // 2. ONLY add to the UI list if the web endpoint was recently hit
            if (discovery_active) {
                addDiscoveredDevice(packet_ip, packet_mac, packet_host);
            }
        }
    }

    // Ping & Timeout
    for (auto& dev : tracked_devices) {
        
        unsigned long timeout_ms = dev.ping_interval * 3; // 3 missed pings = lost

        if (current_time - dev.last_ping >= dev.ping_interval) {
            dev.last_ping = current_time;
            
            if (dev.ip[0] != 0) { 
                ip4_addr_t lwip_ip;
                lwip_ip.addr = dev.ip;
                struct eth_addr *ret_eth_addr;
                const ip4_addr_t *ret_ip_addr;

                ssize_t idx = etharp_find_addr(netif_default, &lwip_ip, &ret_eth_addr, &ret_ip_addr);
                
                if (idx >= 0) {
                    if (!dev.has_mac) {
                        memcpy(dev.mac, ret_eth_addr->addr, 6);
                        dev.has_mac = true;
                    }

                    if (macsMatch(ret_eth_addr->addr, dev.mac)) {
                        dev.last_seen = current_time;
                        if (!dev.is_present) {
                            dev.is_present = true;
                            handleDeviceFound(dev);
                        }
                    }
                } else {
                    etharp_request(netif_default, &lwip_ip); // Send physical ARP Request
                }
            }
        }

        if (dev.is_present && (current_time - dev.last_seen > timeout_ms)) {
            dev.is_present = false;
            handleDeviceLost(dev);
        }
    }

    // Triggered ARP scan
    if (is_sweeping) {
        IPAddress localIP = WiFi.localIP();
        IPAddress subnet = WiFi.subnetMask();
        IPAddress network;
        for (int i = 0; i < 4; i++) network[i] = localIP[i] & subnet[i];

        IPAddress currentIP = network;
        currentIP[3] = sweep_ip_octet;

        if (currentIP != localIP) {
            ip4_addr_t target_ipaddr;
            target_ipaddr.addr = currentIP;
            etharp_request(netif_default, &target_ipaddr);
        }

        sweep_ip_octet++;
        if (sweep_ip_octet >= 254) {
            is_sweeping = false;
            TelnetPrint.println("[NET] ARP sweep complete.");
        }
    }
    for (size_t i = 0; i < ARP_TABLE_SIZE; ++i) {
        ip4_addr_t *ipaddr;
        struct netif *netif;
        struct eth_addr *eth_ret;
        
        // If there is a valid entry in the cache, add it to our list
        if (etharp_get_entry(i, &ipaddr, &netif, &eth_ret)) {
            addDiscoveredDevice(ipaddr->addr, eth_ret->addr, "");
        }
    }

    if (discovery_active) {
        // Harvest LwIP cache only when the UI is actively watching
        for (size_t i = 0; i < ARP_TABLE_SIZE; ++i) {
            ip4_addr_t *ipaddr;
            struct netif *netif;
            struct eth_addr *eth_ret;
            if (etharp_get_entry(i, &ipaddr, &netif, &eth_ret)) {
                addDiscoveredDevice(ipaddr->addr, eth_ret->addr, "");
            }
        }
    } else if (!discovered_devices.empty()) {
        // The 30-second window expired.
        discovered_devices.clear();
        discovered_devices.shrink_to_fit();
        TelnetPrint.println("[NET] Discovery window closed. Cache cleared.");
    }
}

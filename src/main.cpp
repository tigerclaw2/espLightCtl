/*
upload_port = 192.168.1.142
upload_protocol = espota

function assig table:
+------+--------+-------+---+---+-------+-----+-----+-----+-----+
|  f:  | 1 .. 5 |   6   | 7 | 8 |   9   | 10  | 11  | 12  | 13  |
+------+--------+-------+---+---+-------+-----+-----+-----+-----+
| act: |sel 1..5| sel++ |on |off| on/off| br+ | br- | max | min |
+------+--------+-------+---+---+-------+-----+-----+-----+-----+
=======================  //todo: update ir handle x, create ADC handle, create map creator
+---------------------+
|   101...199 (1xx)   |
+---------------------+
| diy 1...99 (diy xx) |
+---------------------+
*/
// check if raw actually does something, probably not
// adc_val is not yet used but should probably not be an int
// fadetick needs to be renamed and better mapped to nms that as well needs to be renamed
// vl is only used in debug, remember to delete when releasing
// cleanup web api that is implemented twice
// cleanup

// Things to actually do:
// change locks
// timers

#include <Arduino.h>
// #include <string>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncWebServer.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <TelnetPrint.h>
#include "FS.h"
#include <untar.h>
#include <TaskScheduler.h>

#include <WiFiUdp.h>

#include "light.h"
#include "webpages.h"
#include "ledScript.h"

extern "C" {
  #include <lwip/etharp.h>
  #include <lwip/netif.h>
  #include <lwip/ip_addr.h>
  }

ADC_MODE(ADC_VCC);
#define VERSION "0.8.3_Debug"
#define DEBUG_USE_TELNET 1

#define SPIFFS_CACHE = (1)
#define SPIFFS_CACHE_WR = (1)

#define DECODE_NEC
// #define DIYTSIZE 112
// #define DIYTSIZE 130
#define MAX_IR 10
#define MAX_BTN 10
#define BTOL 5
#define MAXCH 6

#define irbru 0xF700FF
#define irbrd 0xF7807F
#define irpoff 0xF740BF
#define irpon 0xF7C03F
#define irr 0xF720DF
#define irg 0xF7A05F
#define irb 0xF7609F
#define irw 0xF7E01F
#define irdiy1 0xF710EF
#define irdiy2 0xF7906F
#define irdiy3 0xF750AF
#define irdiy4 0xF730CF
#define irdiy5 0xF7B04F
#define irdiy6 0xF7708F
#define irmax 0xF7D02F
#define irmin 0xF7F00F

int recvPin = -1;

IRrecv irrecv(recvPin);
decode_results results;
AsyncWebServer server(80);
DNSServer dnsServer;
Tar<FS> tar(&SPIFFS);

Scheduler runner;


WiFiUDP Udp;                           //[cite: 2]
unsigned int localUdpPort = 67;       // Listen port under 1024 for debug[cite: 2]
char incomingPacket[255];              // Buffer for incoming packets[cite: 2]

bool enable_net_scan = false;          // Master toggle
uint8_t scan_current_ip = 1;           // Tracks ARP scanning IP

// --- DHCP Lease Storage ---
#define MAX_LEASES 20

struct dhlease {
    // --- Fixed DHCP Header Fields ---
    uint8_t op;              // Message op code (1 = BootRequest)
    uint8_t htype;           // Hardware address type (1 = Ethernet)
    uint8_t hlen;            // Hardware address length (usually 6 for MAC)
    uint8_t hops;            // Relay agent hops
    uint32_t xid;            // Transaction ID (random number chosen by client)
    uint16_t secs;           // Seconds elapsed since client began process
    uint16_t flags;          // Flags (e.g., Broadcast flag)
    IPAddress ciaddr;        // Client IP address (if client already has one)
    IPAddress yiaddr;        // 'Your' (client) IP address
    IPAddress siaddr;        // Next server IP address
    IPAddress giaddr;        // Relay agent IP address
    uint8_t chaddr[16];      // Client hardware address (First 6 bytes are MAC)
    char sname[64];          // Optional server host name (rarely used by clients)
    char file[128];          // Boot file name (rarely used by clients)

    // --- DHCP Options (Variable Length) ---
    uint8_t msg_type;              // Option 53: DHCP Message Type (1=Discover, 3=Request, etc.)
    IPAddress requested_ip;        // Option 50: Requested IP
    String hostname;               // Option 12: Hostname
    String vendor_class;           // Option 60: Vendor Class Identifier
    String client_id;              // Option 61: Client Identifier (often MAC or UUID)
    String fqdn;                   // Option 81: Fully Qualified Domain Name
    uint16_t max_msg_size;         // Option 57: Maximum DHCP Message Size client accepts
    uint8_t param_req_list[50];    // Option 55: Parameter Request List (what the client wants from the router)
    uint8_t param_req_len;         // Length of the Parameter Request List
    
    bool active; // Flag to check if this slot in the array is populated
};

dhlease leases[MAX_LEASES];

//String aaaa2;

// int rboot = 1;
// int winset = 20;
// int eeprommax = 10;

int chout1 = -1;
int chout2 = -1;
int chout3 = -1;
int chout4 = -1;
int chout5 = -1;
int atx = -1;
int adc = -1;
int statusled = LED_BUILTIN;
String webui = "dev";

struct control {
    struct btns {
        int button;
        int function;
    } btn[MAX_BTN], irbtn[MAX_IR];
    int btnmax;
    float btnign;
    int btnskp;
    int irbmax;
    int irbign;
    int irbskp;
} ctrl;

// struct scriptinfo {
//     File file;
//     int meta_chnr;
//     // int current_line;
//     int pbegin;
//     int loopbegin;
//     int loopcount;
//     //unsigned long resumeMillis = 0;
//     bool running = false;
//     //int oldbr[6];
//     String path;
// } script;

unsigned long irtime;
unsigned long lastir;
unsigned long wltim;
unsigned long lastsave;
unsigned long lastwlscan;
//long fadeTotalSteps = 0;
//long fadeCurrentStep = 0;
int sel;
int diynr = 0;
//int chnr = 0;
int anw = 1;
int lastbr[6];
int wltimeout;
int raw;
//int calib[6];
int savetim;
int persistbr[6];
//int targetbr[6];
int adc_val;
int fadetick;
int ntik;
//double nms = 1000;  //<<<<<<<<<<<<<<<<<------------------------
//int currentbr[6];
//int startbr[6];

//double speed[6], currentbr[6];
// float adc_val;
bool wlconf_started, setup_ok, irhold = 0, start_noti, brichanged;

//int spamvar;

//int lastSeenTarget[6]; // adjust size to match your channel count (chnr+1)

void diyedit(int num);
void diyload(int num);
void userInputWatcher();
//void scriptRunner();

void netScannerCallback();

void infoPrintCallback();

// Task Definitions

Task tScriptR(10, TASK_FOREVER, &scriptRunner, &runner, false);
Task tUInputW(50, TASK_FOREVER, &userInputWatcher, &runner, true);
Task tFader(15, TASK_ONCE, &faderCallback, &runner, false);
//Task tLightW(100, TASK_FOREVER, &lightWatcher, &runner, true);
Task tNetScanner(50, TASK_FOREVER, &netScannerCallback, &runner, true);
//Task tInfoPrint(8000, TASK_FOREVER, &infoPrintCallback, &runner, true);


void infoPrintCallback() {
        // debug prints go here
        TelnetPrint.flush();
}

void netScannerCallback() {
    if (!enable_net_scan || WiFi.status() != WL_CONNECTED) return;

    // --- 1. Comprehensive DHCP Sniffer & Parser ---
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        uint8_t buffer[600]; // Increased to 600 to handle maximum standard DHCP packet sizes
        int len = Udp.read(buffer, sizeof(buffer));
        
        // A valid DHCP packet to the magic cookie is 240 bytes
        if (len >= 240) {
            // Check for the DHCP Magic Cookie
            if (buffer[236] == 0x63 && buffer[237] == 0x82 && buffer[238] == 0x53 && buffer[239] == 0x63) {
                
                dhlease tempLease;
                tempLease.active = true;
                
                // --- Parse Fixed Header Fields ---
                tempLease.op    = buffer[0];
                tempLease.htype = buffer[1];
                tempLease.hlen  = buffer[2];
                tempLease.hops  = buffer[3];
                
                // Combine bytes for multi-byte values (Network Byte Order is Big-Endian)
                tempLease.xid   = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
                tempLease.secs  = (buffer[8] << 8) | buffer[9];
                tempLease.flags = (buffer[10] << 8) | buffer[11];
                
                tempLease.ciaddr = IPAddress(buffer[12], buffer[13], buffer[14], buffer[15]);
                tempLease.yiaddr = IPAddress(buffer[16], buffer[17], buffer[18], buffer[19]);
                tempLease.siaddr = IPAddress(buffer[20], buffer[21], buffer[22], buffer[23]);
                tempLease.giaddr = IPAddress(buffer[24], buffer[25], buffer[26], buffer[27]);
                
                memcpy(tempLease.chaddr, &buffer[28], 16);
                memcpy(tempLease.sname, &buffer[44], 64);
                memcpy(tempLease.file, &buffer[108], 128);

                // Initialize defaults for optional fields
                tempLease.msg_type = 0;
                tempLease.requested_ip = IPAddress(0,0,0,0);
                tempLease.hostname = "";
                tempLease.vendor_class = "";
                tempLease.client_id = "";
                tempLease.fqdn = "";
                tempLease.max_msg_size = 0;
                tempLease.param_req_len = 0;

                // --- Parse DHCP Options ---
                int idx = 240;
                while (idx < len && buffer[idx] != 255) { // 255 (0xFF) is the End Option tag
                    uint8_t tag = buffer[idx];
                    if (tag == 0) { // Padding
                        idx++; 
                        continue; 
                    }
                    
                    uint8_t opt_len = buffer[idx + 1];
                    uint8_t* opt_data = &buffer[idx + 2];
                    idx += 2; // Move past Tag and Length
                    
                    switch(tag) {
                        case 12: // Hostname
                            for (int i = 0; i < opt_len; i++) tempLease.hostname += (char)opt_data[i];
                            break;
                        case 50: // Requested IP
                            if (opt_len == 4) tempLease.requested_ip = IPAddress(opt_data[0], opt_data[1], opt_data[2], opt_data[3]);
                            break;
                        case 53: // DHCP Message Type
                            if (opt_len == 1) tempLease.msg_type = opt_data[0];
                            break;
                        case 55: // Parameter Request List
                            tempLease.param_req_len = (opt_len < 50) ? opt_len : 50; // Cap at 50 to prevent array overflow
                            memcpy(tempLease.param_req_list, opt_data, tempLease.param_req_len);
                            break;
                        case 57: // Maximum Message Size
                            if (opt_len == 2) tempLease.max_msg_size = (opt_data[0] << 8) | opt_data[1];
                            break;
                        case 60: // Vendor Class Identifier
                            for (int i = 0; i < opt_len; i++) tempLease.vendor_class += (char)opt_data[i];
                            break;
                        case 61: // Client Identifier (Format: Type byte + Identifier)
                            for (int i = 0; i < opt_len; i++) {
                                // Often includes non-printable hex, so converting to a hex string is safer
                                char hexStr[3];
                                sprintf(hexStr, "%02X", opt_data[i]);
                                tempLease.client_id += hexStr;
                            }
                            break;
                        case 81: // Fully Qualified Domain Name
                            // FQDN option has 3 bytes of flags/rcodes before the actual name
                            if (opt_len > 3) {
                                for (int i = 3; i < opt_len; i++) tempLease.fqdn += (char)opt_data[i];
                            }
                            break;
                    }
                    
                    idx += opt_len; // Advance to the next option
                }

                // --- Store and Print the Parsed Data ---
                // Msg Type 3 = DHCP Request (The client asking the router to confirm an IP)
                if (tempLease.msg_type == 3 || tempLease.msg_type == 1) { // 1 = Discover, 3 = Request
                    
                    int slot = -1;
                    for (int i = 0; i < MAX_LEASES; i++) {
                        // Compare the first 6 bytes of chaddr (the MAC)
                        if (leases[i].active && memcmp(leases[i].chaddr, tempLease.chaddr, 6) == 0) {
                            slot = i; 
                            break;
                        } else if (!leases[i].active && slot == -1) {
                            slot = i; 
                        }
                    }

                    if (slot != -1) {
                        leases[slot] = tempLease; 
                        
                        TelnetPrint.printf("\n[DHCP] Packet Captured -> Slot [%d]\n", slot);
                        TelnetPrint.printf("  |- Msg Type: %d (1=Discover, 3=Request)\n", tempLease.msg_type);
                        TelnetPrint.print("  |- MAC:      ");
                        for (int i = 0; i < 6; i++) {
                            if (tempLease.chaddr[i] < 0x10) TelnetPrint.print("0");
                            TelnetPrint.print(tempLease.chaddr[i], HEX);
                            if (i < 5) TelnetPrint.print(":");
                        }
                        TelnetPrint.printf("\n  |- Req IP:   %s\n", tempLease.requested_ip.toString().c_str());
                        TelnetPrint.printf("  |- Hostname: %s\n", tempLease.hostname.c_str());
                        TelnetPrint.printf("  |- Vendor:   %s\n", tempLease.vendor_class.c_str());
                        TelnetPrint.printf("  |- ClientID: %s\n", tempLease.client_id.c_str());
                        TelnetPrint.printf("  |- FQDN:     %s\n", tempLease.fqdn.c_str());
                        TelnetPrint.printf("  |- Trans ID: %lu\n", tempLease.xid);
                        TelnetPrint.println("------------------------------------------------");
                    }
                }
            }
        }
    
    }

    // --- 2. Non-blocking ARP Scanner ---
    // (Your existing ARP scanning logic remains exactly the same here)
    IPAddress localIP = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress network;
    for (int i = 0; i < 4; i++) network[i] = localIP[i] & subnet[i];

    uint8_t prev_ip_octet = (scan_current_ip == 1) ? 254 : scan_current_ip - 1;
    IPAddress prevIP = network;
    prevIP[3] = prev_ip_octet;

    if (prevIP != localIP) {
        ip4_addr_t lwip_ip;
        lwip_ip.addr = prevIP;
        struct eth_addr *ret_eth_addr;
        const ip4_addr_t *ret_ip_addr;

        ssize_t idx = etharp_find_addr(netif_default, &lwip_ip, &ret_eth_addr, &ret_ip_addr);
        if (idx >= 0) {
            // Optional: You could also cross-reference ARP finds with your 'leases' array here!
            TelnetPrint.print("[ARP] Device found -> IP: ");
            TelnetPrint.print(prevIP.toString());
            TelnetPrint.print(" | MAC: ");
            for (int i = 0; i < 6; i++) {
                if (ret_eth_addr->addr[i] < 0x10) TelnetPrint.print("0");
                TelnetPrint.print(ret_eth_addr->addr[i], HEX);
                if (i < 5) TelnetPrint.print(":");
            }
            TelnetPrint.println();
        }
    }

    IPAddress currentIP = network;
    currentIP[3] = scan_current_ip;

    if (currentIP != localIP) {
        ip4_addr_t target_ipaddr;
        target_ipaddr.addr = currentIP;
        etharp_request(netif_default, &target_ipaddr);
    }

    scan_current_ip++;
    if (scan_current_ip >= 255) scan_current_ip = 1;
}



int needs_update() {
    return 0;  // early return for debug purposes, never save brightness to disk
    // if(!activeScript) {
    //     for (int i = 0; i <= chnr; i++) {
    //         if (targetbr[i] != persistbr[i]) {
    //             return 1;
    //         }
    //     }
    // } else {
    //     //todo
    // }
    // return 0;

}

void btn_loadmap() {
    TelnetPrint.println("BTN LoadMap");
    File file = SPIFFS.open("/map.btn", "r");
    // Read the number of mappings from the first int in the file
    ctrl.btnmax = file.parseInt();
    ctrl.btnign = file.parseFloat();
    for (int i = 1; i < MAX_BTN; i++) {
        ctrl.btn[i].button = file.parseInt();
        ctrl.btn[i].function = file.parseInt();
        if (!file.available()) {
            ctrl.btn[i + 1].button = -1;
            break;
        }
    }
    file.close();
}

// Look up the function assigned to a button
int btn_lookup(int btn) {
    TelnetPrint.println("BTN Lookup");
    for (int i = 1; i <= MAX_BTN; i++) {  // Check if the button is in the btable array
        if (btn > ctrl.btn[i].button - BTOL && btn < ctrl.btn[i].button + BTOL) {
            return ctrl.btn[i].function;
        }
        if (ctrl.btn[i].button == -1) {
            return 0;
        }
        // If the button was not found in the btable array, search the file
        if (i == MAX_BTN && ctrl.btnmax > MAX_BTN) {
            TelnetPrint.println("BTN Lookup file");

            File file = SPIFFS.open("/map.btn", "r");
            file.seek(2 * MAX_BTN * sizeof(int));  // seek the file, there is no need to read again what we already have in ram
            // needs testing

            // Search the file for the button
            for (int i = MAX_BTN; i < ctrl.btnmax; i++) {
                int fileButton = file.parseInt();
                int fileFunction = file.parseInt();
                if (btn > fileButton + BTOL && btn < fileButton - BTOL) {
                    file.close();
                    return fileFunction;
                }
            }
            while (int fileButton = file.parseInt()) {
                if (btn > fileButton - BTOL && btn < fileButton + BTOL) {
                    return file.parseInt();
                } else {
                    file.seek(sizeof(int));
                }
            }
            file.close();
        }
    }
    return 0;
}

void ir_loadmap() {
    TelnetPrint.println("IR LoadMap");

    File file = SPIFFS.open("/map.ir", "r");
    // Read the number of mappings from the first int in the file
    ctrl.btnmax = file.parseInt();
    ctrl.btnign = file.parseFloat();
    for (int i = 1; i < MAX_IR; i++) {
        ctrl.btn[i].button = file.parseInt();
        ctrl.btn[i].function = file.parseInt();
        if (!file.available()) {
            break;
        }
    }
    file.close();
}

// Look up the function assigned to a button
int ir_lookup(int btn) {
    TelnetPrint.println("IR Lookup");

    // Check if the button is in the btable array
    for (int i = 1; i <= MAX_IR; i++) {
        if (ctrl.irbtn[i].button == btn) {
            return ctrl.irbtn[i].function;
        }

        // If the button was not found in the btable array, search the file
        if (i == MAX_IR && ctrl.irbmax > MAX_IR) {
            File file = SPIFFS.open("/map.ir", "r");
            TelnetPrint.println("IR Lookup file");

            // needs testing and correct size calculation

            // file.seek(2*MAX_IR*sizeof(int));

            // Search the file for the button
            for (int i = MAX_IR; i < ctrl.irbmax; i++) {
                int fileButton = file.parseInt();
                int fileFunction = file.parseInt();
                if (fileButton == btn) {
                    file.close();
                    return fileFunction;
                }
            }

            file.close();
        }
    }
    return 0;
}

void notFound(AsyncWebServerRequest *request) {
    request->redirect("/");
}
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    // from https://github.com/smford/esp32-asyncwebserver-fileupload-example/

    if (!index) {
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SPIFFS.open("/t/" + filename, "w");
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
    }

    if (final) {
        request->_tempFile.close();
        File file = SPIFFS.open("/t/" + filename, "r");
        TelnetPrint.println("open /t/");
        if (file) {
            tar.open((Stream *)&file);  // Pass source file as Stream to Tar object
            tar.dest("/w/");
            tar.extract();
            file.close();
            TelnetPrint.println("untar ok");
            TelnetPrint.println(filename);
            // SPIFFS.rmdir("/t/");
            SPIFFS.remove("/t/" + filename);
            TelnetPrint.println("Removed tar");
            TelnetPrint.println(filename);
            Dir dir = SPIFFS.openDir("/");
            while (dir.next()) {
                TelnetPrint.print("In flash:");
                TelnetPrint.println(dir.fileName());
            }
        }
        request->redirect("/thm");
    }
}

void sysreboot(int mode = 0) {
    switch (mode) {
    case 1:
        Serial.println("[SYS] Rebooting now! (UART download mode)");
        TelnetPrint.println("[SYS] Rebooting now! (UART download mode)");
        TelnetPrint.println("[SYS] Rebooting now! (UART download mode)");
        TelnetPrint.flush();
        delay(200);
        ESP.rebootIntoUartDownloadMode();
        break;
    default:
        Serial.println("[SYS] Rebooting now! (software mode)");
        TelnetPrint.println("[SYS] Rebooting now! (software mode)");
        SPIFFS.end();
        Serial.println("[SPIFFS] Unmounted.");
        TelnetPrint.println("[SPIFFS] Unmounted.");
        TelnetPrint.flush();
        ESP.restart();
        break;
    }
}


void wlconf2() {
    wlconf_started = true;
    WiFi.persistent(0);
    File jsnwlan = SPIFFS.open("/wlan.json", "r");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsnwlan);
    if (error) {
        Serial.print(F("[WLAN] JSON deserializeJson() failed: "));
        TelnetPrint.print(F("[WLAN] JSON deserializeJson() failed: "));
        Serial.println(error.f_str());
        TelnetPrint.println(error.f_str());
        WiFi.mode(WIFI_AP);
        WiFi.softAP("esp_LightCtl", "987654321");
    } else {
        switch (doc["wlm"].as<int>()) {
        case 1: {  // ap mode
            WiFi.mode(WIFI_AP);
            WiFi.softAP(doc["apssid"] | "esp_LightCtl", doc["appsk"] | "987654321");
            dnsServer.start(53, "*", WiFi.softAPIP());
            break;
        }
        case 2: {  // client mode
            if (WiFi.SSID().c_str() != doc["ssid"] || WiFi.psk().c_str() != doc["psk"]) {
                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                WiFi.begin(doc["ssid"], doc["psk"] | "");
            }
            break;
        }
        case 3: {  // ap + client mode
            if (WiFi.SSID().c_str() != doc["ssid"] || WiFi.psk().c_str() != doc["psk"] || WiFi.softAPSSID().c_str() != doc["apssid"] || WiFi.softAPPSK().c_str() != doc["appsk"]) {
                WiFi.disconnect();
                WiFi.mode(WIFI_AP_STA);
                WiFi.begin(doc["ssid"], doc["psk"] | "");
                WiFi.softAP(doc["apssid"] | "esp_LightCtl", doc["appsk"] | "987654321");
            }
            dnsServer.start(53, "*", WiFi.softAPIP());
            break;
        }
        }
    }
    jsnwlan.close();
}


void startsrv() {
    Serial.println("[SETUP] Stopping webserver (end)");
    TelnetPrint.println("[SETUP] Stopping webserver (end)");
    server.end();

    //static builtins
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(webui=="dev") {
            request->send_P(200, "text/html", index_html);
        } else {
            request->redirect("/w/" + webui + "/i.htm");
        }
    });

    server.on("/a.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/css",  a_css);
    });

    server.on("/wlcfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", wlan_html);
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", reset_html);
    });

    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", setup_html);
    });

    server.on("/up",    HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", theme_html);
    });

    server.on("/pick",  HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", picker_html);
    });

    server.on("/pick2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", picker2_html);
    });

    server.on("/diym",  HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", diymanager_html);
    });

    server.on("/thm",   HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", theme_html);
    });


    // apis and interactive stuff:

    // set/write apis

    server.on("/a1/wipe", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(request->hasArg("meta_fact")) {
            File jsnw = SPIFFS.open("/cfg.json", "w");
            TelnetPrint.println("open cfg.json WRITE");
            JsonDocument doc;
            doc["meta"]["fact"]=request->arg("meta_fact");
            serializeJson(doc, jsnw);
            jsnw.close();
            if(request->arg("meta_fact").toInt()==1) {
                request->send_P(200, "text/html", success_html);
                delay(1000);
                sysreboot(0);
            }
        } else {
            request->send_P(500, "text/html", error_html);

        }
    });

    server.on("/a1/wlset", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasArg("wlm") && request->hasArg("ssid") && request->hasArg("psk") && request->hasArg("apssid") && request->hasArg("appsk") && request->hasArg("t")) {
            File jsnw = SPIFFS.open("/wlan.json", "w");
            TelnetPrint.println("open wlan.json WRITE");
            JsonDocument doc; //320
            doc["wlm"]=request->arg("wlm");
            doc["ssid"]=request->arg("ssid");
            doc["psk"]=request->arg("psk");
            doc["apssid"]=request->arg("apssid");
            doc["appsk"]=request->arg("appsk");
            doc["t"]=request->arg("t");
            serializeJson(doc, jsnw);
            jsnw.close();
            wlconf2();
            request->send_P(200, "text/html", success_html);
        } else {
            request->send_P(500, "text/html", error_html);
        }
    });

    server.on("/a1/setcfg", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[SETUP] /save Opening cfg.json w");
        TelnetPrint.println("[SETUP] /save Opening cfg.json w");
        File jsncfg = SPIFFS.open("/cfg.json", "w");
        TelnetPrint.println("open cfg.json WRITE");
        JsonDocument doc;
        if(request->hasArg("hw_c1"))
            doc["hw"]["c1"] = request->arg("hw_c1");
        if(request->hasArg("hw_c2"))
            doc["hw"]["c2"] = request->arg("hw_c2");
        if(request->hasArg("hw_c3"))
            doc["hw"]["c3"] = request->arg("hw_c3");
        if(request->hasArg("hw_c4"))
            doc["hw"]["c4"] = request->arg("hw_c4");
        if(request->hasArg("hw_c5"))
            doc["hw"]["c5"] = request->arg("hw_c5");
        if(request->hasArg("hw_st"))
            doc["hw"]["st"] = request->arg("hw_st");
        if(request->hasArg("hw_p"))
            doc["hw"]["p"] = request->arg("hw_p");
        if(request->hasArg("hw_adc")) {
            if(request->arg("hw_adc")) 
                doc["hw"]["adc"] = request->arg("hw_adc");
        } else {
            doc["hw"]["adc"] = -1;
        }
        if(request->hasArg("sw_anw"))
            doc["sw"]["anw"] = request->arg("sw_anw");
        if(request->hasArg("sw_dnr"))
            doc["sw"]["dnr"] = request->arg("sw_dnr");
        if(request->hasArg("sw_b0"))
            doc["sw"]["b0"]   = request->arg("sw_b0");
        if(request->hasArg("sw_b1"))
            doc["sw"]["b1"]  = request->arg("sw_b1");
        if(request->hasArg("sw_b2"))
            doc["sw"]["b2"]  = request->arg("sw_b2");
        if(request->hasArg("sw_b3"))
            doc["sw"]["b3"]  = request->arg("sw_b3");
        if(request->hasArg("sw_b4"))
            doc["sw"]["b4"]  = request->arg("sw_b4");
        if(request->hasArg("sw_b5"))
            doc["sw"]["b5"]  = request->arg("sw_b5");
        if(request->hasArg("sw_tik"))
            doc["sw"]["tik"]  = request->arg("sw_tik");
        if(request->hasArg("sw_rbt"))
            doc["sw"]["rbt"]  = request->arg("sw_rbt");
        serializeJson(doc, jsncfg);
        jsncfg.close();

        request->send_P(200, "text/html", success_html);
        setup_ok=1;
    });

    server.on("/a1/setui", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(request->hasArg("web")) {
            File jsnweb = SPIFFS.open("/web.json", "w");
            TelnetPrint.println("open web.json WRITE");
            JsonDocument doc;
            Dir dir = SPIFFS.openDir("/w/");

            while (dir.next()) {
                if(dir.fileName().endsWith(request->arg("web")+".i")) {
                    File tinfo=SPIFFS.open(dir.fileName(), "r");
                    doc["web"] = tinfo.readString();
                    tinfo.close();
                    TelnetPrint.printf(doc["web"]);
                }
            }
            serializeJson(doc, jsnweb);
            jsnweb.close();
        }
        request->send_P(200, "text/html", success_html);
    });

    server.on("/a1/fup", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleUpload);

    server.on("/a1/shade", HTTP_POST, [](AsyncWebServerRequest *request) {
        raw = 0;
        if (request->hasArg("ch1"))
            Light.setBriSingle(1,request->arg("ch1").toInt());
            //targetbr[1] = request->arg("ch1").toInt();
        if (request->hasArg("ch2"))
            Light.setBriSingle(2,request->arg("ch2").toInt());
            //targetbr[2] = request->arg("ch2").toInt();
        if (request->hasArg("ch3"))
            Light.setBriSingle(3,request->arg("ch3").toInt());
        //targetbr[3] = request->arg("ch3").toInt();
        if (request->hasArg("ch4"))
            Light.setBriSingle(4,request->arg("ch4").toInt());
            //targetbr[4] = request->arg("ch4").toInt();
        if (request->hasArg("ch5"))
            Light.setBriSingle(5,request->arg("ch5").toInt());
            //targetbr[5] = request->arg("ch5").toInt();
        if (request->hasArg("ch0"))
            Light.setBriSingle(0,request->arg("ch0").toInt());
            //targetbr[0] = request->arg("ch0").toInt();
        lastsave = millis();

        request->send_P(200, "text/html", success_html);
    });

    server.on("/a1/dic", HTTP_GET, [](AsyncWebServerRequest *request) {             // diy create 
        SPIFFS.remove("/diy.json");
        request->redirect("/diym");
    });

    server.on("/a1/diyl", HTTP_POST, [](AsyncWebServerRequest *request) {           // diy load
        TelnetPrint.println("[WEB] /diy");
        raw=0;
        if(request->hasArg("dl")) {
            TelnetPrint.println("[WEB] /diy found dl");
            TelnetPrint.println(request->arg("dl").toInt());
            diyload(request->arg("dl").toInt());
        } else if(request->hasArg("de")) {
            TelnetPrint.println("[WEB] /diy found de");
            for(int i=0; i<=Light.getChCount(); i++) {
                lastbr[i]=Light.getBriSingle(i);
                TelnetPrint.println(lastbr[i]);
            }
            TelnetPrint.println(request->arg("de").toInt());
            diyedit(request->arg("de").toInt());
        }
        request->send_P(200, "text/html", success_html);
    });

    // get/read apis

    server.on("/a1/wlist", HTTP_GET, [](AsyncWebServerRequest *request) {
    // async webserver is in a hurry and crashes if it has to wait for a blocking function to return
    // so we need to do this and call the api 2 times in the webui
        if(millis() - lastwlscan > 8000) {
            WiFi.scanNetworks(true, true);
            lastwlscan = millis();
            request->send_P(429, "text/plain", "Scan still in progress, try again in a few seconds");
        } else {
            JsonDocument doc;
            String stat;
            // Add the scanned networks to the JSON document
            for (int i = 0; i < WiFi.scanComplete(); i++) {
                JsonObject wifi = doc.createNestedObject();
                wifi["n"] = WiFi.SSID(i);
                wifi["e"] = WiFi.encryptionType(i);
                wifi["p"] = WiFi.RSSI(i);
            }
            serializeJson(doc, stat);
            request->send_P(200, "application/json", stat.c_str());
        }
    });

    server.on("/a1/lsui", HTTP_GET, [](AsyncWebServerRequest *request) {
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            // stat += concat(dir.fileName().indexOf('/', 1);
            // stat += filename.substring(0,)
            if(dir.fileName().endsWith(".i")) {
                stat += dir.fileName().substring(dir.fileName().lastIndexOf('/') + 1, dir.fileName().length() - 2) + "\n"; //extract the exact filename without path of the theme info file
            }
            TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, "text/plain", stat.c_str());
    });

    server.on("/a1/stat", HTTP_GET, [](AsyncWebServerRequest *request) {
        char stat[72];
        JsonDocument doc;

        for(int i=0; i<=Light.getChCount(); i++) {
            doc["c"+ std::to_string(i)]=Light.getBriSingle(i);
        }
        serializeJson(doc, stat);
        TelnetPrint.println(stat);
        request->send_P(200, "application/json", stat);
    });

    server.on("/a1/cfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/cfg.json", "application/json");
    });

    server.on("/a1/di", HTTP_GET, [](AsyncWebServerRequest *request) {             // diy get            
        request->send(SPIFFS, "/diy.json", "application/json");
    });

    // webUI server
    server.serveStatic("/w/", SPIFFS, "/w/");


    // debug apis

    server.on("/scan/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        enable_net_scan = true;
        TelnetPrint.println("[WEB] Network scanner ENABLED");
        request->send_P(200, "text/plain", "Scanner enabled. Monitoring Telnet...");
    });

    server.on("/scan/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        enable_net_scan = false;
        TelnetPrint.println("[WEB] Network scanner DISABLED");
        request->send_P(200, "text/plain", "Scanner disabled.");
    });

    server.on("/ls", HTTP_GET, [](AsyncWebServerRequest *request) {
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
            TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, "text/plain", "ok");
    });
    server.on("/rmaui", HTTP_GET, [](AsyncWebServerRequest *request) {
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            TelnetPrint.print("Removed: ");
            TelnetPrint.println(dir.fileName());
            SPIFFS.remove(dir.fileName());
        }
        request->send_P(200, "text/plain", "Removed all themes");
    });

    // server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     request->send_P(200, "text/plain", msg);
    // });

    server.on("/rbt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", success_html);
        delay(500);
        sysreboot();
    });

    server.on("/script", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(request->hasArg("p") && !scriptBegin(request->arg("p"))) {
            request->send_P(200, "text/plain", "ok");
        } else {
            if(scriptEnd());
            request->send_P(200, "text/plain", "script end");
        }
        request->send_P(200, "text/plain", "ok");
    });



    server.on("/irs", HTTP_GET, [](AsyncWebServerRequest *request) {
        unsigned long t_timeout = millis();
        while(millis() - t_timeout <= 10000) {
            if (irrecv.decode(&results)) {
                if (results.value != 0xFFFFFFFF) {
                    request->send(200, "text/plain", String(results.value, HEX));  //to finish
                }
                irrecv.resume();
            }
        }
        request->send_P(500, "text/plain", "timeout");
    });
    server.on("/irm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/map.ir", "text/plain");
    });
    server.on("/bts", HTTP_GET, [](AsyncWebServerRequest *request) {
        TelnetPrint.print("/bts\nADC: ");
        TelnetPrint.println(analogRead(adc));
        TelnetPrint.flush();
        int btn_idle_val = analogRead(adc)*100;
        
        unsigned long t_timeout = millis();
        while(millis() - t_timeout <= 10000) {
            if (analogRead(adc)*100 != btn_idle_val) {
                request->send(200, "text/plain", String(analogRead(adc)*100));  //
            }
        }
        request->send_P(500, "text/plain", "timeout");
    });
    server.on("/btm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/map.btn", "text/plain");
    });

    Serial.println("[SETUP] Starting webserver (begin)");
    TelnetPrint.println("[SETUP] Starting webserver (begin)");
    server.onNotFound(notFound);
    server.begin();
}

void setup() {
    Serial.begin(115200);

    Serial.setTimeout(10000);
    Serial.println("[SYS] Starting...\n[SYS] Send 'd' in less then 3 sec to boot in UART download mode or 'w' to wipe SPIFFS partition.");
    // TelnetPrint.println("[SYS] Starting...\n[SYS] Send 'd' in less then 3 sec to boot in UART download mode or 'w' to wipe SPIFFS partition.");
    delay(500);
    while (Serial.available() > 0) {
        switch (Serial.readStringUntil('\n').charAt(0)) {
        case 'd':
            sysreboot(1);
            break;
        case 'w':
            Serial.println("[SYS] Wiping SPIFFS partition");
            TelnetPrint.println("[SYS] Wiping SPIFFS partition");
            SPIFFS.format();
            sysreboot(0);
            break;
        default:
            break;
        }
    }
    if (!SPIFFS.begin()) {
        Serial.println("[SPIFFS] Mount failed. Formatting filesystem in 5 seconds...");
        Serial.println("[SPIFFS] Unplug power NOW to abort!");
        delay(5100);
        Serial.println("[SPIFFS] Formatting...");
        SPIFFS.format();
        sysreboot(0);
    } else {

        wlconf2();

        Udp.begin(localUdpPort);
        TelnetPrint.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

        if (!SPIFFS.exists("/cfg.json")) {
            TelnetPrint.begin();
            Serial.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            TelnetPrint.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            dnsServer.start(53, "*", WiFi.softAPIP());
            startsrv();
            setup_ok = 0;
            while (setup_ok != 1) {
                dnsServer.processNextRequest();
                delay(1000);
            }
            Serial.println("[SETUP] Rebooting to apply new config.");
            TelnetPrint.println("[SETUP] Rebooting to apply new config.");
            sysreboot(0);
        } else {
            File jsnld = SPIFFS.open("/cfg.json", "r");
            TelnetPrint.println("open cfg.json");
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsnld);
            if (error) {
                Serial.print(F("[JSON] deserializeJson() failed: "));
                TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
                Serial.println(error.f_str());
                TelnetPrint.println(error.f_str());
                Serial.println(F("[SPIFFS] Moving broken file to: /cfg.json.broken"));
                TelnetPrint.println(F("[SPIFFS] Moving broken file to: /cfg.json.broken"));
                if (SPIFFS.exists("/cfg.json.broken")) {
                    SPIFFS.remove("/cfg.json.broken");
                }
                SPIFFS.rename("/cfg.json", "/cfg.json.broken");
                sysreboot(0);
                return;
            }
            if (doc["hw"]["p"] != -1) {
                atx = doc["hw"]["p"];
                pinMode(atx, OUTPUT);
            }
            if (doc["hw"]["adc"] != -1) {
                adc = A0;
                pinMode(adc, INPUT);
            }
            if (doc["hw"]["status"] != -1) {
                statusled = doc["hw"]["status"];
                pinMode(statusled, OUTPUT);
            }
            Light.addCh(0, -1, 255);
            for (int i=1; i<6; i++) {
                String hwKey = "c" + String(i);
                String swKey = "b" + String(i);
                Light.addCh(i, doc["hw"][hwKey], doc["sw"][swKey]);
            }
            // if (doc["hw"]["c1"] != -1) {
            //     chnr++;
            //     chout1 = doc["hw"]["c1"];
            //     pinMode(chout1, OUTPUT);
            //     digitalWrite(chout1, LOW);
            // }
            // if (doc["hw"]["c2"] != -1) {
            //     chnr++;
            //     chout2 = doc["hw"]["c2"];
            //     pinMode(chout2, OUTPUT);
            //     digitalWrite(chout2, LOW);
            // }
            // if (doc["hw"]["c3"] != -1) {
            //     chnr++;
            //     chout3 = doc["hw"]["c3"];
            //     pinMode(chout3, OUTPUT);
            //     digitalWrite(chout3, LOW);
            // }
            // if (doc["hw"]["c4"] != -1) {
            //     chnr++;
            //     chout4 = doc["hw"]["c4"];
            //     pinMode(chout4, OUTPUT);
            //     digitalWrite(chout4, LOW);
            // }
            // if (doc["hw"]["c5"] != -1) {
            //     chnr++;
            //     chout5 = doc["hw"]["c5"];
            //     pinMode(chout5, OUTPUT);
            //     digitalWrite(chout5, LOW);
            // }
            TelnetPrint.begin();
            Serial.println("[SYS] Checking factory reset flag...");
            TelnetPrint.println("[SYS] Checking factory reset flag...");
            if (doc["meta"]["fact"] == "1") {
                Serial.println("[SYS] Factory reset flag found! Resetting...");
                TelnetPrint.println("[SYS] Factory reset flag found! Resetting...");
                SPIFFS.format();
                sysreboot(0);
                // stpwzd();
                // doc["meta"]["fact"] = 0; // factory mode off
                // fact = 0;
            }
            // analogWriteRange(doc["sw"]["anw"]);
            anw = doc["sw"]["anw"];
            diynr = doc["sw"]["dnr"];
            // Light.setCalibSingle(0, doc["sw"]["b0"]);
            // Light.setCalibSingle(1, doc["sw"]["b1"]);
            // Light.setCalibSingle(2, doc["sw"]["b2"]);
            // Light.setCalibSingle(3, doc["sw"]["b3"]);
            // Light.setCalibSingle(4, doc["sw"]["b4"]);
            // Light.setCalibSingle(5, doc["sw"]["b5"]);
            savetim = doc["sw"]["rbt"];
            fadetick = doc["sw"]["tik"];
            webui = doc["web"] | "dev";  //.as<String>();

            // const int chnr = doc["sw"]["chnr"];

            // dbg_cfgver=doc["meta"]["cfgver"];
            jsnld.close();
        }
        if (SPIFFS.exists("/web.json")) {
            File jsnweb = SPIFFS.open("/web.json", "r");
            JsonDocument doc;
            //            DeserializationError error = deserializeJson(doc, jsnweb);
            //            webui = doc["web"].as<String>();
            jsnweb.close();
        }
        startsrv();
    }

    Serial.println("[SYS] Enabling IR receiver");
    TelnetPrint.println("[SYS] Enabling IR receiver");
    irrecv.enableIRIn();
    Serial.println("[SYS] Setting up OTA");
    TelnetPrint.println("[SYS] Setting up OTA");

    ArduinoOTA.onStart([]() {  // arduino ota example sketch
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_FS
            type = "filesystem";
        }
        SPIFFS.end();
        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("[OTA] Start updating " + type);
        TelnetPrint.println("[OTA] Start updating " + type);
    });
    ArduinoOTA.begin();

    if(File last = SPIFFS.open("/last", "r")){  // restore brightness and script state
        TelnetPrint.println("open last");
        for (int i = 0; i <= Light.getChCount(); i++) {
            persistbr[i] = last.parseInt();
            Light.setBriSingle(i, persistbr[i]);
        }
        last.close();
    }

    if(File f=SPIFFS.open("/lscript","r")) {
        scriptBegin(f.readStringUntil('\n'));
        f.close();
    }

    //debug: populate /lscript to test functionality

    // if(File f=SPIFFS.open("/lscript","w")){
    //     f.write("/w/effect1.lsc");
    //     f.close();
    // }


    Serial.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    TelnetPrint.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    Serial.println(ESP.getVcc());
    TelnetPrint.println(ESP.getVcc());
    runner.startNow(); // Initializes the scheduler
    Serial.println("[SYS] TaskScheduler Started");
    TelnetPrint.print("[SYS] TaskScheduler Started");

}

void diyedit(int num) {  // saves the current brightness values in the specified slot then calls diyload() to load and apply them.
    File jsndiy = SPIFFS.open("/diy.json", "r");
    TelnetPrint.println("open diy.json");
    JsonDocument doc;  // JSON document with twice the size of the file + the size of new entries
    if (jsndiy) {
        TelnetPrint.println("diyedit spiffs size:");
        // TelnetPrint.println(jsndiy.size());
        DeserializationError error = deserializeJson(doc, jsndiy);
        if (error) {
            Serial.print(F("[JSON] deserializeJson() failed: "));
            TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
            Serial.println(error.f_str());
            TelnetPrint.println(error.f_str());
            // jsndiy.close();
            // SPIFFS.remove("/diy.json");
            // File jsndiy = SPIFFS.open("/diy.json", "w");
            // return;
        }
    }
    jsndiy.close();
    File jsndiyw = SPIFFS.open("/diy.json", "w");
    TelnetPrint.println("open diy.json WRITE");
    if (jsndiyw) {
        TelnetPrint.println("diyedit json");
        TelnetPrint.println(num);
        TelnetPrint.println(diynr);
        if (num > 0 && num <= diynr) {
            TelnetPrint.println("diyedit");
            auto diy = "d" + std::to_string(num);
            TelnetPrint.println(diy.c_str());
            // std::string diy = "d" + std::to_string(num);
            // doc[diy]["all"] = lastbr[0];  ///////////////////////////////////////////////////////////////////
            for (int i = 0; i <= Light.getChCount(); i++) {
                doc[diy]["c" + std::to_string(i)] = lastbr[i];  //////////////////////////////////////////////////////////
            }
        }
        serializeJson(doc, jsndiyw);
        jsndiy.close();
    } else {
        TelnetPrint.println("JSNDIYW error");
    }
    lastir = 0;
    irhold = 0;
    diyload(num);
}

void diyload(int num) {  // loads and applies brightness values stored in the specified "slot" inside a file
    File jsndiy = SPIFFS.open("/diy.json", "r");
    // TelnetPrint.println("open diy.json");
    // TelnetPrint.println("diyload spiffs size:");
    // TelnetPrint.println(jsndiy.size()*2+DIYTSIZE);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsndiy);
    if (error) {
        Serial.print(F("[JSON] deserializeJson() failed: "));
        TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
        Serial.println(error.f_str());
        TelnetPrint.println(error.f_str());
        // TelnetPrint.println(F("removed"));
        jsndiy.close();
        // SPIFFS.remove("/diy.json");
        return;
    }
    TelnetPrint.println("diyload json");
    TelnetPrint.println(jsndiy.size());
    TelnetPrint.println(irhold);
    if (irhold == 0 && num > 0 && num <= diynr) {
        // lastbr[0] = currentbr[0];
        auto diy = "d" + std::to_string(num);
        TelnetPrint.println(diy.c_str());
        TelnetPrint.println("diyload");
        // diy += std::to_string(num);
        for (int i = 0; i <= Light.getChCount(); i++) {
            lastbr[i] = Light.getBriSingle(i);
            Serial.println(lastbr[i]);
            // TelnetPrint.println(lastbr[i]);
            Serial.println(i);
            // TelnetPrint.println(i);
            Light.setBriSingle(i, doc[diy]["c" + std::to_string(i)]);
            TelnetPrint.println(Light.getBriSingle(i));
        }
    }

    jsndiy.close();
    if (irhold == 1 && (millis() - irtime >= 2500)) {
        diyedit(num);
    }
}


void ftable_ex(int fval) {  // function that contains and executes all the functions assigned inside a function table
    // needs heavy modifications to work with current code
    TelnetPrint.println("FTable ex:");
    TelnetPrint.println(fval);
    // ir code switch, select channels, brightness
    switch (fval) {
    case 1: {
        sel = 1;
        break;
    }
    case 2: {
        sel = 2;
        break;
    }
    case 3: {
        sel = 3;
        break;
    }
    case 4: {
        sel = 4;
        break;
    }
    case 5: {
        sel = 0;
        break;
    }
    case 6: {
        sel++;
        if (sel >= Light.getChCount()) {
            sel = 0;
        }
    }
    case 7: {
    }
    case 8: {
    }
    case 9: {
    }
    // case 10: {  // br +
    //     Serial.print(" irbru \n");
    //     TelnetPrint.print(" irbru \n");
    //     targetbr[sel] = targetbr[sel] + 5;
    //     if (sel != 0 && targetbr[sel] >= 100) {
    //         targetbr[sel] = 100;
    //     } else if (targetbr[0] >= 255) {
    //         targetbr[0] = 255;
    //     }
    //     break;
    // }
    // case 11: {  // br -
    //     Serial.print(" irbrd \n");
    //     TelnetPrint.print(" irbrd \n");
    //     targetbr[sel] = targetbr[sel] - 5;
    //     if (targetbr[sel] <= 0) {
    //         targetbr[sel] = 0;
    //     }
    //     break;
    // }
    case 12: {  // irmax
        break;
    }
    case 13: {  // irmin
        Serial.print(" irmin \n");
        TelnetPrint.print(" irmin \n");
        break;
    }

    // case irpon: {
    //     Serial.print(" irpon \n");                           //old kept as reference
    //     TelnetPrint.print(" irpon \n");
    //     diyload(0);
    //     break;
    // }

    case 101 ... 199: {  // diy shortcuts
        diyload(fval % 100);
        if (irhold == 1 && millis() - irtime >= 2500) {
            diyedit(fval % 100);  // FINISH THIS !!!!!!!!!!!!!!!!
        }
        break;
    }
    }
}


void userInputWatcher() {

    while (Serial.available() > 0) {  // accept input from serial and run the assigned functions
        switch (Serial.readStringUntil('\n').charAt(0)) {
        // case 'r': {
        //     targetbr[1] = Serial.parseInt();  // this section needs heavy updating to work with the rest of the code
        //     break;
        // }
        // case 'g': {
        //     targetbr[2] = Serial.parseInt();
        //     break;
        // }
        // case 'b': {
        //     targetbr[3] = Serial.parseInt();
        //     break;
        // }
        // case 'w': {
        //     targetbr[4] = Serial.parseInt();
        //     break;
        // }
        // case 'a': {
        //     targetbr[0] = Serial.parseInt();
        //     break;
        // }
        // case 's': {
        //     for (int i = 0; i <= chnr; i++) {
        //         lastbr[i] = targetbr[i];
        //     }
        //     diyedit(Serial.parseInt());
        //     break;
        // }
        case 'n': {
            //          noti(255, 20, 3);
            //            notidiy();
            break;
        }
        case 'm': {
            //          noti(255, 20, 3);
            for (int i = -1; i <= 10; i++) {
                Serial.print("lastbr[");
                TelnetPrint.print("lastbr[");
                Serial.print(i);
                TelnetPrint.print(i);
                Serial.print("]= ");
                TelnetPrint.print("]= ");
                Serial.println(lastbr[i]);
                TelnetPrint.println(lastbr[i]);
            }
            break;
        }
        case '?': {
            // for (int i = 0; i <= winset; i++) {
            //    // Serial.print(currentset[i]);
            // TelnetPrint.print(currentset[i]);
            //    // Serial.print("\\");
            // TelnetPrint.print("\\");
            // }
            break;
        }
        case 'f': {

            break;
        }
        case '1': {
            diyload(1);
            break;
        }
        case '2': {
            diyload(2);
            break;
        }
        case '3': {
            diyload(3);
            break;
        }
        case '4': {
            diyload(4);
            break;
        }
        case '5': {
            diyload(5);
            break;
        }
        case '6': {
            diyload(6);
            break;
        }
        case '7': {
            diyload(7);
            break;
        }
        case '0': {
            diyload(0);
            break;
        }
        }
    }
    // ir interface
    if (irrecv.decode(&results)) {
        if (results.value != 0xFFFFFFFF) {
            TelnetPrint.println("IR received.");
            irtime = millis();
            irhold = 0;
            lastir = ir_lookup(results.value);
            ftable_ex(lastir);
        } else {
            irhold = 1;
            if (millis() - irtime >= 500) {
                ftable_ex(lastir);
            }
        }
        irrecv.resume();
    }

}


void loop() {
    // auto loop_start=millis();
    ArduinoOTA.handle();
    runner.execute();

}


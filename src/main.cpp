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
//#include <TelnetPrint.h>
#include "FS.h"
#include <untar.h>
#include <TaskScheduler.h>

#include <time.h>

//#include <WiFiUdp.h>

#include "light.h"
#include "webpages.h"
#include "ledScript.h"
#include "sNetFeat.h"

ADC_MODE(ADC_VCC);
#define VERSION "1.0.0_aPERF"
#define DEBUG_USE_TELNET 0

#define SPIFFS_CACHE = (1)
#define SPIFFS_CACHE_WR = (1)

#define DECODE_NEC
// #define DIYTSIZE 112
// #define DIYTSIZE 130
#define MAX_IR 10
#define MAX_BTN 10
#define BTOL 5
#define MAXCH 6

#define SYSCFG_F "/config/sys.json"
#define WLNCFG_F "/wlan.json"
#define SNFCFG_F "/config/snf.json"
#define WEBCFG_F "/config/web.json"
#define DIY_F "/diy.json"
#define SHADOW_F "/shadow_" + String(ESP.getFlashChipId()) + ".json"

#define AUTH_DEFAULTUSER "admin"
#define AUTH_DEFAULTPW "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918"   //supposedly "admin" https://emn178.github.io/online-tools/sha256.html
#define AUTH_DEFAULTPERM PERM_ADMIN

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

// int chout1 = -1;
// int chout2 = -1;
// int chout3 = -1;
// int chout4 = -1;
// int chout5 = -1;
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

unsigned long irtime;
unsigned long lastir;
unsigned long wltim;
unsigned long lastsave;
//unsigned long lastwlscan;
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
//int fadetick; //no longer used for now since we call the setter directly
int ntik;
//double nms = 1000;  //<<<<<<<<<<<<<<<<<------------------------
//int currentbr[6];
//int startbr[6];

//double speed[6], currentbr[6];
// float adc_val;
bool wlconf_started, setup_ok, irhold = 0, start_noti, brichanged, userauth;
time_t now;

//auth
// 16-bit permission mask (allows up to 16 distinct API groups)
enum ApiPermissions : uint16_t {
    PERM_NONE   = 0,
    PERM_UI     = 1 << 0, // 1  (basic user actions)
    PERM_SYS    = 1 << 1, // 2  (wipe, config read/write)
    PERM_INSTALL= 1 << 2, // 4  (app install permission)
    PERM_SNF    = 1 << 3, // 8  (read and configure SNF)
    PERM_DIY    = 1 << 4, // 16 (write preset config)
    PERM_ADDUSER= 1 << 5, // 32 (add and edit users)
    PERM_ADMIN  = 0xFFFF  // All bits set (65535)
};
struct AuthUser {
    String username;
    String passHash;
    uint16_t permissions;
};

struct ActiveSession {
    String token;
    unsigned long expires_at;
    uint16_t permissions;
};

std::vector<AuthUser> loaded_users;
std::vector<ActiveSession> active_sessions;
const unsigned long SESSION_TIMEOUT = 3600000; // 1 hour in ms

char* ntp_tz = nullptr;
char* ntp_url = nullptr;

//int spamvar;

//int lastSeenTarget[6]; // adjust size to match your channel count (chnr+1)

void diyedit(int num);
void diyload(int num);
void userInputWatcher();
void loadUsers();

void cron_1d(){
}
void cron_60m(){
}
void cron_30m(){
    snfDoARPscan();
}
void cron_10m(){
}
void cron_1m(){
}
// Task Definitions

Task tScriptR(10, TASK_FOREVER, &scriptRunner, &runner, false);
Task tUInputW(50, TASK_FOREVER, &userInputWatcher, &runner, true);
Task tFader(15, TASK_ONCE, &faderCallback, &runner, true);
//Task tNetScanner(50, TASK_FOREVER, &netScannerCallback, &runner, true);

Task tSNFMain(50, TASK_ONCE, &snfMain, &runner, true);

Task tcron1d(TASK_HOUR*24, TASK_FOREVER, &cron_1d, &runner, true);
Task tcron60m(TASK_HOUR, TASK_FOREVER, &cron_60m, &runner, true);
Task tcron30m(TASK_HOUR/2, TASK_FOREVER, &cron_30m, &runner, true);
Task tcron10m(TASK_HOUR/6, TASK_FOREVER, &cron_10m, &runner, true);
Task tcron1m(TASK_MINUTE, TASK_FOREVER, &cron_1m, &runner, true);


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

void loadUsers() {
    File file = SPIFFS.open(SHADOW_F, "r");
    if (!file) {
        //TelnetPrint.println("Failed to open shadow file");
        AuthUser u;
        u.username = AUTH_DEFAULTUSER;
        u.passHash = AUTH_DEFAULTPW;
        u.permissions = AUTH_DEFAULTPERM;
        loaded_users.push_back(u);
        //return;
    } else {

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        
        if (error) {
            //TelnetPrint.println("Failed to parse users.json");
            SPIFFS.rename(SHADOW_F,"/shadow.json.broken");
            AuthUser u;
            u.username = AUTH_DEFAULTUSER;
            u.passHash = AUTH_DEFAULTPW;
            u.permissions = AUTH_DEFAULTPERM;
            loaded_users.push_back(u);
            return;
        }

        loaded_users.clear();
        JsonArray users = doc["users"];
        for (JsonObject user : users) {
            AuthUser u;
            u.username = user["u"].as<String>();
            u.passHash = user["h"].as<String>();
            u.permissions = user["p"].as<uint16_t>();
            loaded_users.push_back(u);
        }
        file.close();
    }
    //TelnetPrint.printf("Loaded %d users into RAM\n", loaded_users.size());
}

void btn_loadmap() {
    //TelnetPrint.println("BTN LoadMap");
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
    //TelnetPrint.println("BTN Lookup");
    for (int i = 1; i <= MAX_BTN; i++) {  // Check if the button is in the btable array
        if (btn > ctrl.btn[i].button - BTOL && btn < ctrl.btn[i].button + BTOL) {
            return ctrl.btn[i].function;
        }
        if (ctrl.btn[i].button == -1) {
            return 0;
        }
        // If the button was not found in the btable array, search the file
        if (i == MAX_BTN && ctrl.btnmax > MAX_BTN) {
            //TelnetPrint.println("BTN Lookup file");

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
    //TelnetPrint.println("IR LoadMap");

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
    //TelnetPrint.println("IR Lookup");

    // Check if the button is in the btable array
    for (int i = 1; i <= MAX_IR; i++) {
        if (ctrl.irbtn[i].button == btn) {
            return ctrl.irbtn[i].function;
        }

        // If the button was not found in the btable array, search the file
        if (i == MAX_IR && ctrl.irbmax > MAX_IR) {
            File file = SPIFFS.open("/map.ir", "r");
            //TelnetPrint.println("IR Lookup file");

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
    request->send_P(404, m_json, nfound_fail_json);
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
        //TelnetPrint.println("open /t/");
        if (file) {
            tar.open((Stream *)&file);  // Pass source file as Stream to Tar object
            tar.dest("/w/");
            tar.extract();
            file.close();
            //TelnetPrint.println("untar ok");
            //TelnetPrint.println(filename);
            // SPIFFS.rmdir("/t/");
            SPIFFS.remove("/t/" + filename);
            //TelnetPrint.println("Removed tar");
            //TelnetPrint.println(filename);
            Dir dir = SPIFFS.openDir("/");
            while (dir.next()) {
                //TelnetPrint.print("In flash:");
                //TelnetPrint.println(dir.fileName());
            }
        }
        request->redirect("/thm");
    }
}

void sysreboot(int mode = 0) {
    switch (mode) {
    case 1:
        Serial.println("[SYS] Rebooting now! (UART download mode)");
        //TelnetPrint.println("[SYS] Rebooting now! (UART download mode)");
        //TelnetPrint.println("[SYS] Rebooting now! (UART download mode)");
        //TelnetPrint.flush();
        delay(200);
        ESP.rebootIntoUartDownloadMode();
        break;
    default:
        Serial.println("[SYS] Rebooting now! (software mode)");
        //TelnetPrint.println("[SYS] Rebooting now! (software mode)");
        SPIFFS.end();
        Serial.println("[SPIFFS] Unmounted.");
        //TelnetPrint.println("[SPIFFS] Unmounted.");
        //TelnetPrint.flush();
        ESP.restart();
        break;
    }
}


void wlconf2() {
    wlconf_started = true;
    WiFi.persistent(0);
    File jsnwlan = SPIFFS.open(WLNCFG_F, "r");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsnwlan);
    if (error) {
        Serial.print(F("[WLAN] JSON deserializeJson() failed: "));
        //TelnetPrint.print(F("[WLAN] JSON deserializeJson() failed: "));
        Serial.println(error.f_str());
        //TelnetPrint.println(error.f_str());
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

// auth helpers and functions
String getSHA256(String input) {
    br_sha256_context ctx;
    br_sha256_init(&ctx);
    br_sha256_update(&ctx, input.c_str(), input.length());
    
    byte hash[br_sha256_SIZE];
    br_sha256_out(&ctx, hash);
    
    char hex[65];
    for (int i = 0; i < br_sha256_SIZE; i++) {
        sprintf(&hex[i * 2], "%02x", hash[i]);
    }
    hex[64] = '\0';
    return String(hex);
}

struct PendingChallenge {
    String nonce;
    unsigned long expires_at;
};

std::vector<PendingChallenge> valid_challenges;

// Generate a random 16-character hex string
// https://web.archive.org/web/20160417080207/http://esp8266-re.foogod.com/wiki/Random_Number_Generator
String generateNonce() {
    char nonceBuf[33]; // 32 characters + null terminator (128-bit nonce)
    for (int i = 0; i < 4; i++) {
        sprintf(&nonceBuf[i * 8], "%08x", RANDOM_REG32);
    }
    return String(nonceBuf);
}

bool isAuthorized(AsyncWebServerRequest *request, uint16_t required_perm) {
    //TelnetPrint.println(userauth);
    if(!userauth){
        return true;
    }
    if (!request->hasHeader("Authorization")) {
        request->send(401, m_json, "{\"error\":\"Missing Token\"}");
        return false;
    }

    String authHeader = request->header("Authorization");
    if (!authHeader.startsWith("Bearer ")) {
        request->send(401, m_json, "{\"error\":\"Invalid Token Format\"}");
        return false;
    }

    String token = authHeader.substring(7);
    unsigned long current_time = millis();

    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it) {
        if (it->token == token) {
            if (current_time > it->expires_at) {
                active_sessions.erase(it);
                request->send(401, m_json, "{\"error\":\"Session Expired\"}");
                return false;
            }
            
            if ((it->permissions & required_perm) == required_perm) {
                it->expires_at = current_time + SESSION_TIMEOUT; 
                return true;
            } else {
                request->send(403, m_json, "{\"error\":\"Forbidden\"}");
                return false;
            }
        }
    }
    request->send(401, m_json, "{\"error\":\"Invalid Token\"}");
    return false;
}

void startsrv() {
    Serial.println("[SETUP] Stopping webserver (end)");
    //TelnetPrint.println("[SETUP] Stopping webserver (end)");
    server.end();

    //static builtins
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(webui=="dev") {
            request->send_P(200, m_html, index_html);
        } else {
            request->redirect("/w/" + webui + "/i.htm");
        }
    });

    server.on("/a.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/css",  a_css);
    });

    server.on("/wlcfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, wlan_html);
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, reset_html);
    });

    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, setup_html);
    });
    
    //transitional pages for new api
    server.on("/setup2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, setup2_html);
    });
    server.on("/a1/gettzdata", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/csv", tzdata_csv);
    });
    server.on("/wlcfg2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, wlan2_html);
    });
    server.on("/snf",   HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, snf_html);
    });

    server.on("/up",    HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, theme_html);
    });

    server.on("/pick",  HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, picker_html);
    });

    server.on("/pick2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, picker2_html);
    });

    server.on("/diym",  HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, diymanager_html);
    });

    server.on("/thm",   HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, theme_html);
    });


    // apis and interactive stuff:

    // set/write apis

    server.on("/a1/wipe", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        if(request->hasArg("meta_fact")) {
            File jsnw = SPIFFS.open(SYSCFG_F, "w");
            //TelnetPrint.println("open cfg.json WRITE");
            JsonDocument doc;
            doc["meta"]["fact"]=request->arg("meta_fact");
            serializeJson(doc, jsnw);
            jsnw.close();
            if(request->arg("meta_fact").toInt()==1) {
                request->send_P(200, m_html, success_html);
                delay(1000);
                sysreboot(0);
            }
        } else {
            request->send_P(500, m_html, error_html);

        }
    });

    server.on("/a1/wlset", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        if (request->hasArg("wlm") && request->hasArg("ssid") && request->hasArg("psk") && request->hasArg("apssid") && request->hasArg("appsk") /* && request->hasArg("t")*/ ) {
            File jsnw = SPIFFS.open(WLNCFG_F, "w");
            //TelnetPrint.println("open wlan.json WRITE");
            JsonDocument doc; //320
            doc["wlm"]=request->arg("wlm");
            doc["ssid"]=request->arg("ssid");
            doc["psk"]=request->arg("psk");
            doc["apssid"]=request->arg("apssid");
            doc["appsk"]=request->arg("appsk");
            //doc["t"]=request->arg("t");
            serializeJson(doc, jsnw);
            jsnw.close();
            wlconf2();
            request->send_P(200, m_html, success_html);
        } else {
            request->send_P(500, m_html, error_html);
        }
    });

    // Overwrite the entire file with validated, minified JSON
    server.on("/a1/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        // Acknowledge the request (handled in body callback)
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Accumulate chunks into a temporary string
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        body->concat((const char*)data, len);

        // When the entire body is received
        if (index + len == total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, *body);
            delete body;
            request->_tempObject = NULL;

            if (error) {
                request->send(400, m_json, "{\"status\":\"error\",\"message\":\"Invalid JSON payload\"}");
                return;
            }

            File file = SPIFFS.open(SYSCFG_F, "w");
            if (file) {
                serializeJson(doc, file); // Automatically writes minified JSON
                file.close();
                request->send(200, m_json, "{\"status\":\"success\",\"message\":\"Config overwritten\"}");
            } else {
                request->send(500, m_json, "{\"status\":\"error\",\"message\":\"FS write failed\"}");
            }
        }
    });

    server.on("/a1/config", HTTP_PUT, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        // Acknowledge the request
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            request->_tempObject = new String();
        }
        String* body = (String*)request->_tempObject;
        body->concat((const char*)data, len);

        if (index + len == total) {
            JsonDocument newDoc;
            DeserializationError error = deserializeJson(newDoc, *body);
            delete body;
            request->_tempObject = NULL;

            if (error) {
                request->send(400, m_json, "{\"status\":\"error\",\"message\":\"Invalid JSON payload\"}");
                return;
            }

            // Read existing configuration
            File fileRead = SPIFFS.open(SYSCFG_F, "r");
            JsonDocument currentDoc;
            if (fileRead) {
                deserializeJson(currentDoc, fileRead);
                fileRead.close();
            }

            // --- THE NON-RECURSIVE UPDATE LOGIC ---
            JsonObject rootNew = newDoc.as<JsonObject>();
            JsonObject rootCur = currentDoc.as<JsonObject>();

            for (JsonPair kv : rootNew) {
                const char* topKey = kv.key().c_str();

                // If the incoming value is an object (e.g., "hw" or "sw")
                if (kv.value().is<JsonObject>()) {
                    // Create the object in the current config if it doesn't exist
                    if (!rootCur.containsKey(topKey)) {
                        rootCur.createNestedObject(topKey);
                    }
                    
                    JsonObject targetObj = rootCur[topKey];
                    JsonObject srcObj = kv.value().as<JsonObject>();

                    // Iterate exactly one level deep and apply/append keys
                    for (JsonPair innerKv : srcObj) {
                        targetObj[innerKv.key()] = innerKv.value();
                    }
                } else {
                    // If it's a primitive (like "web") or an array (like "c")
                    // simply overwrite or append it at the root level
                    rootCur[topKey] = kv.value();
                }
            }

            // Write the updated document back to disk
            File fileWrite = SPIFFS.open(SYSCFG_F, "w");
            if (fileWrite) {
                serializeJson(currentDoc, fileWrite);
                fileWrite.close();
                request->send(200, m_json, "{\"status\":\"success\",\"message\":\"Config updated\"}");
            } else {
                request->send(500, m_json, "{\"status\":\"error\",\"message\":\"FS write failed\"}");
            }
        }
    });

    server.on("/a1/setui", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        if(request->hasArg("web")) {
            File jsnweb = SPIFFS.open(WEBCFG_F, "w");
            //TelnetPrint.println("open web.json WRITE");
            JsonDocument doc;
            Dir dir = SPIFFS.openDir("/w/");

            while (dir.next()) {
                if(dir.fileName().endsWith(request->arg("web")+".i")) {
                    File tinfo=SPIFFS.open(dir.fileName(), "r");
                    doc["web"] = tinfo.readString();
                    tinfo.close();
                    //TelnetPrint.printf(doc["web"]);
                }
            }
            serializeJson(doc, jsnweb);
            jsnweb.close();
        }
        request->send_P(200, m_html, success_html);
    });

    server.on("/a1/fup", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_INSTALL)) return;
        request->send(200);
    }, handleUpload);

    server.on("/a1/shade", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_UI)) return;
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

        request->send_P(200, m_html, success_html);
    });

    server.on("/a1/dic", HTTP_GET, [](AsyncWebServerRequest *request) {             // diy clear
        if (!isAuthorized(request, PERM_DIY)) return;
        SPIFFS.remove(DIY_F);
        request->redirect("/diym");
    });

    server.on("/a1/diyl", HTTP_POST, [](AsyncWebServerRequest *request) {           // diy load
        if (!isAuthorized(request, PERM_UI)) return;
        //TelnetPrint.println("[WEB] /diy");
        raw=0;
        if(request->hasArg("dl")) {
            //TelnetPrint.println("[WEB] /diy found dl");
            //TelnetPrint.println(request->arg("dl").toInt());
            diyload(request->arg("dl").toInt());
        } else if(request->hasArg("de")) {
            //TelnetPrint.println("[WEB] /diy found de");
            for(int i=0; i<=Light.getChCount(); i++) {
                lastbr[i]=Light.getBriSingle(i);
                //TelnetPrint.println(lastbr[i]);
            }
            //TelnetPrint.println(request->arg("de").toInt());
            diyedit(request->arg("de").toInt());
        }
        request->send_P(200, m_html, success_html);
    });

    // get/read apis

    server.on("/a1/wlist", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        int s=WiFi.scanComplete();

        switch (s) {
        case WIFI_SCAN_RUNNING: {
            request->send(202, m_json , wlscan_json);
            break;
        }
        case WIFI_SCAN_FAILED: {
            WiFi.scanNetworks(true, true);
            request->send(202, m_json , wlscan_json);
            break;
        }
        default: {   //scan ended
            JsonDocument doc; 
            JsonArray networks = doc.to<JsonArray>(); // Explicitly define the root as an array

            for (int i = 0; i < s; i++) {
                JsonObject wifi = networks.add<JsonObject>();
                wifi["n"] = WiFi.SSID(i);
                wifi["e"] = WiFi.encryptionType(i);
                wifi["p"] = WiFi.RSSI(i);
            }

            String response;
            serializeJson(doc, response);
            WiFi.scanDelete();

            // Send the JSON payload
            request->send(200, m_json, response);
            break;
        }
            
        }

        // async webserver is in a hurry and crashes if it has to wait for a blocking function to return
        // so we need to do this and call the api 2 times in the webui
        // if(millis() - lastwlscan > 8000) {
        //     WiFi.scanNetworks(true, true);
        //     lastwlscan = millis();
        //     request->send_P(429, m_txt, "Scan still in progress, try again in a few seconds");
        // } else {
        //     JsonDocument doc;
        //     String stat;
        //     // Add the scanned networks to the JSON document
        //     for (int i = 0; i < WiFi.scanComplete(); i++) {
        //         JsonObject wifi = doc.createNestedObject();
        //         wifi["n"] = WiFi.SSID(i);
        //         wifi["e"] = WiFi.encryptionType(i);
        //         wifi["p"] = WiFi.RSSI(i);

        //     }
        //     serializeJson(doc, stat);
        //     WiFi.scanDelete();
        //     request->send_P(200, F(m_json), stat.c_str());
        // }
    });

    server.on("/a1/lsui", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            // stat += concat(dir.fileName().indexOf('/', 1);
            // stat += filename.substring(0,)
            if(dir.fileName().endsWith(".i")) {
                stat += dir.fileName().substring(dir.fileName().lastIndexOf('/') + 1, dir.fileName().length() - 2) + "\n"; //extract the exact filename without path of the theme info file
            }
            //TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, m_txt, stat.c_str());
    });
    server.on("/a1/lsscript", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_UI)) return;
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            // stat += concat(dir.fileName().indexOf('/', 1);
            // stat += filename.substring(0,)
            if(dir.fileName().endsWith(".lsc")) {
                stat += dir.fileName()+ "\n";
            }
            //TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, m_txt, stat.c_str());
    });

    server.on("/a1/stat", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_UI)) return;
        char stat[72];
        JsonDocument doc;

        for(int i=0; i<=Light.getChCount(); i++) {
            doc["c"+ std::to_string(i)]=Light.getBriSingle(i);
        }
        serializeJson(doc, stat);
        //TelnetPrint.println(stat);
        request->send_P(200, m_json, stat);
    });

    server.on("/a1/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SYS)) return;
        request->send(SPIFFS, SYSCFG_F, m_json);
    });

    server.on("/a1/di", HTTP_GET, [](AsyncWebServerRequest *request) {             // diy get
        if (!isAuthorized(request, PERM_UI)) return;
        request->send(SPIFFS, DIY_F, m_json);
    });

    // webUI server
    server.serveStatic("/w/", SPIFFS, "/w/");


    // debug apis

       server.on("/a1/getnonce", HTTP_GET, [](AsyncWebServerRequest *request) {
        String nonce = generateNonce();
        valid_challenges.push_back({nonce, millis() + 30000}); 
        request->send(200, m_json, "{\"n\":\"" + nonce + "\"}");
    });
server.on("/a1/adduser", HTTP_POST, [](AsyncWebServerRequest *request) {
        
        // 1. Authorization: Only system admins/managers should create users
        if (!isAuthorized(request, PERM_ADDUSER)) return request->send_P(401, m_json, nauth_fail_json);

        // 2. Validate Inputs: Rigidly require exactly these 3 parameters
        if (!request->hasArg("u") || !request->hasArg("p") || !request->hasArg("h")) {
            return request->send(400, m_json, "{\"error\":\"Invalid argument.\"}");
        }

        String new_u = request->arg("u");
        String new_p = request->arg("h");
        uint16_t new_lvl = request->arg("p").toInt();

        // 3. Prevent Duplicate Usernames (Check RAM vector)
        for (const auto& u : loaded_users) {
            if (u.username == new_u) {
                return request->send(409, m_json, "{\"error\":\"User already exists\"}"); // 409 Conflict
            }
        }

        // 4. Read the existing shadow JSON file from flash
        File shadowFile = SPIFFS.open(SHADOW_F, "r");
        JsonDocument doc;
        
        if (shadowFile) {
            DeserializationError error = deserializeJson(doc, shadowFile);
            shadowFile.close();
            if (error) {
                return request->send(500, m_json, "{\"error\":\"Failed to parse shadow file\"}");
            }
        } 
        
        // Ensure the "users" array exists in the document
        JsonArray usersArray = doc["users"];
        if (!usersArray) {
            usersArray = doc.createNestedArray("users");
        }

        // 5. Append the new user to the JSON document
        JsonObject newUserObj = usersArray.createNestedObject();
        newUserObj["u"] = new_u;
        newUserObj["h"] = new_p;
        newUserObj["p"] = new_lvl;

        // 6. Write the updated JSON back to flash
        File shadowWrite = SPIFFS.open(SHADOW_F, "w");
        if (!shadowWrite) {
            return request->send(500, m_json, "{\"error\":\"Failed to write shadow file\"}");
        }
        serializeJson(doc, shadowWrite);
        shadowWrite.close();

        // 7. Update RAM so the new user can log in immediately
        loaded_users.push_back({new_u, new_p, new_lvl});

        // 8. Success Response
        request->send(200, m_json, "{\"status\":\"success\",\"message\":\"User created\"}");
    });
server.on("/a1/getusers", HTTP_GET, [](AsyncWebServerRequest *request) {
        
        // 1. Authorization check
        if (!isAuthorized(request, PERM_ADDUSER)) return request->send_P(401, m_json, nauth_fail_json);

        // 2. Build the sanitized JSON response
        JsonDocument doc;
        JsonArray usersArray = doc.createNestedArray("users");

        for (const auto& u : loaded_users) {
            JsonObject userObj = usersArray.createNestedObject();
            userObj["u"] = u.username;
            userObj["p"] = u.permissions;
            
            // Send true if a password exists, false if it's a passwordless account ("" or "none")
            if (u.passHash == "") {
                userObj["h"] = false;
            } else {
                userObj["h"] = true;
            }
        }

        String response;
        serializeJson(doc, response);
        request->send(200, m_json, response);
    });
    server.on("/a1/edituser", HTTP_POST, [](AsyncWebServerRequest *request) {
        
        // 1. Authorization check
        if (!isAuthorized(request, PERM_ADDUSER)) return request->send_P(401, m_json, nauth_fail_json);

        // 2. Validate Inputs
        if (!request->hasArg("u")) return request->send(400, m_json, "{\"error\":\"Missing username\"}");
        
        String target_u = request->arg("u");
        bool update_p = request->hasArg("h");
        bool update_lvl = request->hasArg("p");

        if (!update_p && !update_lvl) {
            return request->send(400, m_json, "{\"error\":\"Nothing to update\"}");
        }

        // 3. Find and update the user in RAM
        AuthUser* found_user = nullptr;
        for (auto& u : loaded_users) {
            if (u.username == target_u) {
                found_user = &u;
                break;
            }
        }
        
        if (!found_user) return request->send(404, m_json, "{\"error\":\"User not found\"}");

        // Apply changes to RAM
        if (update_p) found_user->passHash = request->arg("h");
        if (update_lvl) found_user->permissions = request->arg("p").toInt();

        // 4. Read the shadow JSON file from flash
        File shadowFile = SPIFFS.open(SHADOW_F, "r");
        JsonDocument doc;
        
        if (shadowFile) {
            DeserializationError error = deserializeJson(doc, shadowFile);
            shadowFile.close();
            if (error) return request->send(500, m_json, "{\"error\":\"Failed to parse shadow file\"}");
        } else {
            return request->send(500, m_json, "{\"error\":\"Shadow file not found\"}");
        }

        // 5. Find and update the user in the JSON document
        JsonArray usersArray = doc["users"];
        for (JsonObject user : usersArray) {
            if (user["u"] == target_u) {
                if (update_p) user["h"] = request->arg("h");
                if (update_lvl) user["p"] = request->arg("p").toInt();
                break; // Found and updated, exit loop
            }
        }

        // 6. Write the updated JSON back to flash
        File shadowWrite = SPIFFS.open(SHADOW_F, "w");
        if (!shadowWrite) return request->send(500, m_json, "{\"error\":\"Failed to write shadow file\"}");
        
        serializeJson(doc, shadowWrite);
        shadowWrite.close();

        // 7. Success Response
        request->send(200, m_json, "{\"status\":\"success\",\"message\":\"User updated\"}");
    });

server.on("/a1/login", HTTP_POST, [](AsyncWebServerRequest *request) {
        // 1. We always need at least a username
        if (!request->hasArg("u")) return request->send(400, m_txt, "Missing username");
        
        String req_u = request->arg("u");
        AuthUser* found_user = nullptr;
        
        // Find the user in RAM
        for (auto& u : loaded_users) {
            if (u.username == req_u) {
                found_user = &u;
                break;
            }
        }

        // Bail immediately if the username doesn't exist
        if (!found_user) return request->send(401, m_json, "{\"error\":\"Invalid credentials\"}");

        // --- PASSWORDLESS BYPASS ---
        // If the backend has no password configured for this user, issue the token immediately.
        if (found_user->passHash == "") {
            String sessionToken = generateNonce() + generateNonce();
            active_sessions.push_back({sessionToken, millis() + SESSION_TIMEOUT, found_user->permissions});
            return request->send(200, m_json, "{\"token\":\"" + sessionToken + "\"}");
        }

        // --- STANDARD CHALLENGE-RESPONSE ---
        // For standard users, if the frontend didn't send the crypto parameters, reject them.
        if (!request->hasArg("h") || !request->hasArg("n")) {
             return request->send(400, m_txt, "Missing authentication parameters");
        }

        String req_hash = request->arg("h");
        String req_nonce = request->arg("n");

        bool valid_nonce = false;
        unsigned long current_time = millis();
        
        for (auto it = valid_challenges.begin(); it != valid_challenges.end(); ) {
            if (current_time > it->expires_at) {
                it = valid_challenges.erase(it);
            } else if (it->nonce == req_nonce) {
                valid_nonce = true;
                it = valid_challenges.erase(it); 
                break;
            } else {
                ++it;
            }
        }

        if (!valid_nonce) return request->send(401, m_json, "{\"error\":\"Invalid challenge\"}");

        String expected_hash = getSHA256(found_user->passHash + req_nonce);
        
        if (req_hash == expected_hash) {
            String sessionToken = generateNonce() + generateNonce();
            active_sessions.push_back({sessionToken, millis() + SESSION_TIMEOUT, found_user->permissions});
            return request->send(200, m_json, "{\"token\":\"" + sessionToken + "\"}");
        }

        // If math fails, reject
        request->send(401, m_json, "{\"error\":\"Invalid credentials\"}");
    });

    // GET /a1/snfstat -> Returns the live trigger state of tracked devices
    server.on("/a1/snfstat", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SNF)) return;
        JsonDocument doc;
        JsonArray devs = doc.createNestedArray("tracked");
        
        unsigned long current_time = millis();

        for (const auto& d : tracked_devices) {
            JsonObject obj = devs.createNestedObject();
            
            // Return identifiers so the UI knows which device this is
            if (d.user_mac()) {
                char macStr[18];
                snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
                         d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
                obj["m"] = macStr;
            }
            if (d.ip != 0) {
                obj["i"] = IPAddress(d.ip).toString();
            }
            if (d.hostname.length() > 0) obj["h"] = d.hostname;
            
            // Return the live state calculated by our new ICMP/ARP logic
            obj["present"] = d.is_present; // true = Found, false = Lost
            
            // Return how many seconds ago it was last verified alive
            if (d.last_seen > 0) {
                obj["sec_ago"] = (current_time - d.last_seen) / 1000;
            } else {
                obj["sec_ago"] = -1; // Never seen
            }
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, m_json, response);
    });

    server.on("/a1/getsnf", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SNF)) return;
        if (SPIFFS.exists(SNFCFG_F)) {
            request->send(SPIFFS, SNFCFG_F, m_json);
        } else {
            // Return an empty JSON array if no config exists yet
            request->send(200, m_json, "[]"); 
        }
    });

    // 2. GET /a1/getsnfd -> Live Discovery Stream
    server.on("/a1/getsnfd", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SNF)) return;
        
        // Reset the 30-second RAM allocation window
        snfActivateDiscoveryWindow(); 

        // Trigger a fresh sweep if the UI asks for it
        if (request->hasArg("scan") && !is_scanning) {
            discovered_devices.clear(); 
            snfDoARPscan();
        }

        // Build the current network state JSON
        JsonDocument doc;
        doc["scan"] = is_scanning; 
        JsonArray devs = doc.createNestedArray("devices");
        
        for (const auto& d : discovered_devices) {
            JsonObject obj = devs.createNestedObject();
            obj["i"] = IPAddress(d.ip).toString();
            
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
                     d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
            obj["m"] = macStr;
            obj["h"] = d.hostname;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, m_json, response);
    });

    // 3. POST /a1/setsnf -> Save new JSON and immediately apply it
    server.on("/a1/setsnf", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_SNF)) return;

        // Standard handler wrapper (handled by the body callback below)
        request->send(200, m_txt, "OK");
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Body handler: Streams the incoming JSON directly to SPIFFS
        if (index == 0) {
            request->_tempFile = SPIFFS.open(SNFCFG_F, "w");
        }
        
        if (request->_tempFile) {
            request->_tempFile.write(data, len);
        }
        
        if (index + len == total) {
            if (request->_tempFile) {
                request->_tempFile.close();
            }
            //TelnetPrint.println("[WEB] snf.json updated. Reloading...");
            snfLoadConf(SNFCFG_F);
        }
    });

    server.on("/scan/on", HTTP_GET, [](AsyncWebServerRequest *request) {
       // enable_net_scan = true;
        tSNFMain.enable();
        tSNFMain.setIterations(TASK_FOREVER);
        snfLoadConf(SNFCFG_F);
        //TelnetPrint.println("[WEB] Network scanner ENABLED");
        request->send_P(200, m_txt, "Scanner enabled. Monitoring Telnet...");
    });
    
    server.on("/scan/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        tSNFMain.disable();
       // enable_net_scan = false;
        //TelnetPrint.println("[WEB] Network scanner DISABLED");
        request->send_P(200, m_txt, "Scanner disabled.");
    });

    server.on("/ls", HTTP_GET, [](AsyncWebServerRequest *request) {
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/");
        while (dir.next()) {
            //TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, m_txt, "ok");
    });
    server.on("/rmaui", HTTP_GET, [](AsyncWebServerRequest *request) {
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            //TelnetPrint.print("Removed: ");
            //TelnetPrint.println(dir.fileName());
            SPIFFS.remove(dir.fileName());
        }
        //SPIFFS.remove(SHADOW_F);
        request->send_P(200, m_txt, "Removed all themes");
    });

    // server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     request->send_P(200, m_txt, msg);
    // });

    server.on("/rbt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, m_html, success_html);
        delay(500);
        sysreboot();
    });

    server.on("/a1/script", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!isAuthorized(request, PERM_UI)) return;

        if(request->hasArg("p") && !scriptBegin(request->arg("p"))) {
            request->send_P(200, m_txt, "ok");
        } else {
            if(scriptEnd());
            request->send_P(200, m_txt, "script end");
        }
        request->send_P(200, m_txt, "ok");
    });



    server.on("/irs", HTTP_GET, [](AsyncWebServerRequest *request) {
        unsigned long t_timeout = millis();
        //while(millis() - t_timeout <= 10000) {
            if (irrecv.decode(&results)) {
                if (results.value != 0xFFFFFFFF) {
                    request->send(200, m_txt, String(results.value, HEX));  //to finish
                }
                irrecv.resume();
            }
      //  }
        request->send_P(500, m_txt, "timeout");
    });
    server.on("/irm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/map.ir", m_txt);
    });
    server.on("/bts", HTTP_GET, [](AsyncWebServerRequest *request) {
        //TelnetPrint.print("/bts\nADC: ");
        //TelnetPrint.println(analogRead(adc));
        //TelnetPrint.flush();
        int btn_idle_val = analogRead(adc)*100;

        unsigned long t_timeout = millis();
        while(millis() - t_timeout <= 10000) {
            if (analogRead(adc)*100 != btn_idle_val) {
                request->send(200, m_txt, String(analogRead(adc)*100));  //
            }
        }
        request->send_P(500, m_txt, "timeout");
    });
    server.on("/btm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/map.btn", m_txt);
    });

    Serial.println("[SETUP] Starting webserver (begin)");
    //TelnetPrint.println("[SETUP] Starting webserver (begin)");
    server.onNotFound(notFound);
    server.begin();
}

void setup() {
    Serial.begin(115200);

    Serial.setTimeout(10000);
    Serial.println("[SYS] Starting...\n[SYS] Send 'd' in less then 2 sec to boot in UART download mode or 'w' to wipe SPIFFS partition.");
    delay(2000);
    while (Serial.available() > 0) {
        switch (Serial.readStringUntil('\n').charAt(0)) {
        case 'd':
            sysreboot(1);
            break;
        case 'w':
            Serial.println("[SYS] Wiping SPIFFS partition");
            //TelnetPrint.println("[SYS] Wiping SPIFFS partition");
            SPIFFS.format();
            sysreboot(0);
            break;
        default:
            break;
        }
    }
    if (!SPIFFS.begin()) {
        Serial.println("[SPIFFS] Flash is corrupted. Formatting...");
        SPIFFS.format();
        sysreboot(0);
    } else {
        if (!SPIFFS.exists(SYSCFG_F)) {
            //TelnetPrint.begin();
            Serial.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            //TelnetPrint.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            
            wlconf2();
            dnsServer.start(53, "*", WiFi.softAPIP());
            startsrv();
            ArduinoOTA.begin();
            setup_ok = 0;
            while (setup_ok != 1) {
                dnsServer.processNextRequest();
                ArduinoOTA.handle();
            }
            Serial.println("[SETUP] Rebooting to apply new config.");
            //TelnetPrint.println("[SETUP] Rebooting to apply new config.");
            sysreboot(0);
        } else {
            File jsnld = SPIFFS.open(SYSCFG_F, "r");

            JsonDocument doc; 
            DeserializationError error = deserializeJson(doc, jsnld);

            if (error) {
                if (SPIFFS.exists("/cfg.json.broken")) {
                    SPIFFS.remove("/cfg.json.broken");
                }
                SPIFFS.rename(SYSCFG_F, "/cfg.json.broken");
                sysreboot(0);
                return;
            }

            // Hardware settings
            JsonObject hw = doc["hw"];

            if (hw.containsKey("c") && hw["c"].is<JsonArray>()) {
                bool t_ld=hw["ld"] | 0;
                Light.begin(t_ld);
                JsonArray channels = hw["c"];
                for (JsonArray channel : channels) {
                    // Ensure the array contains exactly the expected 3 parameters to avoid indexing crashes
                    if (channel.size() >= 3) { 
                        int chNum = channel[0];
                        int pin   = channel[1];
                        int calib = channel[2];
                        if (!Light.addCh(chNum, pin, calib, t_ld)){
                            //TelnetPrint.printf("Channel %d invalid config", chNum);
                        }
                    }
                }
                analogWriteFreq(611);
                analogWriteRange(32749);
            }

            if (hw.containsKey("p")) {
                atx = hw["p"]; 
                pinMode(atx, OUTPUT);
            }

            if (hw.containsKey("kp")) { 
                adc = hw["kp"]; //this has to be renamed to keypad
                pinMode(adc, INPUT);
            }

            statusled = hw["st"] | LED_BUILTIN;
            pinMode(statusled, OUTPUT);

            //TelnetPrint.begin();
            
            // 3. Software Settings & Defaults
            JsonObject sw = doc["sw"];

            savetim = sw["as"] | 30;

            Light.setFadeSpeed(sw["ftk"] | 1000); 

            webui = sw["wui"] | "dev"; //this is a subkey now

            if(sw["nt"] | 0) {
                const char* tz_ptr  = sw["ntz"] | "UTC0";
                const char* url_ptr = sw["ntp"] | "pool.ntp.org";
                configTime(tz_ptr, url_ptr);
            }
            if(sw["ua"] | 0 ){
                loadUsers();
                userauth=true;
            } else {
                userauth=false;
            }

            
            wlconf2();

            if(sw["snf"] | 0) {
                //Udp.begin(localUdpPort);
                //TelnetPrint.printf("SNF enabled");
                snfLoadConf(SNFCFG_F);
                tSNFMain.enable();
                tSNFMain.setIterations(TASK_FOREVER);
            }
            jsnld.close();
        }
        // if (SPIFFS.exists(WEBCFG_F)) {
        //     File jsnweb = SPIFFS.open(WEBCFG_F, "r");
        //     JsonDocument doc;
        //     //            DeserializationError error = deserializeJson(doc, jsnweb);
        //     //            webui = doc["web"].as<String>();
        //     jsnweb.close();
        // }
        startsrv();
    }

    Serial.println("[SYS] Enabling IR receiver");
    //TelnetPrint.println("[SYS] Enabling IR receiver");
    irrecv.enableIRIn();
    Serial.println("[SYS] Setting up OTA");
    //TelnetPrint.println("[SYS] Setting up OTA");

    ArduinoOTA.onStart([]() {  // arduino ota example sketch
        // String type;
        // if (ArduinoOTA.getCommand() == U_FLASH) {
        //     type = "sketch";
        // } else {  // U_FS
        //     type = "filesystem";
        // }
        SPIFFS.end();
        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Light.end(true);
        analogWriteFreq(1000);
        analogWriteRange(1000);
        Serial.println("[OTA] Start updating");
        //TelnetPrint.println("[OTA] Start updating");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        analogWrite(statusled, 1000 - (millis() % 1001));
        //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        // static unsigned long prev = 0;
        // static int i = 0;
        // if (millis() - prev >= 50 && i < 30) {
        //     prev = millis();
        //     analogWrite(statusled, (i * 100) % 1001);
        //     ++i;
        // }
    });
    ArduinoOTA.onEnd([]() {
        digitalWrite(statusled, HIGH);
    });
    ArduinoOTA.begin();

    if(File last = SPIFFS.open("/last", "r")) { // restore brightness and script state
        ////TelnetPrint.println("open last");
        if(last.peek() == '/'){
            scriptBegin(last.readStringUntil('\n'));
        } else {
            for (int i = 0; i <= Light.getChCount(); i++) {
                persistbr[i] = last.parseInt();
                Light.setBriSingle(i, persistbr[i]);
            }
        }
        last.close();
    } else {
        for (int i = 0; i <= Light.getChCount(); i++) {
            Light.setBriSingle(i, 0);
        }
    }

    //     for (int i = 0; i <= Light.getChCount(); i++) {
    //         persistbr[i] = last.parseInt();
    //         Light.setBriSingle(i, persistbr[i]);
    //     }
    //         //     scriptBegin(f.readStringUntil('\n'));
    //     last.close();


    // if(File f=SPIFFS.open("/lscript","r")) {
    //     scriptBegin(f.readStringUntil('\n'));
    //     f.close();
    // }

    //debug: populate /lscript to test functionality

    // if(File f=SPIFFS.open("/lscript","w")){
    //     f.write("/w/effect1.lsc");
    //     f.close();
    // }


    Serial.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    //TelnetPrint.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    Serial.println(ESP.getVcc());
    //TelnetPrint.println(ESP.getVcc());
    runner.startNow(); // Initializes the scheduler
    Serial.println("[SYS] TaskScheduler Started");
    //TelnetPrint.print("[SYS] TaskScheduler Started");

}

void diyedit(int num) {  // saves the current brightness values in the specified slot then calls diyload() to load and apply them.
    File jsndiy = SPIFFS.open(DIY_F, "r");
    //TelnetPrint.println("open diy.json");
    JsonDocument doc;  // JSON document with twice the size of the file + the size of new entries
    if (jsndiy) {
        //TelnetPrint.println("diyedit spiffs size:");
        // //TelnetPrint.println(jsndiy.size());
        DeserializationError error = deserializeJson(doc, jsndiy);
        if (error) {
            Serial.print(F("[JSON] deserializeJson() failed: "));
            //TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
            Serial.println(error.f_str());
            //TelnetPrint.println(error.f_str());
            // jsndiy.close();
            // SPIFFS.remove(DIY_F);
            // File jsndiy = SPIFFS.open(DIY_F, "w");
            // return;
        }
    }
    jsndiy.close();
    File jsndiyw = SPIFFS.open(DIY_F, "w");
    //TelnetPrint.println("open diy.json WRITE");
    if (jsndiyw) {
        //TelnetPrint.println("diyedit json");
        //TelnetPrint.println(num);
        //TelnetPrint.println(diynr);
        if (num > 0 && num <= diynr) {
            //TelnetPrint.println("diyedit");
            auto diy = "d" + std::to_string(num);
            //TelnetPrint.println(diy.c_str());
            // std::string diy = "d" + std::to_string(num);
            // doc[diy]["all"] = lastbr[0];  ///////////////////////////////////////////////////////////////////
            for (int i = 0; i <= Light.getChCount(); i++) {
                doc[diy]["c" + std::to_string(i)] = lastbr[i];  //////////////////////////////////////////////////////////
            }
        }
        serializeJson(doc, jsndiyw);
        jsndiy.close();
    } else {
        //TelnetPrint.println("JSNDIYW error");
    }
    lastir = 0;
    irhold = 0;
    diyload(num);
}

void diyload(int num) {  // loads and applies brightness values stored in the specified "slot" inside a file
    File jsndiy = SPIFFS.open(DIY_F, "r");
    // //TelnetPrint.println("open diy.json");
    // //TelnetPrint.println("diyload spiffs size:");
    // //TelnetPrint.println(jsndiy.size()*2+DIYTSIZE);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsndiy);
    if (error) {
        Serial.print(F("[JSON] deserializeJson() failed: "));
        //TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
        Serial.println(error.f_str());
        //TelnetPrint.println(error.f_str());
        // //TelnetPrint.println(F("removed"));
        jsndiy.close();
        // SPIFFS.remove(DIY_F);
        return;
    }
    //TelnetPrint.println("diyload json");
    //TelnetPrint.println(jsndiy.size());
    //TelnetPrint.println(irhold);
    if (irhold == 0 && num > 0 && num <= diynr) {
        // lastbr[0] = currentbr[0];
        auto diy = "d" + std::to_string(num);
        //TelnetPrint.println(diy.c_str());
        //TelnetPrint.println("diyload");
        // diy += std::to_string(num);
        for (int i = 0; i <= Light.getChCount(); i++) {
            lastbr[i] = Light.getBriSingle(i);
            Serial.println(lastbr[i]);
            // //TelnetPrint.println(lastbr[i]);
            Serial.println(i);
            // //TelnetPrint.println(i);
            Light.setBriSingle(i, doc[diy]["c" + std::to_string(i)]);
            //TelnetPrint.println(Light.getBriSingle(i));
        }
    }

    jsndiy.close();
    if (irhold == 1 && (millis() - irtime >= 2500)) {
        diyedit(num);
    }
}


void ftable_ex(int fval) {  // function that contains and executes all the functions assigned inside a function table
    // needs heavy modifications to work with current code
    //TelnetPrint.println("FTable ex:");
    //TelnetPrint.println(fval);
    // ir code switch, select channels, brightness
    switch (fval) {
        case 0 ... 5: {
            sel = fval;
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
        case 10: {  // br +
            //Serial.print(" irbru \n");
            //TelnetPrint.print(" irbru \n");
            Light.setBriSingle(sel, min(Light.getBriSingle(sel) + 5, 255));
            break;
        }
        case 11: {  // br -
            //Serial.print(" irbrd \n");
            //TelnetPrint.print(" irbrd \n");
            Light.setBriSingle(sel, max(Light.getBriSingle(sel) - 5, 0));

            break;
        }
        case 12: {  // irmax
                Light.setBriSingle(sel, 255);
                break;
            }
        case 13: {  // irmin
                // Serial.print(" irmin \n");
                // //TelnetPrint.print(" irmin \n");
                Light.setBriSingle(sel, 1);
                break;
            }

        // case irpon: {
        //     Serial.print(" irpon \n");                           //old kept as reference
        //     //TelnetPrint.print(" irpon \n");
        //     diyload(0);
        //     break;
        // }

        case 101 ... 199: {  // diy shortcuts
            diyload(fval - 100);
            if (irhold == 1 && millis() - irtime >= 2500) {
                diyedit(fval - 100);  // FINISH THIS !!!!!!!!!!!!!!!!
            }
            break;
        }
        case 1000000 ... 2000000: {
            // Decode script path offset
            size_t offset = fval - 1000000;
            
            File mapFile = SPIFFS.open("/map.ir", "r");
            mapFile.seek(offset);
            String scriptPath = mapFile.readStringUntil('\n');
            mapFile.close();
            
            scriptBegin(scriptPath, false);
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
                    //TelnetPrint.print("lastbr[");
                    Serial.print(i);
                    //TelnetPrint.print(i);
                    Serial.print("]= ");
                    //TelnetPrint.print("]= ");
                    Serial.println(lastbr[i]);
                    //TelnetPrint.println(lastbr[i]);
                }
                break;
            }
        case '?': {
                // for (int i = 0; i <= winset; i++) {
                //    // Serial.print(currentset[i]);
                // //TelnetPrint.print(currentset[i]);
                //    // Serial.print("\\");
                // //TelnetPrint.print("\\");
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
            
            //TelnetPrint.println("IR received.");
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


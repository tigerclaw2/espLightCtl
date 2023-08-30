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
#include <Arduino.h>
//#include <string>
#include "FS.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <TelnetPrint.h>
#include <DNSServer.h>
#include <untar.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

ADC_MODE(ADC_VCC);
#define DEBUG_USE_TELNET 1

#define SPIFFS_CACHE = (1)
#define SPIFFS_CACHE_WR = (1)

#define DECODE_NEC
//#define DIYTSIZE 112
#define DIYTSIZE 130
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

const char index_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Main menu</h1><hr><a href="/pick">Color Picker</a><br><a href="/pick2">Picker 2</a><br><a href="/diym">DIY Manager</a><br><a href="/thm">Theme Manager</a><hr><a href="/wlcfg">Configure WiFi</a><br><a href="/setup">Configure Hardware</a><br><a href="/rbt">Reboot</a><br><a href="/reset">Factory Reset</a><hr>)rawliteral";
const char wlan_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>WLAN configuration</h1><hr><br><form action="/wlset"method="post"><p>WLAN mode: <select name="wlm"><option value="1">AP only<option value="2">Client only<option value="3">AP+Client</select><p>Client SSID: <input name="ssid"><p>Client PSK : <input name="psk"type="password"><p>AP SSID: <input name="apssid"><p>AP PSK : <input name="appsk"type="password"><p>Reconnect timeout:<input name="t"type="number"max="3600">sec</p><br><input type="submit"value="Save "></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
const char reset_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Factory reset</h1><hr><p>Are you sure?<br><form action="/wipe"method="post"><input type="checkbox"value="1"name="meta_fact">Yes, wipe all settings</p><input type="submit"value="Save "align="right"></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
const char picker_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>Color picker</h1><hr><form action=/shade method=post><p><input type="checkbox" name="p" checked>Persistent<br>CH 1: <input type=number name=ch1><br>CH 2: <input type=number name=ch2><br>CH 3: <input type=number name=ch3><br>CH 4: <input type=number name=ch4><br>CH 5: <input type=number name=ch5><br>Brightness: <input type=number name=ch0></p><input type=submit value="Save"></form><hr><br><a href=/><=Return to main menu</a><script>function sV() {fetch("/stat").then(response=>response.json()).then(json=>{for(let i=0;i<=5;i++){document.getElementsByName(`ch${i}`)[0].value=json[`c${i}`];}});} window.onload=sV</script>)rawliteral";
const char setup_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>Controller Settings</h1><hr><form action=/save method=post>I/O:<p>CH 1: <select name=hw_c1></select><br>CH 2: <select name=hw_c2></select><br>CH 3: <select name=hw_c3></select><br>CH 4: <select name=hw_c4></select><br>CH 5: <select name=hw_c5></select><br>ATX/psu_en: <select name=hw_p></select><br>Status LED: <select name=hw_st></select><hr>Misc:<br>PWM bit:<input type=number name=sw_anw><br>DIY nr:<input type=number name=sw_dnr><br>Save status every<input type=number name=sw_rbt>sec<br>Fade ticks:<input type=number name=sw_tik><hr>Brightness correction:<br>Gamma:<input type=number name=sw_b0><br>CH 1:<input type=number name=sw_b1><br>CH 2:<input type=number name=sw_b2><br>CH 3:<input type=number name=sw_b3><br>CH 4:<input type=number name=sw_b4><br>CH 5:<input type=number name=sw_b5></p><input type=submit value="Save "></form><h6><i>Notes:</i><br>'*' - not recommended<br>'-1' - Disabled<br>'Now' - current value</h6><hr><br><a href=/><=Return to main menu</a><script>window.onload=function(){fetch('/cfg').then(response=> response.json()).then(data=>{for (const sectionKey in data){const section=data[sectionKey]; for (const key in section){const value=section[key]; const select=document.getElementsByName(`${sectionKey}_${key}`)[0]; if (select.tagName==='SELECT'){select.innerHTML=`<option value="${value}" selected>Now: ${value}</option> <option value="-1">Disabled</option><option value="0">0*</option><option value="2">2*</option><option value="4">4</option><option value="5">5</option><option value="12">12</option><option value="13">13</option><option value="14">14</option><option value="15">15</option><option value="16">16*</option>`;}else if(select.tagName==='INPUT'){select.value=value;}}}});};</script>)rawliteral";
const char diymanager_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>DIY Manager</h1><hr><p><form action=/diy method=post><br>Load DIY: <input name=dl type=number> <button type=submit>Load</button></form><form action=/diy method=post><br>Save shade as DIY: <input name=de type=number id=save> <button type=submit>Save</button></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
const char picker2_html[] PROGMEM= R"rawliteral(<html><meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><style>.slid{display:flex;flex-wrap:wrap;justify-content:center;align-items:center;margin-top:20px}.s1{width:400px;height:30px}.s2{transform:rotate(270deg);height:100px;margin-bottom:28px;margin-top:28px}</style><h2>Color Picker 2</h2><div class=slid></div><script>window.onload=function(){Promise.all([fetch("/cfg").then((e=>e.json())),fetch("/stat").then((e=>e.json()))]).then((e=>{const n=e[0].hw,t=document.querySelector(".slid");if(t)for(const l in e[1])"c0"==l?t.innerHTML+=`<input type="range" class="s1" id="${l}" min="0" max="${Math.pow(2,e[0].sw.anw)}" value="${e[1][l]}"/>`:"-1"!==n[l]?t.innerHTML+=`<input type="range" class="s2" id="${l}" min="0" max="255" value="${e[1][l]}"/>`:t.innerHTML+=`<input type="range" class="s2" id="${l}"disabled/>`,document.getElementById(l).addEventListener("input",upres)}))};let timerId=null;function upres(){const e=document.getElementById("c0"),n=document.getElementById("c1"),t=document.getElementById("c2"),l=document.getElementById("c3"),c=document.getElementById("c4"),u=document.getElementById("c5");let a=null,d=null,o=null,i=null,s=null,m=null;e.value===a&&n.value===d&&t.value===o&&l.value===i&&c.value===s&&u.value===m||(a=e.value,d=n.value,o=t.value,i=l.value,s=c.value,m=u.value,timerId&&clearTimeout(timerId),timerId=setTimeout((()=>{const e=new XMLHttpRequest;e.open("POST","/raw",!0),e.setRequestHeader("Content-Type","application/x-www-form-urlencoded"),e.send(`c0=${a}&c1=${d}&c2=${o}&c3=${i}&c4=${s}&c5=${m}`),console.log(`c0=${a}&c1=${d}&c2=${o}&c3=${i}&c4=${s}&c5=${m}`)}),100))}</script></body></html>)rawliteral";
const char success_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Success!</h1><hr>Request completed successfully.<hr><br><a href="/"><=Return to main menu</a>)rawliteral";
const char error_html[] PROGMEM= R"rawliteral(<html><body bgcolor=#800><h1>Error!<hr>Check DevTools for http code or more details.)rawliteral";
const char a_css[] PROGMEM= R"rawliteral(html{max-width:35ch;margin:auto;font-size:150%;color:orange;}a:link,a:visited,a:hover,a:active{color:#add8e6; text-decoration:none;}h1{color:#90ee90;text-decoration:none;}body{background-color:#1E1E1E;})rawliteral";
const char theme_html[] PROGMEM= R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Theme Upload</h1><hr><form method="POST" action="/fup" enctype="multipart/form-data"><input type="file" name="file"><br><input type="submit" value="Upload"></form>)rawliteral";

IRrecv irrecv(recvPin);
decode_results results;
AsyncWebServer server(80);
DNSServer dnsServer;
Tar<FS> tar(&SPIFFS);

//int rboot = 1;
//int winset = 20;
//int eeprommax = 10;
double nms=1000;           //<<<<<<<<<<<<<<<<<------------------------

int chout1 = -1;
int chout2 = -1;
int chout3 = -1;
int chout4 = -1;
int chout5 = -1;
int atx = -1;
int adc = -1;
int statusled = LED_BUILTIN;
String webui="dev";

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

unsigned long irtime, lastir, wltim, lastfade, cron1, lastsave, vloop[30], cron2, lastwlscan, lastbrms, finbrms;
int sel, diynr=0, chnr = 0, anw = 1, lastbr[6], wltimeout, raw, calib[6], savetim, persistbr[6], targetbr[6], lasttarget[6], adc_val, fadetick, ntik, vl, fadetype=1;
double speed[6], currentbr[6];
//float adc_val;
byte wlconf_started, setup_ok, irhold=0, start_noti, brichanged;

void diyedit(int num);
void diyload(int num);

// void setbri() {
//     //unsigned long tfin=millis()+
//     for(int i=0; i<=chnr; i++) {
//         speed[i]=(targetbr[i]-currentbr[i])/nms;
//         TelnetPrint.print("Speed result: ");
//         TelnetPrint.println(speed[i],6);
//         TelnetPrint.print("Expected end: ");
//         TelnetPrint.println(millis()+nms);
//         finbrms=millis()+nms+100;
//     }
//     fadetype=1;
//     lastbrms=millis();


//     // if(fadetype==1) { // new fading type using speed formula speed=tick/(current-target)
//     //     for(int i=0; i<=chnr; i++) {
//     //         speed[i]=fadetick/(currentbr[i]-target[i]);
//     //         targetbr[i]=target[i];
//     //     }
//     // } else if(fadetype==2) { // cpu|tick dependent fading
//     //     for(int i=0; i<=chnr; i++) {
//     //         targetbr[i]=target[i];
//     //     }
//     // } else if(fadetype==3) { // no fade/direct change
//     //     for(int i=0; i<=chnr; i++) {
//     //         currentbr[i]=targetbr[i]=target[i];
//     //     }
//     // }
// }
int needs_update() {
    return 0;                       // for debug purposes, never save brightness to disk
    for(int i=0; i<=chnr; i++) {
        if(targetbr[i]!=persistbr[i]) {
            return 1;
        }
    }
    return 0;
}

void btn_loadmap() {
    TelnetPrint.println("BTN LoadMap");
    File file = SPIFFS.open("/map.btn", "r");
    // Read the number of mappings from the first int in the file
    ctrl.btnmax = file.parseInt();
    ctrl.btnign = file.parseFloat();
    for (int i = 1; i < MAX_BTN; i++) {
        ctrl.btn[i].button=file.parseInt();
        ctrl.btn[i].function=file.parseInt();
        if(!file.available()) {
            ctrl.btn[i+1].button=-1;
            break;
        }
    }
    file.close();
}

// Look up the function assigned to a button
int btn_lookup(int btn) {
    // Check if the button is in the btable array
    TelnetPrint.println("BTN Lookup");
    for (int i = 1; i <= MAX_BTN; i++) {
        if (btn > ctrl.btn[i].button - BTOL && btn < ctrl.btn[i].button + BTOL) {
            return ctrl.btn[i].function;
        }
        if(ctrl.btn[i].button == -1) {
            return 0;
        }
        // If the button was not found in the btable array, search the file
        if (i == MAX_BTN && ctrl.btnmax > MAX_BTN) {
            TelnetPrint.println("BTN Lookup file");

            File file = SPIFFS.open("/map.btn", "r");
            file.seek(2*MAX_BTN*sizeof(int));       //seek the file, there is no need to read again what we already have in ram
            // Seek to the first byte of the 33rd entry in the file
//            file.seek(MAX_BTN * 2, SeekSet);

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
        ctrl.btn[i].button=file.parseInt();
        ctrl.btn[i].function=file.parseInt();
        if(!file.available()) {
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
            // Seek to the first byte of the 33rd entry in the file
            file.seek(MAX_IR * 2, SeekSet);

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
// int reached_shade(){  // dirty way of ckecking if the current shade is the same as the requested one, aka the fade effect stopped
//     int res=1;
//     for(int i=0; i<=chnr; i++){
//         if(currentbr[i] != targetbr[i]){
//             res=0;
//         }
//     }
//     return res;
// }
// int btnlookup (auto bid, auto btable){
// for (int i=1; i <= btable[0].id; i++){
//     if(btable[i].id == bid){
//         return btable[i].function;
//     }
//     return 0;
// }

// void btableload(){
//     File func= SPIFFS.open("/func","r");
//     while (func){
//         button btable[i];

//     }

// }
// }
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/html", error_html);
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
        File file = SPIFFS.open("/t/"+ filename, "r");
        TelnetPrint.println("open /t/");
        if(file) {
            tar.open((Stream*)&file);		// Pass source file as Stream to Tar object
            tar.dest("/w/");
            tar.extract();
            file.close();
            TelnetPrint.println("untar ok");
            //SPIFFS.rmdir("/t/");
            SPIFFS.remove("/t/"+ filename);
        }
        request->redirect("/up");
    }
}

void sysreboot(int mode=0) {
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
        //delay(10);
        ESP.restart();
        break;
    }
}

void awrite(int mode=0) {
    //analog write lol
    // switch (mode) {
    // case 1: {           //raw write: currentbr has values
    //     //TelnetPrint.println("[AWRITE] Raw write");
    //     //analogWriteRange(targetbr[0]);
    //     analogWriteResolution(anw);
    //     analogWrite(chout1, currentbr[1]);
    //     analogWrite(chout2, currentbr[2]);
    //     analogWrite(chout3, currentbr[3]);
    //     analogWrite(chout4, currentbr[4]);
    //     analogWrite(chout5, currentbr[5]);
    //     break;
    // }
    // // default: {          //percent write: 0 has brightness value, others have % of brightness
    // //     // TelnetPrint.println("[AWRITE] Percent write");
    // //     analogWriteResolution(anw);
    // //     analogWrite(chout1, currentbr[0] * currentbr[1] / 1024);
    // //     analogWrite(chout2, currentbr[0] * currentbr[2] / 1024);
    // //     analogWrite(chout3, currentbr[0] * currentbr[3] / 1024);
    // //     analogWrite(chout4, currentbr[0] * currentbr[4] / 1024);
    // //     analogWrite(chout5, currentbr[0] * currentbr[5] / 1024);
    // //     break;
    // // }
    // default: {
    analogWriteResolution(anw);
    analogWrite(chout1, map(currentbr[1],0,255,0,currentbr[0]));
    analogWrite(chout2, map(currentbr[2],0,255,0,currentbr[0]));
    analogWrite(chout3, map(currentbr[3],0,255,0,currentbr[0]));
    analogWrite(chout4, map(currentbr[4],0,255,0,currentbr[0]));
    analogWrite(chout5, map(currentbr[5],0,255,0,currentbr[0]));
    // }
    // }
}

void notifade(int ldiy=9, int tick=20) {                //blocking function

    int noti[chnr],done=0;
    for(int i=0; i<=chnr; i++) {
        noti[i]=targetbr[i];
    }
    TelnetPrint.println(ldiy);
    TelnetPrint.println(tick);
    diyload(ldiy);
    while(done<=chnr+1) {
        // if(micros() - lastfade >= 2) {
        for(int i=0; i<=chnr; i++) {
            done++;
            if (currentbr[0] !=0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if(targetbr[i]>currentbr[i]) {
                currentbr[i]++;
                done=0;
            }
            if(targetbr[i]<currentbr[i]) {
                currentbr[i]--;
            }
            awrite(anw);
        }
        //     lastfade=micros();
        // }
    }
    for(int i=0; i<=chnr; i++) {
        targetbr[i]=noti[i];
    }
    done=0;
    while(done<=chnr+1) {
        // if(micros() - lastfade >= 2) {
        for(int i=0; i<=chnr; i++) {
            done++;
            if (currentbr[0] !=0) {
                digitalWrite(atx, HIGH);
            } else {
                digitalWrite(atx, LOW);
            }
            if(targetbr[i]>currentbr[i]) {
                currentbr[i]++;
                done=0;
            }
            if(targetbr[i]<currentbr[i]) {
                currentbr[i]--;
            }
            awrite(anw);
        }
        //     lastfade=micros();
        // }
    }
    TelnetPrint.println("finished notifade");
}

// void wlconf2() {
//     int wlmode;
//     wlconf_started=true;
//     if (WiFi.waitForConnectResult() != WL_CONNECTED) {
//         File jsnwlan = SPIFFS.open("/wlan.json","r");
//         TelnetPrint.println("open wlan.json");
//         StaticJsonDocument<320> doc;
//         DeserializationError error = deserializeJson(doc, jsnwlan);
//         if (error) {
//             Serial.print(F("[WLAN] JSON deserializeJson() failed: "));
//             TelnetPrint.print(F("[WLAN] JSON deserializeJson() failed: "));
//             Serial.println(error.f_str());
//             TelnetPrint.println(error.f_str());
//             wltimeout=10;
//             wlmode=1;
//         } else {
//             wlmode=doc["wlm"];
//             wltimeout=doc["t"]|10;
//         }

//         switch (wlmode) {
//         case 1: {
//             Serial.println("[WLAN] Disabled");
//             TelnetPrint.println("[WLAN] Disabled");
//             WiFi.disconnect();
//             Serial.println("[WLAN] Started AP");
//             TelnetPrint.println("[WLAN] Started AP");
//             WiFi.setPhyMode(WIFI_PHY_MODE_11N);
//             WiFi.mode(WIFI_AP);
//             WiFi.softAP(doc["apssid"]|"esp_LightCtl", doc["appsk"]|"987654321");
//             Serial.print("[WLAN] SSID=");
//             TelnetPrint.print("[WLAN] SSID=");
//             Serial.println("[WLAN] PSK=");
//             TelnetPrint.println("[WLAN] PSK=");
//             break;
//         }
//         case 2: {
//             Serial.println("[WLAN] Disabled");
//             TelnetPrint.println("[WLAN] Disabled");
//             WiFi.disconnect();
//             Serial.println("[WLAN] Started STA");
//             TelnetPrint.println("[WLAN] Started STA");
//             WiFi.setPhyMode(WIFI_PHY_MODE_11N);
//             WiFi.mode(WIFI_STA);
//             if(WiFi.SSID().c_str() != doc["ssid"] || WiFi.psk().c_str()!=doc["psk"]) {
//                 WiFi.disconnect();
//                 Serial.println("[WLAN] Connection details changed on disk. Updating...");
//                 TelnetPrint.println("[WLAN] Connection details changed on disk. Updating...");
//                 WiFi.begin(doc["ssid"], doc["psk"]|"");
//             } else {
//                 WiFi.begin();
//             }
//             Serial.println("[WLAN] Connecting to WiFi...");
//             TelnetPrint.println("[WLAN] Connecting to WiFi...");
//             wlconf_started=false;
//             break;
//         }
//         case 3: {
//             Serial.println("[WLAN] Disabled");
//             TelnetPrint.println("[WLAN] Disabled");
//             WiFi.disconnect();
//             Serial.println("[WLAN] Started STA+AP");
//             TelnetPrint.println("[WLAN] Started STA+AP");
//             WiFi.setPhyMode(WIFI_PHY_MODE_11N);
//             WiFi.mode(WIFI_AP_STA);
//             WiFi.begin(doc["ssid"], doc["psk"]|"");
//             WiFi.softAP(doc["apssid"] |"esp_LightCtl", doc["appsk"]|"987654321");
//             Serial.print("[WLAN] SSID=");
//             TelnetPrint.print("[WLAN] SSID=");
//             Serial.println("[WLAN] PSK=");
//             TelnetPrint.println("[WLAN] PSK=");
//             break;
//         }
//         default:
//             break;
//         }
//         jsnwlan.close();

//     }
// }

void wlconf2() {
    wlconf_started=true;
    WiFi.persistent(0);
    File jsnwlan = SPIFFS.open("/wlan.json","r");
    StaticJsonDocument<320> doc;
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
        case 1: {                               //ap mode
            WiFi.mode(WIFI_AP);
            WiFi.softAP(doc["apssid"]|"esp_LightCtl", doc["appsk"]|"987654321");
            break;
        }
        case 2: {                               //client mode
            if(WiFi.SSID().c_str() != doc["ssid"] || WiFi.psk().c_str()!=doc["psk"]) {
                WiFi.disconnect();
                WiFi.mode(WIFI_STA);
                WiFi.begin(doc["ssid"], doc["psk"]|"");
            }
            break;
        }
        case 3: {                               //ap + client mode
            if(WiFi.SSID().c_str() != doc["ssid"] || WiFi.psk().c_str()!=doc["psk"] || WiFi.softAPSSID().c_str() != doc["apssid"] || WiFi.softAPPSK().c_str() != doc["appsk"]) {
                WiFi.disconnect();
                WiFi.mode(WIFI_AP_STA);
                WiFi.begin(doc["ssid"], doc["psk"]|"");
                WiFi.softAP(doc["apssid"]|"esp_LightCtl", doc["appsk"]|"987654321");
            }
            break;
        }

        }
    }
}

void startsrv() {
    Serial.println("[SETUP] Stopping webserver (end)");
    TelnetPrint.println("[SETUP] Stopping webserver (end)");
    server.end();
    Serial.println("[SETUP] Starting webserver on /");
    TelnetPrint.println("[SETUP] Starting webserver on /");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(webui=="dev") {
            request->send_P(200, "text/html", index_html);
        } else {
            request->redirect("/w/" + webui + "/i.htm");
        }
    });

    Serial.println("[SETUP] Starting webserver on /a.css");
    TelnetPrint.println("[SETUP] Starting webserver on /a.css");
    server.on("/a.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/css", a_css);
    });

    Serial.println("[SETUP] Starting webserver on /rbt");
    TelnetPrint.println("[SETUP] Starting webserver on /rbt");
    server.on("/rbt", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", success_html);
        delay(500);
        sysreboot();
    });

    Serial.println("[SETUP] Starting webserver on /reset");
    TelnetPrint.println("[SETUP] Starting webserver on /reset");
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", reset_html);
    });

    Serial.println("[SETUP] Starting webserver on /wipe");
    TelnetPrint.println("[SETUP] Starting webserver on /wipe");
    server.on("/wipe", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(request->hasArg("meta_fact")) {
            File jsnw = SPIFFS.open("/cfg.json", "w");
            TelnetPrint.println("open cfg.json");
            StaticJsonDocument<256> doc;
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

    Serial.println("[SETUP] Starting webserver on /wlcfg");
    TelnetPrint.println("[SETUP] Starting webserver on /wlcfg");
    server.on("/wlcfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", wlan_html);
    });

    Serial.println("[SETUP] Starting webserver on /wlset");
    TelnetPrint.println("[SETUP] Starting webserver on /wlset");
    server.on("/wlset", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasArg("wlm") && request->hasArg("ssid") && request->hasArg("psk") && request->hasArg("apssid") && request->hasArg("appsk") && request->hasArg("t")) {
            File jsnw = SPIFFS.open("/wlan.json", "w");
            TelnetPrint.println("open wlan.json");
            StaticJsonDocument<320> doc; //320
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
    server.on("/wlist", HTTP_GET, [](AsyncWebServerRequest *request) {
// async webserver is in a hurry and crashes if it has to wait for a blocking function to return
// so we need to do this and call the api 2 times in the webui
        if(millis() - lastwlscan > 8000) {
            WiFi.scanNetworks(true, true);
            lastwlscan = millis();
            request->send_P(500, "text/plain", "Call again in 3 seconds but no more than 8");
        } else {
            DynamicJsonDocument doc(2048);
            String stat;
            // Add the scanned networks to the JSON document
            for (int i = 0; i < WiFi.scanComplete(); i++) {
                JsonObject wifi = doc.createNestedObject();
                wifi["sid"] = WiFi.SSID(i);
                wifi["enc"] = WiFi.encryptionType(i);
                wifi["rsi"] = WiFi.RSSI(i);
            }
            serializeJson(doc, stat);
            request->send_P(200, "application/json", stat.c_str());
        }
    });

    server.on("/lsui", HTTP_GET, [](AsyncWebServerRequest *request) {
        String stat;
        String filename;
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            //stat += concat(dir.fileName().indexOf('/', 1);
            //stat += filename.substring(0,)
            //stat += dir.fileName() + "\n";
            TelnetPrint.println(dir.fileName());
        }
        request->send_P(200, "text/plain", stat.c_str());
    });
    server.on("/rmaui", HTTP_GET, [](AsyncWebServerRequest *request) {
        Dir dir = SPIFFS.openDir("/w/");
        while (dir.next()) {
            SPIFFS.remove(dir.fileName());
        }
        request->send_P(200, "text/plain", "Removed all themes");
    });

    Serial.println("[SETUP] Starting webserver on /setup");
    TelnetPrint.println("[SETUP] Starting webserver on /setup");
    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", setup_html);
    });

    Serial.println("[SETUP] Starting webserver on /save");
    TelnetPrint.println("[SETUP] Starting webserver on /save");
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[SETUP] /save Opening cfg.json w");
        TelnetPrint.println("[SETUP] /save Opening cfg.json w");
        File jsncfg = SPIFFS.open("/cfg.json", "w");
        TelnetPrint.println("open cfg.json");
        StaticJsonDocument<381> doc;
        // Serial.println("[SETUP] /save Opened cfg.json w");
        // TelnetPrint.println("[SETUP] /save Opened cfg.json w");
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
        if(request->hasArg("web"))
            doc["web"] = request->arg("web");
        if(request->hasArg("sw_rbt"))
            doc["sw"]["rbt"]  = request->arg("sw_rbt");



        // Serial.println("[SETUP] /save Write done to struct. status led next");
        // TelnetPrint.println("[SETUP] /save Write done to struct. status led next");
        // if(request->hasArg("hw_st"))
        //     if(request->arg("hw_st").toInt()==-2) {
        //         doc["hw"]["st"] = LED_BUILTIN;
        //     } else {
        //         doc["hw"]["st"] = request->arg("hw_st").toInt();
        //     };
        //doc["meta"]["cfgver"]  = request->arg("meta_cfgver");
        // Serial.println("[SETUP] Writing doc file with values:");
        // TelnetPrint.println("[SETUP] Writing doc file with values:");
        // Serial.println("[hw]");
        // TelnetPrint.println("[hw]");
        // Serial.print("[chout1] ");
        // TelnetPrint.print("[chout1] ");
        // Serial.println(doc["hw"]["c1"].as<int>());
        // TelnetPrint.println(doc["hw"]["c1"].as<int>());
        // Serial.print("[chout2] ");
        // TelnetPrint.print("[chout2] ");
        // Serial.println(doc["hw"]["c2"].as<int>());
        // TelnetPrint.println(doc["hw"]["c2"].as<int>());
        // Serial.print("[chout3] ");
        // TelnetPrint.print("[chout3] ");
        // Serial.println(doc["hw"]["c3"].as<int>());
        // TelnetPrint.println(doc["hw"]["c3"].as<int>());
        // Serial.print("[chout4] ");
        // TelnetPrint.print("[chout4] ");
        // Serial.println(doc["hw"]["c4"].as<int>());
        // TelnetPrint.println(doc["hw"]["c4"].as<int>());
        // Serial.print("[chout5] ");
        // TelnetPrint.print("[chout5] ");
        // Serial.println(doc["hw"]["c5"].as<int>());
        // TelnetPrint.println(doc["hw"]["c5"].as<int>());
        // Serial.print("[status] ");
        // TelnetPrint.print("[status] ");
        // Serial.println(doc["hw"]["status"].as<int>());
        // TelnetPrint.println(doc["hw"]["status"].as<int>());
        // Serial.println("[SETUP] /save Serializing cfg.json w");
        // TelnetPrint.println("[SETUP] /save Serializing cfg.json w");
        serializeJson(doc, jsncfg);
        jsncfg.close();

        request->send_P(200, "text/html", success_html);

        // File fil = SPIFFS.open("/cfg.json", "r");
        // if (!fil) {
        //     Serial.println("[SPIFFS] Failed to open file for reading.");
        //     TelnetPrint.println("[SPIFFS] Failed to open file for reading.");
        //     return;
        // }
        // while (fil.available()) {
        //     fil.sendAll(Serial);
        //     Serial.println();
        //     TelnetPrint.println();
        // }
        // fil.close();
        setup_ok=1;
        // } else {
        //     request->send_P(500, "text/html", error_html);
        // }
        //int fact=std::stoi(request->getParam("meta_fact", true)->value().c_str());
    });

    server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request) {
//        request->send(200, "text/html", "<form method=\"POST\" action=\"/fup\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"file\"><br><input type=\"submit\" value=\"Upload\"></form>");
        request->send(200, "text/html", theme_html);
    });
    server.on("/fup", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleUpload);
// server.on("/fdn", HTTP_POST, [](AsyncWebServerRequest *request) {
//     if(request->hasArg("url")){
//         HTTPClient httpClient;
//         httpClient.begin(request->arg("url"));
//         int httpCode = httpClient.GET();

//         if (httpCode == 200) {
//             Serial.println("File downloaded!");

//             request->send_P(200, "text/html", success_html);
//         } else {
//             request->send_P(200, "text/plain", "Failed: " + httpCode);
//         }
//     } else {
//         request->send_P(200, "text/plain", "Failed: no URL");
//     }
// });
    server.on("/pick", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", picker_html);
    });
    server.on("/shade", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[WEB] /shade");
        TelnetPrint.println("[WEB] /shade");
        raw=0;
        if(request->hasArg("ch1"))
            targetbr[1]=request->arg("ch1").toInt();
        if(request->hasArg("ch2"))
            targetbr[2]=request->arg("ch2").toInt();
        if(request->hasArg("ch3"))
            targetbr[3]=request->arg("ch3").toInt();
        if(request->hasArg("ch4"))
            targetbr[4]=request->arg("ch4").toInt();
        if(request->hasArg("ch5"))
            targetbr[5]=request->arg("ch5").toInt();
        if(request->hasArg("ch0"))
            targetbr[0]=request->arg("ch0").toInt();
        lastsave=millis();
        //setbri();
        // if(request->hasArg("p") && request->arg("p").toInt()==1) {
        //     TelnetPrint.println("[WEB] /shade Writing to file");
        //     File last = SPIFFS.open("/last", "w");
        //     for(int i=0; i<=chnr; i++) {
        //         last.println(targetbr[i]);
        //     }
        //     last.println(raw);
        //     last.close();
        // }
        request->send_P(200, "text/html", success_html);

    });
    server.on("/pick2", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", picker2_html);
    });
    server.on("/raw", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[SETUP] /raw");
        TelnetPrint.println("[SETUP] /raw");

        if(request->hasArg("c0"))
            targetbr[0] = request->arg("c0").toInt();
        if(request->hasArg("c1"))
            targetbr[1] = map(request->arg("c1").toInt(), 0, 255, 0, calib[1]);
        if(request->hasArg("c2"))
            targetbr[2] = map(request->arg("c2").toInt(), 0, 255, 0, calib[2]);
        if(request->hasArg("c3"))
            targetbr[3] = map(request->arg("c3").toInt(), 0, 255, 0, calib[3]);
        if(request->hasArg("ch4"))
            targetbr[4] = map(request->arg("c4").toInt(), 0, 255, 0, calib[4]);
        if(request->hasArg("c5"))
            targetbr[5] = map(request->arg("c5").toInt(), 0, 255, 0, calib[5]);
        lastsave=millis();
        request->send_P(200, "text/plain", "ok");
    });

    server.on("/stat", HTTP_GET, [](AsyncWebServerRequest *request) {
        char stat[72];
        DynamicJsonDocument doc(130);

        for(int i=0; i<=chnr; i++) {
            doc["c"+ std::to_string(i)]=targetbr[i];
        }
        serializeJson(doc, stat);
        request->send_P(200, "application/json", stat);
    });

    server.on("/cfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/cfg.json", "application/json");
    });

    server.on("/a/plus", HTTP_POST, [](AsyncWebServerRequest *request) {
        raw=0;
        if(request->hasArg("ch1"))
            targetbr[1]=targetbr[1] + request->arg("ch1").toInt();
        if(request->hasArg("ch2"))
            targetbr[2]=targetbr[2] + request->arg("ch2").toInt();
        if(request->hasArg("ch3"))
            targetbr[3]=targetbr[3] + request->arg("ch3").toInt();
        if(request->hasArg("ch4"))
            targetbr[4]=targetbr[4] + request->arg("ch4").toInt();
        if(request->hasArg("ch5"))
            targetbr[5]=targetbr[5] + request->arg("ch5").toInt();
        if(request->hasArg("ch0"))
            targetbr[0]=targetbr[0] + request->arg("ch0").toInt();
        request->send_P(200, "text/html", success_html);
    });
    server.on("/a/minus", HTTP_POST, [](AsyncWebServerRequest *request) {
        raw=0;
        if(request->hasArg("ch1"))
            targetbr[1]=targetbr[1] - request->arg("ch1").toInt();
        if(request->hasArg("ch2"))
            targetbr[2]=targetbr[2] - request->arg("ch2").toInt();
        if(request->hasArg("ch3"))
            targetbr[3]=targetbr[3] - request->arg("ch3").toInt();
        if(request->hasArg("ch4"))
            targetbr[4]=targetbr[4] - request->arg("ch4").toInt();
        if(request->hasArg("ch5"))
            targetbr[5]=targetbr[5] - request->arg("ch5").toInt();
        if(request->hasArg("ch0"))
            targetbr[0]=targetbr[0] - request->arg("ch0").toInt();
        request->send_P(200, "text/html", success_html);
    });
    server.on("/diym", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", diymanager_html);
    });
    server.on("/diy", HTTP_POST, [](AsyncWebServerRequest *request) {
        TelnetPrint.println("[WEB] /diy");
        raw=0;
        if(request->hasArg("dl")) {
            TelnetPrint.println("[WEB] /diy found dl");
            TelnetPrint.println(request->arg("dl").toInt());
            diyload(request->arg("dl").toInt());
        } else if(request->hasArg("de")) {
            TelnetPrint.println("[WEB] /diy found de");
            for(int i=0; i<=chnr; i++) {
                lastbr[i]=targetbr[i];
                TelnetPrint.println(lastbr[i]);
            }
            TelnetPrint.println(request->arg("de").toInt());
            diyedit(request->arg("de").toInt());
        }
        request->send_P(200, "text/html", success_html);
    });
    server.on("/di", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/diy.json", "application/json");
    });
    server.on("/dic", HTTP_GET, [](AsyncWebServerRequest *request) {
        SPIFFS.remove("/diy.json");
        request->redirect("/diym");
    });
    server.on("/thm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", theme_html);
    });
    server.on("/irs", HTTP_GET, [](AsyncWebServerRequest *request) {
        cron1 = millis();
        while(millis() - cron1 <= 10000) {
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
        while(millis() - cron1 <= 10000) {
            if (analogRead(adc)*100 != btn_idle_val) {
                request->send(200, "text/plain", String(analogRead(adc)*100));  //
            }
        }
        request->send_P(500, "text/plain", "timeout");
    });
    server.on("/btm", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/map.btn", "text/plain");
    });
    server.serveStatic("/w/", SPIFFS, "/w/");

// api version 1
// GET
    server.on("/api1/stat", HTTP_GET, [](AsyncWebServerRequest *request) {
        char stat[72];
        DynamicJsonDocument doc(130);

        for(int i=0; i<=chnr; i++) {
            doc["c"+ std::to_string(i)]=targetbr[i];
        }
        serializeJson(doc, stat);
        request->send_P(200, "application/json", stat);
    });
    server.on("/api1/di", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/diy.json", "application/json");
    });
    server.on("/api1/cfg", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/cfg.json", "application/json");
    });
// POST
    server.on("/api1/shade", HTTP_POST, [](AsyncWebServerRequest *request) {
        raw=0;
        if(request->hasArg("ch1"))
            targetbr[1]=request->arg("ch1").toInt();
        if(request->hasArg("ch2"))
            targetbr[2]=request->arg("ch2").toInt();
        if(request->hasArg("ch3"))
            targetbr[3]=request->arg("ch3").toInt();
        if(request->hasArg("ch4"))
            targetbr[4]=request->arg("ch4").toInt();
        if(request->hasArg("ch5"))
            targetbr[5]=request->arg("ch5").toInt();
        if(request->hasArg("ch0"))
            targetbr[0]=request->arg("ch0").toInt();
        if(request->hasArg("fa")) {
            fadetype=request->arg("fa").toInt();
        } else {
            fadetype=1;
        }
        lastsave=millis();
        request->send_P(200, "text/plain", "ok");
    });
    server.on("/api1/diy", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(request->hasArg("dl")) {
            diyload(request->arg("dl").toInt());
        } else if(request->hasArg("de")) {
            for(int i=0; i<=chnr; i++) {
                lastbr[i]=targetbr[i];
            }
            diyedit(request->arg("de").toInt());
        }
        request->send_P(200, "text/plain", "ok");
    });
    server.on("/api1/n", HTTP_POST, [](AsyncWebServerRequest *request) {
        if(request->hasArg("nt")) {
            start_noti=request->arg("nt").toInt();
        }
        if(request->hasArg("ntk")) {
            ntik=request->arg("ntk").toInt();
        }
        // for(int i=0;i<=chnr;i++){
        //     noti[i]=targetbr[i];
        // }
        // diyload(ntype);
        // while(reached_shade()!=1){
        //     if(micros() - lastfade >= fadetick) {
        //         for(int i=0; i<=chnr; i++) {
        //             if (currentbr[0] !=0) {
        //                 digitalWrite(atx, HIGH);
        //             } else {
        //                 digitalWrite(atx, LOW);
        //             }

        //             if(targetbr[i]>currentbr[i]) {
        //                 currentbr[i]++;
        //             }
        //             if(targetbr[i]<currentbr[i]) {
        //                 currentbr[i]--;
        //             }
        //             awrite(anw);
        //         }
        //         lastfade=micros();
        //     }
        // }
        // for(int i=0;i<=chnr;i++){
        //     targetbr[i]=noti[i];
        // }
        request->send_P(200, "text/plain", "ok");
    });
// end apiv1

    Serial.println("[SETUP] Starting webserver (begin)");
    TelnetPrint.println("[SETUP] Starting webserver (begin)");
    server.onNotFound(notFound);
    server.begin();
}

void setup() {
    Serial.begin(115200);

    Serial.setTimeout(10000);
    // Serial.println("[SYS] Starting...\n[SYS] Send 'd' in less then 3 sec to boot in UART download mode or 'w' to wipe SPIFFS partition.");
    // TelnetPrint.println("[SYS] Starting...\n[SYS] Send 'd' in less then 3 sec to boot in UART download mode or 'w' to wipe SPIFFS partition.");
    //delay(3100);
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
        // TelnetPrint.println("[SPIFFS] Mount failed. Formatting filesystem in 5 seconds...");
        Serial.println("[SPIFFS] Unplug power NOW to abort!");
        // TelnetPrint.println("[SPIFFS] Unplug power NOW to abort!");
        delay(5100);
        Serial.println("[SPIFFS] Formatting...");
        // TelnetPrint.println("[SPIFFS] Formatting...");
        SPIFFS.format();
        sysreboot(0);
    } else {
        // Serial.println("[SPIFFS] Mount OK.");
        // TelnetPrint.println("[SPIFFS] Mount OK.");
        //WiFi.persistent(true);
        // fs::FSInfo fs_info;
        // SPIFFS.info(fs_info);
        // Serial.print("[SYS] ESP flash size: ");
        // TelnetPrint.print("[SYS] ESP flash size: ");
        // Serial.println(ESP.getFlashChipSize());
        // TelnetPrint.println(ESP.getFlashChipSize());
        // Serial.print("[SYS] ESP flash REAL size: ");
        // TelnetPrint.print("[SYS] ESP flash REAL size: ");
        // Serial.println(ESP.getFlashChipRealSize());
        // TelnetPrint.println(ESP.getFlashChipRealSize());
        // Serial.print("[SPIFFS] Usable kbytes: ");
        // TelnetPrint.print("[SPIFFS] Usable kbytes: ");
        // Serial.println(fs_info.totalBytes/1000);
        // TelnetPrint.println(fs_info.totalBytes/1000);
        // Serial.print("[SPIFFS] Used kbytes: ");
        // TelnetPrint.print("[SPIFFS] Used kbytes: ");
        // Serial.println(fs_info.usedBytes/1000);
        // TelnetPrint.println(fs_info.usedBytes/1000);

        wlconf2();
        //startsrv();

        if(!SPIFFS.exists("/cfg.json")) {
            TelnetPrint.begin();
            Serial.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            TelnetPrint.println("[SPIFFS] Configuration does not exist. Pausing setup.");
            // File jsnw = SPIFFS.open("/cfg.json", "w");
            // StaticJsonDocument<256> doc;
            //startsrv();
            //wlconf2();
            dnsServer.start(53, "*", WiFi.softAPIP());
            startsrv();
            setup_ok=0;
            while (setup_ok !=1 ) {
                dnsServer.processNextRequest();
                delay(1000);
            }
            Serial.println("[SETUP] Rebooting to apply new config.");
            TelnetPrint.println("[SETUP] Rebooting to apply new config.");
            sysreboot(0);
        } else {
            File jsnld = SPIFFS.open("/cfg.json", "r");
            TelnetPrint.println("open cfg.json");
            StaticJsonDocument<445> doc;
            DeserializationError error = deserializeJson(doc, jsnld);
            if (error) {
                Serial.print(F("[JSON] deserializeJson() failed: "));
                TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
                Serial.println(error.f_str());
                TelnetPrint.println(error.f_str());
                Serial.println(F("[SPIFFS] Moving broken file to: /cfg.json.broken"));
                TelnetPrint.println(F("[SPIFFS] Moving broken file to: /cfg.json.broken"));
                if(SPIFFS.exists("/cfg.json.broken")) {
                    SPIFFS.remove("/cfg.json.broken");
                }
                SPIFFS.rename("/cfg.json","/cfg.json.broken");
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
            if (doc["hw"]["c1"] != -1) {
                chnr++;
                chout1 = doc["hw"]["c1"];
                pinMode(chout1, OUTPUT);
                digitalWrite(chout1, LOW);
            }
            if (doc["hw"]["c2"] != -1) {
                chnr++;
                chout2 = doc["hw"]["c2"];
                pinMode(chout2, OUTPUT);
                digitalWrite(chout2, LOW);
            }
            if (doc["hw"]["c3"] != -1) {
                chnr++;
                chout3 = doc["hw"]["c3"];
                pinMode(chout3, OUTPUT);
                digitalWrite(chout3, LOW);
            }
            if (doc["hw"]["c4"] != -1) {
                chnr++;
                chout4 = doc["hw"]["c4"];
                pinMode(chout4, OUTPUT);
                digitalWrite(chout4, LOW);
            }
            if (doc["hw"]["c5"] != -1) {
                chnr++;
                chout5 = doc["hw"]["c5"];
                pinMode(chout5, OUTPUT);
                digitalWrite(chout5, LOW);
            }
            TelnetPrint.begin();
            Serial.println("[SYS] Checking factory reset flag...");
            TelnetPrint.println("[SYS] Checking factory reset flag...");
            if (doc["meta"]["fact"] == "1") {
                Serial.println("[SYS] Factory reset flag found! Resetting...");
                TelnetPrint.println("[SYS] Factory reset flag found! Resetting...");
                SPIFFS.format();
                sysreboot(0);
                // stpwzd();
                //doc["meta"]["fact"] = 0; // factory mode off
                // fact = 0;
            }
            //analogWriteRange(doc["sw"]["anw"]);
            anw = doc["sw"]["anw"];
            diynr = doc["sw"]["dnr"];
            calib[0]= doc["sw"]["b0"];
            calib[1]= doc["sw"]["b1"];
            calib[2]= doc["sw"]["b2"];
            calib[3]= doc["sw"]["b3"];
            calib[4]= doc["sw"]["b4"];
            calib[5]= doc["sw"]["b5"];
            savetim = doc["sw"]["rbt"];
            fadetick = doc["sw"]["tik"];
            webui = doc["web"]|"dev"; //.as<String>();

            // const int chnr = doc["sw"]["chnr"];

            // dbg_cfgver=doc["meta"]["cfgver"];
            jsnld.close();
        }
        startsrv();
    }

    Serial.println("[SYS] Enabling IR receiver");
    TelnetPrint.println("[SYS] Enabling IR receiver");
    irrecv.enableIRIn();
    Serial.println("[SYS] Setting up OTA");
    TelnetPrint.println("[SYS] Setting up OTA");

    ArduinoOTA.onStart([]() {       //arduino ota example sketch
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }
        SPIFFS.end();
        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("[OTA] Start updating " + type);
        TelnetPrint.println("[OTA] Start updating " + type);
    });
    ArduinoOTA.begin();

    File last = SPIFFS.open("/last", "r");      //restore brightness
    TelnetPrint.println("open last");
    for(int i=0; i<=chnr; i++) {
        targetbr[i] = persistbr[i] = last.parseInt();
    }
    // raw=last.parseInt();
    last.close();

    Serial.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    TelnetPrint.print("[SYS] Boot completed.\n[SYS] ESP voltage: ");
    Serial.println(ESP.getVcc());
    TelnetPrint.println(ESP.getVcc());

}

void fadeto(int fadeval) {
    // fade api
//     if (fadeval < 0) {
//         fadeval = 0;
//     }
//     if (fadeval > 255) {
//         fadeval = 255;
//     }
//     //  lastbr[0] = fadeval;
//     for (; currentbr[0] > fadeval; currentbr[0]--) {    //decrease
//         awrite(0);
//         delay(2);
//     }

//     for (; currentbr[0] < fadeval; currentbr[0]++) {    //increase
//         awrite(0);
//         delay(2);
//     }

}

void diyedit(int num) {                         // saves the current brightness values in the specified slot then calls diyload() to load and applit.
    File jsndiy = SPIFFS.open("/diy.json", "r");
    TelnetPrint.println("open diy.json");
    DynamicJsonDocument doc(jsndiy.size()*2 + DIYTSIZE);
    if(jsndiy) {
        TelnetPrint.println("diyedit spiffs size:");
        //TelnetPrint.println(jsndiy.size());
        DeserializationError error = deserializeJson(doc, jsndiy);
        if (error) {
            Serial.print(F("[JSON] deserializeJson() failed: "));
            TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
            Serial.println(error.f_str());
            TelnetPrint.println(error.f_str());
            // jsndiy.close();
            // SPIFFS.remove("/diy.json");
            // File jsndiy = SPIFFS.open("/diy.json", "w");
            //return;
        }
    }
    jsndiy.close();
    File jsndiyw = SPIFFS.open("/diy.json", "w");
    TelnetPrint.println("open diy.json");
    if(jsndiyw) {
        TelnetPrint.println("diyedit json");
        TelnetPrint.println(num);
        TelnetPrint.println(diynr);
        if (num > 0 && num <= diynr) {
            TelnetPrint.println("diyedit");
            auto diy ="d"+std::to_string(num);
            TelnetPrint.println(diy.c_str());
            //std::string diy = "d" + std::to_string(num);
            //doc[diy]["all"] = lastbr[0];  ///////////////////////////////////////////////////////////////////
            for (int i = 0; i <= chnr; i++) {
                doc[diy]["c" + std::to_string(i)] = lastbr[i]; //////////////////////////////////////////////////////////
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
void diyload(int num) {                         // loads and applies values stored in the specified slot
    File jsndiy = SPIFFS.open("/diy.json", "r");
    TelnetPrint.println("open diy.json");
    TelnetPrint.println("diyload spiffs size:");
    TelnetPrint.println(jsndiy.size()*2+DIYTSIZE);
    DynamicJsonDocument doc(jsndiy.size()*2+DIYTSIZE);
    DeserializationError error = deserializeJson(doc, jsndiy);
    if (error) {
        Serial.print(F("[JSON] deserializeJson() failed: "));
        TelnetPrint.print(F("[JSON] deserializeJson() failed: "));
        Serial.println(error.f_str());
        TelnetPrint.println(error.f_str());
        //TelnetPrint.println(F("removed"));
        jsndiy.close();
        //SPIFFS.remove("/diy.json");
        return;
    }
    TelnetPrint.println("diyload json");
    TelnetPrint.println(jsndiy.size());
    TelnetPrint.println(irhold);
    if (irhold == 0 && num > 0 && num <= diynr) {
        //lastbr[0] = currentbr[0];
        auto diy ="d"+std::to_string(num);
        TelnetPrint.println(diy.c_str());
        TelnetPrint.println("diyload");
        //diy += std::to_string(num);
        for (int i = 0; i <= chnr; i++) {
            lastbr[i] = currentbr[i];
            Serial.println(lastbr[i]);
            //TelnetPrint.println(lastbr[i]);
            Serial.println(i);
            // TelnetPrint.println(i);
            targetbr[i]=doc[diy]["c" + std::to_string(i)];
            TelnetPrint.println(targetbr[i]);
        }
    }
    // power off
//     if (num == 0 && pwr == 0) {
//         for (int i = 1; i <= chnr; i++) {
// //            currentbr[i] = std::stoi(ini.get("chcfg").get("c"+std::to_string(i)));
//         }
//         //fadeto(doc["chcfg"]["pall"]);
//         pwr = 1;
//     } else if (num == 0 && pwr == 1) {
//         for (int i = 0; i <= chnr; i++) {
//             pwrstate[i] = currentbr[i];
//         }
//         fadeto(0);
//         pwr = 0;
//     }
    jsndiy.close();
    if (irhold == 1 && (millis() - irtime >= 2500)) {
        diyedit(num);
    }
}


// void irswitch(unsigned int swval ) {
//     // ir code switch, select channels, brightness
//     switch (swval) {
//     case irr: {
//         Serial.print(" irr \n");
//         TelnetPrint.print(" irr \n");
//         sel = 1;
//         break;
//     }
//     case irg: {
//         Serial.print(" irg \n");
//         TelnetPrint.print(" irg \n");
//         sel = 2;
//         break;
//     }
//     case irb: {
//         Serial.print(" irb \n");
//         TelnetPrint.print(" irb \n");
//         sel = 3;
//         break;
//     }
//     case irw: {
//         Serial.print(" irwhite \n");
//         TelnetPrint.print(" irwhite \n");
//         sel = 4;
//         break;
//     }
//     case irpoff: {
//         Serial.print(" irpon \n");
//         TelnetPrint.print(" irpon \n");
//         sel = 0;
//         break;
//     }
//     case irdiy1: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(1);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(1);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irdiy2: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(2);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(2);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irdiy3: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(3);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(3);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irdiy4: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(4);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(4);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irdiy5: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(5);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(5);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irdiy6: {
//         Serial.print(" irdiy1 \n");
//         TelnetPrint.print(" irdiy1 \n");
//         diyload(6);
//         if (irhold == 1 && millis() - irtime >= 2500) {
//             diyedit(6);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
//         }
//         break;
//     }
//     case irmax: {
//         Serial.print(" irmax \n");
//         TelnetPrint.print(" irmax \n");
//         fadeto(255);
//         break;
//     }
//     case irmin: {
//         Serial.print(" irmin \n");
//         TelnetPrint.print(" irmin \n");
//         fadeto(1);
//         break;
//     }
//     case irbru: {
//         Serial.print(" irbru \n");
//         TelnetPrint.print(" irbru \n");
//         currentbr[sel] = currentbr[sel] + 5;
//         if (sel != 0 && currentbr[sel] >= 100) {
//             currentbr[sel] = 100;
//         } else if (currentbr[0] >= 255) {
//             currentbr[0] = 255;
//         }
//         break;
//     }
//     case irbrd: {
//         Serial.print(" irbrd \n");
//         TelnetPrint.print(" irbrd \n");
//         currentbr[sel] = currentbr[sel] - 5;
//         if (currentbr[sel] <= 0) {
//             currentbr[sel] = 0;
//         }
//         break;
//     }
//     case irpon: {
//         Serial.print(" irpon \n");
//         TelnetPrint.print(" irpon \n");
//         diyload(0);
//         break;
//     }
//     }
// }

void ftable_ex(int fval ) {
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
        if(sel>=chnr) {
            sel=0;
        }
    }
    case 7: {

    }
    case 8: {

    }
    case 9: {

    }
    case 10: { //br +
        Serial.print(" irbru \n");
        TelnetPrint.print(" irbru \n");
        currentbr[sel] = currentbr[sel] + 5;
        if (sel != 0 && currentbr[sel] >= 100) {
            currentbr[sel] = 100;
        } else if (currentbr[0] >= 255) {
            currentbr[0] = 255;
        }
        break;
    }
    case 11: { //br -
        Serial.print(" irbrd \n");
        TelnetPrint.print(" irbrd \n");
        currentbr[sel] = currentbr[sel] - 5;
        if (currentbr[sel] <= 0) {
            currentbr[sel] = 0;
        }
        break;
    }
    case 12: { //irmax
        fadeto(255);
        break;
    }
    case 13: { //irmin
        Serial.print(" irmin \n");
        TelnetPrint.print(" irmin \n");
        fadeto(1);
        break;
    }

    // case irpon: {
    //     Serial.print(" irpon \n");                           //old kept as reference
    //     TelnetPrint.print(" irpon \n");
    //     diyload(0);
    //     break;
    // }

    case 101 ... 199 : { //diy shortcuts
        diyload(fval%100);
        if (irhold == 1 && millis() - irtime >= 2500) {
            diyedit(fval%100);                                                                        // FINISH THIS !!!!!!!!!!!!!!!!
        }
        break;
    }
    }
}

void loop() {
    auto loop_start=millis();
    ArduinoOTA.handle();
    dnsServer.processNextRequest();

    while (Serial.available() > 0) {
        switch (Serial.readStringUntil('\n').charAt(0)) {
        case 'r': {
            currentbr[1] = Serial.parseInt();
            break;
        }
        case 'g': {
            currentbr[2] = Serial.parseInt();
            break;
        }
        case 'b': {
            currentbr[3] = Serial.parseInt();
            break;
        }
        case 'w': {
            currentbr[4] = Serial.parseInt();
            break;
        }
        case 'a': {
            currentbr[0] = Serial.parseInt();
            break;
        }
        case 's': {
            for (int i = 0; i <= chnr; i++) {
                lastbr[i] = currentbr[i];
            }
            diyedit(Serial.parseInt());
            break;
        }
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
            //TelnetPrint.print(currentset[i]);
            //    // Serial.print("\\");
            //TelnetPrint.print("\\");
            // }
            break;
        }
        case 'f': {
            fadeto(Serial.parseInt());
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
    //ir interface
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

    // adc_val=analogRead(adc)*100;
    // if(adc_val!=ctrl.btnign){
    //     TelnetPrint.println("ADC changed:");
    //     TelnetPrint.println(adc_val*100);
    //     ftable_ex(btn_lookup(adc_val));
    // }

    // if(WiFi.status() == WL_CONNECTED) {
    //     wltim=millis();
    //     wlconf_started=false;
    // }
    // if(millis()-wltim >= wltimeout*1000 && wlconf_started==false) {
    //     Serial.println("[SYS] WLAN timeout.");
    //     TelnetPrint.println("[SYS] WLAN timeout.");
    //     wlconf2();
    //     wltim=millis();
    // }

// new fade thing
    if(fadetype==1) {                                           // new fade effect
        for(int i=0; i<=chnr; i++) {
            if(lasttarget[i]!=targetbr[i]) {
                speed[i]=(targetbr[i]-currentbr[i])/nms;
                TelnetPrint.print("Speed result: ");
                TelnetPrint.println(speed[i],6);
                TelnetPrint.print("Expected end: ");
                TelnetPrint.println(millis()+nms);
                finbrms=millis()+nms+100;
                lastbrms=millis();
                lasttarget[i]=targetbr[i];
            }
        }
        for(int i=0; i<=chnr; i++) {
            if(targetbr[i]!=currentbr[i]) {
                if(((speed[i]<0) && (currentbr[i]+speed[i]*(millis()-lastbrms)<targetbr[i]))||(speed[i]>0) && (currentbr[i]+speed[i]*(millis()-lastbrms)>targetbr[i])) {
                    currentbr[i]=targetbr[i];
                    TelnetPrint.println(millis());
                    continue;
                }
                currentbr[i]+=speed[i]*(millis()-lastbrms);
                brichanged=1;
            }
        }
        lastbrms=millis();
        if(millis()>=finbrms) {
            for(int i=0; i<=chnr; i++) {
                if(targetbr[i]!=currentbr[i]) {
                    TelnetPrint.print("Channel failed brightness: ");
                    TelnetPrint.print(i);
                    TelnetPrint.print(" had: ");
                    TelnetPrint.print(currentbr[i],6);
                    TelnetPrint.print(" expected: ");
                    TelnetPrint.println(targetbr[i],6);
                    currentbr[i]=targetbr[i];
                    brichanged=1;
                }
            }
        }

    } else if(fadetype==2) {                                    // classic fade effect
        if(micros() - lastfade >= fadetick) {
            for(int i=0; i<=chnr; i++) {
                if(targetbr[i]>currentbr[i]) {
                    currentbr[i]++;
                    brichanged=1;
                }
                if(targetbr[i]<currentbr[i]) {
                    currentbr[i]--;
                    brichanged=1;
                }
            }
            lastfade=micros();
        }
    }

    if(brichanged==1) {
        if (currentbr[0] !=0) {
            digitalWrite(atx, HIGH);
        } else {
            digitalWrite(atx, LOW);
        }
        awrite(anw);
        brichanged=0;
    }


//fade thing - old version
// if(micros() - lastfade >= fadetick) {
//         for(int i=0; i<=chnr; i++) {
//             if (currentbr[0] !=0) {
//                 digitalWrite(atx, HIGH);
//             } else {
//                 digitalWrite(atx, LOW);
//             }

//             if(targetbr[i]>currentbr[i]) {
//                 currentbr[i]++;
//             }
//             if(targetbr[i]<currentbr[i]) {
//                 currentbr[i]--;
//             }
//             awrite(anw);
//         }
//         lastfade=micros();
//     }


    if(millis() - cron1 >= 1000) {
        // Dir dir = SPIFFS.openDir ("/t/");
        // while (dir.next ()) {
        // TelnetPrint.println (dir.fileName());
        // SPIFFS.remove(dir.fileName());
        // }
        float vlmed=0;
        for(int i=1; i<=30; i++) {
            vlmed+=vloop[i];
        }
        TelnetPrint.print("Loop avg (ms): ");
        TelnetPrint.println(vlmed/30,3);
        //TelnetPrint.println(analogRead(adc));
        //TelnetPrint
        cron1=millis();
    }
    if(millis() - cron2 >= 10000) {
        TelnetPrint.print("ADC: ");
        TelnetPrint.println(analogRead(adc));
        cron2=millis();
    }

    if(start_noti!=0) {
        notifade(start_noti,ntik);
        start_noti=0;
    }
    if(millis() - lastsave >= savetim*1000) {
        if(needs_update()) {
            TelnetPrint.println("update /last on disk");
            TelnetPrint.println(savetim);
            File last = SPIFFS.open("/last", "w");
            for(int i=0; i<=chnr; i++) {
                last.println(targetbr[i]);
                persistbr[i]=targetbr[i];
            }
            last.close();
        }

        lastsave=millis();
    }
    //TelnetPrint.flush();
    digitalWrite(statusled, millis() % 1000 > 500 ? HIGH : LOW);

    //awrite(anw);//                              <<<<------------

    vl++;
    if(vl>30) {
        vl=0;
    }
    vloop[vl]=millis()-loop_start;
}

#include "ledScript.h"
#include <TelnetPrint.h>
#include <TaskSchedulerDeclarations.h>
#include "light.h"

// External dependencies from main.cpp
//extern int chnr;
//extern int targetbr[6];
//extern double nms;
extern Task tScriptR;

// Initialize the pointer to null
scriptinfo* activeScript = nullptr;

void scriptEnd() {
    if (!activeScript) return; // Prevent crashing if called while not running

    TelnetPrint.println("Script end");
    if (activeScript->file) {
        activeScript->file.close();
    }

    // Restore target brightness from the script's internal saved state
    for (int i = 0; i <= Light.getChCount(); i++) {
        Light.setBriSingle(i, activeScript->savedbr[i]);
    }

    tScriptR.disable();

    // Free the memory and reset pointer
    delete activeScript;
    activeScript = nullptr;
}

void scriptBegin(String path) {
    if(!SPIFFS.exists(path)) {
        TelnetPrint.println("Script file not found");
        return;
    }
    // If a script is already running, clean it up first
    if(activeScript) {
        scriptEnd();
    }

    // Dynamically allocate memory for the new script
    activeScript = new scriptinfo();
    activeScript->path = path;
    TelnetPrint.println("Script begin reading metadata for: " + path);

    // Save the current state BEFORE the script modifies it
    for (int i = 0; i <= Light.getChCount(); i++) {
        activeScript->savedbr[i] = Light.getBriSingle(i);
    }

    activeScript->file = SPIFFS.open(path, "r");
    String line = activeScript->file.readStringUntil('\n');
    switch (line[0]) {
        case 'N':
            activeScript->meta_chnr = line[1] - '0';
            break;
    }
    activeScript->running = true;
    tScriptR.setInterval(0); 
    tScriptR.restart();
    TelnetPrint.println("Script begin done");
}

void scriptRunner() {
    // Safety check
    if (!activeScript || !activeScript->running) return;

    if (tScriptR.getInterval() > 0) {
        tScriptR.setInterval(0);
    }

    String line = activeScript->file.readStringUntil('\n');
    TelnetPrint.println(line);

    switch (line[0]) {
    case 'F':
        Light.setFadeSpeed(line.substring(1).toInt());
        break;
    case 'S': {
        String s = line.substring(1);
        int i = 0;
        int j = 0;
        while (s.indexOf(',', i) != -1) {
            Light.setBriSingle(j, s.substring(i, s.indexOf(',', i)).toInt());
            i = s.indexOf(',', i) + 1;
            j++;
        }
        Light.setBriSingle(j, s.substring(i).toInt());
        break;
    }
    case 'C': {
        String s = line.substring(1);
        int i = s.indexOf(',');
        Light.setBriSingle(s.substring(0, i).toInt(), s.substring(i + 1).toInt());
        break;
    }
    case 'P': {
        TelnetPrint.print("P found, stored position: ");
        activeScript->pbegin = activeScript->file.position();
        TelnetPrint.println(activeScript->pbegin);
        break;
    }
    case 'Q': {
        TelnetPrint.println("Q found seeking to P");
        activeScript->file.seek(activeScript->pbegin);
        TelnetPrint.println("Seek to P done");
        break;
    }
    case 'L': {
        TelnetPrint.print("L found, storing position: ");
        String s = line.substring(1);
        activeScript->loopbegin = activeScript->file.position();
        TelnetPrint.println(activeScript->loopbegin);
        TelnetPrint.print("Loop count: ");
        activeScript->loopcount = s.toInt();
        TelnetPrint.println(activeScript->loopcount);
        break;
    }
    case 'O': {
        if(activeScript->loopcount > 0) {
            activeScript->file.seek(activeScript->loopbegin);
            activeScript->loopcount--;
            TelnetPrint.print("Loops remaining:");
            TelnetPrint.println(activeScript->loopcount);
            break;
        } else {
            TelnetPrint.print("End loop: ");
            TelnetPrint.println(activeScript->loopcount);
            break;
        }
    }
    case 'W': {
        TelnetPrint.print("W suspending until: ");
        String s = line.substring(1);
        tScriptR.setInterval(s.toInt()); // this should be  s.toInt() * 10
        break;
    }
    case 'R': {
        TelnetPrint.print("R random number: ");
        String s = line.substring(1);
        int i = s.indexOf(',');
        int j = s.indexOf(',', i + 1);
        int randomnr = random(s.substring(i + 1, j).toInt(), s.substring(j + 1).toInt());
        TelnetPrint.println(randomnr);
        Light.setBriSingle(s.substring(0, i).toInt(), randomnr);
        break;
    }

    default: {
        TelnetPrint.print("Unknown command: ");
        TelnetPrint.println(line);
        if (line == "") {
            scriptEnd();
        }
        break;
    }
    }
}
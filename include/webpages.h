/*
This file is huge and everything has to be gzipped
*/

#include <Arduino.h>

//mimetypes
constexpr const char* m_json = "application/json";
constexpr const char* m_html = "text/html";
constexpr const char* m_csv  = "text/csv";
constexpr const char* m_txt  = "text/plain";

//json answers
extern const char wlscan_json[] PROGMEM = R"rawliteral()rawliteral";
extern const char nfound_fail_json[] PROGMEM = R"rawliteral({"msg": "File not found or UA compression mismatch"})rawliteral";
extern const char nauth_fail_json[] PROGMEM = R"rawliteral({"msg": "Permission denied."})rawliteral";

//webpages
extern const char index_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Main menu</h1><hr><a href="/pick">Color Picker</a><br><a href="/pick2">Picker 2</a><br><a href="/diym">DIY Manager</a><br><a href="/thm">Theme Manager</a><hr><a href="/wlcfg">Configure WiFi</a><br><a href="/setup">Configure Hardware</a><br><a href="/rbt">Reboot</a><br><a href="/reset">Factory Reset</a><hr>)rawliteral";
extern const char wlan_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>WLAN configuration</h1><hr><br><form action="/wlset"method="post"><p>WLAN mode: <select name="wlm"><option value="1">AP only<option value="2">Client only<option value="3">AP+Client</select><p>Client SSID: <input name="ssid"><p>Client PSK : <input name="psk"type="password"><p>AP SSID: <input name="apssid"><p>AP PSK : <input name="appsk"type="password"><p>Reconnect timeout:<input name="t"type="number"max="3600">sec</p><br><input type="submit"value="Save "></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
extern const char reset_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Factory reset</h1><hr><p>Are you sure?<br><form action="/wipe"method="post"><input type="checkbox"value="1"name="meta_fact">Yes, wipe all settings</p><input type="submit"value="Save "align="right"></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
extern const char picker_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>Color picker</h1><hr><form action=/a1/shade method=post><p><input type="checkbox" name="p" checked>Persistent<br>CH 1: <input type=number name=ch1><br>CH 2: <input type=number name=ch2><br>CH 3: <input type=number name=ch3><br>CH 4: <input type=number name=ch4><br>CH 5: <input type=number name=ch5><br>Brightness: <input type=number name=ch0></p><input type=submit value="Save"></form><hr><br><a href=/><=Return to main menu</a><script>function sV() {fetch("/a1/stat").then(response=>response.json()).then(json=>{for(let i=0;i<=5;i++){document.getElementsByName(`ch${i}`)[0].value=json[`c${i}`];}});} window.onload=sV</script>)rawliteral";
extern const char setup_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>Controller Settings</h1><hr><form action=/save method=post>I/O:<p>CH 1: <select name=hw_c1></select><br>CH 2: <select name=hw_c2></select><br>CH 3: <select name=hw_c3></select><br>CH 4: <select name=hw_c4></select><br>CH 5: <select name=hw_c5></select><br>ATX/psu_en: <select name=hw_p></select><br>Status LED: <select name=hw_st></select><hr>Misc:<br>PWM bit:<input type=number name=sw_anw><br>DIY nr:<input type=number name=sw_dnr><br>Save status every<input type=number name=sw_rbt>sec<br>Fade ticks:<input type=number name=sw_tik><hr>Brightness correction:<br>Gamma:<input type=number name=sw_b0><br>CH 1:<input type=number name=sw_b1><br>CH 2:<input type=number name=sw_b2><br>CH 3:<input type=number name=sw_b3><br>CH 4:<input type=number name=sw_b4><br>CH 5:<input type=number name=sw_b5></p><input type=submit value="Save "></form><h6><i>Notes:</i><br>'*' - not recommended<br>'-1' - Disabled<br>'Now' - current value</h6><hr><br><a href=/><=Return to main menu</a><script>window.onload=function(){fetch('/a1/cfg').then(response=> response.json()).then(data=>{for (const sectionKey in data){const section=data[sectionKey]; for (const key in section){const value=section[key]; const select=document.getElementsByName(`${sectionKey}_${key}`)[0]; if (select.tagName==='SELECT'){select.innerHTML=`<option value="${value}" selected>Now: ${value}</option> <option value="-1">Disabled</option><option value="0">0*</option><option value="2">2*</option><option value="4">4</option><option value="5">5</option><option value="12">12</option><option value="13">13</option><option value="14">14</option><option value="15">15</option><option value="16">16*</option>`;}else if(select.tagName==='INPUT'){select.value=value;}}}});};</script>)rawliteral";
extern const char diymanager_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><center><h1>DIY Manager</h1><hr><p><form action=/diy method=post><br>Load DIY: <input name=dl type=number> <button type=submit>Load</button></form><form action=/diy method=post><br>Save shade as DIY: <input name=de type=number id=save> <button type=submit>Save</button></form><hr><br><a href="/"><=Return to main menu</a>)rawliteral";
extern const char picker2_html[] PROGMEM = R"rawliteral(<html><meta content="width=device-width,initial-scale=1"name=viewport><link href=a.css rel=stylesheet><style>.slid{display:flex;flex-wrap:wrap;justify-content:center;align-items:center;margin-top:20px}.s1{width:400px;height:30px}.s2{transform:rotate(270deg);height:100px;margin-bottom:28px;margin-top:28px}</style><h2>Color Picker 2</h2><div class=slid></div><script>window.onload=function(){Promise.all([fetch("/a1/cfg").then((e=>e.json())),fetch("/stat").then((e=>e.json()))]).then((e=>{const n=e[0].hw,t=document.querySelector(".slid");if(t)for(const l in e[1])"c0"==l?t.innerHTML+=`<input type="range" class="s1" id="${l}" min="0" max="${Math.pow(2,e[0].sw.anw)}" value="${e[1][l]}"/>`:"-1"!==n[l]?t.innerHTML+=`<input type="range" class="s2" id="${l}" min="0" max="255" value="${e[1][l]}"/>`:t.innerHTML+=`<input type="range" class="s2" id="${l}"disabled/>`,document.getElementById(l).addEventListener("input",upres)}))};let timerId=null;function upres(){const e=document.getElementById("c0"),n=document.getElementById("c1"),t=document.getElementById("c2"),l=document.getElementById("c3"),c=document.getElementById("c4"),u=document.getElementById("c5");let a=null,d=null,o=null,i=null,s=null,m=null;e.value===a&&n.value===d&&t.value===o&&l.value===i&&c.value===s&&u.value===m||(a=e.value,d=n.value,o=t.value,i=l.value,s=c.value,m=u.value,timerId&&clearTimeout(timerId),timerId=setTimeout((()=>{const e=new XMLHttpRequest;e.open("POST","/raw",!0),e.setRequestHeader("Content-Type","application/x-www-form-urlencoded"),e.send(`c0=${a}&c1=${d}&c2=${o}&c3=${i}&c4=${s}&c5=${m}`),console.log(`c0=${a}&c1=${d}&c2=${o}&c3=${i}&c4=${s}&c5=${m}`)}),100))}</script></body></html>)rawliteral";
extern const char success_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Success!</h1><hr>Request completed successfully.<hr><br><a href="/"><=Return to main menu</a>)rawliteral";
extern const char error_html[] PROGMEM = R"rawliteral(<html><body bgcolor=#800><h1>Error!<hr>Check DevTools for http code or more details.)rawliteral";
extern const char a_css[] PROGMEM = R"rawliteral(html{max-width:35ch;margin:auto;font-size:150%;color:orange;}a:link,a:visited,a:hover,a:active{color:#add8e6; text-decoration:none;}h1{color:#90ee90;text-decoration:none;}body{background-color:#1E1E1E;})rawliteral";
extern const char theme_html[] PROGMEM = R"rawliteral(<meta content="width=device-width,initial-scale=1"name="viewport"><link href="a.css"rel="stylesheet"><center><h1>Theme Upload</h1><hr><form method="POST" action="/a1/fup" enctype="multipart/form-data"><input type="file" name="file"><br><input type="submit" value="Upload"></form>)rawliteral";

extern const char snf_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Smart Network Features</title>
    <link href="/w/ts/css.css" rel="stylesheet">
    <style>
        .hidden { display: none !important; }
        table { width: 100%; margin-bottom: 1rem; }
        .status { padding: 1rem; border-radius: 5px; text-align: center; font-weight: bold; margin-top: 1rem; }
        .success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .badge { padding: 0.2rem 0.5rem; border-radius: 3px; font-size: 0.8em; color: white; }
        .bg-green { background: #28a745; }
        .bg-red { background: #dc3545; }
        .bg-gray { background: #6c757d; }
        dialog { max-width: 500px; width: 90%; }
        .flex-row { display: flex; justify-content: space-between; align-items: center; }
    </style>
</head>
<body>
<header>
    <h1>Smart Network Features</h1>
    <nav>
        <ul>
            <li><a href="/">Home</a></li>
            <li><a href="/set.htm">System Settings</a></li>
            <li><a href="/snf.htm">Network Tracking</a></li>
        </ul>
    </nav>
</header>

<main>
    <fieldset>
        <legend>Live Tracker Status</legend>
        <table>
            <thead>
                <tr>
                    <th>Device</th>
                    <th>State</th>
                    <th>Last Seen</th>
                </tr>
            </thead>
            <tbody id="statusBody">
                <tr><td colspan="3">Loading live status...</td></tr>
            </tbody>
        </table>
        <button type="button" onclick="fetchStatus()">⟳ Refresh Status</button>
    </fieldset>

    <details>
        <summary style="font-weight: bold; font-size: 1.1em; cursor: pointer; padding: 0.5rem 0;">📡 Network Discovery Scanner</summary>
        <div style="margin-top: 1rem;">
            <div class="flex-row">
                <button type="button" onclick="startScan()">Start Full Subnet Scan</button>
                <small id="scanState">Idle</small>
            </div>
            <table>
                <thead>
                    <tr>
                        <th>IP / MAC / Host</th>
                        <th>Action</th>
                    </tr>
                </thead>
                <tbody id="discoveryBody">
                    <tr><td colspan="2">Click scan to discover devices...</td></tr>
                </tbody>
            </table>
        </div>
    </details>

    <form id="snfForm" onsubmit="event.preventDefault(); saveConfig();">
        <fieldset>
            <legend>Tracked Devices Configuration</legend>
            <table>
                <thead>
                    <tr>
                        <th>Identifiers</th>
                        <th>Ping Config</th>
                        <th>Actions (Found / Lost)</th>
                        <th>Edit</th>
                    </tr>
                </thead>
                <tbody id="trackedBody">
                    </tbody>
            </table>
            
            <button type="button" onclick="openDeviceDialog(-1)">+ Add New Tracked Device</button>
            <br><br>
            <center><button type="submit">Save & Apply Rules</button></center>
            <div id="cfgStatus" class="status hidden"></div>
        </fieldset>
    </form>
</main>

<dialog id="deviceDialog">
    <h2 id="dialogTitle">Edit Device</h2>
    <form method="dialog" onsubmit="saveDeviceToMemory();">
        <input type="hidden" id="dev_id">
        
        <fieldset>
            <legend>Identifiers (At least one required)</legend>
            <p>
                <label>MAC Address:</label> <input type="text" id="dev_m" placeholder="e.g. AA:BB:CC:DD:EE:FF"><br>
                <small>Recommended for DHCP devices.</small>
            </p>
            <p>
                <label>IP Address:</label> <input type="text" id="dev_i" placeholder="e.g. 192.168.1.50"><br>
                <small>Leave blank unless device has a strict Static IP.</small>
            </p>
            <p><label>Hostname:</label> <input type="text" id="dev_h" placeholder="e.g. Johns-iPhone"></p>
        </fieldset>

        <fieldset>
            <legend>Tracking Method</legend>
            <p><label>Interval (ms):</label> <input type="number" id="dev_p" placeholder="5000" min="0"></p>
            <p>
                <label>Protocol:</label>
                <select id="dev_pt">
                    <option value="0">0 - None / Passive (DHCP & Cache only)</option>
                    <option value="1">1 - Active ARP (Cache Query)</option>
                    <option value="2">2 - Active ICMP (AsyncPing)</option>
                    <option value="3">3 - Active HTTP (Healthcheck)</option>
                </select>
                <small>Set interval to 0 to force Passive tracking.</small>
            </p>
        </fieldset>

        <fieldset>
            <legend>Trigger Actions</legend>
            <p><strong>On Found:</strong></p>
            <div class="flex-row">
                <select id="dev_f_t">
                    <option value="">None</option>
                    <option value="s">Script Path</option>
                    <option value="d">DIY Slot</option>
                    <option value="p">Power (1=On, 0=Off)</option>
                </select>
                <input type="text" id="dev_f_v" placeholder="Value...">
            </div>
            
            <p><strong>On Lost:</strong></p>
            <div class="flex-row">
                <select id="dev_l_t">
                    <option value="">None</option>
                    <option value="s">Script Path</option>
                    <option value="d">DIY Slot</option>
                    <option value="p">Power (1=On, 0=Off)</option>
                </select>
                <input type="text" id="dev_l_v" placeholder="Value...">
            </div>
        </fieldset>
        
        <div class="flex-row" style="margin-top: 1rem;">
            <button type="button" onclick="document.getElementById('deviceDialog').close()" style="background: #6c757d;">Cancel</button>
            <button type="submit">Done</button>
        </div>
    </form>
</dialog>

<script>
    let snfData = [];
    let scanPollInterval = null;

    // --- Initialization ---
    window.onload = () => {
        fetchConfig();
        fetchStatus();
    };

    function showMsg(id, text, isSuccess) {
        const el = document.getElementById(id);
        el.textContent = text;
        el.className = `status ${isSuccess ? 'success' : 'error'}`;
        setTimeout(() => el.classList.add('hidden'), 4000);
    }

    // --- 1. Tracked Devices Config (CRUD) ---
    function fetchConfig() {
        fetch('/a1/getsnf')
            .then(res => res.json())
            .then(data => {
                snfData = Array.isArray(data) ? data : [];
                renderTrackedTable();
            })
            .catch(err => {
                console.error("Failed to load SNF config", err);
                snfData = [];
                renderTrackedTable();
            });
    }

    function renderTrackedTable() {
        const tbody = document.getElementById('trackedBody');
        tbody.innerHTML = '';
        
        if (snfData.length === 0) {
            tbody.innerHTML = '<tr><td colspan="4">No devices tracked. Add one below.</td></tr>';
            return;
        }

        snfData.forEach((dev, index) => {
            const idents = [dev.i, dev.m, dev.h].filter(x => x).join('<br>');
            
            // Translate the Enum for the UI
            let ptStr = "Passive";
            if (dev.pt == 1) ptStr = "ARP";
            else if (dev.pt == 2 || dev.pt === undefined) ptStr = "ICMP"; // Defaults to ICMP
            else if (dev.pt == 3) ptStr = "HTTP";
            
            const method = dev.p == 0 ? "Passive" : `${dev.p}ms (${ptStr})`;
            
            // Format actions for display
            const fmtAct = (act) => {
                if(!act) return "None";
                if(act[0]==='s') return `Script: ${act.substring(1)}`;
                if(act[0]==='d') return `DIY: ${act.substring(1)}`;
                if(act[0]==='p') return `Power: ${act.substring(1)}`;
                return act;
            };

            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td><small>${idents || '<i>Empty</i>'}</small></td>
                <td><small>${method}</small></td>
                <td>
                    <small><b>F:</b> ${fmtAct(dev.f)}<br><b>L:</b> ${fmtAct(dev.l)}</small>
                </td>
                <td>
                    <button type="button" onclick="openDeviceDialog(${index})">✎</button>
                    <button type="button" class="bg-red" style="color:white; border:none;" onclick="deleteDevice(${index})">X</button>
                </td>
            `;
            tbody.appendChild(tr);
        });
    }

    function saveConfig() {
        fetch('/a1/setsnf', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(snfData)
        })
        .then(res => res.ok ? showMsg('cfgStatus', 'Rules saved & applied!', true) : Promise.reject())
        .catch(() => showMsg('cfgStatus', 'Failed to save rules.', false));
    }

    function deleteDevice(index) {
        if(confirm("Remove this device?")) {
            snfData.splice(index, 1);
            renderTrackedTable();
        }
    }

    // --- 2. Dialog Editor Logic ---
    function parseActionStr(actStr) {
        if (!actStr) return { t: '', v: '' };
        return { t: actStr[0], v: actStr.substring(1) };
    }

    function openDeviceDialog(index, prefillIp = "", prefillMac = "", prefillHost = "") {
        const d = document.getElementById('deviceDialog');
        document.getElementById('dialogTitle').innerText = index === -1 ? "New Tracked Device" : "Edit Device";
        document.getElementById('dev_id').value = index;

        if (index === -1) {
            // New or Prefilled from Discovery
            document.getElementById('dev_i').value = prefillIp; 
            document.getElementById('dev_m').value = prefillMac;
            document.getElementById('dev_h').value = prefillHost;
            document.getElementById('dev_p').value = 5000;
            document.getElementById('dev_pt').value = "2"; // Default to Active ICMP
            document.getElementById('dev_f_t').value = ""; document.getElementById('dev_f_v').value = "";
            document.getElementById('dev_l_t').value = ""; document.getElementById('dev_l_v').value = "";
        } else {
            // Edit existing
            const dev = snfData[index];
            document.getElementById('dev_i').value = dev.i || "";
            document.getElementById('dev_m').value = dev.m || "";
            document.getElementById('dev_h').value = dev.h || "";
            document.getElementById('dev_p').value = dev.p !== undefined ? dev.p : 5000;
            document.getElementById('dev_pt').value = dev.pt !== undefined ? dev.pt : 2;
            
            const fA = parseActionStr(dev.f);
            document.getElementById('dev_f_t').value = fA.t; document.getElementById('dev_f_v').value = fA.v;
            
            const lA = parseActionStr(dev.l);
            document.getElementById('dev_l_t').value = lA.t; document.getElementById('dev_l_v').value = lA.v;
        }
        d.showModal();
    }

    function saveDeviceToMemory() {
        const idx = parseInt(document.getElementById('dev_id').value);
        
        // Helper to compile the action string
        const compAct = (tId, vId) => {
            const t = document.getElementById(tId).value;
            const v = document.getElementById(vId).value.trim();
            return (t && v) ? t + v : undefined;
        };

        const newDev = {};
        const i = document.getElementById('dev_i').value.trim(); if(i) newDev.i = i;
        const m = document.getElementById('dev_m').value.trim(); if(m) newDev.m = m;
        const h = document.getElementById('dev_h').value.trim(); if(h) newDev.h = h;
        
        newDev.p = parseInt(document.getElementById('dev_p').value) || 0;
        newDev.pt = parseInt(document.getElementById('dev_pt').value) || 0;
        
        const f = compAct('dev_f_t', 'dev_f_v'); if(f) newDev.f = f;
        const l = compAct('dev_l_t', 'dev_l_v'); if(l) newDev.l = l;

        if (idx === -1) {
            snfData.push(newDev);
        } else {
            snfData[idx] = newDev;
        }
        renderTrackedTable();
    }

    // --- 3. Live Status (snfstat) ---
    function fetchStatus() {
        fetch('/a1/snfstat')
            .then(res => res.json())
            .then(data => {
                const tbody = document.getElementById('statusBody');
                tbody.innerHTML = '';
                
                if (!data.tracked || data.tracked.length === 0) {
                    tbody.innerHTML = '<tr><td colspan="3">No active tracking rules loaded in RAM.</td></tr>';
                    return;
                }

                data.tracked.forEach(d => {
                    const idents = [d.i, d.m, d.h].filter(x => x).join(' / ');
                    const stateBadge = d.present 
                        ? `<span class="badge bg-green">Found</span>` 
                        : `<span class="badge bg-gray">Lost</span>`;
                    
                    const timeStr = d.sec_ago >= 0 ? `${d.sec_ago}s ago` : "Never";

                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td><small>${idents}</small></td>
                        <td>${stateBadge}</td>
                        <td><small>${timeStr}</small></td>
                    `;
                    tbody.appendChild(tr);
                });
            })
            .catch(err => console.error("Failed to fetch live status", err));
    }

    // --- 4. Network Discovery (getsnfd) ---
    function startScan() {
        document.getElementById('scanState').innerText = "Scanning Subnet (takes ~13s)...";
        fetch('/a1/getsnfd?scan=1')
            .then(() => {
                if(scanPollInterval) clearInterval(scanPollInterval);
                scanPollInterval = setInterval(pollScan, 1500);
            });
    }

    function pollScan() {
        fetch('/a1/getsnfd')
            .then(res => res.json())
            .then(data => {
                const tbody = document.getElementById('discoveryBody');
                tbody.innerHTML = '';

                if (data.devices && data.devices.length > 0) {
                    data.devices.forEach(d => {
                        const tr = document.createElement('tr');
                        // Notice the empty string '' passed instead of d.i for the IP address
                        tr.innerHTML = `
                            <td><small>IP: ${d.i || '-'}<br>MAC: ${d.m || '-'}<br>Host: ${d.h || '-'}</small></td>
                            <td><button type="button" onclick="openDeviceDialog(-1, '', '${d.m||''}', '${d.h||''}')">+ Track</button></td>
                        `;
                        tbody.appendChild(tr);
                    });
                } else {
                    tbody.innerHTML = '<tr><td colspan="2">No devices found.</td></tr>';
                }

                if (!data.scan) {
                    clearInterval(scanPollInterval);
                    document.getElementById('scanState').innerText = "Scan Complete.";
                }
            });
    }
</script>
</body>
</html>)rawliteral";

extern const char setup2_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Controller Settings</title>
    <link href="/w/ts/css.css" rel="stylesheet">
    <style>
        .hidden { display: none !important; }
        <!-- .flex-row { display: flex; align-items: center; justify-content: space-between; gap: 1rem; } -->
        table { width: 100%; }
        <!-- td input { margin: 0; width: 100%; } -->
        .status { padding: 1rem; border-radius: 5px; text-align: center; font-weight: bold; }
        .success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
    </style>
</head>
<body>
<header>
    <h1>Controller Settings</h1>
    <nav>
        <ul>
            <li><a href="/">Home</a></li>
            <li><a href="/pk1.htm">Picker</a></li>
            <li><a href="/pk2.htm">Picker2</a></li>
            <li><a href="/dm.htm">DIY Manager</a></li>
            <li><a href="/thm.htm">Themes</a></li>
            <li><a href="/set.htm">Settings</a></li>
        </ul>
    </nav>
</header>

<main>
    <form id="sysForm" onsubmit="event.preventDefault(); saveSystemConfig();">
        <fieldset>
            <legend>System</legend>
                <table id="channelTable">
				<caption>Basic Outputs</caption>
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>GPIO Pin</th>
                            <th>Calibration</th>
                            <th></th>
							<th></th>
                        </tr>
                    </thead>
                    <tbody id="channelBody">
                        </tbody>
                </table>
                <button type="button" onclick="addChannelRow('', '')">+ Add Channel</button>
            <fieldset>
                <legend>Advanced Outputs</legend>
                <p>
                    <label>
                        <input type="checkbox" id="hw_ld" value="1">
                        High Default (Starts with all channels ON and fades OFF)<br>
						<small>This is a workaround for hardware where channels are default ON and normal mode would cause a quick flash at boot</small>
                    </label>
                </p>
                <p>
                    <label for="hw_p">PSU Enable Pin <small>For 2 stage power supplies.</small></label>
                    <input type="number" id="hw_p" min=0>
                </p>
                <p>
                    <label for="hw_st">Status LED</label>
                    <input type="number" id="hw_st" placeholder="Board default" min=0>
                </p>
            </fieldset>

            <fieldset>
                <legend>Inputs</legend>
                <p>
                    <label for="hw_kp">Analogue keypad</label>
                    <input type="number" id="hw_kp" min=0 value=17 ><br>
					<small>For esp8266: A0 is 17, for other platform refer to official documentation.</small>
                </p>
                <p>
                    <label for="hw_ir">IR Receiver</label>
                    <input type="number" id="hw_ir" min=0>
					
                </p>
            </fieldset>

            <fieldset>
                <legend>Misc&amp;Advanced</legend>
                
                <p>
                    <label for="sw_ftk">Transitions</label>
                    <input type="number" id="sw_ftk" min=250 max=3600000 value=2000> ms
                </p>
                <p>
                    <label for="sw_as">Autosave Interval</label>
                    <input type="number" id="sw_as" value=30 min=0 max=86400> sec
                </p>
                
                <p>
                    <label for="sw_web">Webserver</label>
                    <select id="sw_web" onchange="updateVisibility()">
                        <option value="1">HTTP Only</option>
                        <option value="2">HTTP + HTTPS</option>
                        <option value="3">HTTPS Only</option>
						<option value="0">Disabled</option>
                    </select>
					<dialog id="ww">
					<h2>Warning!</h2>
					<p>If you disable the webserver and do not have any hardware reset button or firmware flashing method you can permanently lock yourself out of the controller with NO WAY BACK IN!<br>3<sup>rd</sup> Party Apps will also stop working.<br>Furthermore: there will be no way for you to reconfigure the WiFi, even if fallback Hotspot is on!<br><small>A better idea is to restrict access from Users &amp; Restrictions.</small></p>
					<center><button type="button" onclick="document.getElementById('ww').close()">There is a hardware reset method or I agree with the above risk.</button></center>
					</dialog>
                </p>
                <p>
                    <label for="sw_wui">WebUI Theme</label>
                    <input type="text" id="sw_wui">
                </p>

                <p>
                    <label><input type="checkbox" id="sw_snf" value="1"> Enable Smart Network Features</label><br>
                    <label><input type="checkbox" id="sw_md" value="1"> Enable mDNS Discovery</label><br>
                    <label><input type="checkbox" id="sw_sr" value="1"> Enable Serial Interface</label><br>
                    <label><input type="checkbox" id="sw_tl" value="1"> Enable Telnet</label><br>
                    <label><input type="checkbox" id="sw_ua" value="1"> Enable Users &amp; Restrictions</label><br>
					<label><input type="checkbox" id="sw_au" value="1" onchange="updateVisibility()"> Automatic software upgrades</label>
                </p>
                <p id="div_ota" class="hidden">
                    <label for="sw_ota">OTA Server</label>
                    <input type="url" id="sw_ota">
                </p>
                <p>
                    <label for="sw_nt">Network Time</label>
                    <select id="sw_nt" onchange="updateVisibility()">
                        <option value="0">Disabled</option>
                        <option value="1">Specified Server</option>
                        <option value="2">Auto (DHCP First, Specified Fallback)</option>
                        <option value="3">Auto (Specified First, DHCP Fallback)</option>
                    </select>
                </p>
                <p id="div_ntp" class="hidden">
                    <label for="sw_ntp">NTP Server</label>
                    <input type="text" id="sw_ntp" placeholder="pool.ntp.org">
                </p>
                <p id="div_ntz" class="hidden">
                    <label for="sw_ntz">Timezone</label>
                    <select id="sw_ntz">
                    </select>
                </p>
            </fieldset>
            
            <center><button type="submit">Save System Configuration</button></center>
            <div id="sysStatus" class="status hidden"></div>
        </fieldset>
    </form>

    <form id="wlanForm" onsubmit="event.preventDefault(); saveWlan();">
        <fieldset>
            <legend>WiFi &amp; Connectivity</legend>

            <p>
                <label for="wlm">Operation Mode</label>
                <select id="wlm" onchange="updateVisibility()">
                    <option value="1">Hotspot</option>
                    <option value="2" selected>Join Network</option>
                    <option value="3">Hotspot + Join Network</option>
                </select>
				<dialog id="snfw">
				<h2>Warning!</h2>
				<p>Smart Network Features cannot work reliably while the controller is transmitting it's own WiFi network.</p>
				<center><button type="button" onclick="document.getElementById('snfw').close()">Close</button></center>
				</dialog>
            </p>

            <div id="sta-section">
                <div class="flex-row">
                    <label style="margin:0;"><strong>Join Network</strong></label>
                    <small id="scan-status" style="color: var(--accent);">Scanning...</small>
                </div>
                <p>
                    <select id="scanned_ssids" onchange="handleNetworkSelection()"></select>
                    <input type="text" id="ssid" class="hidden" placeholder="Network name...">
                    <button type="button" onclick="fetchWlanNetworks()">⟳ Rescan</button>
                </p>
                <p id="psk-group">
                    <label for="psk">Password</label>
                    <input type="password" id="psk" placeholder="Leave blank if open">
                </p>
                <p>
                    <label for="t">Fallback Timeout</label>
                    <input type="number" id="t" value="5" min=0 max=3600> minutes<br>
                    <small>Time before fallback to Hotspot mode if connection fails (0=disable).</small>
                </p>
            </div>

            <div id="ap-section" class="hidden">
                <p><strong>Hotspot Settings</strong></p>
                <p>
                    <label for="apssid">Hotspot Name (SSID)</label>
                    <input type="text" id="apssid" placeholder="esp_LightCtl">
                </p>
                <p>
                    <label for="appsk">Hotspot Password</label>
                    <input type="text" id="appsk" placeholder="987654321">
                </p>
            </div>

            <center><button type="submit">Apply & Reboot WiFi</button></center>
            <div id="wlStatus" class="status hidden"></div>
        </fieldset>
    </form>
</main>

<script>
    let fullConfig = {};
    let loadedTimezone = "";

    // --- Initialization & Data Fetching ---
    window.onload = () => {
        fetchConfig();
		fetchTimezones();
        fetchWlanNetworks();
    };

    function fetchTimezones() {
        fetch('/a1/gettzdata')
            .then(res => res.text())
            .then(csv => {
                const tzSelect = document.getElementById('sw_ntz');
                <!-- tzSelect.innerHTML = '<option value="">-- Select Timezone --</option>'; -->
                
                const lines = csv.split('\n');
                lines.forEach(line => {
                    const match = line.match(/"([^"]+)","([^"]+)"/);
                    if (match) {
                        const opt = document.createElement('option');
                        opt.value = match[2]; // POSIX string
                        opt.text = match[1];  // Region name
                        tzSelect.appendChild(opt);
                    }
                });
                if (loadedTimezone) tzSelect.value = loadedTimezone;
            })
            .catch(err => console.error("Could not load timezones", err));
    }

    function fetchConfig() {
        fetch('/a1/config')
            .then(res => res.json())
            .then(data => {
                fullConfig = data || {};
                populateForm(fullConfig);
            })
            .catch(err => console.error("Failed to load config", err));
    }

    // --- Form Population & UI Logic ---
    function populateForm(data) {
        if (data.hw) {
            setVal('hw_p', data.hw.p);
            setVal('hw_st', data.hw.st);
            setVal('hw_kp', data.hw.kp);
            setVal('hw_ir', data.hw.ir);
            setCheckbox('hw_ld', data.hw.ld);

            if (data.hw.c && Array.isArray(data.hw.c)) {
                document.getElementById('channelBody').innerHTML = "";
                data.hw.c.sort((a, b) => a[0] - b[0]).forEach(ch => addChannelRow(ch[1], ch[2]));
            }
        }

        if (data.sw) {
            setVal('sw_as', data.sw.as);
            setVal('sw_ftk', data.sw.ftk);
            setVal('sw_web', data.sw.web);
            setVal('sw_wui', data.sw.wui);
            setVal('sw_nt', data.sw.nt);
            setVal('sw_ntp', data.sw.ntp);
            setVal('sw_ota', data.sw.ota);
            
            setCheckbox('sw_snf', data.sw.snf);
            setCheckbox('sw_au', data.sw.au);
            setCheckbox('sw_md', data.sw.md);
            setCheckbox('sw_sr', data.sw.sr);
            setCheckbox('sw_tl', data.sw.tl);
            setCheckbox('sw_ua', data.sw.ua);

            if (data.sw.ntz) {
                loadedTimezone = data.sw.ntz;
                const tzSelect = document.getElementById('sw_ntz');
                if (tzSelect.options.length > 1) tzSelect.value = loadedTimezone;
            }
        }
        updateVisibility();
    }

    function setVal(id, val) {
        if (val !== undefined && val !== null) document.getElementById(id).value = val;
    }

    function setCheckbox(id, val) {
        if (val !== undefined && val !== null) document.getElementById(id).checked = (val === 1);
    }

    function updateVisibility() {
        // Auto Update
        const auEnabled = document.getElementById('sw_au').checked;
        document.getElementById('div_ota').classList.toggle('hidden', !auEnabled);

        // Network Time
        const ntMode = document.getElementById('sw_nt').value;
        const timeHidden = (ntMode === "0");
        document.getElementById('div_ntp').classList.toggle('hidden', timeHidden);
        document.getElementById('div_ntz').classList.toggle('hidden', timeHidden);

        // WLAN Modes
        const wlm = document.getElementById('wlm').value;
        document.getElementById('sta-section').classList.toggle("hidden", wlm === "1");
        document.getElementById('ap-section').classList.toggle("hidden", wlm === "2");
		(wlm === "1" || wlm === "3") && document.getElementById('snfw').showModal();
		
		// Webserver Warning
		(document.getElementById('sw_web').value === "0") && document.getElementById('ww').showModal();
    }

    // --- Channel Table ---
    function addChannelRow(pin, calib) {
        const tbody = document.getElementById('channelBody');
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td class="ch-id"></td>
            <td><input type="number" class="ch-pin" value="${pin}" placeholder="Pin" required></td>
            <td><input type="number" class="ch-cal" value="${calib}" placeholder="0-255" required></td>
			<td><button type="button" onclick="this.closest('tr').remove(); updateChannelIDs();">Test</button></td>
            <td><button type="button" style="background:#dc3545; border:none; padding:5px; border-radius:3px;" onclick="this.closest('tr').remove(); updateChannelIDs();">X</button></td>
        `;
        tbody.appendChild(tr);
        updateChannelIDs();
    }

    function updateChannelIDs() {
        document.querySelectorAll('#channelBody tr').forEach((row, index) => {
            row.querySelector('.ch-id').textContent = index + 1;
        });
    }

    // --- WLAN Scanning Logic ---
    function fetchWlanNetworks() {
        const status = document.getElementById('scan-status');
        const select = document.getElementById('scanned_ssids');
                
        fetch('/a1/wlist')
            .then(res => {
                if (res.status === 202) throw new Error('SCAN_IN_PROGRESS');
                if (!res.ok) throw new Error('SERVER_ERROR');
                return res.json();
            })
            .then(networks => {
                status.textContent = "";
                select.innerHTML = '<option value="">⚙️ Other / Hidden Network...</option>';
                networks.forEach(net => {
                    const isOpen = (net.e === 7);
                    const icon = isOpen ? "🔓" : "🔐";
                    let quality = Math.min(Math.max(2 * (net.p + 100), 0), 100) + "%";
                    
                    const opt = document.createElement('option');
                    opt.value = net.n;
                    opt.dataset.open = isOpen;
                    opt.textContent = `${icon} ${net.n} (${quality})`;
                    select.appendChild(opt);
                });
            })
            .catch(err => {
                if (err.message === 'SCAN_IN_PROGRESS') {
                    status.textContent = "Scanning...";
                    setTimeout(fetchWlanNetworks, 1000);
                } else {
                    status.textContent = "Scan failed.";
                }
            });
    }

    function handleNetworkSelection() {
        const select = document.getElementById('scanned_ssids');
        const customInput = document.getElementById('ssid');
        const pskGroup = document.getElementById('psk-group');
        const selectedOption = select.options[select.selectedIndex];

        if (select.value === "") {
            customInput.classList.remove('hidden');
            customInput.value = "";
            pskGroup.classList.remove('hidden');
        } else {
            customInput.classList.add('hidden');
            customInput.value = select.value;
            
            if (selectedOption && selectedOption.dataset.open === "true") {
                pskGroup.classList.add('hidden');
                document.getElementById('psk').value = "";
            } else {
                pskGroup.classList.remove('hidden');
            }
        }
    }

    // --- Save Actions ---
    function saveSystemConfig() {
        // Construct HW
        const hw = { c: [] };
        const addNum = (obj, key, domId) => {
            const val = document.getElementById(domId).value;
            if (val !== "") obj[key] = parseInt(val);
        };
        const addStr = (obj, key, domId) => {
            const val = document.getElementById(domId).value;
            if (val.trim() !== "") obj[key] = val.trim();
        };
        const addBool = (obj, key, domId) => {
            obj[key] = document.getElementById(domId).checked ? 1 : 0;
        };

        addNum(hw, 'p', 'hw_p');
        addNum(hw, 'st', 'hw_st');
        addNum(hw, 'kp', 'hw_kp');
        addNum(hw, 'ir', 'hw_ir');
        addBool(hw, 'ld', 'hw_ld');

        document.querySelectorAll('#channelBody tr').forEach((row, index) => {
            const pin = parseInt(row.querySelector('.ch-pin').value);
            const calib = parseInt(row.querySelector('.ch-cal').value);
            if (!isNaN(pin)) hw.c.push([index + 1, pin, !isNaN(calib) ? calib : 255]);
        });

        // Construct SW
        const sw = {};
        addNum(sw, 'as', 'sw_as');
        addNum(sw, 'ftk', 'sw_ftk');
        addNum(sw, 'web', 'sw_web');
        addStr(sw, 'wui', 'sw_wui');
        addNum(sw, 'nt', 'sw_nt');
        
        if (sw.nt !== 0) {
            addStr(sw, 'ntp', 'sw_ntp');
            addStr(sw, 'ntz', 'sw_ntz');
        }

        addBool(sw, 'snf', 'sw_snf');
        addBool(sw, 'au', 'sw_au');
        if (sw.au === 1) addStr(sw, 'ota', 'sw_ota');

        addBool(sw, 'md', 'sw_md');
        addBool(sw, 'sr', 'sw_sr');
        addBool(sw, 'tl', 'sw_tl');
        addBool(sw, 'ua', 'sw_ua');

        fullConfig.hw = hw;
        fullConfig.sw = sw;

        fetch('/a1/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(fullConfig)
        })
        .then(res => res.ok ? showMsg('sysStatus', 'System configuration saved!', true) : Promise.reject())
        .catch(() => showMsg('sysStatus', 'Failed to save configuration.', false));
    }

    function saveWlan() {
        const payload = new URLSearchParams();
        const addParam = (key, domId) => {
            const val = document.getElementById(domId).value;
            if(val !== "") payload.append(key, val);
        };

        addParam('wlm', 'wlm');
        addParam('ssid', 'ssid');
        addParam('psk', 'psk');
        addParam('apssid', 'apssid');
        addParam('appsk', 'appsk');
        addParam('t', 't');

        fetch('/a1/wlset', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: payload.toString()
        })
        .then(res => res.ok ? showMsg('wlStatus', 'WiFi configuration saved! Rebooting...', true) : Promise.reject())
        .catch(() => showMsg('wlStatus', 'Failed to save WiFi configuration.', false));
    }

    function showMsg(elementId, text, isSuccess) {
        const el = document.getElementById(elementId);
        el.textContent = text;
        el.className = `status ${isSuccess ? 'success' : 'error'}`;
        setTimeout(() => el.classList.add('hidden'), 4000);
    }
</script>
</body>
</html>)rawliteral";

extern const char wlan2_html[] PROGMEM = R"rawliteral()rawliteral";


//this should maybe be modified to a json and compressed some way, maybe divided in subregions
extern const char tzdata_csv[] PROGMEM = R"rawliteral("Africa/Abidjan","GMT0"
"Africa/Accra","GMT0"
"Africa/Addis_Ababa","EAT-3"
"Africa/Algiers","CET-1"
"Africa/Asmara","EAT-3"
"Africa/Bamako","GMT0"
"Africa/Bangui","WAT-1"
"Africa/Banjul","GMT0"
"Africa/Bissau","GMT0"
"Africa/Blantyre","CAT-2"
"Africa/Brazzaville","WAT-1"
"Africa/Bujumbura","CAT-2"
"Africa/Cairo","EET-2EEST,M4.5.5/0,M10.5.4/24"
"Africa/Casablanca","<+01>-1"
"Africa/Ceuta","CET-1CEST,M3.5.0,M10.5.0/3"
"Africa/Conakry","GMT0"
"Africa/Dakar","GMT0"
"Africa/Dar_es_Salaam","EAT-3"
"Africa/Djibouti","EAT-3"
"Africa/Douala","WAT-1"
"Africa/El_Aaiun","<+01>-1"
"Africa/Freetown","GMT0"
"Africa/Gaborone","CAT-2"
"Africa/Harare","CAT-2"
"Africa/Johannesburg","SAST-2"
"Africa/Juba","CAT-2"
"Africa/Kampala","EAT-3"
"Africa/Khartoum","CAT-2"
"Africa/Kigali","CAT-2"
"Africa/Kinshasa","WAT-1"
"Africa/Lagos","WAT-1"
"Africa/Libreville","WAT-1"
"Africa/Lome","GMT0"
"Africa/Luanda","WAT-1"
"Africa/Lubumbashi","CAT-2"
"Africa/Lusaka","CAT-2"
"Africa/Malabo","WAT-1"
"Africa/Maputo","CAT-2"
"Africa/Maseru","SAST-2"
"Africa/Mbabane","SAST-2"
"Africa/Mogadishu","EAT-3"
"Africa/Monrovia","GMT0"
"Africa/Nairobi","EAT-3"
"Africa/Ndjamena","WAT-1"
"Africa/Niamey","WAT-1"
"Africa/Nouakchott","GMT0"
"Africa/Ouagadougou","GMT0"
"Africa/Porto-Novo","WAT-1"
"Africa/Sao_Tome","GMT0"
"Africa/Tripoli","EET-2"
"Africa/Tunis","CET-1"
"Africa/Windhoek","CAT-2"
"America/Adak","HST10HDT,M3.2.0,M11.1.0"
"America/Anchorage","AKST9AKDT,M3.2.0,M11.1.0"
"America/Anguilla","AST4"
"America/Antigua","AST4"
"America/Araguaina","<-03>3"
"America/Argentina/Buenos_Aires","<-03>3"
"America/Argentina/Catamarca","<-03>3"
"America/Argentina/Cordoba","<-03>3"
"America/Argentina/Jujuy","<-03>3"
"America/Argentina/La_Rioja","<-03>3"
"America/Argentina/Mendoza","<-03>3"
"America/Argentina/Rio_Gallegos","<-03>3"
"America/Argentina/Salta","<-03>3"
"America/Argentina/San_Juan","<-03>3"
"America/Argentina/San_Luis","<-03>3"
"America/Argentina/Tucuman","<-03>3"
"America/Argentina/Ushuaia","<-03>3"
"America/Aruba","AST4"
"America/Asuncion","<-03>3"
"America/Atikokan","EST5"
"America/Bahia","<-03>3"
"America/Bahia_Banderas","CST6"
"America/Barbados","AST4"
"America/Belem","<-03>3"
"America/Belize","CST6"
"America/Blanc-Sablon","AST4"
"America/Boa_Vista","<-04>4"
"America/Bogota","<-05>5"
"America/Boise","MST7MDT,M3.2.0,M11.1.0"
"America/Cambridge_Bay","MST7MDT,M3.2.0,M11.1.0"
"America/Campo_Grande","<-04>4"
"America/Cancun","EST5"
"America/Caracas","<-04>4"
"America/Cayenne","<-03>3"
"America/Cayman","EST5"
"America/Chicago","CST6CDT,M3.2.0,M11.1.0"
"America/Chihuahua","CST6"
"America/Costa_Rica","CST6"
"America/Creston","MST7"
"America/Cuiaba","<-04>4"
"America/Curacao","AST4"
"America/Danmarkshavn","GMT0"
"America/Dawson","MST7"
"America/Dawson_Creek","MST7"
"America/Denver","MST7MDT,M3.2.0,M11.1.0"
"America/Detroit","EST5EDT,M3.2.0,M11.1.0"
"America/Dominica","AST4"
"America/Edmonton","MST7MDT,M3.2.0,M11.1.0"
"America/Eirunepe","<-05>5"
"America/El_Salvador","CST6"
"America/Fortaleza","<-03>3"
"America/Fort_Nelson","MST7"
"America/Glace_Bay","AST4ADT,M3.2.0,M11.1.0"
"America/Godthab","<-02>2<-01>,M3.5.0/-1,M10.5.0/0"
"America/Goose_Bay","AST4ADT,M3.2.0,M11.1.0"
"America/Grand_Turk","EST5EDT,M3.2.0,M11.1.0"
"America/Grenada","AST4"
"America/Guadeloupe","AST4"
"America/Guatemala","CST6"
"America/Guayaquil","<-05>5"
"America/Guyana","<-04>4"
"America/Halifax","AST4ADT,M3.2.0,M11.1.0"
"America/Havana","CST5CDT,M3.2.0/0,M11.1.0/1"
"America/Hermosillo","MST7"
"America/Indiana/Indianapolis","EST5EDT,M3.2.0,M11.1.0"
"America/Indiana/Knox","CST6CDT,M3.2.0,M11.1.0"
"America/Indiana/Marengo","EST5EDT,M3.2.0,M11.1.0"
"America/Indiana/Petersburg","EST5EDT,M3.2.0,M11.1.0"
"America/Indiana/Tell_City","CST6CDT,M3.2.0,M11.1.0"
"America/Indiana/Vevay","EST5EDT,M3.2.0,M11.1.0"
"America/Indiana/Vincennes","EST5EDT,M3.2.0,M11.1.0"
"America/Indiana/Winamac","EST5EDT,M3.2.0,M11.1.0"
"America/Inuvik","MST7MDT,M3.2.0,M11.1.0"
"America/Iqaluit","EST5EDT,M3.2.0,M11.1.0"
"America/Jamaica","EST5"
"America/Juneau","AKST9AKDT,M3.2.0,M11.1.0"
"America/Kentucky/Louisville","EST5EDT,M3.2.0,M11.1.0"
"America/Kentucky/Monticello","EST5EDT,M3.2.0,M11.1.0"
"America/Kralendijk","AST4"
"America/La_Paz","<-04>4"
"America/Lima","<-05>5"
"America/Los_Angeles","PST8PDT,M3.2.0,M11.1.0"
"America/Lower_Princes","AST4"
"America/Maceio","<-03>3"
"America/Managua","CST6"
"America/Manaus","<-04>4"
"America/Marigot","AST4"
"America/Martinique","AST4"
"America/Matamoros","CST6CDT,M3.2.0,M11.1.0"
"America/Mazatlan","MST7"
"America/Menominee","CST6CDT,M3.2.0,M11.1.0"
"America/Merida","CST6"
"America/Metlakatla","AKST9AKDT,M3.2.0,M11.1.0"
"America/Mexico_City","CST6"
"America/Miquelon","<-03>3<-02>,M3.2.0,M11.1.0"
"America/Moncton","AST4ADT,M3.2.0,M11.1.0"
"America/Monterrey","CST6"
"America/Montevideo","<-03>3"
"America/Montreal","EST5EDT,M3.2.0,M11.1.0"
"America/Montserrat","AST4"
"America/Nassau","EST5EDT,M3.2.0,M11.1.0"
"America/New_York","EST5EDT,M3.2.0,M11.1.0"
"America/Nipigon","EST5EDT,M3.2.0,M11.1.0"
"America/Nome","AKST9AKDT,M3.2.0,M11.1.0"
"America/Noronha","<-02>2"
"America/North_Dakota/Beulah","CST6CDT,M3.2.0,M11.1.0"
"America/North_Dakota/Center","CST6CDT,M3.2.0,M11.1.0"
"America/North_Dakota/New_Salem","CST6CDT,M3.2.0,M11.1.0"
"America/Nuuk","<-02>2<-01>,M3.5.0/-1,M10.5.0/0"
"America/Ojinaga","CST6CDT,M3.2.0,M11.1.0"
"America/Panama","EST5"
"America/Pangnirtung","EST5EDT,M3.2.0,M11.1.0"
"America/Paramaribo","<-03>3"
"America/Phoenix","MST7"
"America/Port-au-Prince","EST5EDT,M3.2.0,M11.1.0"
"America/Port_of_Spain","AST4"
"America/Porto_Velho","<-04>4"
"America/Puerto_Rico","AST4"
"America/Punta_Arenas","<-03>3"
"America/Rainy_River","CST6CDT,M3.2.0,M11.1.0"
"America/Rankin_Inlet","CST6CDT,M3.2.0,M11.1.0"
"America/Recife","<-03>3"
"America/Regina","CST6"
"America/Resolute","CST6CDT,M3.2.0,M11.1.0"
"America/Rio_Branco","<-05>5"
"America/Santarem","<-03>3"
"America/Santiago","<-04>4<-03>,M9.1.6/24,M4.1.6/24"
"America/Santo_Domingo","AST4"
"America/Sao_Paulo","<-03>3"
"America/Scoresbysund","<-02>2<-01>,M3.5.0/-1,M10.5.0/0"
"America/Sitka","AKST9AKDT,M3.2.0,M11.1.0"
"America/St_Barthelemy","AST4"
"America/St_Johns","NST3:30NDT,M3.2.0,M11.1.0"
"America/St_Kitts","AST4"
"America/St_Lucia","AST4"
"America/St_Thomas","AST4"
"America/St_Vincent","AST4"
"America/Swift_Current","CST6"
"America/Tegucigalpa","CST6"
"America/Thule","AST4ADT,M3.2.0,M11.1.0"
"America/Thunder_Bay","EST5EDT,M3.2.0,M11.1.0"
"America/Tijuana","PST8PDT,M3.2.0,M11.1.0"
"America/Toronto","EST5EDT,M3.2.0,M11.1.0"
"America/Tortola","AST4"
"America/Vancouver","PST8PDT,M3.2.0,M11.1.0"
"America/Whitehorse","MST7"
"America/Winnipeg","CST6CDT,M3.2.0,M11.1.0"
"America/Yakutat","AKST9AKDT,M3.2.0,M11.1.0"
"America/Yellowknife","MST7MDT,M3.2.0,M11.1.0"
"Antarctica/Casey","<+08>-8"
"Antarctica/Davis","<+07>-7"
"Antarctica/DumontDUrville","<+10>-10"
"Antarctica/Macquarie","AEST-10AEDT,M10.1.0,M4.1.0/3"
"Antarctica/Mawson","<+05>-5"
"Antarctica/McMurdo","NZST-12NZDT,M9.5.0,M4.1.0/3"
"Antarctica/Palmer","<-03>3"
"Antarctica/Rothera","<-03>3"
"Antarctica/Syowa","<+03>-3"
"Antarctica/Troll","<+00>0<+02>-2,M3.5.0/1,M10.5.0/3"
"Antarctica/Vostok","<+05>-5"
"Arctic/Longyearbyen","CET-1CEST,M3.5.0,M10.5.0/3"
"Asia/Aden","<+03>-3"
"Asia/Almaty","<+05>-5"
"Asia/Amman","<+03>-3"
"Asia/Anadyr","<+12>-12"
"Asia/Aqtau","<+05>-5"
"Asia/Aqtobe","<+05>-5"
"Asia/Ashgabat","<+05>-5"
"Asia/Atyrau","<+05>-5"
"Asia/Baghdad","<+03>-3"
"Asia/Bahrain","<+03>-3"
"Asia/Baku","<+04>-4"
"Asia/Bangkok","<+07>-7"
"Asia/Barnaul","<+07>-7"
"Asia/Beirut","EET-2EEST,M3.5.0/0,M10.5.0/0"
"Asia/Bishkek","<+06>-6"
"Asia/Brunei","<+08>-8"
"Asia/Chita","<+09>-9"
"Asia/Choibalsan","<+08>-8"
"Asia/Colombo","<+0530>-5:30"
"Asia/Damascus","<+03>-3"
"Asia/Dhaka","<+06>-6"
"Asia/Dili","<+09>-9"
"Asia/Dubai","<+04>-4"
"Asia/Dushanbe","<+05>-5"
"Asia/Famagusta","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Asia/Gaza","EET-2EEST,M3.4.4/50,M10.4.4/50"
"Asia/Hebron","EET-2EEST,M3.4.4/50,M10.4.4/50"
"Asia/Ho_Chi_Minh","<+07>-7"
"Asia/Hong_Kong","HKT-8"
"Asia/Hovd","<+07>-7"
"Asia/Irkutsk","<+08>-8"
"Asia/Jakarta","WIB-7"
"Asia/Jayapura","WIT-9"
"Asia/Jerusalem","IST-2IDT,M3.4.4/26,M10.5.0"
"Asia/Kabul","<+0430>-4:30"
"Asia/Kamchatka","<+12>-12"
"Asia/Karachi","PKT-5"
"Asia/Kathmandu","<+0545>-5:45"
"Asia/Khandyga","<+09>-9"
"Asia/Kolkata","IST-5:30"
"Asia/Krasnoyarsk","<+07>-7"
"Asia/Kuala_Lumpur","<+08>-8"
"Asia/Kuching","<+08>-8"
"Asia/Kuwait","<+03>-3"
"Asia/Macau","CST-8"
"Asia/Magadan","<+11>-11"
"Asia/Makassar","WITA-8"
"Asia/Manila","PST-8"
"Asia/Muscat","<+04>-4"
"Asia/Nicosia","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Asia/Novokuznetsk","<+07>-7"
"Asia/Novosibirsk","<+07>-7"
"Asia/Omsk","<+06>-6"
"Asia/Oral","<+05>-5"
"Asia/Phnom_Penh","<+07>-7"
"Asia/Pontianak","WIB-7"
"Asia/Pyongyang","KST-9"
"Asia/Qatar","<+03>-3"
"Asia/Qyzylorda","<+05>-5"
"Asia/Riyadh","<+03>-3"
"Asia/Sakhalin","<+11>-11"
"Asia/Samarkand","<+05>-5"
"Asia/Seoul","KST-9"
"Asia/Shanghai","CST-8"
"Asia/Singapore","<+08>-8"
"Asia/Srednekolymsk","<+11>-11"
"Asia/Taipei","CST-8"
"Asia/Tashkent","<+05>-5"
"Asia/Tbilisi","<+04>-4"
"Asia/Tehran","<+0330>-3:30"
"Asia/Thimphu","<+06>-6"
"Asia/Tokyo","JST-9"
"Asia/Tomsk","<+07>-7"
"Asia/Ulaanbaatar","<+08>-8"
"Asia/Urumqi","<+06>-6"
"Asia/Ust-Nera","<+10>-10"
"Asia/Vientiane","<+07>-7"
"Asia/Vladivostok","<+10>-10"
"Asia/Yakutsk","<+09>-9"
"Asia/Yangon","<+0630>-6:30"
"Asia/Yekaterinburg","<+05>-5"
"Asia/Yerevan","<+04>-4"
"Atlantic/Azores","<-01>1<+00>,M3.5.0/0,M10.5.0/1"
"Atlantic/Bermuda","AST4ADT,M3.2.0,M11.1.0"
"Atlantic/Canary","WET0WEST,M3.5.0/1,M10.5.0"
"Atlantic/Cape_Verde","<-01>1"
"Atlantic/Faroe","WET0WEST,M3.5.0/1,M10.5.0"
"Atlantic/Madeira","WET0WEST,M3.5.0/1,M10.5.0"
"Atlantic/Reykjavik","GMT0"
"Atlantic/South_Georgia","<-02>2"
"Atlantic/Stanley","<-03>3"
"Atlantic/St_Helena","GMT0"
"Australia/Adelaide","ACST-9:30ACDT,M10.1.0,M4.1.0/3"
"Australia/Brisbane","AEST-10"
"Australia/Broken_Hill","ACST-9:30ACDT,M10.1.0,M4.1.0/3"
"Australia/Currie","AEST-10AEDT,M10.1.0,M4.1.0/3"
"Australia/Darwin","ACST-9:30"
"Australia/Eucla","<+0845>-8:45"
"Australia/Hobart","AEST-10AEDT,M10.1.0,M4.1.0/3"
"Australia/Lindeman","AEST-10"
"Australia/Lord_Howe","<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"
"Australia/Melbourne","AEST-10AEDT,M10.1.0,M4.1.0/3"
"Australia/Perth","AWST-8"
"Australia/Sydney","AEST-10AEDT,M10.1.0,M4.1.0/3"
"Europe/Amsterdam","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Andorra","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Astrakhan","<+04>-4"
"Europe/Athens","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Belgrade","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Berlin","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Bratislava","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Brussels","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Bucharest","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Budapest","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Busingen","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Chisinau","EET-2EEST,M3.5.0,M10.5.0/3"
"Europe/Copenhagen","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Dublin","IST-1GMT0,M10.5.0,M3.5.0/1"
"Europe/Gibraltar","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Guernsey","GMT0BST,M3.5.0/1,M10.5.0"
"Europe/Helsinki","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Isle_of_Man","GMT0BST,M3.5.0/1,M10.5.0"
"Europe/Istanbul","<+03>-3"
"Europe/Jersey","GMT0BST,M3.5.0/1,M10.5.0"
"Europe/Kaliningrad","EET-2"
"Europe/Kyiv","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Kirov","MSK-3"
"Europe/Lisbon","WET0WEST,M3.5.0/1,M10.5.0"
"Europe/Ljubljana","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/London","GMT0BST,M3.5.0/1,M10.5.0/2"
"Europe/Luxembourg","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Madrid","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Malta","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Mariehamn","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Minsk","<+03>-3"
"Europe/Monaco","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Moscow","MSK-3"
"Europe/Oslo","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Paris","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Podgorica","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Prague","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Riga","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Rome","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Samara","<+04>-4"
"Europe/San_Marino","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Sarajevo","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Saratov","<+04>-4"
"Europe/Simferopol","MSK-3"
"Europe/Skopje","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Sofia","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Stockholm","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Tallinn","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Tirane","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Ulyanovsk","<+04>-4"
"Europe/Uzhgorod","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Vaduz","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Vatican","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Vienna","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Vilnius","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Volgograd","MSK-3"
"Europe/Warsaw","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Zagreb","CET-1CEST,M3.5.0,M10.5.0/3"
"Europe/Zaporozhye","EET-2EEST,M3.5.0/3,M10.5.0/4"
"Europe/Zurich","CET-1CEST,M3.5.0,M10.5.0/3"
"Indian/Antananarivo","EAT-3"
"Indian/Chagos","<+06>-6"
"Indian/Christmas","<+07>-7"
"Indian/Cocos","<+0630>-6:30"
"Indian/Comoro","EAT-3"
"Indian/Kerguelen","<+05>-5"
"Indian/Mahe","<+04>-4"
"Indian/Maldives","<+05>-5"
"Indian/Mauritius","<+04>-4"
"Indian/Mayotte","EAT-3"
"Indian/Reunion","<+04>-4"
"Pacific/Apia","<+13>-13"
"Pacific/Auckland","NZST-12NZDT,M9.5.0,M4.1.0/3"
"Pacific/Bougainville","<+11>-11"
"Pacific/Chatham","<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45"
"Pacific/Chuuk","<+10>-10"
"Pacific/Easter","<-06>6<-05>,M9.1.6/22,M4.1.6/22"
"Pacific/Efate","<+11>-11"
"Pacific/Enderbury","<+13>-13"
"Pacific/Fakaofo","<+13>-13"
"Pacific/Fiji","<+12>-12"
"Pacific/Funafuti","<+12>-12"
"Pacific/Galapagos","<-06>6"
"Pacific/Gambier","<-09>9"
"Pacific/Guadalcanal","<+11>-11"
"Pacific/Guam","ChST-10"
"Pacific/Honolulu","HST10"
"Pacific/Kiritimati","<+14>-14"
"Pacific/Kosrae","<+11>-11"
"Pacific/Kwajalein","<+12>-12"
"Pacific/Majuro","<+12>-12"
"Pacific/Marquesas","<-0930>9:30"
"Pacific/Midway","SST11"
"Pacific/Nauru","<+12>-12"
"Pacific/Niue","<-11>11"
"Pacific/Norfolk","<+11>-11<+12>,M10.1.0,M4.1.0/3"
"Pacific/Noumea","<+11>-11"
"Pacific/Pago_Pago","SST11"
"Pacific/Palau","<+09>-9"
"Pacific/Pitcairn","<-08>8"
"Pacific/Pohnpei","<+11>-11"
"Pacific/Port_Moresby","<+10>-10"
"Pacific/Rarotonga","<-10>10"
"Pacific/Saipan","ChST-10"
"Pacific/Tahiti","<-10>10"
"Pacific/Tarawa","<+12>-12"
"Pacific/Tongatapu","<+13>-13"
"Pacific/Wake","<+12>-12"
"Pacific/Wallis","<+12>-12"
"Etc/GMT","GMT0"
"Etc/GMT-0","GMT0"
"Etc/GMT-1","<+01>-1"
"Etc/GMT-2","<+02>-2"
"Etc/GMT-3","<+03>-3"
"Etc/GMT-4","<+04>-4"
"Etc/GMT-5","<+05>-5"
"Etc/GMT-6","<+06>-6"
"Etc/GMT-7","<+07>-7"
"Etc/GMT-8","<+08>-8"
"Etc/GMT-9","<+09>-9"
"Etc/GMT-10","<+10>-10"
"Etc/GMT-11","<+11>-11"
"Etc/GMT-12","<+12>-12"
"Etc/GMT-13","<+13>-13"
"Etc/GMT-14","<+14>-14"
"Etc/GMT0","GMT0"
"Etc/GMT+0","GMT0"
"Etc/GMT+1","<-01>1"
"Etc/GMT+2","<-02>2"
"Etc/GMT+3","<-03>3"
"Etc/GMT+4","<-04>4"
"Etc/GMT+5","<-05>5"
"Etc/GMT+6","<-06>6"
"Etc/GMT+7","<-07>7"
"Etc/GMT+8","<-08>8"
"Etc/GMT+9","<-09>9"
"Etc/GMT+10","<-10>10"
"Etc/GMT+11","<-11>11"
"Etc/GMT+12","<-12>12"
"Etc/UCT","UTC0"
"Etc/UTC","UTC0"
"Etc/Greenwich","GMT0"
"Etc/Universal","UTC0"
"Etc/Zulu","UTC0")rawliteral";
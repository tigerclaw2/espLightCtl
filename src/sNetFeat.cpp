#include "sNetFeat.h"
//#include <//TelnetPrint.h>
#include "ledScript.h"
#include "light.h"

extern "C" {
#include <lwip/etharp.h>
#include <lwip/netif.h>
#include <lwip/ip_addr.h>
}

std::vector<TrackedDevice> tracked_devices;

WiFiUDP Udp;

bool is_scanning = false;
uint8_t ip_octet = 1;

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

// --- 2. Action Execution Handlers ---

void handleDeviceFound(TrackedDevice& dev) {
	//TelnetPrint.printf("[SNF] Device Found! IP: %d\n", dev.ip);//.toString().c_str());

	switch (dev.on_found.type) {
	case ACT_SCRIPT:
		//TelnetPrint.printf("  -> Trigger Script: %s\n", dev.on_found.str_val.c_str());
		// [ENTRY POINT] - Found Script Handler
		scriptBegin(dev.on_found.str_val, false);
		break;

	case ACT_DIY:
		//TelnetPrint.printf("  -> Trigger DIY: %d\n", dev.on_found.num_val);
		// [ENTRY POINT] - Found DIY Handler
		//diyload(dev.on_found.num_val);
		break;

	case ACT_POWER:
		//TelnetPrint.printf("  -> Trigger Power: %d\n", dev.on_found.num_val);
		// [ENTRY POINT] - Found Power Handler
		Light.setBriSingle(0, dev.on_found.num_val == 1 ? 255 : 0);
		break;

	case ACT_NONE:
	default:
		break;
	}
}

void handleDeviceLost(TrackedDevice& dev) {
	////TelnetPrint.printf("[SNF] Device Lost! IP: %d\n", dev.ip); //.toString().c_str());

	switch (dev.on_lost.type) {
	case ACT_SCRIPT:
		////TelnetPrint.printf("  -> Trigger Script: %s\n", dev.on_lost.str_val.c_str());
		// [ENTRY POINT] - Lost Script Handler
		//scriptEnd();
		scriptBegin(dev.on_lost.str_val, false);
		break;

	case ACT_DIY:
		////TelnetPrint.printf("  -> Trigger DIY: %d\n", dev.on_lost.num_val);
		// [ENTRY POINT] - Lost DIY Handler
		break;

	case ACT_POWER:
		////TelnetPrint.printf("  -> Trigger Power: %d\n", dev.on_lost.num_val);
		// [ENTRY POINT] - Lost Power Handler
		Light.setBriSingle(0, dev.on_lost.num_val == 1 ? 0 : 255);
		break;

	case ACT_NONE:
	default:
		break;
	}
}

void checkDevice(DiscoveredDevice device) {
				////TelnetPrint.printf("[SNF] Check mac: %02X:%02X:%02X:%02X:%02X:%02X, ip: %d, host: %s\n",
								//    device.mac[0], device.mac[1], device.mac[2],
								//    device.mac[3], device.mac[4], device.mac[5],
								//    device.ip,//.toString().c_str(),
								//    device.hostname.c_str());
				for (auto& dev : tracked_devices) {
					uint8_t matches=0;
					////TelnetPrint.printf("[SNF] Compare with mac: %02X:%02X:%02X:%02X:%02X:%02X, ip: %d, host: %s, flags: %d\n",
								//    dev.mac[0], dev.mac[1], dev.mac[2],
								//    dev.mac[3], dev.mac[4], dev.mac[5],
								//    dev.ip,//.toString().c_str(),
								//    dev.hostname.c_str(), dev.user_fcount());
					if(dev.user_ip() && dev.ip == device.ip){   //ip match
						matches++;
						////TelnetPrint.printf("[SNF]	->Match IP, matches: %d\n",matches);

						//other specific actions
					}
					if (dev.user_mac() && macsMatch(dev.mac, device.mac)) { //mac match
						matches++;
						////TelnetPrint.printf("[SNF]	->Match mac, matches: %d\n",matches);
						//other specific actions
					}
					if (dev.user_hostname() && dev.hostname.equalsIgnoreCase(device.hostname)) {   //hostname match
						matches++;
						////TelnetPrint.printf("[SNF]	->Match host, matches: %d\n",matches);
						//other specific actions
					}
					
					if(matches == dev.user_fcount()) {
						////TelnetPrint.printf("[SNF] Device mac: %02X:%02X:%02X:%02X:%02X:%02X, ip: %d, host: %s matched Device mac: %02X:%02X:%02X:%02X:%02X:%02X, ip: %d, host: %s, flags: %d\n",
								// device.mac[0], device.mac[1], device.mac[2],
								// device.mac[3], device.mac[4], device.mac[5],
								// device.ip,//.toString().c_str(),
								// device.hostname.c_str(),

								// dev.mac[0], dev.mac[1], dev.mac[2],
								// dev.mac[3], dev.mac[4], dev.mac[5],
								// dev.ip,//.toString().c_str(),
								// dev.hostname.c_str(), dev.user_fcount());
						// this is the device we are looking for as it matched the same number of flags as it has

						if(!dev.user_ip()) {
							dev.ip = device.ip;
						}
						if(!dev.user_mac()) {
							memcpy(dev.mac, device.mac, 6);
						} 
						dev.is_present=true;
						dev.last_seen = millis();
						handleDeviceFound(dev);
					}
				}

}

// Parses "d5", "p0", or "s/path" into the enum struct directly from the JSON pointer
TriggerAction parseAction(const char* val) {
	TriggerAction action;
	if (!val || val[0] == '\0') return action;

	if (val[0] == 's') {
		action.type = ACT_SCRIPT;
		action.str_val = String(val + 1);
	} else if (val[0] == 'd') {
		action.type = ACT_DIY;
		action.num_val = atoi(val + 1);
	} else if (val[0] == 'p') {
		action.type = ACT_POWER;
		action.num_val = atoi(val + 1);
	}
	return action;
}



// --- 3. Core System Logic ---

void snfLoadConf(const char* cnfp) {
	// Remove the pointers for async ping
	for (auto& dev : tracked_devices) {
		if (dev.icmp_pinger) {
			delete dev.icmp_pinger;
		}
	}
	tracked_devices.clear();
	tracked_devices.shrink_to_fit();

	File file = SPIFFS.open(cnfp, "r");
	if (!file) {
		////TelnetPrint.printf("[SNF] %s not found. Tracking disabled.", cnfp);
		return;
	}

	JsonDocument doc;
	DeserializationError error = deserializeJson(doc, file);
	file.close();

	if (error) {
		////TelnetPrint.printf("[SNF] Failed to parse snf.json: %s\n", error.c_str());
		return;
	}

	for (JsonObject item : doc.as<JsonArray>()) {
		TrackedDevice dev;
		IPAddress parsed_ip;

		// Dynamic Identifiers
		if (item.containsKey("i")) {
			//const char* ip_str = ;
            
            parsed_ip.fromString(item["i"].as<const char*>());
                
            dev.ip = (uint32_t)parsed_ip;
			dev.user_ip(true);
		}
		///else dev.ip = IPAddress(0, 0, 0, 0);

		if (item.containsKey("m")) {
			parseMacString(item["m"].as<const char*>(), dev.mac);
			dev.user_mac(true);
		} else {
			memset(dev.mac, 0, 6);
			dev.user_mac(false);
		}

		if (item.containsKey("h")) {
			dev.hostname = item["h"] | "";
			dev.hostname.toLowerCase();
			dev.user_hostname(true);
		}
		///dev.hostname = item["h"] | "";

		// Rules
		if (item.containsKey("f")) {
			dev.on_found = parseAction(item["f"]);
		}
		if (item.containsKey("l")) {
			dev.on_lost = parseAction(item["l"]);
		}

		// Timings & State
		dev.ping_interval = item["p"] | 5000;
		dev.ping_type = static_cast<PingType>(item["pt"] | PING_ICMP);
		dev.is_present = false;
		dev.last_seen = 0;
		dev.last_ping = 0;

		// Ping inits
		switch (dev.ping_type) {
		case PING_ICMP: {
				dev.icmp_pinger = new AsyncPing();
				dev.icmp_pinger->on(true, [](const AsyncPingResponse & response) {
					if (response.answer) {
						//IPAddress resp_addr(response.addr);
						//unsigned long current_time = millis();

						for (auto& d : tracked_devices) {
							if (d.ip == response.addr.v4()) {
								if(d.user_mac()) {	//device with specified mac
									if(macsMatch(d.mac, response.mac->addr)){
										d.last_seen = millis();
										if (!d.is_present) {
											d.is_present = true;
											handleDeviceFound(d);
										}
									}
								} else {	//this is a device with only ip
									d.last_seen = millis();
									if (!d.is_present) {
										d.is_present = true;
										handleDeviceFound(d);
									}
								}
								break;
							}
						}
					}
					return false;
				});
				break;
			}
		default: {
				break;
			}
		}
		tracked_devices.push_back(dev);
	}
	Udp.stopAll();
	Udp.begin(67);
	//TelnetPrint.printf("[SNF] Loaded %d rules from snf.json\n", tracked_devices.size());
}

void snfActivateDiscoveryWindow() {
	last_discovery_request = millis();
}

void addDiscoveredDevice(const DiscoveredDevice& device) {
	for (auto& dev : discovered_devices) {
		if (macsMatch(dev.mac, device.mac)) {
			// Update missing info if we found it
			//if (dev.ip == IPAddress(0, 0, 0, 0) && device.ip[0] != 0) {
			if (dev.ip == 0 && device.ip != 0) {
				dev.ip = device.ip;
			}
			if (dev.hostname == "" && device.hostname != "") {
				dev.hostname = device.hostname;
				//dev.hostname.toLowerCase();
			}
			return;
		}
	}
	// It's a new device, add it to the list
	// DiscoveredDevice newDev;
	// newDev.ip = ip;
	// memcpy(newDev.mac, mac, 6);
	// newDev.hostname = host;
	discovered_devices.push_back(device);
}

void snfDoARPscan() {
	//TelnetPrint.println("[SNF] snfDoARPscan()");
	if (WiFi.status() != WL_CONNECTED || tracked_devices.empty()) return;
	is_scanning = true;
	ip_octet = 1;
	//TelnetPrint.println("[SNF] Starting ARP sweep...");
}

void snfMain() {
	if (WiFi.status() != WL_CONNECTED) {
		return; // We can only work while connected
	}
	if (WiFi.getMode() != WIFI_STA) {
		//TelnetPrint.println("[SNF] WARN: WiFi STA needed for dhcp snooping");
	}
	//unsigned long current_time = millis();
	bool discovery_active = (millis() - last_discovery_request < DISCOVERY_WINDOW_MS);

	// --- DHCP Snooping ---
	int parsed=0;
	uint8_t buffer[577]; //rfc2131
	int packetSize;
	while ((packetSize = Udp.parsePacket()) > 0 && parsed < MAX_DHCP_PER_TICK) {
		//TelnetPrint.println("[SNF] Got DHCP? packet");
		parsed++;
		if (packetSize >= 240) {
			   
			int len = Udp.read(buffer, sizeof(buffer));

			if (buffer[236] == 0x63 && buffer[237] == 0x82 && buffer[238] == 0x53 && buffer[239] == 0x63) {
				DiscoveredDevice dev;
				//uint8_t packet_mac[6];
				memcpy(dev.mac, &buffer[28], 6);
				memcpy(&dev.ip, &buffer[12], 4);
				
				// dev.ip[0] = buffer[12];
				// dev.ip[1] = buffer[13];
				// dev.ip[2] = buffer[14];
				// dev.ip[3] = buffer[15];
				//(buffer[12], buffer[13], buffer[14], buffer[15]);      //this is the requested addr
				//IPAddress packet_ip(buffer[16], buffer[17], buffer[18], buffer[19]);    //this is the yiaddr

				//String packet_host = "";
				int idx = 240;
				while (idx < len) {

					while(idx < len && buffer[idx]==0) { //ignore padding
						idx++;
					}
					if (buffer[idx]==255) {
						break;
					}

					uint8_t tag = buffer[idx++];
					uint8_t tag_len = buffer[idx++];    //possible read over the array if tag is the last byte

					if (idx + tag_len > len) {
						//TelnetPrint.println("[SNF] Malformed DHCP packet.");
						idx=len;
						return;  //exit entirely
					} else {
						switch (tag) {
						case 12: {  //hostname option
								char temp_host[tag_len + 1];
								memcpy(temp_host, &buffer[idx], tag_len);
								temp_host[tag_len] = '\0';
								dev.hostname=temp_host;
								break;
							}
						case 50: {  //requested ip option
								if(tag_len == 4) {
									memcpy(&dev.ip, &buffer[idx], 4);
									//dev.ip = ((uint32_t)buffer[idx] << 24) | ((uint32_t)buffer[idx+1] << 16) | ((uint32_t)buffer[idx+2] <<  8) | ((uint32_t)buffer[idx+4] <<  0);
									// dev.ip[0] = buffer[idx];
									// dev.ip[1] = buffer[idx+1];
									// dev.ip[2] = buffer[idx+2];
									// dev.ip[3] = buffer[idx+3];
									//packet_ip = IPAddress(buffer[idx], buffer[idx+1], buffer[idx+2], buffer[idx+3]);
								} else {
									//TelnetPrint.println("[SNF] Malformed DHCP IP packet.");
								}
								break;
							}
						default: {   //unhandled option
								//TelnetPrint.printf("[SNF] Unhandled DHCP option %d length %d bytes\n", tag, tag_len);
								break;
							}

						}
						idx+=tag_len;
					}

				}

				//TelnetPrint.printf("[SNF] DHCP snoop mac: %02X:%02X:%02X:%02X:%02X:%02X, ip: %d, host: %s\n",
								//    dev.mac[0], dev.mac[1], dev.mac[2],
								//    dev.mac[3], dev.mac[4], dev.mac[5],
								//    dev.ip,//.toString().c_str(),
								//    dev.hostname.c_str());

				// 1. Always run the matching logic for devices of interest
				checkDevice(dev);

				// 2. ONLY add to the UI list if the web endpoint was recently hit
				if (discovery_active) {
					addDiscoveredDevice(dev);
				}
			}
		}
	}

	// Ping & Timeout
	for (auto& dev : tracked_devices) {

		unsigned long timeout_ms = dev.ping_interval * 3; // 3 missed pings = lost

		// Check if enough time has passed since we last checked this specific device
		// by comparing the elapsed time to the device's configured ping interval.
		if (millis() - dev.last_ping >= dev.ping_interval) {

			// Reset the ping timer for this device to the current time so we don't check again too soon.
			dev.last_ping = millis();

			switch (dev.ping_type) {
			case PING_ARP: {
					//if (dev.ip[0] != 0) {
					if (dev.ip != 0) {

						// Create a low-level LwIP IPv4 address structure required by the ESP8266 network stack.
						ip4_addr_t lwip_ip;

						// Copy our standard Arduino IPAddress into the LwIP-compatible structure.
						lwip_ip.addr = dev.ip;

						// Create a pointer that will hold the MAC address returned by the ARP cache query.
						struct eth_addr *ret_eth_addr;

						// Create a pointer that will hold the IP address returned by the ARP cache query.
						const ip4_addr_t *ret_ip_addr;

						// Query the ESP8266's internal ARP cache on the default network interface.
						// It looks for 'lwip_ip' and points the two pointers above to the result.
						// It returns an index >= 0 if the device is currently in the cache.
						ssize_t idx = etharp_find_addr(netif_default, &lwip_ip, &ret_eth_addr, &ret_ip_addr);

						// If idx is 0 or higher, the device was found in the local ARP cache (it recently spoke on the network).
						if (idx >= 0) {

							// If our JSON configuration didn't provide a MAC address for this device yet...
							if (!dev.user_mac()) {

								// Copy the 6 bytes of the MAC address from the ARP cache into our device's memory slot.
								memcpy(dev.mac, ret_eth_addr->addr, 6);

								// Set the flag to true so we know we've successfully learned and stored its MAC address.
								//dev.user_mac( true);
							}

							// Security/sanity check: ensure the MAC address in the ARP cache perfectly matches
							// the MAC address we are tracking for this device (prevents false positives from IP conflicts).
							if (macsMatch(ret_eth_addr->addr, dev.mac)) {

								// Update the device's "last seen" timestamp to right now, preventing the timeout logic from marking it lost.
								dev.last_seen = millis();

								// If the device was previously marked as "away" or "lost" in our system...
								if (!dev.is_present) {

									// Flip its state to "present" (online/home).
									dev.is_present = true;

									// Trigger your custom action
									handleDeviceFound(dev);
								}
							}
						} else {
							// If the device was NOT found in the local ARP cache (idx < 0)...

							// Force the ESP8266 hardware to broadcast a physical ARP request to the router ("Who has this IP?").
							// If the device is alive, it will reply silently in the background, populating the cache for our next loop check.
							etharp_request(netif_default, &lwip_ip);
						}
					}
					break;
				}
			case PING_ICMP: {
					if (dev.ip != 0 && dev.icmp_pinger) {
						dev.icmp_pinger->begin(dev.ip, 1, 5000);
					}
					break;
				}
			default: {
					break;
				}
			}

		}

		if (dev.is_present && (millis() - dev.last_seen > timeout_ms)) {
			dev.is_present = false;
			if(!dev.user_ip()) {
				dev.ip=0;
			}
			handleDeviceLost(dev);
		}
	}

	// Triggered ARP scan
	if (is_scanning) {
		IPAddress localIP = WiFi.localIP();
		IPAddress subnet = WiFi.subnetMask();
		IPAddress network;
		for (int i = 0; i < 4; i++) {
			network[i] = localIP[i] & subnet[i];
		}

		IPAddress currentIP = network;
		currentIP[3] = ip_octet;

		if (currentIP != localIP) {
			ip4_addr_t target_ipaddr;
			target_ipaddr.addr = currentIP;
			etharp_request(netif_default, &target_ipaddr);
		}

		ip_octet++;
		if (ip_octet >= 255) {
			is_scanning = false;
			//TelnetPrint.println("[SNF] ARP scan complete.");
		}

		for (size_t i = 0; i < ARP_TABLE_SIZE; ++i) {
            ip4_addr_t *ip;
            struct netif *netif;
            struct eth_addr *mac;
			
            if (etharp_get_entry(i, &ip, &netif, &mac)) {
                
                // Pack the cache entry into our standard struct
                DiscoveredDevice temp_dev;
                temp_dev.ip = ip->addr;
                memcpy(temp_dev.mac, mac->addr, 6);
                temp_dev.hostname = ""; // ARP does not provide hostnames

                // 1. Run the unified matching and learning logic unconditionally
                checkDevice(temp_dev);

                // 2. ONLY add to the UI discovery array if the window is open
                if (discovery_active) {
                    addDiscoveredDevice(temp_dev); 
                }
            }
        }
	}

	// if (discovery_active) {
	// 	// Harvest LwIP cache only when the UI is actively watching
	// 	for (size_t i = 0; i < ARP_TABLE_SIZE; ++i) {
	// 		ip4_addr_t *ip;
	// 		struct netif *netif;
	// 		struct eth_addr *mac;
			
	// 		if (etharp_get_entry(i, &ip, &netif, &mac)) {


	// 			// 1. Always run the matching logic for devices of interest
	// 			for (auto& dev : tracked_devices) {
	// 				bool matched = false;
	// 				if (dev.user_mac() && macsMatch(dev.mac, mac->addr)) { //mac match
	// 					matched = true;
	// 				} else if (dev.ip != IPAddress(0, 0, 0, 0) && dev.ip == ip->addr) { //ip match
	// 					matched = true;
	// 				}
	// 				// else if (dev.hostname.length() > 0 && dev.hostname.equalsIgnoreCase(packet_host)) {   //hostname match (let's just not do mdns lookups for now)
	// 				// matched = true;
	// 				// }

	// 				if (matched) {
	// 					if (!dev.user_mac()) { //get mac from scan if we don't have one
	// 						memcpy(dev.mac, mac->addr, 6);
	// 						//dev.user_mac( true);
	// 					}
	// 					if (dev.ip == IPAddress(0, 0, 0, 0) && ip->addr != 0) { //get ip from scan if we don't have one (the most common case)
	// 						dev.ip = ip->addr;
	// 					}

	// 					dev.last_seen = current_time;
	// 					if (!dev.is_present) {  //mark present as we obviously just interacted with the device
	// 						dev.is_present = true;
	// 						handleDeviceFound(dev);
	// 					}
	// 				}
	// 			}

	// 			// 2. ONLY add to the UI list if the web endpoint was recently hit
	// 			if (discovery_active) {
					
	// 				//addDiscoveredDevice(ip->addr, mac->addr, ""); //this has to be updated with new signature
	// 			}
	// 		}
	// 	}
	// } else if (!discovered_devices.empty()) {

	if (!discovered_devices.empty() && !discovery_active) {
		// The 30-second window expired.
		discovered_devices.clear();
		discovered_devices.shrink_to_fit();
		//TelnetPrint.println("[SNF] Discovery window closed. Cache cleared.");
	}
}

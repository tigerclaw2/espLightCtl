This is the developement branch of LightCtl.
The software found here is most likely untested and unstable.
I am not responsible for broken esps or anything that happens if you decide to compile and run this code.

# espLightCtl v0.3 dev (Pre Release 2)

espLightCtl is an open-source LED control firmware designed for ESP devices. It offers a comprehensive set of features to facilitate flexible and customizable LED control. The primary focus of this project is to provide a smart LED control experience without relying on external internet servers or any other 3rd parties including Home Assistant or other self-hosted solutions.

## Features

- **Customizable Output Channels**: Tailor the firmware to your specific needs by adjusting the number of output channels.
- **Power Supply Enable Pin**: Set a designated pin as a power supply enable, allowing you to use ATX power supplies turning them on only when it's needed.
- **Maximum Brightness Control**: Fine-tune the maximum brightness for each channel, ensuring precise color calibration or consistent brightness across different LED strips.
- **PWM Resolution Configuration**: Configure the PWM resolution according to your controller's capabilities. Specify the AnalogWrite resolution, for compatibility with various controllers like Arduino (8-bit) or ESP (10-bit).
- **Auto Save Timer**: Save and restore the last known brightness values after a reboot using the customizable auto save timer.
- **Built-in Web Interface**: Access the LED control interface directly through a built-in web interface, simplifying configuration and management.
- **Customizable Interface Themes**: Personalize the interface to your liking with themes packaged as .tar files.
- **DIY/Presets**: Create and save custom colors or brightness levels as presets.
- **RESTful API** (sort of): Interact with the firmware using HTTP GET and POST requests with a JSON-based RESTful API. Query the current status and change settings effortlessly.
- **Notification Light API**: Utilize the firmware's API function to create a notification light effect. The LED slowly fades in and out based on the preset specified in the API call.
- **Flexible Operation Modes**: espLightCtl can run in three different modes: connected to Wi-Fi, broadcasting its own management AP, or both. No external server or internet connection is required for operation.
- **Telnet Logging**: Debug the firmware over Wi-Fi using Telnet.

## Work in Progress

- **Project Technical Documentation and Guides**
- **Schedules and NTP clock support**
- **Hardware Keypad Support**: Enable support for hardware keypads to enhance control options.
- **Serial Support**: Half-implemented support for control via a USB connection.
- **IR Remote Support**: Partially implemented (disabled in this version) support for control using an IR remote.
- **Function Tables**: Define function mappings to buttons on remotes or keypads for seamless control.
- **Programs**: Create captivating light shows with programmable sequences. 
- **Addressable LED Support**: Future support for addressable LED strips. Implementation pending availability of hardware as I do not have any addressable LED strips.
- **Project Technical Documentation and Guides**


###### Yes, this description was _mostly_ generated using ChatGPT, I automate and program stuff, not create novels

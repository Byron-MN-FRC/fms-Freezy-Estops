# CyBears Estops

CyBears Estops is an application designed to manage emergency stop systems efficiently. This application ensures that all emergency stop mechanisms are monitored and controlled effectively to enhance safety and reliability.

This is a fork of the Freezy Arena EStops which uses the ESP32-S3-DevKitM-1 board.  It also adds support for Team Stack Lights.

## How it works

The ESP32-S3 board is wired to all of stop buttons at the specified GPIO pins, continually monitoring for state changes.

Through the network connection to the 10.0.100.0/24 network, it polls a HTTP RESTful interface provided by the CyBears version the FMS arena server (10.0.100.5).
- POST switch state
- GET Field Stack Light state
- GET Team Stack Light state (CyBears addition)

## Board Types / Build Environments 

- **ESP32-S3-DevKitM-1**
  - [ESP32-S3 ETH Development Board w/ POE](https://a.co/d/5kxDrxe)
  - This board is used for Ethernet connectivity.
  - Pins connected to the stop buttons: 
    - 33  // Field stop
    - 34  // 1E stop
    - 35  // 1A stop
    - 36  // 2E stop
    - 37  // 2A stop
    - 38  // 3E stop
    - 39  // 3A stop
  - Start match button pin: 40
  - LED strip pin: 17


## Features / Limitations

The primary use of this is as follows:
- Ethernet with either Static or DHCP address (Set in GlobalSettings.h)
- 

-- Work In Progress
- **Web-Based Configuration**: Easily configure the device settings through a web interface.
  - **Alliance Color Selection**: Choose between Red, Blue, and Field alliance colors.
  - **Network Configuration**: Set up the device IP, arena IP, and port.
  - **DHCP Support**: 

- **Real-Time Status Updates**: Monitor and update the status of the device in real-time.
  - **LED Indicators**: Visual feedback of filed status through LED indicators for different statuses.
  - **Stop Button Monitoring**: Monitor multiple stop buttons and update their status.

- **Ethernet Connectivity**: Reliable network connection using Ethernet.

- **SPIFFS File System**: Store and serve static files such as images from the ESP32's file system.
  - **Image Serving**: Serve images for the web interface directly from the ESP32.

- **REST API Integration**: Communicate with the arena system using REST API.
  - **Start Match**: Send a request to start the match.
  - **Stop Status Update**: Update the stop status through API calls.

- **User-Friendly Interface**: Intuitive and easy-to-use web interface for configuration and monitoring.

- **Preferences Storage**: Save and retrieve configuration settings using non-volatile storage.
  - **Persistent Settings**: Ensure settings are retained across device reboots.

## How to use

### Getting your environment ready
1.  Install the latest VSCode
2.  Install the PlatformIO extension
3.  From the command line, clone this repo.
    1.  `git clone https://github.com/Byron-MN-FRC/fms-Freezy-Estops.git`
4.  In VSCode:
    1.  Open the cloned `fms-Freezy-Estops` folder
    2.  Open the PlatformIO extension and open the `fms-Freezy-Estops` folder as a project.
    3.  Open the `src/GlobalSettings.h` file
        1.  Customize the various settings for your network and usage of the ESP32 you want to program.

**To flash the ESP32:**
1.  Plug it in to your USB port.  This will power it on and make it available as a USB Serial port.
2. In VSCode/PlatformIO:
  1. Click the Build option to compile
  2. Click on the Upload and Monitor option to flash the ESP32 and start monitoring the USB Serial interface. 

## Contributing

We welcome contributions! Please follow these steps to contribute:

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes.
4. Commit your changes (`git commit -m 'Add some feature'`).
5. Push to the branch (`git push origin feature-branch`).
6. Open a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

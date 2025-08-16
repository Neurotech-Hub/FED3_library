## This library controls [FED3](https://github.com/KravitzLabDevices/FED3), a device for training mice. 
<p>
The files on this Github repository should be considered "beta", for the most recent stable release please install the FED3 library from the Arduino IDE as decribed below.  See the [Wiki](https://github.com/KravitzLabDevices/FED3_library/wiki) for documentation on how to use the library.   This library is in development, please report bugs using Issues. 

### Please see the [Getting Started](https://github.com/KravitzLabDevices/FED3_library/wiki/Get-started) page for help getting going with FED3!

## Library Structure
This version of the FED3 library uses a modular architecture to improve maintainability and performance:

### Core Components
- `FED3.h` - Main header file with class definitions
- `FED3.cpp` - Core functionality and initialization

### Modular Systems
- `FED3_Audio.cpp` - Sound generation and audio feedback
- `FED3_BNC.cpp` - BNC input/output control
- `FED3_Bandit.cpp` - Probability-based behavioral tasks
- `FED3_Display.cpp` - Screen updates and visual feedback
- `FED3_Feed.cpp` - Pellet dispensing and motor control
- `FED3_Menus.cpp` - User interface and device configuration
- `FED3_Pixel.cpp` - LED and visual indicator control
- `FED3_Poke.cpp` - Nose poke detection and timing
- `FED3_RTC.cpp` - Real-time clock management
- `FED3_SD.cpp` - Data logging and storage operations

Each module encapsulates related functionality while maintaining compatibility with both ESP32 and M0 hardware platforms. This organization makes it easier to maintain, debug, and extend the library's capabilities.

## Cloud Integration
This version of the FED3 library supports integration with [hublink.cloud](https://hublink.cloud/) when using a retrofitted ESP32-S3 Feather module (available from the [Neurotech Hub](https://neurotechhub.wustl.edu/)). The hardware modification:
- Replaces the original Feather M0 with an ESP32-S3
- Uses a daughterboard configuration that repurposes the BNC output pin for SD card control
- Enables wireless data syncing through the [Hublink Node Library](https://github.com/Neurotech-Hub/Hublink-Node)

Note: The BNC output functionality must be carefully managed when using the ESP32-S3 configuration with Hublink support.

# MicroCraft-ESP32

![Minecraft Protocol](https://img.shields.io/badge/Minecraft_Protocol-1.8.x-success?style=for-the-badge&logo=minecraft)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif)
![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B)

Running a Java-based Minecraft Server requires gigabytes of RAM. But what if we bypass the rules, reverse-engineer the protocol, and force the game to run on a **$5 IoT microcontroller with only 520KB of RAM?** This project is an experimental hardware flex. It acts as a raw TCP server, manipulating Minecraft's network protocol packets to inject players into a dynamically generated, in-memory 1-block Skyblock world—without any external SD cards or heavy libraries.

## ✨ The Vision & Features

Unlike standard server ports that rely on heavy external dependencies, this is built for absolute optimisation and raw socket manipulation.

* **Zero Dependencies:** No external Minecraft libraries. Uses pure `WiFi.h` and raw hex byte streams.
* **Protocol Bypass (No Auth):** Skips Mojang's authentication and Handshake/Login states directly via custom `0x02` (Login Success) packets.
* **Dynamic Chunk Generation:** Instead of storing a 12KB chunk file in the limited flash memory, the code dynamically generates a 16x256x16 chunk layout (with a single Grass Block at the center) on the fly using a targeted loop. 
* **Instant Teleportation:** Manipulates `0x08` (Player Position) packets to drop the client directly on top of the generated block in Creative Mode.
* **Anti-Timeout Shield:** Background `KeepAlive` packets prevent the strict Minecraft client from disconnecting the ESP32.

## 🛠️ How It Works (Under the Hood)

When you connect to the ESP32 IP, the server ignores incoming movement packets to save buffer space and fires back the exact byte-sequence the game client demands to render a world:

1.  **Handshake & Login:** Ignored. ESP32 forcefully replies with `Packet 0x02`, assigning a dummy UUID.
2.  **Join Game (`0x01`):** Sends Gamemode 1 (Creative), Overworld dimension, and max players.
3.  **Chunk Data (`0x21`):** Generates `12,544 bytes` of uncompressed, structured block/lighting data in real-time, injecting `0x20` (Grass Block) precisely at `0,0,0`.
4.  **Player Position (`0x08`):** Sets X:0.5, Y:2.0, Z:0.5 to drop the player right onto the matrix.

## 🚀 Getting Started

### Prerequisites
* An ESP32 board (ESP32-C3, WROOM, etc.)
* VS Code with [PlatformIO](https://platformio.org/) installed
* Minecraft Java Edition **1.8.x**

### Installation
1.  Clone this repository:
    ```bash
    git clone [https://https://github.com/bayeggex/MicroCraft-ESP32.git](https://https://github.com/bayeggex/MicroCraft-ESP32.git)
    ```
2.  Open the project in PlatformIO.
3.  Open `src/main.cpp` and update your Wi-Fi credentials:
    ```cpp
    const char* WIFI_SSID     = "YOUR_WIFI_SSID";
    const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
    ```
4.  Build and flash the code to your ESP32.
5.  Open the Serial Monitor (Baud: 115200) to find the ESP32's local IP address.
6.  Open Minecraft 1.8.8, go to Multiplayer -> Add Server, and enter the IP.
7.  Join and enjoy the hardware flex!

## ⚠️ Disclaimer
This is a proof-of-concept project for educational and reverse-engineering purposes. It is not a fully functional survival server. Block breaking/placing is client-side only (ghost blocks), as the server drops incoming block-change packets to prevent memory overflow.

---

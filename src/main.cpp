/**
 * @file      main.cpp
 * @author    bayeggex
 * @brief     Micro Minecraft Server Protocol implementation for ESP32.
 * Generates a dynamic 1-block Skyblock world in Creative mode
 * without relying on external SD cards or large RAM footprints.
 * * @note      Target Client Version: Minecraft 1.8.x
 */

#include <WiFi.h>

// --- NETWORK CONFIGURATION ---
const char* WIFI_SSID     = "TURKNET_AC0C3";
const char* WIFI_PASSWORD = "zcYzS6DA";
const int   SERVER_PORT   = 25565;

WiFiServer mcServer(SERVER_PORT);

// --- PROTOCOL HELPERS ---

/**
 * @brief Writes an integer as a Minecraft VarInt to the network client.
 */
void writeVarInt(WiFiClient &client, int value) {
  while (true) {
    if ((value & ~0x7F) == 0) {
      client.write(value);
      return;
    }
    client.write((value & 0x7F) | 0x80);
    value >>= 7;
  }
}

/**
 * @brief Writes a Minecraft-formatted String (VarInt length + characters).
 */
void writeString(WiFiClient &client, String str) {
  writeVarInt(client, str.length());
  client.print(str);
}

// --- WORLD GENERATION & PACKETS ---

/**
 * @brief Packet 0x02 & 0x01: Skips authentication and forces player login.
 */
void sendLoginSequence(WiFiClient &client, String username) {
  // 1. Login Success (0x02)
  String uuid = "00000000-0000-0000-0000-000000000000"; // Dummy UUID
  writeVarInt(client, 1 + uuid.length() + 1 + username.length() + 1); 
  writeVarInt(client, 0x02); 
  writeString(client, uuid);
  writeString(client, username);

  // 2. Join Game (0x01)
  String levelType = "default";
  int joinLen = 1 + 4 + 1 + 1 + 1 + 1 + (1 + levelType.length()) + 1;
  writeVarInt(client, joinLen); 
  writeVarInt(client, 0x01);    
  
  // Entity ID (0)
  client.write((uint8_t)0); client.write((uint8_t)0); client.write((uint8_t)0); client.write((uint8_t)1); 
  client.write((uint8_t)1); // Gamemode: 1 (Creative)
  client.write((uint8_t)0); // Dimension: 0 (Overworld)
  client.write((uint8_t)0); // Difficulty: 0 (Peaceful)
  client.write((uint8_t)20); // Max Players
  writeString(client, levelType);
  client.write((uint8_t)0); // Reduced Debug Info: false
}

/**
 * @brief Packet 0x21: Generates a 16x256x16 chunk dynamically.
 * Injects a single Grass Block at X:0, Y:0, Z:0.
 */
void sendDynamicSkyblock(WiFiClient &client) {
  int chunkX = 0;
  int chunkZ = 0;
  bool continuous = true;
  uint16_t bitmask = 1; // Send only bottom 16x16x16 section
  
  int dataSize = 12544; // Total size for 1 sub-chunk uncompressed data
  int packetLen = 1 + 4 + 4 + 1 + 2 + 2 + dataSize; 
  
  writeVarInt(client, packetLen);
  writeVarInt(client, 0x21); // Packet ID
  
  // Coordinates
  client.write((uint8_t)(chunkX >> 24)); client.write((uint8_t)(chunkX >> 16)); client.write((uint8_t)(chunkX >> 8)); client.write((uint8_t)chunkX);
  client.write((uint8_t)(chunkZ >> 24)); client.write((uint8_t)(chunkZ >> 16)); client.write((uint8_t)(chunkZ >> 8)); client.write((uint8_t)chunkZ);
  
  client.write((uint8_t)continuous);
  client.write((uint8_t)(bitmask >> 8)); client.write((uint8_t)bitmask);
  
  writeVarInt(client, dataSize);
  
  // Block Data Generation
  for(int i = 0; i < 4096; i++) {
    if(i == 0) { 
      client.write((uint8_t)0x20); // Inject Grass Block (ID: 2 << 4)
      client.write((uint8_t)0x00);
    } else { 
      client.write((uint8_t)0x00); // Air
      client.write((uint8_t)0x00);
    }
  }
  
  for(int i = 0; i < 2048; i++) client.write((uint8_t)0xFF); // Block Light
  for(int i = 0; i < 2048; i++) client.write((uint8_t)0xFF); // Sky Light
  for(int i = 0; i < 256; i++) client.write((uint8_t)0x01);  // Biome: Plains
  
  Serial.println("[SERVER] Generated dynamic single-block world data.");
}

/**
 * @brief Packet 0x08: Teleports player to the top of the generated block.
 */
void teleportToSkyblock(WiFiClient &client) {
  writeVarInt(client, 34); 
  writeVarInt(client, 0x08); // Packet ID
  
  // X = 0.5 (Center of block) - IEEE 754 format
  client.write((uint8_t)0x3F); client.write((uint8_t)0xE0);
  for(int i=0; i<6; i++) client.write((uint8_t)0);
  
  // Y = 2.0 (Drop above block)
  client.write((uint8_t)0x40); client.write((uint8_t)0x00);
  for(int i=0; i<6; i++) client.write((uint8_t)0);
  
  // Z = 0.5 (Center of block)
  client.write((uint8_t)0x3F); client.write((uint8_t)0xE0);
  for(int i=0; i<6; i++) client.write((uint8_t)0);
  
  for(int i=0; i<8; i++) client.write((uint8_t)0); // Yaw & Pitch
  client.write((uint8_t)0); // Flags
}

// --- MAIN CORE ---

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n[SYSTEM] Booting ESP32 Micro-Minecraft Server...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n[SYSTEM] Network Connected!");
  Serial.print("[SYSTEM] Connect your Minecraft to IP: ");
  Serial.println(WiFi.localIP());
  
  mcServer.begin();
  Serial.println("[SERVER] Listening on port 25565.");
}

void loop() {
  WiFiClient client = mcServer.available();   
  
  if (client) {
    if (client.connected() && client.available()) {
      Serial.println("\n[NETWORK] Incoming connection detected...");
      
      // Flush incoming Handshake & Login packets
      delay(100); 
      while(client.available()) client.read(); 

      // Execute Protocol Bypass Sequence
      sendLoginSequence(client, "bayeggex");
      sendDynamicSkyblock(client);
      teleportToSkyblock(client);
      
      Serial.println("[SERVER] Player successfully injected into the matrix.");
      
      unsigned long keepAliveTimer = millis();

      // Main Server Loop per Client
      while(client.connected()) {
        
        // Discard incoming player movements/actions to prevent buffer overflow
        while(client.available()) {
            client.read();
        }

        // Packet 0x00: KeepAlive (Prevent Timeout)
        if (millis() - keepAliveTimer > 2000) {
            writeVarInt(client, 2);    
            writeVarInt(client, 0x00); 
            writeVarInt(client, 0);    
            keepAliveTimer = millis();
        }
        
        delay(10); // Yield to prevent watchdog reset
      }
      
      Serial.println("[NETWORK] Client disconnected.");
    }
  }
}
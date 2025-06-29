#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <time.h>

// WiFi credentials
#define WIFI_SSID "Minh"
#define WIFI_PASSWORD "7777777777"

// Firebase configuration
#define FIREBASE_HOST "auto-checkout-b3ea1-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "cGZLewEccSoy4R1Z4wZKdjYGtW41JYKVG6UFACt8"

// RFID pins
#define RST_PIN 17
#define SS_PIN 16

// Servo pin
#define SERVO_PIN 13

// Initialize objects
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo gateServo;
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// NTP server configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT+7 for Vietnam (7 * 3600)
const int daylightOffset_sec = 0;

// RFID card database - map card UIDs to names
struct CardData {
  String uid;
  String name;
};

CardData authorizedCards[] = {
  {"1CB18048", "104240035@student,vgu,edu,vn"},
  {"1CB18548", "Bob"},
  {"E5F6G7H8", "Charlie"},
  // Add more cards as needed
};

const int numCards = sizeof(authorizedCards) / sizeof(authorizedCards[0]);

// Variable to track previous in_process state
int previousInProcess = 0;

// Add duplicate detection variables (from scanning.ino)
byte lastUID[10];
byte lastUIDSize = 0;
unsigned long lastReadTime = 0;
const unsigned long CARD_READ_INTERVAL = 3000; // 3 seconds between same card reads

void setup() {
  Serial.begin(115200);
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Initialize servo
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Initial position
  
  // Initialize duplicate detection
  lastUIDSize = 0;
  lastReadTime = 0;
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time synchronization...");
  
  // Wait for time to be set
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (attempts < 10) {
    Serial.println("\nTime synchronized!");
    Serial.println(&timeinfo, "Current time: %A, %B %d %Y %H:%M:%S");
  } else {
    Serial.println("\nFailed to synchronize time");
  }
  
  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Set initial Firebase values
  Firebase.setInt(firebaseData, "/gate1", 0);
  Firebase.setInt(firebaseData, "/in_process", 0);
  Firebase.setInt(firebaseData, "/clear", 0);
  Serial.println("RFID Access Control System Ready");
  Serial.println("Present your card...");
}

void loop() {
  // Check if system is available (in_process = 0)
  if (Firebase.getInt(firebaseData, "/in_process")) {
    int inProcess = firebaseData.intData();
    
    // Check if in_process changed from 1 to 0
    if (previousInProcess == 1 && inProcess == 0) {
      Serial.println("in_process changed from 1 to 0 - Deleting rfid_logs");
      deleteRfidLogs();
    }
    
    // Update previous state
    previousInProcess = inProcess;
    
    if (inProcess != 0) {
      delay(100); // Reduced delay for better responsiveness
      return; // Don't process cards if system is busy
    }
  } else {
    Serial.println("Failed to read in_process status");
    delay(500); // Reduced delay
    return;
  }
  
  // Improved card reading logic (from scanning.ino)
  // Look for new cards - separate checks for better reliability
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }
  
  // Check if it's the same card as before (prevent multiple reads)
  bool sameCard = (mfrc522.uid.size == lastUIDSize);
  if (sameCard) {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] != lastUID[i]) {
        sameCard = false;
        break;
      }
    }
  }
  
  // If same card and read recently, skip
  if (sameCard && (millis() - lastReadTime < CARD_READ_INTERVAL)) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(100);
    return;
  }
  
  // Store current card info
  lastUIDSize = mfrc522.uid.size;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    lastUID[i] = mfrc522.uid.uidByte[i];
  }
  lastReadTime = millis();
  
  // Read card UID
  String cardUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) cardUID += "0";
    cardUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
  
  Serial.print("Card detected: ");
  Serial.println(cardUID);
  
  // Check if card is authorized
  String userName = getAuthorizedUser(cardUID);
  if (userName != "") {
    Serial.print("Access granted for: ");
    Serial.println(userName);
    
    // Process authorized access
    processAuthorizedAccess(userName);
  } else {
    Serial.println("Access denied - Unknown card");
  }
  
  // Properly halt PICC and stop encryption (from scanning.ino)
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  delay(2000); // Wait before next read
  Serial.println("Ready for next card...");
}

String getAuthorizedUser(String cardUID) {
  for (int i = 0; i < numCards; i++) {
    if (authorizedCards[i].uid == cardUID) {
      return authorizedCards[i].name;
    }
  }
  return ""; // Card not found
}

String getCurrentDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Unknown";
  }
  
  // Format as YYYY-MM-DD
  char dateString[11];
  strftime(dateString, sizeof(dateString), "%Y-%m-%d", &timeinfo);
  return String(dateString);
}

void processAuthorizedAccess(String userName) {
  // Set in_process = 1 to indicate system is busy
  if (Firebase.setInt(firebaseData, "/in_process", 1)) {
    Serial.println("System marked as busy (in_process = 1)");
  } else {
    Serial.println("Failed to set in_process flag");
  }
  
  // Get current date in YYYY-MM-DD format
  String currentDate = getCurrentDate();
  Serial.print("Current date: ");
  Serial.println(currentDate);
  
  // Upload user data to Firebase with username as container and date instead of timestamp
  String firebasePath = "/rfid_logs/" + userName + "/date";
  
  // Store current date in the user's container
  if (Firebase.setString(firebaseData, firebasePath, currentDate)) {
    Serial.println("User access logged successfully at: " + firebasePath);
    Serial.println("Date stored: " + currentDate);
  } else {
    Serial.println("Failed to log user access");
    Serial.println(firebaseData.errorReason());
  }
  
  // Set gate1 = 1 and control servo
  if (Firebase.setInt(firebaseData, "/gate1", 1)) {
    Serial.println("Gate1 set to 1");
    
    // Spin servo 90 degrees
    Serial.println("Opening gate...");
    gateServo.write(90);
    
    // Hold for 5 seconds
    delay(5000);
    
    // Return servo to original position
    Serial.println("Closing gate...");
    gateServo.write(00);
    
    // Set gate1 = 0
    if (Firebase.setInt(firebaseData, "/gate1", 0)) {
      Serial.println("Gate1 set to 0");
    } else {
      Serial.println("Failed to set gate1 to 0");
      Serial.println(firebaseData.errorReason());
    }
    
  } else {
    Serial.println("Failed to set gate1 to 1");
    Serial.println(firebaseData.errorReason());
    
    // Reset in_process even if there was an error
    Firebase.setInt(firebaseData, "/in_process", 0);
  }
}

// Function to delete rfid_logs when in_process changes from 1 to 0
void deleteRfidLogs() {
  if (Firebase.deleteNode(firebaseData, "/rfid_logs")) {
    Serial.println("rfid_logs deleted successfully");
  } else {
    Serial.println("Failed to delete rfid_logs");
    Serial.println(firebaseData.errorReason());
  }
}

// Function to add new authorized cards (call this in setup if needed)
void addAuthorizedCard(String uid, String name) {
  // This is a simple example - in a real system you might want
  // dynamic card management through Firebase
  Serial.print("Card ");
  Serial.print(uid);
  Serial.print(" authorized for ");
  Serial.println(name);
}
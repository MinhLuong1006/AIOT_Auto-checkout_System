#include <WiFi.h>
#include <FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// Wi-Fi credentials
#define WIFI_SSID "Minh"
#define WIFI_PASSWORD "7777777777"

// Firebase credentials
#define FIREBASE_HOST "auto-checkout-b3ea1-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "cGZLewEccSoy4R1Z4wZKdjYGtW41JYKVG6UFACt8"

// Pins
#define BUZZER_PIN 12
#define SS_PIN 16
#define RST_PIN 17
#define SERVO_PIN 13
#define SERVO2_PIN 4  // New servo pin

// RFID setup
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Firebase objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Servos
Servo gateServo;
Servo clearServo;  // New servo for clear functionality

// RFID card database
struct CardData {
  String uid;
  String name;
};

CardData authorizedCards[] = {
  {"1CB18048", "104240035@student,vgu,edu,vn"},
  {"1CB18078", "Mike"},
  {"1CB18548", "Bob"},
  {"0C438448", "Bob2"}
};

const int numCards = sizeof(authorizedCards) / sizeof(authorizedCards[0]);

// Variables
String currentActiveUser = "";
int previousInProcess = -1;
int previousClear = -1;  // Track previous clear value

// RFID reliability variables
byte lastUID[10];
byte lastUIDSize = 0;
unsigned long lastReadTime = 0;
const unsigned long RFID_READ_INTERVAL = 1000; // 1 second between reads of SAME card
unsigned long lastRFIDCheck = 0;
const unsigned long RFID_CHECK_INTERVAL = 100; // Check every 100ms
const unsigned long CARD_MEMORY_TIMEOUT = 3000; // Clear card memory after 3 seconds

// Firebase operation intervals
unsigned long lastFirebaseUpdate = 0;
const unsigned long FIREBASE_UPDATE_INTERVAL = 500; // 500ms between Firebase operations

void setup() {
  Serial.begin(115200);
  
  // Initialize SPI and MFRC522
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Verify RFID reader initialization
  if (!verifyRFIDReader()) {
    Serial.println("RFID reader initialization failed!");
    while (1) {
      delay(1000);
      Serial.println("Retrying RFID initialization...");
      mfrc522.PCD_Init();
      if (verifyRFIDReader()) {
        Serial.println("RFID reader initialized successfully!");
        break;
      }
    }
  }

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");

  // Firebase setup
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Servo setup
  gateServo.attach(SERVO_PIN);
  gateServo.write(0); // Start closed
  
  clearServo.attach(SERVO2_PIN);  // Attach second servo
  clearServo.write(90);            // Start at original position (0 degrees)
  
  Serial.println("RFID Exit Gate Ready with dual servo control");
}

void loop() {
  // Handle Firebase operations at controlled intervals
  if (millis() - lastFirebaseUpdate > FIREBASE_UPDATE_INTERVAL) {
    updateActiveUser();
    monitorInProcess();
    monitorClear();  // Monitor clear value changes
    lastFirebaseUpdate = millis();
  }
  
  // Handle RFID reading at controlled intervals
  if (millis() - lastRFIDCheck > RFID_CHECK_INTERVAL) {
    handleRFIDReading();
    lastRFIDCheck = millis();
  }
  
  // Small delay to prevent overwhelming the system
  delay(10);
}

bool verifyRFIDReader() {
  // Check if RFID reader is responding
  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("MFRC522 Version: 0x");
  Serial.println(version, HEX);
  
  // Valid version numbers for MFRC522
  return (version == 0x91 || version == 0x92);
}

void handleRFIDReading() {
  // Check for new card presence
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Try to read the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Convert UID to hex string
  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();
  Serial.println("Scanned UID: " + uidStr);

  // Process the card
  processCard(uidStr);

  // Properly halt the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  // Add a small delay to prevent rapid re-reading
  delay(500);
}

void processCard(String uidStr) {
  // Get current in_process status
  int inProcess = 0;
  if (!Firebase.getInt(firebaseData, "/in_process")) {
    Serial.println("Failed to read in_process status");
    return;
  }
  inProcess = firebaseData.intData();

  // Check if UID is valid
  String username = getAuthorizedUser(uidStr);
  
  if (username != "") {
    Serial.println("Access granted for " + username);

    // Check if this user matches the current active user and system is in process
    if (inProcess == 1 && username == currentActiveUser) {
      Serial.println("Valid exit attempt by active user: " + username);
      
      // Check "over" value from Firebase before proceeding
      int overValue = 0;
      if (!Firebase.getInt(firebaseData, "/over")) {
        Serial.println("Failed to read over value: " + firebaseData.errorReason());
        return;
      }
      overValue = firebaseData.intData();
      Serial.println("Over value: " + String(overValue));
      
      // Only proceed if "over" equals 1
      if (overValue == 1) {
        Serial.println("over = 1, setting clear to 1");
        
        // Set clear to 1 since over = 1 and card was scanned
        if (Firebase.setInt(firebaseData, "/clear", 1)) {
          Serial.println("clear set to 1 - Card scanned with over = 1");
        } else {
          Serial.println("Failed to set clear: " + firebaseData.errorReason());
        }
      } else {
        Serial.println("Access denied - over != 1. Current over value: " + String(overValue));
        Serial.println("Gate will not open until over = 1");
      }
    } else if (inProcess == 0) {
      Serial.println("No active session. User must enter through entrance first.");
    } else if (username != currentActiveUser) {
      Serial.println("Access denied - Different user is currently active: " + currentActiveUser);
    }
  } else {
    Serial.println("Unknown card - Access denied");
  }
}

String getAuthorizedUser(String cardUID) {
  for (int i = 0; i < numCards; i++) {
    if (authorizedCards[i].uid == cardUID) {
      return authorizedCards[i].name;
    }
  }
  return "";
}

void updateActiveUser() {
  if (!Firebase.get(firebaseData, "/rfid_logs")) {
    if (currentActiveUser != "") {
      Serial.println("rfid_logs cleared - No active session");
      currentActiveUser = "";
    }
    return;
  }
  
  if (firebaseData.dataType() == "json") {
    FirebaseJson json = firebaseData.jsonObject();
    size_t len = json.iteratorBegin();
    String key, value = "";
    int type = 0;
    
    if (len > 0) {
      json.iteratorGet(0, type, key, value);
      if (key != currentActiveUser && key != "") {
        currentActiveUser = key;
        Serial.println("Active user detected: " + currentActiveUser);
      }
    } else {
      if (currentActiveUser != "") {
        Serial.println("No active user session found");
        currentActiveUser = "";
      }
    }
    json.iteratorEnd();
  }
}

void monitorClear() {
  int currentClear = 0;
  if (!Firebase.getInt(firebaseData, "/clear")) {
    Serial.println("Failed to read clear status for monitoring: " + firebaseData.errorReason());
    return;
  }
  currentClear = firebaseData.intData();
  
  // Check for state change from 0 to 1
  if (previousClear == 0 && currentClear == 1) {
    Serial.println("clear changed from 0 to 1 - Moving clear servo to 90 degrees");
    clearServo.write(0);  // Turn to 90 degrees and hold
  }
  
  previousClear = currentClear;
}

void monitorInProcess() {
  int currentInProcess = 0;
  if (!Firebase.getInt(firebaseData, "/in_process")) {
    Serial.println("Failed to read in_process status for monitoring: " + firebaseData.errorReason());
    return;
  }
  currentInProcess = firebaseData.intData();
  
  // Check for state change from 1 to 0
  if (previousInProcess == 1 && currentInProcess == 0) {
    Serial.println("in_process changed from 1 to 0 - Opening gate and returning clear servo to original position");
    
    // Return clear servo to original position (0 degrees)
    clearServo.write(90);
    Serial.println("Clear servo returned to original position (0 degrees)");
    
    if (Firebase.setInt(firebaseData, "/gate2", 1)) {
      Serial.println("gate2 set to 1");
      
      // Buzzer
      digitalWrite(BUZZER_PIN, HIGH);
      delay(300);
      digitalWrite(BUZZER_PIN, LOW);

      // Open gate
      Serial.println("Opening exit gate...");
      gateServo.write(90);
      delay(5000);

      // Close gate
      Serial.println("Closing exit gate...");
      gateServo.write(0);

      // Reset gate2
      if (Firebase.setInt(firebaseData, "/gate2", 0)) {
        Serial.println("gate2 reset to 0");
      } else {
        Serial.println("Failed to reset gate2: " + firebaseData.errorReason());
      }
      
      // Reset over values
      Firebase.setInt(firebaseData, "/over", 0);
      Firebase.setInt(firebaseData, "/over1", 0);
      Firebase.setInt(firebaseData, "/over2", 0);
      Serial.println("Reset over, over1, and over2 to 0");
      
      // Clear display data
      if (Firebase.deleteNode(firebaseData, "/display/items")) {
        Serial.println("display/items node deleted successfully");
      }
      
      if (Firebase.setFloat(firebaseData, "/display/total", 0.0)) {
        Serial.println("display/total reset to 0");
      }
      
    } else {
      Serial.println("Failed to set gate2: " + firebaseData.errorReason());
    }
  }
  
  previousInProcess = currentInProcess;
}
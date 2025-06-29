#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <time.h>

// WiFi credentials
#define WIFI_SSID "Minh"
#define WIFI_PASSWORD "7777777777"

// Firebase credentials
#define FIREBASE_HOST "auto-checkout-b3ea1-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "cGZLewEccSoy4R1Z4wZKdjYGtW41JYKVG6UFACt8"

#define SS_PIN 16
#define RST_PIN 17
#define SERVO_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);
Servo myServo;
//
// Second servo from Singularity.ino
Servo secondServo;
const int secondServoPin = 13; // Change this pin as needed

// Timing variables for second servo
unsigned long lastServoMoveTime = 0;
const unsigned long SERVO_INTERVAL = 5000; // 6 seconds total cycle
bool servoAt90 = true; // Track servo position
// Firebase objects
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// NTP server configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; // GMT+7 for Vietnam (7 * 3600)
const int daylightOffset_sec = 0;

byte lastUID[10];
byte lastUIDSize = 0;
unsigned long lastReadTime = 0;

// Variable to store current active user
String currentActiveUser = "";

// Variable to track last clear check time
unsigned long lastClearCheckTime = 0;
const unsigned long CLEAR_CHECK_INTERVAL = 2000; // Check every 2 seconds

// Variable to track last over check time
unsigned long lastOverCheckTime = 0;
const unsigned long OVER_CHECK_INTERVAL = 1000; // Check every 1 second

// Countdown variables
unsigned long countdownStartTime = 0;
bool countdownActive = false;
const unsigned long COUNTDOWN_DURATION = 15000; // 15 seconds in milliseconds
int lastCountdownValue = 0;
bool over1Uploaded = false;

// Product database - Add your products here
const int MAX_PRODUCTS = 10;
String productUIDs[MAX_PRODUCTS] = {
  "0451DD123A4189",     // kitkat
  "0411FA133A4189",     // coke
  "0421FA133A4189",     // chips
  "04113E163A4189",     // candy
  "0461DD123A4189",     // water
  "",             // Add more as needed
  "",
  "",
  "",
  ""
};

String productNames[MAX_PRODUCTS] = {
  "gum_can",
  "mentos", 
  "MM",
  "tape",
  "triangle",
  "",     // Empty slots
  "",
  "",
  "",
  ""
};

// Product prices (in cents to avoid floating point issues, or use float if preferred)
float productPrices[MAX_PRODUCTS] = {
  1.00,   // gum
  1.50,   // mentos
  2.00,   // MM
  2.50,   // tape
  3.00,   // triangle
  0.00,   // Empty slots
  0.00,
  0.00,
  0.00,
  0.00
};

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();
  myServo.attach(SERVO_PIN);
  myServo.write(80);  // Initial position
  Serial.println("RFID Product Scanner with Pricing Initialized");
  //
  // Initialize second servo
  secondServo.attach(secondServoPin);
  secondServo.write(90); // Initial position
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP: ");
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
  
  // Configure Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("System ready. Scan your RFID card...");
}

void loop() {
  // Handle second servo continuous movement
  handleSecondServo();
  // Check for active user before processing any cards
  if (!updateActiveUser()) {
    delay(1000);
    return;
  }
  //


  // Handle countdown logic
  handleCountdown();
  
  // Check for clear command periodically
  if (millis() - lastClearCheckTime > CLEAR_CHECK_INTERVAL) {
    checkForClearCommand();
    lastClearCheckTime = millis();
  }
  
  // Check for over1 and over2 values periodically
  if (millis() - lastOverCheckTime > OVER_CHECK_INTERVAL) {
    checkOverValues();
    lastOverCheckTime = millis();
  }
  
  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  // Select one of the cards
  if (!rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // Check if it's the same card as before (prevent multiple reads)
  bool sameCard = (rfid.uid.size == lastUIDSize);
  if (sameCard) {
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] != lastUID[i]) {
        sameCard = false;
        break;
      }
    }
  }
  
  // If same card and read recently, skip
  if (sameCard && (millis() - lastReadTime < 3000)) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(100);
    return;
  }
  
  // Store current card info
  lastUIDSize = rfid.uid.size;
  for (byte i = 0; i < rfid.uid.size; i++) {
    lastUID[i] = rfid.uid.uidByte[i];
  }
  lastReadTime = millis();
  
  // Convert UID to string
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  
  Serial.print("Card UID: ");
  Serial.println(uidString);
  
  // Look up product
  String productName = "";
  float productPrice = 0.0;
  bool productFound = false;
  
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (productUIDs[i].length() > 0 && productUIDs[i].equals(uidString)) {
      productName = productNames[i];
      productPrice = productPrices[i];
      productFound = true;
      break;
    }
  }
  
  if (productFound) {
    Serial.print("Product: ");
    Serial.print(productName);
    Serial.print(" - Price: $");
    Serial.println(productPrice, 2);
    

    // Move servo for valid product
    Serial.println("Valid product detected. Moving servo...");
    delay(800);
    myServo.write(0);
    delay(2000);
    myServo.write(80);
    Serial.println("Servo returned to original position");
        // Reset countdown when item is scanned
    resetCountdown();
    // ADD THIS LINE HERE:
    resetOver1IfSet();
    // Increment counter and update total in Firebase for current active user
    incrementProductCounterAndTotal(productName, productPrice);
    
  } else {
    Serial.print("Unknown product! UID: ");
    Serial.println(uidString);
  }

  // Properly halt the card and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  // Wait before next read
  delay(1000);
  Serial.println("Ready for next card...");
}

void checkOverValues() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  String over1Path = "over1";
  String over2Path = "over2";
  String overPath = "over";
  
  int over1Value = 0;
  int over2Value = 0;
  int currentOverValue = 0;
  
  // Get over1 value
  bool over1Success = false;
  if (Firebase.getInt(firebaseData, over1Path)) {
    if (firebaseData.dataType() == "int") {
      over1Value = firebaseData.intData();
      over1Success = true;
    }
  }
  
  // Get over2 value
  bool over2Success = false;
  if (Firebase.getInt(firebaseData, over2Path)) {
    if (firebaseData.dataType() == "int") {
      over2Value = firebaseData.intData();
      over2Success = true;
    }
  }
  
  // Get current "over" value
  bool overSuccess = false;
  if (Firebase.getInt(firebaseData, overPath)) {
    if (firebaseData.dataType() == "int") {
      currentOverValue = firebaseData.intData();
      overSuccess = true;
    }
  }
  
  // Only proceed if we successfully got both over1 and over2 values
  if (over1Success && over2Success) {
    // Case 1: Both over1 and over2 are equal to 1 -> set over = 1
    if (over1Value == 1 && over2Value == 1) {
      // Check if "over" is not already 1 to avoid unnecessary writes
      if (!overSuccess || currentOverValue != 1) {
        if (Firebase.setInt(firebaseData, overPath, 1)) {
          Serial.println("✓ Successfully uploaded 'over = 1' to Firebase!");
        } else {
          Serial.println("✗ Failed to upload 'over = 1' to Firebase!");
          Serial.println("Error: " + firebaseData.errorReason());
        }
      }
    }
    // Case 2: Either over1 or over2 (or both) is not 1 -> set over = 0
    else {
      // Only reset over to 0 if it's currently 1
      if (overSuccess && currentOverValue == 1) {
        if (Firebase.setInt(firebaseData, overPath, 0)) {
          Serial.println("✓ Successfully reset 'over = 0' to Firebase (over1 or over2 went back to 0)!");
        } else {
          Serial.println("✗ Failed to reset 'over = 0' to Firebase!");
          Serial.println("Error: " + firebaseData.errorReason());
        }
      }
    }
  }
}
void handleCountdown() {
  if (!countdownActive) {
    return;
  }
  
  unsigned long elapsed = millis() - countdownStartTime;
  unsigned long remaining = 0;
  
  if (elapsed < COUNTDOWN_DURATION) {
    remaining = COUNTDOWN_DURATION - elapsed;
  } else {
    remaining = 0;
  }
  
  int countdownSeconds = (remaining + 999) / 1000; // Round up to nearest second
  
  // Display countdown every second
  if (countdownSeconds != lastCountdownValue) {
    lastCountdownValue = countdownSeconds;
    if (countdownSeconds > 0) {
      Serial.print("Countdown: ");
      Serial.print(countdownSeconds);
      Serial.println(" seconds remaining");
    }
  }
  
  // Check if countdown reached 0
  if (remaining == 0 && !over1Uploaded) {
    Serial.println("Countdown reached 0! Uploading over1 = 1 to Firebase...");
    uploadOver1ToFirebase();
    countdownActive = false;
    over1Uploaded = true;
  }
}

void resetCountdown() {
  if (!countdownActive) {
    Serial.println("Starting 15-second countdown...");
  } else {
    Serial.println("Resetting countdown to 15 seconds...");
  }
  
  countdownStartTime = millis();
  countdownActive = true;
  lastCountdownValue = 16; // Force display update
  over1Uploaded = false;
}

void uploadOver1ToFirebase() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot upload over1 to Firebase.");
    return;
  }

  String over1Path = "over1";
  
  if (Firebase.setInt(firebaseData, over1Path, 1)) {
    Serial.println("✓ over1 = 1 uploaded successfully to Firebase!");
  } else {
    Serial.println("✗ Failed to upload over1 to Firebase!");
    Serial.println("Error: " + firebaseData.errorReason());
  }
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

String generateRandomID() {
  String randomID = "";
  const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  
  // Generate a 10-character random ID
  for (int i = 0; i < 10; i++) {
    randomID += chars[random(0, strlen(chars))];
  }
  
  return randomID;
}
////////////////decrease storage
void decreaseInventory(String productName) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot update inventory.");
    return;
  }
  
  String storagePath = "storage/" + productName;
  
  Serial.print("Checking inventory for: ");
  Serial.println(productName);
  
  // Get current count from Firebase
  if (Firebase.getInt(firebaseData, storagePath)) {
    int currentCount = 0;
    
    if (firebaseData.dataType() == "int") {
      currentCount = firebaseData.intData();
    } else {
      Serial.println("Inventory data type is not integer, assuming 0");
      currentCount = 0;
    }
    
    Serial.print("Current inventory count for ");
    Serial.print(productName);
    Serial.print(": ");
    Serial.println(currentCount);
    
    if (currentCount > 0) {
      int newCount = currentCount - 1;
      
      // Update inventory in Firebase
      if (Firebase.setInt(firebaseData, storagePath, newCount)) {
        Serial.println("✓ Inventory updated successfully!");
        Serial.print(productName);
        Serial.print(" inventory decreased from ");
        Serial.print(currentCount);
        Serial.print(" to ");
        Serial.println(newCount);
      } else {
        Serial.println("✗ Failed to update inventory!");
        Serial.println("Error: " + firebaseData.errorReason());
      }
    } else {
      Serial.println("⚠️ WARNING: Product is out of stock!");
      Serial.print(productName);
      Serial.println(" inventory is already 0 or negative");
    }
  } else {
    Serial.println("Failed to read current inventory count");
    Serial.println("Error: " + firebaseData.errorReason());
    
    // If the storage path doesn't exist, you might want to create it with 0
    // or handle this case according to your needs
    Serial.println("Creating new inventory entry with count 0");
    if (Firebase.setInt(firebaseData, storagePath, 0)) {
      Serial.println("✓ New inventory entry created");
    } else {
      Serial.println("✗ Failed to create inventory entry");
    }
  }
}
/////////////////////
float getProductPrice(String productName) {
  for (int i = 0; i < MAX_PRODUCTS; i++) {
    if (productNames[i] == productName) {
      return productPrices[i];
    }
  }
  return 0.0; // Return 0 if product not found
}

bool updateActiveUser() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return false;
  }

  // Get the first user key from rfid_logs (since we only have one active user at a time)
  if (Firebase.get(firebaseData, "/rfid_logs")) {
    if (firebaseData.dataType() == "json") {
      FirebaseJson json = firebaseData.jsonObject();
      FirebaseJsonData jsonData;
      
      // Get all keys from rfid_logs
      size_t len = json.iteratorBegin();
      String key, value = "";
      int type = 0;
      
      if (len > 0) {
        // Get the first key (should be the active user)
        json.iteratorGet(0, type, key, value);
        if (key != currentActiveUser) {
          currentActiveUser = key;
          Serial.print("Active user updated to: ");
          Serial.println(currentActiveUser);
        }
        json.iteratorEnd();
        return true;
      } else {
        if (currentActiveUser != "") {
          Serial.println("No active user found. Waiting...");
          currentActiveUser = "";
        }
        json.iteratorEnd();
        return false;
      }
    } else {
      if (currentActiveUser != "") {
        Serial.println("No active user found. Waiting...");
        currentActiveUser = "";
      }
      return false;
    }
  } else {
    Serial.println("Failed to get rfid_logs data");
    Serial.println("Error: " + firebaseData.errorReason());
    return false;
  }
}

void incrementProductCounterAndTotal(String productName, float productPrice) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot upload to Firebase.");
    return;
  }

  if (currentActiveUser == "") {
    Serial.println("No active user. Cannot log product.");
    return;
  }
  //
  decreaseInventory(productName);
  // Existing paths for rfid_logs
  String itemPath = "rfid_logs/" + currentActiveUser + "/items/" + productName;
  String totalPath = "rfid_logs/" + currentActiveUser + "/total";
  
  // New paths for display (outside rfid_logs)
  String displayItemPath = "display/items/" + productName;
  String displayTotalPath = "display/total";  // New path for display total
  
  int currentCount = 0;
  int displayCurrentCount = 0;
  float currentTotal = 0.0;

  Serial.print("Processing product: ");
  Serial.print(productName);
  Serial.print(" (Price: $");
  Serial.print(productPrice, 2);
  Serial.print(") for user: ");
  Serial.println(currentActiveUser);

  // Get current item count from Firebase (rfid_logs)
  if (Firebase.getInt(firebaseData, itemPath)) {
    if (firebaseData.dataType() == "int") {
      currentCount = firebaseData.intData();
      Serial.print("Current count in rfid_logs for ");
      Serial.print(productName);
      Serial.print(": ");
      Serial.println(currentCount);
    } else {
      Serial.println("Item count data type is not an integer, assuming 0.");
      currentCount = 0;
    }
  } else {
    Serial.println("Failed to get current item count from rfid_logs, assuming 0.");
    Serial.println("Error: " + firebaseData.errorReason());
    currentCount = 0;
  }

  // Get current item count from Firebase (display/items)
  if (Firebase.getInt(firebaseData, displayItemPath)) {
    if (firebaseData.dataType() == "int") {
      displayCurrentCount = firebaseData.intData();
      Serial.print("Current count in display/items for ");
      Serial.print(productName);
      Serial.print(": ");
      Serial.println(displayCurrentCount);
    } else {
      Serial.println("Display item count data type is not an integer, assuming 0.");
      displayCurrentCount = 0;
    }
  } else {
    Serial.println("Failed to get current display item count, assuming 0.");
    Serial.println("Error: " + firebaseData.errorReason());
    displayCurrentCount = 0;
  }

  // Get current total from Firebase
  if (Firebase.getFloat(firebaseData, totalPath)) {
    if (firebaseData.dataType() == "float" || firebaseData.dataType() == "double") {
      currentTotal = firebaseData.floatData();
      Serial.print("Current total: $");
      Serial.println(currentTotal, 2);
    } else if (firebaseData.dataType() == "int") {
      currentTotal = (float)firebaseData.intData();
      Serial.print("Current total (from int): $");
      Serial.println(currentTotal, 2);
    } else {
      Serial.println("Total data type is unexpected, assuming 0.");
      currentTotal = 0.0;
    }
  } else {
    Serial.println("Failed to get current total, assuming 0.");
    Serial.println("Error: " + firebaseData.errorReason());
    currentTotal = 0.0;
  }

  // Calculate new values
  int newCount = currentCount + 1;
  int displayNewCount = displayCurrentCount + 1;
  float newTotal = currentTotal + productPrice;

  Serial.print("New count for rfid_logs: ");
  Serial.println(newCount);
  Serial.print("New count for display/items: ");
  Serial.println(displayNewCount);
  Serial.print("New total: $");
  Serial.println(newTotal, 2);

  // Update item count in Firebase (rfid_logs)
  if (Firebase.setInt(firebaseData, itemPath, newCount)) {
    Serial.println("✓ Item counter updated successfully in rfid_logs!");
  } else {
    Serial.println("✗ Failed to update item counter in rfid_logs!");
    Serial.println("Error: " + firebaseData.errorReason());
    return; // Don't continue if rfid_logs update fails
  }

  // Update item count in Firebase (display/items)
  if (Firebase.setInt(firebaseData, displayItemPath, displayNewCount)) {
    Serial.println("✓ Item counter updated successfully in display/items!");
  } else {
    Serial.println("✗ Failed to update item counter in display/items!");
    Serial.println("Error: " + firebaseData.errorReason());
    // Continue even if display/items update fails, as rfid_logs is the primary storage
  }

  // Update total in Firebase (rfid_logs)
  if (Firebase.setFloat(firebaseData, totalPath, newTotal)) {
    Serial.println("✓ Total updated successfully in rfid_logs!");
  } else {
    Serial.println("✗ Failed to update total in rfid_logs!");
    Serial.println("Error: " + firebaseData.errorReason());
  }

  // Update total in Firebase (display/total) - NEW
  if (Firebase.setFloat(firebaseData, displayTotalPath, newTotal)) {
    Serial.println("✓ Total updated successfully in display/total!");
  } else {
    Serial.println("✗ Failed to update total in display/total!");
    Serial.println("Error: " + firebaseData.errorReason());
    // Continue even if display/total update fails, as rfid_logs is the primary storage
  }

  // Final status message
  Serial.print("User: ");
  Serial.print(currentActiveUser);
  Serial.print(" - Total: $");
  Serial.println(newTotal, 2);
  Serial.print("Last item: ");
  Serial.print(productName);
  Serial.print(" (rfid_logs: ");
  Serial.print(newCount);
  Serial.print(" items, display: ");
  Serial.print(displayNewCount);
  Serial.println(" items)");
}

void createPurchaseHistory() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot create purchase history.");
    return;
  }

  if (currentActiveUser == "") {
    Serial.println("No active user. Cannot create purchase history.");
    return;
  }

  Serial.println("Creating purchase history...");

  // Get date from rfid_logs
  String datePath = "rfid_logs/" + currentActiveUser + "/date";
  String purchaseDate = "";
  
  if (Firebase.getString(firebaseData, datePath)) {
    if (firebaseData.dataType() == "string") {
      purchaseDate = firebaseData.stringData();
      Serial.print("Purchase date: ");
      Serial.println(purchaseDate);
    } else {
      purchaseDate = getCurrentDate(); // Fallback to current date
      Serial.print("Using current date as fallback: ");
      Serial.println(purchaseDate);
    }
  } else {
    purchaseDate = getCurrentDate(); // Fallback to current date
    Serial.print("Failed to get date, using current date: ");
    Serial.println(purchaseDate);
  }

  // Get items from rfid_logs
  String itemsPath = "rfid_logs/" + currentActiveUser + "/items";
  
  if (Firebase.get(firebaseData, itemsPath)) {
    if (firebaseData.dataType() == "json") {
      FirebaseJson itemsJson = firebaseData.jsonObject();
      
      // Iterate through all items
      size_t len = itemsJson.iteratorBegin();
      String key, value = "";
      int type = 0;
      
      for (size_t i = 0; i < len; i++) {
        itemsJson.iteratorGet(i, type, key, value);
        
        String itemName = key;
        int itemAmount = value.toInt();
        float itemPrice = getProductPrice(itemName);
        
        // Generate random ID for this purchase record
        String randomID = generateRandomID();
        
        // Create history entry path
        String historyBasePath = "users/" + currentActiveUser + "/history/" + randomID;
        
        Serial.print("Creating history entry for: ");
        Serial.print(itemName);
        Serial.print(" (Amount: ");
        Serial.print(itemAmount);
        Serial.print(", Price: $");
        Serial.print(itemPrice * itemAmount, 2);
        Serial.println(")");
        
        // Create the history entry
        if (Firebase.setString(firebaseData, historyBasePath + "/item", itemName)) {
          Serial.println("✓ Item name saved");
        } else {
          Serial.println("✗ Failed to save item name");
          continue;
        }
        
        if (Firebase.setString(firebaseData, historyBasePath + "/date", purchaseDate)) {
          Serial.println("✓ Date saved");
        } else {
          Serial.println("✗ Failed to save date");
        }
        
        if (Firebase.setInt(firebaseData, historyBasePath + "/amount", itemAmount)) {
          Serial.println("✓ Amount saved");
        } else {
          Serial.println("✗ Failed to save amount");
        }
        
        if (Firebase.setFloat(firebaseData, historyBasePath + "/price", itemPrice * itemAmount)) {
          Serial.println("✓ Price saved");
        } else {
          Serial.println("✗ Failed to save price");
        }
        
        Serial.print("History entry created with ID: ");
        Serial.println(randomID);
        Serial.println("---");
      }
      
      itemsJson.iteratorEnd();
      Serial.println("Purchase history creation completed!");
      
    } else {
      Serial.println("Items data is not in JSON format");
    }
  } else {
    Serial.println("Failed to get items data for history");
    Serial.println("Error: " + firebaseData.errorReason());
  }
}

void checkForClearCommand() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (currentActiveUser == "") {
    return;
  }

  String clearPath = "clear";
  
  // Check if clear is set to 1
  if (Firebase.getInt(firebaseData, clearPath)) {
    if (firebaseData.dataType() == "int" && firebaseData.intData() == 1) {
      Serial.println("Clear command detected! Processing checkout...");
      
      // Create purchase history before processing checkout
      createPurchaseHistory();
      
      processCheckout();
    }
  } else {
    // Clear might not exist yet, which is normal
    // Serial.println("No clear command found or error: " + firebaseData.errorReason());
  }
}

void processCheckout() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot process checkout.");
    return;
  }

  if (currentActiveUser == "") {
    Serial.println("No active user. Cannot process checkout.");
    return;
  }

  String totalPath = "rfid_logs/" + currentActiveUser + "/total";
  String userBalancePath = "users/" + currentActiveUser + "/balance";
  String clearPath = "clear";
  
  float currentTotal = 0.0;
  float userBalance = 0.0;

  Serial.print("Processing checkout for user: ");
  Serial.println(currentActiveUser);

  // Get current total from rfid_logs
  if (Firebase.getFloat(firebaseData, totalPath)) {
    if (firebaseData.dataType() == "float" || firebaseData.dataType() == "double") {
      currentTotal = firebaseData.floatData();
    } else if (firebaseData.dataType() == "int") {
      currentTotal = (float)firebaseData.intData();
    } else {
      Serial.println("Could not get total amount. Checkout cancelled.");
      resetClearFlag();
      return;
    }
  } else {
    Serial.println("Failed to get total amount. Checkout cancelled.");
    Serial.println("Error: " + firebaseData.errorReason());
    resetClearFlag();
    return;
  }

  Serial.print("Total amount to deduct: $");
  Serial.println(currentTotal, 2);

  // Get user's current balance
  if (Firebase.getFloat(firebaseData, userBalancePath)) {
    if (firebaseData.dataType() == "float" || firebaseData.dataType() == "double") {
      userBalance = firebaseData.floatData();
    } else if (firebaseData.dataType() == "int") {
      userBalance = (float)firebaseData.intData();
    } else {
      Serial.println("Could not get user balance. Checkout cancelled.");
      resetClearFlag();
      return;
    }
  } else {
    Serial.println("Failed to get user balance. Checkout cancelled.");
    Serial.println("Error: " + firebaseData.errorReason());
    resetClearFlag();
    return;
  }

  Serial.print("Current user balance: $");
  Serial.println(userBalance, 2);

  // Check if user has sufficient balance
  if (userBalance < currentTotal) {
    Serial.println("INSUFFICIENT BALANCE! Checkout cancelled.");
    Serial.print("Required: $");
    Serial.print(currentTotal, 2);
    Serial.print(", Available: $");
    Serial.println(userBalance, 2);
    resetClearFlag();
    return;
  }

  // Calculate new balance
  float newBalance = userBalance - currentTotal;
  Serial.print("New balance will be: $");
  Serial.println(newBalance, 2);

  // Update user's balance
  if (Firebase.setFloat(firebaseData, userBalancePath, newBalance)) {
    Serial.println("✓ User balance updated successfully!");
    
    // Reset the total to 0
    if (Firebase.setFloat(firebaseData, totalPath, 0.0)) {
      Serial.println("✓ Shopping total reset to $0.00");
      
      // Reset clear flag to 0
      if (Firebase.setInt(firebaseData, clearPath, 0)) {
        Serial.println("✓ Clear flag reset successfully");
        Serial.println("=== CHECKOUT COMPLETED SUCCESSFULLY ===");
        Serial.print("User: ");
        Serial.println(currentActiveUser);
        Serial.print("Amount charged: $");
        Serial.println(currentTotal, 2);
        Serial.print("Remaining balance: $");
        Serial.println(newBalance, 2);
        Serial.println("=====================================");
        
        // Stop countdown after successful checkout
        countdownActive = false;
        over1Uploaded = false;
      } else {
        Serial.println("✗ Failed to reset clear flag");
        Serial.println("Error: " + firebaseData.errorReason());
      }
    } else {
      Serial.println("✗ Failed to reset total");
      Serial.println("Error: " + firebaseData.errorReason());
      resetClearFlag();
    }
  } else {
    Serial.println("✗ Failed to update user balance!");
    Serial.println("Error: " + firebaseData.errorReason());
    resetClearFlag();
  }
}

void resetClearFlag() {
  String clearPath = "clear";
  if (Firebase.setInt(firebaseData, clearPath, 0)) {
    Serial.println("Clear flag reset to 0");
  } else {
    Serial.println("Failed to reset clear flag");
  }
}
void resetOver1IfSet() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  String over1Path = "over1";
  
  // Check if over1 is currently set to 1
  if (Firebase.getInt(firebaseData, over1Path)) {
    if (firebaseData.dataType() == "int" && firebaseData.intData() == 1) {
      // Reset over1 to 0
      if (Firebase.setInt(firebaseData, over1Path, 0)) {
        Serial.println("✓ over1 reset to 0 after product scan");
        over1Uploaded = false; // Allow over1 to be uploaded again when countdown reaches 0
      } else {
        Serial.println("✗ Failed to reset over1 to 0");
        Serial.println("Error: " + firebaseData.errorReason());
      }
    }
  }
}
void handleSecondServo() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastServoMoveTime >= SERVO_INTERVAL) {
    if (servoAt90) {
      // Move to 170 degrees
      secondServo.write(170);
      servoAt90 = false;
      Serial.println("Second servo moved to 170 degrees");
    } else {
      // Move back to 90 degrees
      secondServo.write(90);
      servoAt90 = true;
      Serial.println("Second servo moved to 90 degrees");
    }
    lastServoMoveTime = currentTime;
  }
}
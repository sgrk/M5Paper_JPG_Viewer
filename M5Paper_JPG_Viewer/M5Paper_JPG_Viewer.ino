/*
 * M5Paper JPG Viewer
 * 
 * This sketch displays JPG images from an SD card on the M5Paper e-ink display.
 * 
 * Hardware required:
 * - M5Paper device
 * - SD card with JPG files
 * 
 * The SD card should be inserted into the M5Paper's SD card slot.
 * 
 * Features:
 * - Scans SD card for JPG files (including subdirectories)
 * - Displays images with navigation using buttons
 * - Shows battery level and file information
 * - Supports different refresh modes for better image quality
 */

#include <M5EPD.h>
#include <SD.h>

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas statusBar(&M5.EPD);

// Configuration
const char* DIRECTORY_PATH = "/";  // Root directory of SD card
const int MAX_FILES = 100;         // Maximum number of files to scan
const bool SCAN_SUBDIRECTORIES = true; // Whether to scan subdirectories
const int STATUS_BAR_HEIGHT = 40;  // Height of the status bar in pixels
const int AUTO_ADVANCE_INTERVAL = 10000; // Time in milliseconds between auto-advances (10 seconds)

// Global variables
String jpgFiles[MAX_FILES];        // Array to store JPG file paths
bool AUTO_ADVANCE = true;    // Whether to automatically advance to next image
int fileCount = 0;                 // Number of JPG files found
int currentFileIndex = 0;          // Index of currently displayed file
unsigned long lastButtonPress = 0; // Time of last button press for debouncing
unsigned long lastImageChange = 0; // Time of last image change for auto-advance
unsigned long autoAdvanceOffTime = 0; // Time when AUTO_ADVANCE was turned off
const unsigned long SLEEP_TIMEOUT = 60000; // Time in milliseconds before sleep (1 minute)
bool sleepTimerActive = false;    // Whether the sleep timer is active


void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println("M5Paper JPG Viewer starting...");
  
  // Initialize M5Paper
  M5.begin();
  M5.RTC.begin();  // Initialize RTC for time display
  M5.EPD.SetRotation(1);  // Set rotation to 1 (90 degrees clockwise)
  M5.EPD.Clear(true);      // Clear the screen
  
  // Initialize canvases
  canvas.createCanvas(540, 960 - STATUS_BAR_HEIGHT);  // Main canvas for images
  canvas.setTextSize(2);
  canvas.setTextColor(0);
  
  statusBar.createCanvas(540, STATUS_BAR_HEIGHT);  // Status bar canvas
  statusBar.setTextSize(2);
  statusBar.setTextColor(0);
  
  // Show startup screen
  showStartupScreen();
  
  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("SD Card initialization failed!");
    showErrorMessage("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized successfully");
  
// Scan for JPG files
  scanJpgFiles();
  
  // Check if auto-advance should be disabled based on conditions
  checkAutoAdvanceConditions();
  
  // Display first image if available
  if (fileCount > 0) {
    displayImage(jpgFiles[currentFileIndex]);
  } else {
    showErrorMessage("No JPG files found on SD card!");
  }
}

void loop() {
  M5.update();
  
  // Button handling for navigation with debounce
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > 500) {  // 500ms debounce
    bool buttonPressed = false;
    
    if (M5.BtnL.wasPressed()) {
      // Previous image
      Serial.println("Left button pressed - showing previous image");
      buttonPressed = true;
      currentFileIndex = (currentFileIndex > 0) ? currentFileIndex - 1 : fileCount - 1;
      displayImage(jpgFiles[currentFileIndex]);
    }
    
    if (M5.BtnR.wasPressed()) {
      // Next image
      Serial.println("Right button pressed - showing next image");
      buttonPressed = true;
      currentFileIndex = (currentFileIndex < fileCount - 1) ? currentFileIndex + 1 : 0;
      displayImage(jpgFiles[currentFileIndex]);
    }
    
    if (M5.BtnP.wasPressed()) {
      // Middle button - toggle auto-advance
      Serial.println("Middle button pressed - toggling auto-advance");
      buttonPressed = true;
      toggleAutoAdvance();
    }
    
    if (buttonPressed) {
      lastButtonPress = currentTime;
      lastImageChange = currentTime;  // Reset auto-advance timer
      
      // Reset sleep timer if active
      if (sleepTimerActive && !AUTO_ADVANCE) {
        autoAdvanceOffTime = currentTime;
        Serial.println("Sleep timer reset - device will sleep in 1 minute of inactivity");
      }
    }
  }
  
  // Check auto-advance conditions periodically
  if (currentTime % 30000 == 0) { // Check every 30 seconds
    checkAutoAdvanceConditions();
  }
  
  // Auto-advance to next image if enabled and interval has passed
  if (AUTO_ADVANCE && (currentTime - lastImageChange >= AUTO_ADVANCE_INTERVAL) && fileCount > 1) {
    Serial.println("Auto-advancing to next image");
    lastImageChange = currentTime;
    currentFileIndex = (currentFileIndex < fileCount - 1) ? currentFileIndex + 1 : 0;
    displayImage(jpgFiles[currentFileIndex]);
  }
  
  // Check if sleep timer is active and timeout has been reached
  if (sleepTimerActive && !AUTO_ADVANCE && (currentTime - autoAdvanceOffTime >= SLEEP_TIMEOUT)) {
    Serial.println("Sleep timeout reached - putting device to sleep");
    
    // Show sleep message on status bar
    statusBar.fillCanvas(0xFFFF);
    statusBar.drawString("Device sleeping - Press any button to wake", 10, 10);
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
    
    // Wait for the status message to be displayed
    delay(1000);
    
    // Put the device to sleep while maintaining the display
    // M5.shutdown() would turn off the display, so we use M5.disableEXTPower() instead
    M5.disableEXTPower();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, 0); // Wake on touch
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_37 | GPIO_SEL_39, ESP_EXT1_WAKEUP_ANY_HIGH); // Wake on buttons
    esp_deep_sleep_start();
    
    // Code will resume from setup() after waking up
  }
  
  delay(50);  // Small delay for power efficiency
}

// Check if auto-advance should be disabled based on conditions
void checkAutoAdvanceConditions() {
  bool previousState = AUTO_ADVANCE;
  String disableReason = "";
  
  // Get battery level
  uint32_t vol = M5.getBatteryVoltage();
  int batteryLevel = map(vol, 3300, 4350, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);
  
  // Check battery level - disable if below 20%
  if (batteryLevel < 20) {
    if (AUTO_ADVANCE) {
      // Only activate sleep timer if AUTO_ADVANCE is being turned off now
      autoAdvanceOffTime = millis();
      sleepTimerActive = true;
      Serial.println("Sleep timer activated - device will sleep in 1 minute");
    }
    AUTO_ADVANCE = false;
    disableReason = "Low battery";
    Serial.println("Auto-advance disabled due to low battery");
  }
  
  // Check number of images - disable if only one image
  else if (fileCount <= 1) {
    if (AUTO_ADVANCE) {
      // Only activate sleep timer if AUTO_ADVANCE is being turned off now
      autoAdvanceOffTime = millis();
      sleepTimerActive = true;
      Serial.println("Sleep timer activated - device will sleep in 1 minute");
    }
    AUTO_ADVANCE = false;
    disableReason = "Only one image";
    Serial.println("Auto-advance disabled due to only one image");
  }
  
  // If conditions are met and auto-advance was previously disabled by conditions, re-enable it
  else if (!previousState) {
    AUTO_ADVANCE = true;
    Serial.println("Auto-advance re-enabled as conditions are now met");
  }
  
  // If state changed, update the status bar
  if (previousState != AUTO_ADVANCE && fileCount > 0) {
    // Show status message
    statusBar.fillCanvas(0xFFFF);
    if (!AUTO_ADVANCE && disableReason.length() > 0) {
      statusBar.drawString("Auto-advance OFF: " + disableReason, 10, 10);
    } else {
      statusBar.drawString("Auto-advance: " + String(AUTO_ADVANCE ? "ON" : "OFF"), 10, 10);
    }
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
    
    // Restore status bar after a brief delay
    delay(1000);
    updateStatusBar(jpgFiles[currentFileIndex]);
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
  }
}

// Toggle auto-advance feature
void toggleAutoAdvance() {
  // Only toggle if conditions allow
  uint32_t vol = M5.getBatteryVoltage();
  int batteryLevel = map(vol, 3300, 4350, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);
  
  if (batteryLevel < 20) {
    // Show message that auto-advance can't be enabled due to low battery
    statusBar.fillCanvas(0xFFFF);
    statusBar.drawString("Can't enable: Low battery", 10, 10);
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
  }
  else if (fileCount <= 1) {
    // Show message that auto-advance can't be enabled due to only one image
    statusBar.fillCanvas(0xFFFF);
    statusBar.drawString("Can't enable: Only one image", 10, 10);
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
  }
  else {
    // Toggle auto-advance
    AUTO_ADVANCE = !AUTO_ADVANCE;
    Serial.printf("Auto-advance %s\n", AUTO_ADVANCE ? "enabled" : "disabled");
    
    // If auto-advance is turned off, start the sleep timer
    if (!AUTO_ADVANCE) {
      autoAdvanceOffTime = millis();
      sleepTimerActive = true;
      Serial.println("Sleep timer activated - device will sleep in 1 minute");
    } else {
      sleepTimerActive = false;
    }
    
    // Show status message
    statusBar.fillCanvas(0xFFFF);
    statusBar.drawString("Auto-advance: " + String(AUTO_ADVANCE ? "ON" : "OFF"), 10, 10);
    statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
  }
  
  // Reset timer
  lastImageChange = millis();
  
  // Restore status bar after a brief delay
  delay(1000);
  updateStatusBar(jpgFiles[currentFileIndex]);
  statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
}

// Show startup screen with logo and info
void showStartupScreen() {
  canvas.fillCanvas(0xFFFF);
  canvas.setTextSize(4);
  canvas.drawString("M5Paper", 180, 200);
  canvas.drawString("JPG Viewer", 150, 250);
  canvas.setTextSize(2);
  canvas.drawString("Version 1.0", 210, 320);
  canvas.drawString("Initializing...", 200, 380);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
  delay(1000);
}

// Show error message
void showErrorMessage(String message) {
  canvas.fillCanvas(0xFFFF);
  canvas.drawString(message, 20, 20);
  canvas.drawString("Please check your SD card and try again.", 20, 60);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
}

// Scan SD card for JPG files
void scanJpgFiles() {
  fileCount = 0;
  
  // Clear the canvas and show scanning message
  canvas.fillCanvas(0xFFFF);
  canvas.drawString("Scanning SD card for JPG files...", 20, 20);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
  
  // Start scanning from root directory
  scanDirectory(DIRECTORY_PATH);
  
  // Display scan results
  Serial.printf("Found %d JPG files\n", fileCount);
  canvas.fillCanvas(0xFFFF);
  canvas.drawString("Found " + String(fileCount) + " JPG files", 20, 20);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
  delay(1000);  // Show message briefly
}

// Recursively scan directory for JPG files
void scanDirectory(String dirPath) {
  File dir = SD.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("Failed to open directory: %s\n", dirPath.c_str());
    return;
  }
  
  Serial.printf("Scanning directory: %s\n", dirPath.c_str());
  
  while (fileCount < MAX_FILES) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files in this directory
      break;
    }
    
    String entryName = String(entry.name());
    String entryPath = dirPath;
    if (dirPath != "/" && !dirPath.endsWith("/")) {
      entryPath += "/";
    }
    entryPath += entryName;
    
    if (entry.isDirectory()) {
      // If it's a directory and we're scanning subdirectories, recurse into it
      if (SCAN_SUBDIRECTORIES) {
        entry.close();
        scanDirectory(entryPath);
      }
    } else {
      // Check if it's a JPG file (case insensitive)
      String lowerName = entryName;
      lowerName.toLowerCase();
      if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) {
        jpgFiles[fileCount] = entryPath;
        Serial.printf("Found JPG: %s\n", entryPath.c_str());
        fileCount++;
      }
    }
    
    entry.close();
  }
  
  dir.close();
}

// Display JPG image on screen
void displayImage(String filePath) {
  Serial.printf("Displaying image: %s\n", filePath.c_str());
  
  // Clear the canvases
  canvas.fillCanvas(0xFFFF);
  statusBar.fillCanvas(0xFFFF);
  

  // Try to draw the JPG file
  // Note: The M5EPD library doesn't support direct rotation of JPG files
  // We'll need to use the M5EPD.SetRotation() method instead
  bool success = canvas.drawJpgFile(SD, filePath.c_str());
  
  if (!success) {
    Serial.println("Failed to draw JPG file");
    canvas.fillCanvas(0xFFFF);
    canvas.drawString("Error loading image:", 20, 20);
    canvas.drawString(filePath, 20, 60);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
    return;
  }
  
  // Update the status bar with file info, battery level, and auto-advance status
  updateStatusBar(filePath);
  
  // Push image to display with high quality refresh
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  
  // Push status bar with faster refresh
  statusBar.pushCanvas(0, 960 - STATUS_BAR_HEIGHT, UPDATE_MODE_DU);
}

// Update status bar with file info and battery level
void updateStatusBar(String filePath) {
  statusBar.fillCanvas(0xFFFF);
  
  // Draw a line at the top of the status bar
  statusBar.drawFastHLine(0, 0, 540, 0);
  
  // Extract filename from path
  String fileName = filePath;
  int lastSlash = filePath.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < filePath.length() - 1) {
    fileName = filePath.substring(lastSlash + 1);
  }
  
  // Truncate filename if too long
  if (fileName.length() > 30) {
    fileName = fileName.substring(0, 27) + "...";
  }
  
  // Display file info
  statusBar.drawString(fileName, 10, 10);
  statusBar.drawString(String(currentFileIndex + 1) + "/" + String(fileCount), 450, 10);
  
  // Get battery info once
  uint32_t vol = M5.getBatteryVoltage();
  int batteryLevel = map(vol, 3300, 4350, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);
  
  // Display auto-advance status with reason if disabled
  if (AUTO_ADVANCE) {
    statusBar.drawString("Auto", 200, 10);
  }
  else if (batteryLevel < 20) {
    statusBar.drawString("Auto OFF (Battery) - Sleep in 1m", 100, 10);
  }
  else if (fileCount <= 1) {
    statusBar.drawString("Auto OFF (1 Image) - Sleep in 1m", 100, 10);
  }
  else if (!AUTO_ADVANCE) {
    statusBar.drawString("Auto OFF - Sleep in 1m", 150, 10);
  }
  
  // Draw battery icon
  int batteryX = 350;
  int batteryY = 12;
  int batteryWidth = 40;
  int batteryHeight = 20;
  
  // Battery outline
  statusBar.drawRect(batteryX, batteryY, batteryWidth, batteryHeight, 0);
  statusBar.fillRect(batteryX + batteryWidth, batteryY + 4, 4, batteryHeight - 8, 0);
  
  // Battery level fill
  int fillWidth = map(batteryLevel, 0, 100, 0, batteryWidth - 4);
  statusBar.fillRect(batteryX + 2, batteryY + 2, fillWidth, batteryHeight - 4, 0);
  
  // Battery percentage
  statusBar.drawString(String(batteryLevel) + "%", 280, 10);
}

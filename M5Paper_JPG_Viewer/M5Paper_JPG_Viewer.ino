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

// Global variables
String jpgFiles[MAX_FILES];        // Array to store JPG file paths
int fileCount = 0;                 // Number of JPG files found
int currentFileIndex = 0;          // Index of currently displayed file
unsigned long lastButtonPress = 0; // Time of last button press for debouncing

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
    if (M5.BtnL.wasPressed()) {
      // Previous image
      Serial.println("Left button pressed - showing previous image");
      lastButtonPress = currentTime;
      currentFileIndex = (currentFileIndex > 0) ? currentFileIndex - 1 : fileCount - 1;
      displayImage(jpgFiles[currentFileIndex]);
    }
    
    if (M5.BtnR.wasPressed()) {
      // Next image
      Serial.println("Right button pressed - showing next image");
      lastButtonPress = currentTime;
      currentFileIndex = (currentFileIndex < fileCount - 1) ? currentFileIndex + 1 : 0;
      displayImage(jpgFiles[currentFileIndex]);
    }
    
    if (M5.BtnP.wasPressed()) {
      // Middle button - toggle refresh mode or other function
      Serial.println("Middle button pressed");
      lastButtonPress = currentTime;
      // You can add additional functionality here
    }
  }
  
  delay(50);  // Small delay for power efficiency
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
  
  // Show loading message
  canvas.drawString("Loading: " + filePath, 20, 20);
  canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
  
  
  // Try to draw the JPG file
  // Note: The M5EPD library doesn't support direct rotation of JPG files
  // We'll need to use the M5EPD.SetRotation() method instead
  success = canvas.drawJpgFile(SD, filePath.c_str());
  
  if (!success) {
    Serial.println("Failed to draw JPG file");
    canvas.fillCanvas(0xFFFF);
    canvas.drawString("Error loading image:", 20, 20);
    canvas.drawString(filePath, 20, 60);
    canvas.pushCanvas(0, 0, UPDATE_MODE_DU);
    return;
  }
  
  // Update the status bar with file info and battery level
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
  
  // Get and display battery info
  uint32_t vol = M5.getBatteryVoltage();
  int batteryLevel = map(vol, 3300, 4350, 0, 100);
  batteryLevel = constrain(batteryLevel, 0, 100);
  
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

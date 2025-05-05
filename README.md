# M5Paper JPG Viewer

This Arduino sketch allows you to display JPG images from an SD card on the M5Paper e-ink display.

## Prerequisites

Before using this sketch, you need to set up your Arduino IDE with the necessary libraries and board support:

1. **Install Arduino IDE**: If you haven't already, download and install the [Arduino IDE](https://www.arduino.cc/en/software).

2. **Install ESP32 Board Support**:
   - Open Arduino IDE
   - Go to File > Preferences
   - Add the following URL to the "Additional Boards Manager URLs" field:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install the "ESP32 by Espressif Systems"

3. **Install Required Libraries**:
   - Go to Sketch > Include Library > Manage Libraries
   - Search for and install the following libraries:
     - "M5EPD" by M5Stack (for M5Paper support)

## Hardware Setup

1. Format your SD card to FAT32 format
2. Copy your JPG images to the root directory of the SD card
3. Insert the SD card into the M5Paper's SD card slot
4. Connect your M5Paper to your computer via USB

## Uploading the Sketch

1. Open the `M5Paper_JPG_Viewer.ino` file in Arduino IDE
2. Select the correct board:
   - Tools > Board > ESP32 Arduino > M5Stack-Paper
3. Select the correct port:
   - Tools > Port > (select the COM port where your M5Paper is connected)
4. Click the Upload button (right arrow icon)

## Using the JPG Viewer

Once the sketch is uploaded to your M5Paper:

1. The device will automatically scan the SD card for JPG files
2. It will display the first JPG file found
3. Use the left and right buttons on the M5Paper to navigate between images:
   - Left button: Previous image
   - Right button: Next image

## Customization

You can modify the sketch to change various aspects:

- `DIRECTORY_PATH`: Change this if you want to look for JPG files in a specific directory on the SD card
- `MAX_FILES`: Increase this if you have more than 50 JPG files
- Screen rotation: Modify the `M5.EPD.SetRotation(1)` line if needed (0=normal, 1=90° clockwise, 2=180°, 3=270° clockwise)

## Troubleshooting

- If no images are displayed, check that your JPG files are in the root directory of the SD card
- Make sure your JPG files have the extension `.jpg` or `.jpeg` (case insensitive)
- If the SD card isn't recognized, try reformatting it to FAT32
- Check that the M5Paper is properly powered and connected

## Notes

- The M5Paper has an e-ink display which refreshes slowly. Be patient when navigating between images.
- Large JPG files may take longer to load and display.
- The display shows the current file path and navigation information at the bottom of the screen.
- Images are automatically rotated 90 degrees to the right for better viewing on the M5Paper.

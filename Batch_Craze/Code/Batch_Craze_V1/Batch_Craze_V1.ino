////////////////////////////////////////////////////
//The below code is used for a game I'm calling Batch Craze
//An SD card formatted in FAT32 is required. On the SD card will be 3 files:
//Master_List.txt   (Holds a list of all words and phrases, each on a new line)
//Submitted.txt     (Any user submitted suggestions will be stored in this document for review)
//Used_Words.txt    (Once a word/phrase is displayed, it is moved to this file. 
//                   This ensures no duplicates are played, even between games)
//
//Hosted AP named "BatchCraze" will be created. You can connect to this AP, then open
//the following webpages for additional functionality.
//http://192.168.4.1/                  (Enter suggested phrases/words which will be added to Submitted.txt)
//http://192.168.4.1/review/      (You can go to this link and review suggestions. Approved suggestions 
//                                 will be added to Master_List.txt, denied will be deleted.)
//
//This is designed to be used with an ESP32-2432S028R aka Cheap Yellow Display
//Start with the following documentation, which will help get you familiar with
//the Cheap Yellow Display. Prerequisites I followed can also be found on this page
// in the "Seup and Configuration options" page.
//https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/SETUP.md
///////////////////////////////////////////////////

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

//Libraries that will be used. Some included in Arduino IDE, some need downloaded and installed
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
// https://github.com/Bodmer/TFT_eSPI
#include <XPT2046_Bitbang.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046_Bitbang_Slim")
// https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library

// Define SD card pin
#define SD_CS 5

// The CYD touch uses some non default
// SPI pins
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Define TFT display and touch screen
TFT_eSPI tft = TFT_eSPI();  // Invoke library
XPT2046_Bitbang ts = XPT2046_Bitbang(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS); // Touchscreen object

// Web server instance
WebServer server(80);

// Variables
String currentWord;

//Original plan was to have a button, but it was removed and you
//can press anywhere on the screen to get the next word
//const int buttonX = 80; // X position of the button
//const int buttonY = 140; // Y position of the button
//const int buttonW = 180; // Button width
//const int buttonH = 60;  // Button height

// Functions
void handleRoot();
void handleSubmit();
void handleReview();
void handleReviewAction();
void displayRandomWord();
void handleTouch();

//Initial Setup items
void setup() {
  // Start serial communication
  Serial.begin(115200);
  //Pin 4, 16, 17 are for the RGB LED on the back
  pinMode(4, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  //Since the LED won't be seen, turn it off (in this case HIGH)
  digitalWrite(4, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(17, HIGH);

  // Initialize TFT display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setSwapBytes(true);

  // Initialize touch screen
  ts.begin();

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    tft.println("SD card initialization failed!");
    while (true); // Halt if SD card fails
  }

  // Ensure necessary files exist
  if (!SD.exists("/Master_List.txt")) {
    File file = SD.open("/Master_List.txt", FILE_WRITE);
    file.close();
  }
  if (!SD.exists("/Used_Words.txt")) {
    File file = SD.open("/Used_Words.txt", FILE_WRITE);
    file.close();
  }
  if (!SD.exists("/Submitted.txt")) {
    File file = SD.open("/Submitted.txt", FILE_WRITE);
    file.close();
  }

  // Set up the Wi-Fi access point
  WiFi.softAP("BatchCraze");
  Serial.println("Access Point started. IP address: ");
  Serial.println(WiFi.softAPIP());

  // Set up the web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/review", HTTP_GET, handleReview);
  server.on("/review_action", HTTP_POST, handleReviewAction);
  server.begin();
  Serial.println("Web server started.");

  // Initialize random seed
  randomSeed(analogRead(0));

  // Display initial welcome message
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_RED);
  tft.setCursor(60, 10);
  tft.setTextSize(6);
  tft.println("Batch");
  tft.setCursor(60, 85);
  tft.println("Craze");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(0, 160);
  tft.setTextSize(3);
  tft.println("Press the screen");
  tft.println("to start the");
  tft.println("game!");
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Handle touch input
  handleTouch();
}

// Handle the root URL "/"
void handleRoot() {
  String html = "<html><head><title>CatchPhrase Submission</title></head><body>";
  html += "<h1>Submit a Catch Phrase</h1>";
  html += "<form action=\"/submit\" method=\"POST\">";
  html += "<input type=\"text\" name=\"phrase\" required />";
  html += "<input type=\"submit\" value=\"Submit\" />";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle form submissions at "/submit"
void handleSubmit() {
  if (server.method() == HTTP_POST) {
    String phrase = server.arg("phrase");
    phrase.trim(); // Remove leading and trailing whitespace

    if (phrase.length() > 0) {
      // Open Submitted.txt to append the new phrase
      File file = SD.open("/Submitted.txt", FILE_APPEND);
      if (file) {
        file.println(phrase);
        file.close();
        server.send(200, "text/html", "<html><body><h1>Thank you!</h1><p>Your suggestion has been recorded.</p><a href=\"/\">Back</a></body></html>");
      } else {
        server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open file on SD card.</p><a href=\"/\">Back</a></body></html>");
      }
    } else {
      server.send(400, "text/html", "<html><body><h1>Invalid Input</h1><p>Please enter a valid phrase.</p><a href=\"/\">Back</a></body></html>");
    }
  } else {
    server.send(405, "text/html", "<html><body><h1>Method Not Allowed</h1><p>Only POST requests are allowed.</p><a href=\"/\">Back</a></body></html>");
  }
}

// Handle review page at "/review"
void handleReview() {
  File file = SD.open("/Submitted.txt", FILE_READ);
  if (!file) {
    server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open Submitted.txt.</p></body></html>");
    return;
  }

  String html = "<html><head><title>Review Submissions</title></head><body>";
  html += "<h1>Review Submitted Phrases</h1>";

  if (file.available()) {
    html += "<form action=\"/review_action\" method=\"POST\">";
    html += "<table border='1' cellpadding='5' cellspacing='0'>";
    html += "<tr><th>Phrase</th><th>Action</th></tr>";

    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        html += "<tr>";
        html += "<td>" + line + "</td>";
        html += "<td>";
        html += "<button type=\"submit\" name=\"action\" value=\"approve:" + line + "\">Approve</button> ";
        html += "<button type=\"submit\" name=\"action\" value=\"deny:" + line + "\">Deny</button>";
        html += "</td>";
        html += "</tr>";
      }
    }

    html += "</table>";
    html += "<button type=\"submit\" name=\"action\" value=\"approve_all\">Approve All</button>"; // Approve All button
    html += "</form>";
  } else {
    html += "<p>No submissions to review.</p>";
  }

  html += "</body></html>";
  file.close();

  server.send(200, "text/html", html);
}

// Handle review actions at "/review_action"
void handleReviewAction() {
  if (server.method() == HTTP_POST) {
    String actionParam = server.arg("action");

    if (actionParam.equals("approve_all")) {
      // Approve all phrases
      File submittedFile = SD.open("/Submitted.txt", FILE_READ);
      if (!submittedFile) {
        server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open Submitted.txt.</p><a href=\"/review\">Back</a></body></html>");
        return;
      }

      File masterFile = SD.open("/Master_List.txt", FILE_APPEND);
      if (!masterFile) {
        server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open Master_List.txt.</p><a href=\"/review\">Back</a></body></html>");
        return;
      }

      while (submittedFile.available()) {
        String phrase = submittedFile.readStringUntil('\n');
        phrase.trim();
        if (phrase.length() > 0) {
          masterFile.println(phrase);
        }
      }
      submittedFile.close();
      masterFile.close();

      // Clear the Submitted.txt file
      SD.remove("/Submitted.txt");
      File clearFile = SD.open("/Submitted.txt", FILE_WRITE);
      clearFile.close();

      server.send(200, "text/html", "<html><body><h1>All Approved</h1><p>All phrases have been approved and added to Master_List.txt.</p><a href=\"/review\">Back</a></body></html>");
    } else {
      // Handle individual approve or deny actions (existing functionality)
      int separatorIndex = actionParam.indexOf(':');
      if (separatorIndex == -1) {
        server.send(400, "text/html", "<html><body><h1>Error</h1><p>Invalid action format.</p><a href=\"/review\">Back</a></body></html>");
        return;
      }

      String action = actionParam.substring(0, separatorIndex);
      String phrase = actionParam.substring(separatorIndex + 1);
      phrase.trim();

      if (phrase.length() == 0) {
        server.send(400, "text/html", "<html><body><h1>Error</h1><p>Invalid phrase.</p><a href=\"/review\">Back</a></body></html>");
        return;
      }

      // Open the Submitted.txt file
      File file = SD.open("/Submitted.txt", FILE_READ);
      if (!file) {
        server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open Submitted.txt.</p><a href=\"/review\">Back</a></body></html>");
        return;
      }

      // Create a temporary file for rewriting
      File tempFile = SD.open("/temp.txt", FILE_WRITE);
      if (!tempFile) {
        server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open temporary file.</p><a href=\"/review\">Back</a></body></html>");
        file.close();
        return;
      }

      bool actionSuccess = false;

      // Iterate over each line in Submitted.txt
      while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.equals(phrase)) {
          if (action.equals("approve")) {
            // Append the approved phrase to Master_List.txt
            File masterFile = SD.open("/Master_List.txt", FILE_APPEND);
            if (!masterFile) {
              server.send(500, "text/html", "<html><body><h1>Error</h1><p>Unable to open Master_List.txt.</p><a href=\"/review\">Back</a></body></html>");
              tempFile.close();
              file.close();
              return;
            }
            masterFile.println(line);
            masterFile.close();
          }
          actionSuccess = true; // Mark that we handled this line
        } else {
          tempFile.println(line); // Write all other lines to temp.txt
        }
      }

      file.close();
      tempFile.close();

      // Replace the old Submitted.txt with the new one
      SD.remove("/Submitted.txt");
      SD.rename("/temp.txt", "/Submitted.txt");

      if (actionSuccess) {
        server.send(200, "text/html", "<html><body><h1>Action Completed</h1><p>The phrase has been " + action + "d.</p><a href=\"/review\">Back</a></body></html>");
      } else {
        server.send(404, "text/html", "<html><body><h1>Not Found</h1><p>The phrase was not found in Submitted.txt.</p><a href=\"/review\">Back</a></body></html>");
      }
    }
  } else {
    server.send(405, "text/html", "<html><body><h1>Method Not Allowed</h1><p>Only POST requests are allowed.</p><a href=\"/review\">Back</a></body></html>");
  }
}

// Display a random word from Master_List.txt
void displayRandomWord() {
  // Open Master_List.txt for reading
  File masterFile = SD.open("/Master_List.txt", FILE_READ);
  if (!masterFile) {
    Serial.println("Failed to open Master_List.txt");
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.println("Failed to open Master_List.txt");
    return;
  }

  // Read all non-empty lines into a vector
  std::vector<String> lines;
  while (masterFile.available()) {
    String line = masterFile.readStringUntil('\n');
    line.trim();  // Remove any leading/trailing whitespace
    if (line.length() > 0) {
      lines.push_back(line);
    }
  }
  masterFile.close();

  //If Master_List.txt has no more words/phrases, it is replenished by taking all word/phrases out of Used_Words.txt
  if (lines.size() == 0) {
    // If Master_List.txt is empty, restore from Used_Words.txt
    File usedFile = SD.open("/Used_Words.txt", FILE_READ);
    if (usedFile) {
      File masterFile = SD.open("/Master_List.txt", FILE_WRITE);
      if (masterFile) {
        while (usedFile.available()) {
          String line = usedFile.readStringUntil('\n');
          line.trim();
          if (line.length() > 0) {
            masterFile.println(line);
          }
        }
        masterFile.close();
        usedFile.close();

        // Clear Used_Words.txt
        SD.remove("/Used_Words.txt");
        File clearUsed = SD.open("/Used_Words.txt", FILE_WRITE);
        clearUsed.close();

        Serial.println("Master_List.txt replenished from Used_Words.txt.");
        tft.fillScreen(TFT_WHITE);
        tft.setTextColor(TFT_BLACK);
        tft.setCursor(10, 10);
        tft.setTextSize(2);
        tft.println("Master list replenished.");
        return;
      } else {
        Serial.println("Failed to open Master_List.txt for writing.");
        tft.fillScreen(TFT_WHITE);
        tft.setTextColor(TFT_RED);
        tft.setCursor(10, 10);
        tft.setTextSize(2);
        tft.println("Failed to write to Master_List.txt");
        usedFile.close();
        return;
      }
    } else {
      Serial.println("No words available to display.");
      tft.fillScreen(TFT_WHITE);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor(10, 10);
      tft.setTextSize(2);
      tft.println("No words available.");
      return;
    }
  }

  // Select a random word
  int randomIndex = random(lines.size());
  currentWord = lines[randomIndex];

  // Remove selected word from lines
  lines.erase(lines.begin() + randomIndex);

  // Write updated Master_List.txt
  masterFile = SD.open("/Master_List.txt", FILE_WRITE);
  if (masterFile) {
    for (const auto& line : lines) {
      masterFile.println(line);
    }
    masterFile.close();
  } else {
    Serial.println("Failed to open Master_List.txt for writing.");
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.println("Failed to update Master_List.txt");
    return;
  }

  // Add word to Used_Words.txt
  File usedFile = SD.open("/Used_Words.txt", FILE_APPEND);
  if (usedFile) {
    usedFile.println(currentWord);
    usedFile.close();
  } else {
    Serial.println("Failed to open Used_Words.txt for appending.");
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.println("Failed to update Used_Words.txt");
    return;
  }

  // Display the word
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(4);

  int screenWidth = tft.width() - 20;  // Adjust for padding
  int lineHeight = tft.fontHeight();
  int cursorX = 10;
  int cursorY = 60;

  String wordToDisplay = "";
  for (int i = 0; i < currentWord.length(); i++) {
    char c = currentWord[i];
    if (c == ' ') {
      int wordWidth = tft.textWidth(wordToDisplay);
      if (cursorX + wordWidth > screenWidth) {
        // Move to next line if current line is full
        cursorX = 10;
        cursorY += lineHeight;
      }
      tft.setCursor(cursorX, cursorY);
      tft.print(wordToDisplay + " ");
      cursorX += wordWidth + tft.textWidth(" ");
      wordToDisplay = "";
    } else {
      wordToDisplay += c;
    }
  }

  // Print the remaining part of the word
  if (wordToDisplay.length() > 0) {
    int wordWidth = tft.textWidth(wordToDisplay);
    if (cursorX + wordWidth > screenWidth) {
      cursorX = 10;
      cursorY += lineHeight;
    }
    tft.setCursor(cursorX, cursorY);
    tft.print(wordToDisplay);
  }
  
  //Here is the code that was going to be used for a button area that had to be touched to progress
  //to the next word. Figured it was unneeded and touching anywhere was fine
  //tft.fillRect(buttonX, buttonY, buttonW, buttonH, TFT_BLUE);
  //tft.setTextColor(TFT_WHITE);
  //tft.setCursor(buttonX + 30, buttonY + 20);
  //tft.setTextSize(2);
  //tft.println("New Word");
}

// Handle touch input to trigger new word display
void handleTouch() {

  //Checks if screen has been touched
  TouchPoint touch = ts.getTouch();

  // Display touches that have a pressure value greater than 500 (Z)
  if (touch.zRaw > 500) {
    //Writes the registered touch to variable t
    TouchPoint t = touch;

    //No longer used due to removing button
    // Adjust the touch coordinates based on your screen orientation
    //t.x = map(t.x, 0, 240, 0, tft.width());
    //t.y = map(t.y, 0, 320, 0, tft.height());

    // Debounce touch to avoid multiple triggers
    static unsigned long lastTouchTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > 1000) {  // 1s debounce time
      lastTouchTime = currentTime;

      //more unused button code
      //if (t.x > buttonX && t.x < buttonX + buttonW &&
          //t.y > buttonY && t.y < buttonY + buttonH) {
        //displayRandomWord();
      //}

      //Displays the next random word
      displayRandomWord();
    }
  }
}
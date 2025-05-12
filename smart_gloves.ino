#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <MPU6050_tockn.h>
#include <Wire.h>

// MPU6050 initialization
MPU6050 mpu6050(Wire);

// WiFi credentials+
const char* ssid = "SmartGloves";
const char* password = "123456";

// Gesture and flex sensor messages
const char* messages[] = {
  "Move left", "Stop", "I don't like", "emergency needs", "Move right", 
  "I am scared", "water","Hungry", "NO", "Help",
  "Fine/ok", "Need medicine", "Emergency call"
};

volatile int currentMessageIndex = -1; // Index to track the current message
AsyncWebServer server(80);

// Flex sensor variables
int SF1, SF2, SF3, SF4, SF5; // Calibration values
int flex1, flex2, flex3, flex4, flex5; // Current readings
int thm, ind, mid, rng, sm; // Differences from calibration

long timer = 0;

void setup() {
  // Serial monitor setup
  Serial.begin(115200);

  // MPU6050 initialization
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  // Initialize calibration values for flex sensors
  SF1 = analogRead(39);
  SF2 = analogRead(34);
  SF3 = analogRead(35);
  SF4 = analogRead(32);
  SF5 = analogRead(33);
  delay(100);

  // Connect to Wi-Fi
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Configure the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    const char webpage[] = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>ESP32 Gesture Assistant</title>
      <style>
        h2 { text-align: center; font-size: 2em; font-weight: bold; margin-top: 20px; }
        #message { text-align: center; font-size: 1.5em; font-weight: bold; color: #333; margin-top: 30px; }
      </style>
    </head>
    <body>
      <h2>ESP32 Gesture Assistant</h2>
      <div id="message">Waiting for gesture...</div>
      <script>
        let lastMessage = "Waiting for gesture...";
        function fetchMessage() {
          fetch("/message")
            .then(response => response.text())
            .then(text => {
              const messageDiv = document.getElementById("message");
              if (text && text !== lastMessage) {
                messageDiv.innerText = text;
                lastMessage = text;
                if (text !== "Waiting for gesture...") {
                  let utterance = new SpeechSynthesisUtterance(text);
                  speechSynthesis.speak(utterance);
                }
              }
            });
        }
        setInterval(fetchMessage, 1000);
      </script>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", webpage);
  });

  server.on("/message", HTTP_GET, [](AsyncWebServerRequest *request) {
    static String lastSentMessage = "";
    if (currentMessageIndex >= 0 && currentMessageIndex < 16) {
      String newMessage = messages[currentMessageIndex];
      if (newMessage != lastSentMessage) {
        lastSentMessage = newMessage;
        request->send(200, "text/plain", newMessage);
      } else {
        request->send(200, "text/plain", "Waiting for gesture...");
      }
    } else {
      request->send(200, "text/plain", "Waiting for gesture...");
    }
  });

  server.begin();
  Serial.println("Server started");
}


void loop() 
{
  // Update MPU6050 readings
  mpu6050.update();
  if (touchRead(12) < 30) 
  {
    mpu6050.calcGyroOffsets(true);
    Serial.println("Calibrating...");
      SF1 = analogRead(39);
      SF2 = analogRead(34);
      SF3 = analogRead(35);
      SF4 = analogRead(32);
      SF5 = analogRead(33);
    delay(100);
  }
  

  if (millis() - timer > 100) 
  {
    float angleX = mpu6050.getAngleX();
    float angleY = mpu6050.getAngleY();
    float angleZ = mpu6050.getAngleZ();

    Serial.print("angleX : "); Serial.print(angleX);
    Serial.print(" | angleY : "); Serial.print(angleY);
    Serial.print(" | angleZ : "); Serial.print(angleZ);

    int newMessageIndex = -1;

     // Read flex sensor values
  flex1 = analogRead(39);
  flex2 = analogRead(34);
  flex3 = analogRead(35);
  flex4 = analogRead(32);
  flex5 = analogRead(33);
  delay(100);

  thm = abs(flex1 - SF1);
  ind = abs(flex2 - SF2);
  mid = abs(flex3 - SF3);
  rng = abs(flex4 - SF4);
  sm = abs(flex5 - SF5);
  delay(100);

  print_flex(); //disable this for fast working of the code

  // Flex sensor gesture conditions
  if (thm > 15 && mid > 15 && rng > 15 && sm > 15) {
    newMessageIndex = 6; // "I am scared"
  } else if (mid > 15 && rng > 15 && sm > 15) {
    newMessageIndex = 7; // "water"
  } else if (thm>15 && ind > 15 && mid > 15 && rng > 15 && sm > 15) {
    newMessageIndex = 8; // "hungry"
  } else if (thm > 15 && rng > 15 && sm > 15) {
    newMessageIndex = 9; // "NO"
  }else if (thm > 15 && mid > 15 && rng > 15) {
    newMessageIndex = 10; // "help"
  } else if (thm >30 && ind > 30) {
    newMessageIndex = 11; // "Fine/ok"
  } else if (thm > 15 && sm > 15) {
    newMessageIndex = 12; // Need medicine 
  } else if (ind > 15 && mid > 15 && rng > 30) {
    newMessageIndex = 13; // Emergency Call
  } //else if (sm > 50) {
  // newMessageIndex = 14;
  // }

    else if (angleX < -90 && angleY > 40 ) {
      newMessageIndex = 4; // Move left
    } else if (angleX < -90 && angleZ > 150) {
      newMessageIndex = 5; // Hello. how are you?
    } 
    // else if (angleY > 150 ) {
    //   newMessageIndex = 1; // Stop
    // }
     else if (angleZ >300) {
      newMessageIndex = 1; // I don't like
    } else if (angleY < -200) {
      newMessageIndex = 3; // emergency needs
    } else if (angleX > 90 && angleY > 40) {
      newMessageIndex = 0; // Move right
    }

    if (newMessageIndex != currentMessageIndex) 
    {
      currentMessageIndex = newMessageIndex;
      if (currentMessageIndex >= 0) 
      {
        Serial.println(messages[currentMessageIndex]);
        delay(1000);
      }
    }

    timer = millis();
  }
  delay(100);
}

void print_flex()
{
    // Serial.println(String("Flex1: ") + flex1 + String(" | Flex2: ") + flex2 + 
    //              String(" | Flex3: ") + flex3 + String(" | Flex4: ") + flex4 + 
    //              String(" | Flex5: ") + flex5);

  // Print differences from calibration 
  Serial.print("   ");
  Serial.println(String("Thumb: ") + thm + String(" | Index: ") + ind + 
                 String(" | Mid: ") + mid + String(" | Ring: ") + rng + 
                 String(" | Sml: ") + sm);
}


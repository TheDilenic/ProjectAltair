#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* ssid = "RedmiWinG";
const char* password = "jwpa4099";

// Web server and WebSocket server
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Motor Pins for MD2 #1 (Front Left and Right)
const int FL_PWM = D1;
const int FL_DIR = D2;
const int FR_PWM = D5;
const int FR_DIR = D6;

// Motor Pins for MD2 #2 (Back Left and Right)
const int BL_PWM = D7;
const int BL_DIR = D3;
const int BR_PWM = D4;
const int BR_DIR = D8;

const int vacuumPin = D0;

int vacuumOn = 0;
bool autoMode = false;
float joyX = 0;
float joyY = 0;

void setupPins() {
  pinMode(FL_PWM, OUTPUT);
  pinMode(FL_DIR, OUTPUT);
  pinMode(FR_PWM, OUTPUT);
  pinMode(FR_DIR, OUTPUT);
  pinMode(BL_PWM, OUTPUT);
  pinMode(BL_DIR, OUTPUT);
  pinMode(BR_PWM, OUTPUT);
  pinMode(BR_DIR, OUTPUT);
  pinMode(vacuumPin, OUTPUT);
  digitalWrite(vacuumPin, LOW);
}

void setMotor(int pwmPin, int dirPin, int speed, bool forward) {
  speed = constrain(speed, 0, 1023);  // Ensure within PWM range
  analogWrite(pwmPin, speed);
  digitalWrite(dirPin, forward ? HIGH : LOW);
}

void applyMotorLogic() {
  int baseSpeed = map(joyY, -100, 100, -1023, 1023);
  int turnAdjust = map(joyX, -100, 100, -512, 512);

  int leftSpeed = constrain(baseSpeed + turnAdjust, -1023, 1023);
  int rightSpeed = constrain(baseSpeed - turnAdjust, -1023, 1023);

  setMotor(FL_PWM, FL_DIR, abs(leftSpeed), leftSpeed > 0);
  setMotor(BL_PWM, BL_DIR, abs(leftSpeed), leftSpeed > 0);
  setMotor(FR_PWM, FR_DIR, abs(rightSpeed), rightSpeed > 0);
  setMotor(BR_PWM, BR_DIR, abs(rightSpeed), rightSpeed > 0);
}

void runAutoMovement() {
  joyY = 100;
  joyX = 0;
  applyMotorLogic();
  delay(2000); // Can be replaced with non-blocking logic if needed
  joyY = 0;
  applyMotorLogic();
}

void onWebSocketMessage(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_TEXT: {
      String msg = String((const char*)payload); // Safe conversion
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, msg);

      if (!error) {
        if (doc.containsKey("x")) joyX = doc["x"];
        if (doc.containsKey("y")) joyY = doc["y"];

        if (doc.containsKey("start")) {
          vacuumOn = doc["start"];
          digitalWrite(vacuumPin, vacuumOn ? HIGH : LOW);
        }

        if (doc.containsKey("\"command\":\"auto\"")) autoMode = doc["\"command\":\"auto\""];

        Serial.printf("Joystick: X = %.2f, Y = %.2f\n", joyX, joyY);
        Serial.printf("Vacuum: %d | Auto: %d\n", vacuumOn, autoMode);
      } else {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
      }
      break;
    }

    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("Client %u connected from %s\n", num, ip.toString().c_str());
      break;
    }

    case WStype_DISCONNECTED: {
      Serial.printf("Client %u disconnected\n", num);
      break;
    }

    default:
      break;
  }
}

void handleClient() {
  webSocket.loop();

  if (autoMode) {
    runAutoMovement();
  } else {
    applyMotorLogic();
  }
}

void setup() {
  Serial.begin(115200);
  setupPins();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  webSocket.begin();
  webSocket.onEvent(onWebSocketMessage);
  server.begin();
}

void loop() {
  handleClient();
  yield(); // Ensure background tasks run
}

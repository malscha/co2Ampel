#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Mhz19.h>
#include <SoftwareSerial.h>

// Pin Definitions
#define CO2_IN 27
#define MH_Z19_RX 16
#define MH_Z19_TX 17
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define DHT_SENSOR_PIN 26
#define DHT_SENSOR_TYPE DHT22
#define LED_GREEN 12
#define LED_YELLOW 13
#define LED_RED 14
#define ONBOARD_LED 2
#define BUZZER 25

// WiFi Credentials
const char *ssid = "P10";
const char *password = "nichtsicher";

// Server
AsyncWebServer server(80);

// Sensors
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
SoftwareSerial mySerial(MH_Z19_RX, MH_Z19_TX);
Mhz19 myMHZ19;

// Display
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup()
{
  // Setup Pins
  pinMode(ONBOARD_LED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(CO2_IN, INPUT);

  // Setup Serial
  Serial.begin(9600);

  // Setup Sensors
  dht_sensor.begin();

  mySerial.begin(9600);
  myMHZ19.begin(&mySerial);

  myMHZ19.setMeasuringRange(Mhz19MeasuringRange::Ppm_5000);
  myMHZ19.enableAutoBaseCalibration();

  Serial.println("Preheating..."); // Preheating, ~3 minutes
  while (!myMHZ19.isReady())
  {
    delay(50);
  }

  // Setup Display
  setupDisplay();

  // Setup WiFi
  setupWiFi();

  // Setup Server
  setupServer();

  // Initial delay
  delay(2000);

  // Initial Display
  initialDisplay();

  // Initial Audio
  tone(BUZZER, 1500, 100);
}

void loop()
{
  // Read Sensors
  float temp = dht_sensor.readTemperature();
  float hum = dht_sensor.readHumidity();
  auto carbonDioxide = myMHZ19.getCarbonDioxide();
  Serial.println(String(carbonDioxide) + " ppm");
  Serial.println(String(temp) + "C");
  Serial.println(String(hum) + "%");

  // Update Display
  updateDisplay(temp, hum, carbonDioxide);

  // Update LEDs
  updateLEDs(temp, hum);

  delay(10000);
}

void setupDisplay()
{
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }
}

void setupWiFi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());
}

void setupServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("ESP32 Web Server: New request received:");  // for debugging
    Serial.println("GET /");                                    // for debugging

    // get temperature from sensor
    float temperature = dht_sensor.readTemperature();
    String temperatureStr = String(temperature, 1);

    float humidity = dht_sensor.readHumidity();
    String humidityStr = String(humidity, 1);

    String html = "<!DOCTYPE HTML>";
    html += "<html>";
    html += "<head>";
    html += "<link rel=\"icon\" href=\"data:,\">";
    html += "</head>";
    html += "<p>";
    html += "Temperature: <span style=\"color: red;\">";
    html += temperatureStr;
    html += "&deg;C</span>";
    html += "<br>";
    html += "Humidity: <span style=\"color: red;\">";
    html += humidityStr;
    html += "&#37;</span>";
    html += "</p>";
    html += "</html>";

    request->send(200, "text/html", html); });

  // Start the server
  server.begin();
  server.begin();
}

void initialDisplay()
{
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 10);
  oled.println(".......");
  oled.display();
}

void updateDisplay(float temp, float hum, float co2)
{
  String temp_str = String(temp);
  String hum_str = String(hum);
  String co2_str = String(co2);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.println("Temp: " + temp_str + "C");
  oled.println("Humi: " + hum_str + "%");
  oled.println("CO2: " + co2_str + " ppm");
  oled.display();
}

void updateLEDs(float temp, float hum)
{
  if (hum < 40 && temp < 40)
  {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
  }
  else if (hum < 50 && temp < 40)
  {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
  }
  else
  {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER, 1500, 200);
  }
}
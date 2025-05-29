#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi Credentials
const char* ssid = "mr.eldhose";
const char* password = "Eldhose@1077";

WiFiServer server(80);
Servo carGate;
Servo bikeGate;

// LCD Setup (I2C Address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// IR sensors for parking availability
const int carSlot1 = 33;
const int carSlot2 = 32;
const int bikeSlot1 = 35;
const int bikeSlot2 = 34;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  server.begin();
  Serial.println("Server started");
  Serial.print("ESP32-WROOM IP Address: ");
  Serial.println(WiFi.localIP());

  // Servo pin configurations
  carGate.attach(14);
  bikeGate.attach(27);

  // IR sensor pin configurations
  pinMode(carSlot1, INPUT);
  pinMode(carSlot2, INPUT);
  pinMode(bikeSlot1, INPUT);
  pinMode(bikeSlot2, INPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println("Received HTTP request: " + request);
    client.flush();

    if (request.indexOf("vehicle=Car") >= 0) {
      handleVehicleEntry("Car");
    } else if (request.indexOf("vehicle=Bike") >= 0) {
      handleVehicleEntry("Bike");
    }

    checkParkingAvailability();
    delay(10);
    client.stop();
  }
}

void handleVehicleEntry(String vehicleType) {
  if (vehicleType == "Car") {
    if (isParkingFull("Car")) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Car No Space");
      Serial.println("Car parking full, gate not opening");
    } else {
      Serial.println("Car detected, opening left gate");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Car Arrived");
      openGate(carGate);
    }
  } else if (vehicleType == "Bike") {
    if (isParkingFull("Bike")) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bike No Space");
      Serial.println("Bike parking full, gate not opening");
    } else {
      Serial.println("Bike detected, opening right gate");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bike Arrived");
      openGate(bikeGate);
    }
  }
  showParkingAvailability(vehicleType);
}

void openGate(Servo &gate) {
  Serial.println("Opening gate...");
  gate.write(0); // Open the gate
  delay(5000);
  Serial.println("Closing gate...");
  gate.write(90); // Close the gate
}

bool isParkingFull(String vehicleType) {
  if (vehicleType == "Car") {
    return (digitalRead(carSlot1) == LOW && digitalRead(carSlot2) == LOW);
  } else if (vehicleType == "Bike") {
    return (digitalRead(bikeSlot1) == LOW && digitalRead(bikeSlot2) == LOW);
  }
  return false;
}

void checkParkingAvailability() {
  Serial.print("Car Slots: ");
  Serial.print(digitalRead(carSlot1) == LOW ? "Full " : "Available ");
  Serial.println(digitalRead(carSlot2) == LOW ? "Full" : "Available");

  Serial.print("Bike Slots: ");
  Serial.print(digitalRead(bikeSlot1) == LOW ? "Full " : "Available ");
  Serial.println(digitalRead(bikeSlot2) == LOW ? "Full" : "Available");
}

void showParkingAvailability(String vehicleType) {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (vehicleType == "Car") {
    lcd.print("Car Slots:");
    lcd.setCursor(0, 1);
    lcd.print(digitalRead(carSlot1) == LOW ? "Full " : "Available ");
    lcd.print(digitalRead(carSlot2) == LOW ? "Full" : "Available");
  } else if (vehicleType == "Bike") {
    lcd.print("Bike Slots:");
    lcd.setCursor(0, 1);
    lcd.print(digitalRead(bikeSlot1) == LOW ? "Full " : "Available ");
    lcd.print(digitalRead(bikeSlot2) == LOW ? "Full" : "Available");
  }
  delay(3000);
}

// }

#include <ZMPT101B.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "EmonLib.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

EnergyMonitor emon1; // Create an instance
FirebaseData firebasedata;
FirebaseAuth auth;
FirebaseConfig config;

#define API_KEY "AIzaSyA8HUDOTB5FRIAP1MF_RCL6HyezDs5idDs"
#define URL "https://house-metering-and-monit-adede-default-rtdb.firebaseio.com/"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define SENSITIVITY 1394.0f
ZMPT101B voltageSensor(34, 50.0);

float voltage;
float Irms;
double total_kWh = 0;
double totalCost = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 500;  // 10 seconds interval

#define USER_EMAIL "lahirueshan999@gmail.com"
#define USER_PASSWORD "Ngleshan459"

// Function declarations
void WIFI_INIT(void);
void firebase_INIT(void);
void OLED_DISP(void);

// Initialize Firebase
void firebase_INIT(void) {
    config.api_key = API_KEY;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    config.database_url = URL;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    Firebase.setFloatDigits(2);  // round off values 
    config.timeout.serverResponse = 20000;
}

// Initialize WiFi
void WIFI_INIT() {
    WiFi.mode(WIFI_STA);
    WiFi.begin("Hirusha12", "12345678");
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("#");
        Serial.print("connecting");
        delay(400);
    }
    Serial.print(" ");
    Serial.println("CONNECTED");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("MAC ADDRESS: " + WiFi.macAddress());
}

// Display on OLED
void OLED_DISP() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    Serial.println("DISPLAY CONNECTING");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    display.print("V :  ");
    display.println(voltage);
     display.setCursor(0,20 );
    display.print("Current :  ");
    display.println(Irms);

         display.setCursor(0,30 );
    display.print("kWh :  ");
    display.println(total_kWh);

             display.setCursor(0,40 );
    display.print("Cost  Rs ");
    display.println(totalCost);


    


    display.display();
}

// Function to calculate price based on total kWh consumption
double calculatePrice(double total_kWh) {
    if (total_kWh <= 30) {
        return total_kWh * 6+100;  // Price for the first 100 kWh
    } else if (total_kWh <= 60) {
        return (30 * 6) + ((total_kWh - 30) * 9) + 250;  // Price for 100-500 kWh
    } else {
        return (30 * 6) + (30 * 9) + ((total_kWh - 60) * 18) + 400;  // Price above 500 kWh
    }
}

void setup() {
    Serial.begin(115200);
    voltageSensor.setSensitivity(SENSITIVITY);
    emon1.current(35,5); // Initialize current sensor
    Wire.begin(27, 14, 100000); // Initialize I2C
    lastUpdate = millis();  // Initialize the timer
        WIFI_INIT();
    firebase_INIT();
    pinMode(32,OUTPUT);
    delay(1000);
}

void loop() {
    unsigned long currentMillis = millis();
    voltage = voltageSensor.getRmsVoltage(1480);
    
    // Prevent small fluctuations in voltage
    if (voltage <= 50) {
        voltage = 0;
    }

    // Measure current
    Irms = emon1.calcIrms(1480);  // Calculate Irms only
    double apparentPower = voltage * Irms;

    // Calculate time in hours
    double timeHours = (currentMillis - lastUpdate) / 3600000.0;  // Time in hours

    // Calculate total energy in kWh
    total_kWh += (apparentPower * timeHours) / 1000.0;

    // Calculate total cost based on tiered pricing
    totalCost = calculatePrice(total_kWh);

    // Check if 10 seconds have passed
    if (currentMillis - lastUpdate >= updateInterval) {
        lastUpdate = currentMillis;  // Reset the timer

        // Print voltage, current, apparent power, kWh, and cost
        Serial.print("Voltage: ");
        Serial.println(voltage);
        
        Serial.print("Current: ");
        Serial.println(Irms, 3);
        
        Serial.print("Apparent Power: ");
        Serial.println(apparentPower);
        
        Serial.print("Energy (kWh): ");
        Serial.println(total_kWh, 4);
        
        Serial.print("Cost ($): ");
        Serial.println(totalCost, 4);

        // Optional: Update the OLED display
        OLED_DISP();

       
        if (Firebase.RTDB.setFloat(&firebasedata, "/hirusha/voltage", voltage)) {
            Serial.println("Voltage Data Sent");
        } else {
            Serial.print("Could not send: ");
            Serial.println(firebasedata.errorReason());
        }
                if (Firebase.RTDB.setFloat(&firebasedata, "/hirusha/Current",Irms )) {
            Serial.println("Current Data Sent");
        } else {
            Serial.print("Could not send: ");
            Serial.println(firebasedata.errorReason());
        }
                        if (Firebase.RTDB.setFloat(&firebasedata, "/hirusha/Energy", total_kWh)) {
            Serial.println("KWh data sent");
        } else {
            Serial.print("Could not send: ");
            Serial.println(firebasedata.errorReason());
        }
                                if (Firebase.RTDB.setFloat(&firebasedata, "/hirusha/Price", totalCost)) {
            Serial.println("Total cost sent");
        } else {
            Serial.print("Could not send: ");
            Serial.println(firebasedata.errorReason());
        }
    }


        if (Firebase.RTDB.getString(&firebasedata, "/hirusha/relayState")) {
    String relayStateStr = firebasedata.stringData();
    bool relayState = (relayStateStr == "1");  // Convert "1" to true and "0" to false

    Serial.print("Relay State: ");
    Serial.println(relayState ? "ON" : "OFF");

    // Control the relay
    digitalWrite(32, relayState ? LOW : HIGH);
} else {
    Serial.print("Could not read relay state: ");
    Serial.println(firebasedata.errorReason());
}

    
    
}

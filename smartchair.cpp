#include <SoftwareSerial.h>
#include "MAX30100_PulseOximeter.h"
#include "MPU6050_tockn.h"
#include "Wire.h"

// Pins for SoftwareSerial (Pulse Oximeter and MPU6050)
SoftwareSerial MySerial(2, 3); // RX | TX ---> TX | RX (HC-05)

// Pulse Oximeter settings
#define REPORTING_PERIOD_MS 1000 // ms
#define MIN_HEART_RATE 40
#define MAX_HEART_RATE 180
#define NUM_READINGS 60 // Number of readings to calculate the average

PulseOximeter pox;
uint32_t tsLastReport = 0;

void onBeatDetected() {
    Serial.println("Beat!");
}

// Variables for average heart rate calculation
float heartRateReadings[NUM_READINGS]; // Array to store heart rate readings
int readIndex = 0; // Index of the current reading
int validReadingsCount = 0; // Number of valid readings
float totalHeartRate = 0; // Sum of heart rate readings
float averageHeartRate = 0; // Average heart rate

MPU6050 mpu6050(Wire);

long timer = 0;
long sleep_timer_start, sleep_timer_end, sleep_timer_end2;
float x, y, z;
int activate = 0, interrupt = 0, stage_sleep_time = 0, interrupt_sleep_timer = 0, interrupt_for_deep_sleep = 0, total_sleep = 0, total_deep_sleep = 0, total_light_sleep = 0, deep_sleep = 0, light_sleep = 0, interrupt_timer = 0;

void setup() {
    Serial.begin(9600);
    Serial.print("Initializing pulse oximeter..");
    MySerial.begin(9600);  
    if (!pox.begin()) {
        Serial.println("FAILED");
        for (;;);
    } else {
        Serial.println("SUCCESS");
    }

    pox.setOnBeatDetectedCallback(onBeatDetected); // Callback for beat detection

    // Initialize the heart rate readings array
    for (int i = 0; i < NUM_READINGS; i++) {
        heartRateReadings[i] = 0;
    }

    // Setup for MPU6050
    Wire.begin();
    mpu6050.begin();
    //mpu6050.calcGyroOffsets(true);
}

void loop() {
    // Pulse oximeter update
    pox.update(); // Update the sensor readings

    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        // Get the current heart rate
        float currentHeartRate = pox.getHeartRate();

        // Check if the current heart rate is within the valid range
        if (currentHeartRate >= MIN_HEART_RATE && currentHeartRate <= MAX_HEART_RATE) {
            // Subtract the last reading from the total only if it was valid
            totalHeartRate -= heartRateReadings[readIndex];

            // Add the current valid heart rate to the readings array
            heartRateReadings[readIndex] = currentHeartRate;

            // Add the current reading to the total
            totalHeartRate += currentHeartRate;

            // Advance to the next position in the array
            readIndex = (readIndex + 1) % NUM_READINGS;

            // If the buffer is not yet full, increment the valid readings count
            if (validReadingsCount < NUM_READINGS) {
                validReadingsCount++;
            }

            // Calculate the average only with valid readings
            averageHeartRate = totalHeartRate / validReadingsCount;
        }/-strong/-heart:>:o:-((:-h // Debugging output
        Serial.print("Heart rate: ");
        Serial.print(currentHeartRate);
        Serial.print(" bpm / SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.println(" %");

        // Print average heart rate
        Serial.print("Average heart rate: ");
        Serial.print(averageHeartRate);
        Serial.println(" bpm");

        // Send data over Bluetooth
        MySerial.print(currentHeartRate);
        //MySerial.print(" bpm ");
        MySerial.print(",");
        MySerial.print(pox.getSpO2());
        //MySerial.println(" %");
        MySerial.print(",");
        // Send average heart rate over Bluetooth
        MySerial.print(averageHeartRate);
        //MySerial.println(" bpm");
        MySerial.print(",");

        tsLastReport = millis();
    }

    // MPU6050 update
    mpu6050.update();

    if (millis() - timer > 1000) {
        x = mpu6050.getGyroX();
        y = mpu6050.getGyroY();
        z = mpu6050.getGyroZ();

        if (activate == 0) { // first sleep confirmation
            if ((x <= 20 && x >= -20) && (y <= 20 && y >= -20) && (z <= 20 && z >= -20)) {
                sleep_timer_start = millis() / 1000 - sleep_timer_end;
                if (sleep_timer_start == 300) {
                    activate = 1;
                }
            }
            if ((x >= 20 || x <= -20) || (y >= 20 || y <= -20) || (z >= 20 || z <= -20)) {
                sleep_timer_end = millis() / 1000;
            }
        }

        if (activate == 1) { // sleeping mode
            light_sleep = millis() / 1000 - sleep_timer_end;

            if (interrupt == 0) {
                if (light_sleep >= 4200) {
                    if (interrupt_for_deep_sleep > 4200) {
                        if (light_sleep - interrupt_sleep_timer >= 600) {
                            deep_sleep = light_sleep - interrupt_for_deep_sleep;
                        }
                    }
                }
            }
            light_sleep = light_sleep - deep_sleep;

            if ((x >= 20 || x <= -20) || (y >= 20 || y <= -20) || (z >= 20 || z <= -20)) {
                interrupt_sleep_timer = millis() / 1000 - sleep_timer_end; 
                interrupt_for_deep_sleep = light_sleep;
                interrupt++;
                delay(8000);
            }

            if (millis() / 1000 - sleep_timer_end - interrupt_sleep_timer > 300) {
                interrupt = 0; 
            }

            if (millis() / 1000 - sleep_timer_end - interrupt_sleep_timer <= 300) {
                if (interrupt >= 5) {
                    sleep_timer_end = millis() / 1000;
                    if (light_sleep >= 900) { // second sleep confirmation
                        total_light_sleep += light_sleep;
                        total_deep_sleep += deep_sleep;
                        total_sleep = total_light_sleep + total_deep_sleep; 
                    }
                    activate = 0;
                    interrupt = 0;}/-strong/-heart:>:o:-((:-h deep_sleep = 0;
                    light_sleep = 0;
                    interrupt_sleep_timer = 0;
                    interrupt_for_deep_sleep = 0;
                }
            }
        }
        stage_sleep_time = light_sleep + deep_sleep; 
        if (stage_sleep_time >= 5400) {
            sleep_timer_end = millis() / 1000;
            total_light_sleep += light_sleep;
            total_deep_sleep += deep_sleep;
            total_sleep = total_light_sleep + total_deep_sleep; 
            activate = 0;
            interrupt = 0;
            deep_sleep = 0;
            light_sleep = 0;
            interrupt_sleep_timer = 0;
            interrupt_for_deep_sleep = 0; 
        }

        Serial.print(sleep_timer_start); // just to know code and sensor working fine
        Serial.print(",");
        if (light_sleep >= 60) { 
            Serial.print(light_sleep / 60);
            Serial.print(",");
            Serial.print(deep_sleep / 60);
            Serial.print(","); 
            MySerial.print(total_light_sleep / 60);
            MySerial.print(",");
            MySerial.print(total_deep_sleep / 60);
            MySerial.print(",");
            MySerial.print(total_sleep / 60);
            
            
        } else {
            Serial.print(0);
            Serial.print(",");
            Serial.print(0);
            Serial.print(","); 
            MySerial.print(sleep_timer_start / 5);
            MySerial.print(",");
            MySerial.print(total_deep_sleep / 60);
            MySerial.print(",");
            MySerial.print(total_sleep / 60);
            
        }

        timer = millis(); // Update the timer
    }
}
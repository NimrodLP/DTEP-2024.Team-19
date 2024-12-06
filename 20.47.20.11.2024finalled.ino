#include <stdio.h>
#include <Adafruit_NeoPixel.h>

#define PIN 7            // The signal pin connected to Arduino    
#define LED_COUNT 120    // Total number of LEDs on the strip

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800);

// Define proximity sensor pins
const int trigPins[3] = {8, 10, 12};  // Trig pins for each sensor
const int echoPins[3] = {9, 11, 13}; // Echo pins for each sensor

// Distance measurement variables
long durations[3];                    // To store pulse durations for each sensor
int distances[3][3];                  // Array to hold multiple distance readings for each sensor
const int HALLWAY_LENGTH = LED_COUNT * 1.67;

// LED control variables
int ledBrightness[LED_COUNT];         // Brightness levels for each LED
const int fadeSpeed = 25;             // Fade speed for each LED
const int maxBrightness = 200;        // Maximum brightness for warm white

// Target loop frequency
const int targetCycleTime = 12500;    // 12.5 ms for 80 Hz

// Sensor thresholds
const int MIN_DISTANCE_CM = 2;        // Minimum valid distance in cm
const int MAX_DISTANCE_CM = HALLWAY_LENGTH; // Maximum valid distance in cm

void setup()
{
  Serial.begin(9600);
  leds.begin();
  clearLEDs();
  leds.show();

  // Initialize pins for each sensor
  for (int s = 0; s < 3; s++) {
    pinMode(trigPins[s], OUTPUT);
    pinMode(echoPins[s], INPUT);
  }
}

// Comparison function for qsort
int compare(const void* a, const void* b)
{
    return (*(int*)a - *(int*)b);
}

// Function to find the median of the array
float medianFn(int arr[], int size)
{
    qsort(arr, size, sizeof(int), compare);
    return (size % 2 == 0) ? (arr[size / 2 - 1] + arr[size / 2]) / 2.0 : arr[size / 2];
}

void loop()
{
  unsigned long startTime = micros(); // Track the start of the loop
  int activeSensor = -1;              // To track the sensor with the closest detected object
  int closestDistance = HALLWAY_LENGTH; // Initialize with max distance

  // Process each sensor to determine the closest detected object
  for (int s = 0; s < 3; s++) {
    int validReadings = 0; // Count valid readings for this sensor

    for (int i = 0; i < 3; i++) { // Take three readings per sensor
      digitalWrite(trigPins[s], LOW);
      delayMicroseconds(10);
      digitalWrite(trigPins[s], HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPins[s], LOW);

      durations[s] = pulseIn(echoPins[s], HIGH, targetCycleTime); // Timeout in microseconds

      // Convert duration to distance
      int distance = durations[s] * 0.034 / 2;

      // Validate the distance
      if (durations[s] > 0 && distance >= MIN_DISTANCE_CM && distance <= MAX_DISTANCE_CM) {
        distances[s][validReadings] = distance;
        validReadings++;
      }
    }

    if (validReadings == 0) {
      continue; // Skip this sensor if no valid readings
    }

    int medianDistance = medianFn(distances[s], validReadings);

    if (medianDistance < closestDistance) {
      closestDistance = medianDistance;
      activeSensor = s;
    }
  }

  if (closestDistance >= HALLWAY_LENGTH) {
    clearLEDs();
    leds.show();
  } else {
    int targetLED = closestDistance / 1.67;  // Convert distance to LED index
    int range = 8;                          // Number of LEDs to illuminate around the target

    /*
    Serial.print("Active Sensor: ");
    Serial.print(activeSensor + 1);
    Serial.print(" Closest Distance: ");
    Serial.println(closestDistance);
    */

    for (int i = targetLED - range; i < targetLED + range; i++) {
      if (i >= 0 && i < LED_COUNT) {
        ledBrightness[i] = maxBrightness;
      }
    }
  }

  for (int i = 0; i < LED_COUNT; i++) {
    if (ledBrightness[i] > 0) {
      ledBrightness[i] -= fadeSpeed;
      if (ledBrightness[i] < 0) ledBrightness[i] = 0;
    }
    leds.setPixelColor(i, leds.Color((int)(ledBrightness[i] * 1.0), (int)(ledBrightness[i] * 0.65), (int)(ledBrightness[i] * 0.25))); // Warm white tone
  }

  leds.show();

  // Calculate elapsed time and wait to maintain 80 Hz
  unsigned long elapsedTime = micros() - startTime;
  if (elapsedTime < targetCycleTime) {
    delayMicroseconds(targetCycleTime - elapsedTime);
  }
}

void clearLEDs()
{
  for (int i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, 0);
    ledBrightness[i] = 0;   
  }
}

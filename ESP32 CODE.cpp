#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "realme 8 5G";
const char* password = "2yg4ysmr";
const char* serverName = "http://192.168.242.192:5000/api/data";

// Sensor pins
const int piezoPin1 = 35;
const int piezoPin2 = 34;
const int piezoPin3 = 39;
const int numSensors = 3;
const int sensorPins[numSensors] = {piezoPin1, piezoPin2, piezoPin3};

// Signal processing parameters
const int movingAverageSize = 10;  // Number of samples for moving average
const int environmentalNoiseThreshold = 25;  // Threshold for environmental noise

// Base thresholds for demo
const int baseLeakThreshold = 230;      // Light tap = leak
const int baseBurstThreshold = 600;    // Hard tap = burst
const int baseCatastrophicThreshold = 1000; // Very hard tap = catastrophic

// LED and control pins
const int greenLEDPin = 12;
const int redLEDPin = 26;
const int buzzerPin = 27;


// Timing variables
unsigned long previousMillis = 0;
const long burstBlinkInterval = 200;  // Blink every 200ms for burst
bool redLEDBlinkState = false;
unsigned long lastHttpSend = 0;
const long httpInterval = 100; // Send data every 100ms

// Dismiss state variables
bool burstDismissed = false;
unsigned long lastDismissCheck = 0;
const long dismissCheckInterval = 1000; // Check dismiss state every 1 second

// Signal processing structures
struct SignalProcessor {
  int rawValue;
  int filteredValue;
  int movingAverage[movingAverageSize];
  int averageIndex;
  int environmentalNoise;
};

struct SimpleSensor {
  int currentValue;
  int maxValue;
  bool isActive;
  SignalProcessor processor;
};

SimpleSensor sensors[numSensors];

// Detection state
struct DetectionState {
  bool leakDetected;
  bool burstDetected;
  bool catastrophicDetected;
  String burstType;
  String location;
  int activeSensors;
  float confidence;
  float burstIntensity;
  bool environmentalNoise;
  float correlationScore;
};

DetectionState detectionState;

// Initialize signal processor
void initSignalProcessor(SignalProcessor* processor) {
  processor->rawValue = 0;
  processor->filteredValue = 0;
  processor->averageIndex = 0;
  processor->environmentalNoise = 0;
  
  for (int i = 0; i < movingAverageSize; i++) {
    processor->movingAverage[i] = 0;
  }
}

// Calculate moving average
int calculateMovingAverage(SignalProcessor* processor, int newValue) {
  processor->movingAverage[processor->averageIndex] = newValue;
  processor->averageIndex = (processor->averageIndex + 1) % movingAverageSize;
  
  int sum = 0;
  for (int i = 0; i < movingAverageSize; i++) {
    sum += processor->movingAverage[i];
  }
  return sum / movingAverageSize;
}

// Detect environmental noise
bool detectEnvironmentalNoise(SignalProcessor* processor) {
  int variance = 0;
  int mean = 0;
  
  // Calculate mean
  for (int i = 0; i < movingAverageSize; i++) {
    mean += processor->movingAverage[i];
  }
  mean /= movingAverageSize;
  
  // Calculate variance
  for (int i = 0; i < movingAverageSize; i++) {
    int diff = processor->movingAverage[i] - mean;
    variance += diff * diff;
  }
  variance /= movingAverageSize;
  
  processor->environmentalNoise = variance;
  return variance > environmentalNoiseThreshold;
}

// Check dismiss state from backend
void checkDismissState() {
  if (WiFi.status() == WL_CONNECTED && (millis() - lastDismissCheck >= dismissCheckInterval)) {
    lastDismissCheck = millis();
    
    HTTPClient http;
    http.setTimeout(1500);
    http.begin("http://192.168.242.192:5000/api/status");
    
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      // Simple JSON parsing for burst_dismissed field
      if (payload.indexOf("\"burst_dismissed\":true") != -1) {
        if (!burstDismissed) {
          burstDismissed = true;
          Serial.println("🔘 BURST ALERT DISMISSED FROM DASHBOARD");
        }
      } else if (payload.indexOf("\"burst_dismissed\":false") != -1) {
        burstDismissed = false;
      }
    }
    http.end();
  }
}

// Determine location based on sensor values (highest values indicate leak/burst location)
String determineLocation(SimpleSensor* sensors, int numSensors, int activeSensorCount) {
  // Find the two sensors with highest values
  int max1 = 0, max2 = 0;
  int sensor1 = -1, sensor2 = -1;
  
  // Find highest value sensor
  for (int i = 0; i < numSensors; i++) {
    if (sensors[i].currentValue > max1) {
      max2 = max1;
      sensor2 = sensor1;
      max1 = sensors[i].currentValue;
      sensor1 = i;
    } else if (sensors[i].currentValue > max2) {
      max2 = sensors[i].currentValue;
      sensor2 = i;
    }
  }
  
  // If only one sensor is active
  if (max2 < 20) {
    if (sensor1 == 0) return "Near Sensor 1 - Main Pipeline Section";
    if (sensor1 == 1) return "Near Sensor 2 - Secondary Pipeline Section";
    if (sensor1 == 2) return "Near Sensor 3 - Pipeline Junction Area";
    return "No activity detected";
  }
  
  // If two or more sensors are active, determine location between them
  if (sensor1 == 0 && sensor2 == 1) {
    return "Between Sensor 1 and Sensor 2 - Main Pipeline Section";
  } else if (sensor1 == 1 && sensor2 == 2) {
    return "Between Sensor 2 and Sensor 3 - Secondary Pipeline Section";
  } else if (sensor1 == 0 && sensor2 == 2) {
    return "Between Sensor 1 and Sensor 3 - Pipeline Junction Area";
  } else if (sensor1 == 1 && sensor2 == 0) {
    return "Between Sensor 2 and Sensor 1 - Main Pipeline Section";
  } else if (sensor1 == 2 && sensor2 == 1) {
    return "Between Sensor 3 and Sensor 2 - Secondary Pipeline Section";
  } else if (sensor1 == 2 && sensor2 == 0) {
    return "Between Sensor 3 and Sensor 1 - Pipeline Junction Area";
  }
  
  // If all three sensors are active
  if (activeSensorCount >= 3) {
    return "Multiple sensors - Pipeline section affected";
  }
  
  return "Unknown location";
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  
  // Initialize sensors and signal processors
  for (int i = 0; i < numSensors; i++) {
    sensors[i].currentValue = 0;
    sensors[i].maxValue = 0;
    sensors[i].isActive = false;
    initSignalProcessor(&sensors[i].processor);
  }
  
  // Initialize detection state
  detectionState.leakDetected = false;
  detectionState.burstDetected = false;
  detectionState.catastrophicDetected = false;
  detectionState.burstType = "NORMAL FLOW";
  detectionState.location = "No activity detected";
  detectionState.activeSensors = 0;
  detectionState.confidence = 0;
  detectionState.burstIntensity = 0;
  detectionState.environmentalNoise = false;
  detectionState.correlationScore = 0;
  
  // Setup pins
  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  // LED test
  digitalWrite(greenLEDPin, HIGH);
  digitalWrite(redLEDPin, HIGH);
  delay(1000);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(redLEDPin, LOW);
  
  Serial.println("🎯 SIGNAL PROCESSING DEMO READY!");
  Serial.println("Light tap = LEAK (Red LED solid)");
  Serial.println("Hard tap = BURST (Red LED blink until dismissed from dashboard)");
  Serial.println("Very hard tap = CATASTROPHIC (Red LED fast blink until dismissed from dashboard)");
  Serial.println("Signal processing: Moving average, Environmental noise detection");
  Serial.println("Click dismiss button on dashboard to stop burst alerts");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check dismiss state from backend
  checkDismissState();
  
  // Read and process all sensors
  int maxSensorValue = 0;
  int activeSensorCount = 0;
  int totalIntensity = 0;
  bool anyEnvironmentalNoise = false;
  
  for (int i = 0; i < numSensors; i++) {
    // Read raw sensor value
    int rawValue = analogRead(sensorPins[i]);
    sensors[i].processor.rawValue = rawValue;
    
    // Apply moving average filter
    sensors[i].processor.filteredValue = calculateMovingAverage(&sensors[i].processor, rawValue);
    
    // Detect environmental noise
    bool hasNoise = detectEnvironmentalNoise(&sensors[i].processor);
    if (hasNoise) anyEnvironmentalNoise = true;
    
    // Use filtered value for detection
    int sensorValue = sensors[i].processor.filteredValue;
    sensors[i].currentValue = sensorValue;
    
    // Update max value for this sensor
    if (sensorValue > sensors[i].maxValue) {
      sensors[i].maxValue = sensorValue;
    }
    
    // Check if sensor is active (above threshold)
    if (sensorValue > baseLeakThreshold) {
      sensors[i].isActive = true;
      activeSensorCount++;
      totalIntensity += sensorValue;
    } else {
      sensors[i].isActive = false;
    }
    
    // Find the highest sensor value
    if (sensorValue > maxSensorValue) {
      maxSensorValue = sensorValue;
    }
  }
  
  // Update detection state with signal processing results
  detectionState.activeSensors = activeSensorCount;
  detectionState.burstIntensity = (activeSensorCount > 0) ? (float)totalIntensity / activeSensorCount : 0;
  detectionState.environmentalNoise = anyEnvironmentalNoise;
  detectionState.correlationScore = activeSensorCount * 25;
  
  // Detection logic
  if (maxSensorValue >= baseCatastrophicThreshold) {
    detectionState.catastrophicDetected = true;
    detectionState.burstDetected = true;
    detectionState.leakDetected = true;
    detectionState.burstType = "CATASTROPHIC BURST";
    detectionState.confidence = 95;
    burstDismissed = false;  // Reset dismiss state for new burst
  } else if (maxSensorValue >= baseBurstThreshold) {
    detectionState.catastrophicDetected = false;
    detectionState.burstDetected = true;
    detectionState.leakDetected = true;
    detectionState.burstType = "PIPELINE BURST";
    detectionState.confidence = 85;
    burstDismissed = false;  // Reset dismiss state for new burst
  } else if (maxSensorValue >= baseLeakThreshold) {
    detectionState.catastrophicDetected = false;
    detectionState.burstDetected = false;
    detectionState.leakDetected = true;
    detectionState.burstType = "PIPELINE LEAK";
    detectionState.confidence = 75;
  } else {
    detectionState.catastrophicDetected = false;
    detectionState.burstDetected = false;
    detectionState.leakDetected = false;
    detectionState.burstType = "NORMAL FLOW";
    detectionState.confidence = 0;
  }
  
  detectionState.location = determineLocation(sensors, numSensors, activeSensorCount);
  
  // LED control - SYNC WITH DETECTION AND DISMISS STATE
  if (detectionState.catastrophicDetected && !burstDismissed) {
    // Catastrophic burst - Very fast blinking until dismissed
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(buzzerPin, HIGH);
    
    if (currentMillis - previousMillis >= 100) { // Fast blink
      previousMillis = currentMillis;
      redLEDBlinkState = !redLEDBlinkState;
      digitalWrite(redLEDPin, redLEDBlinkState ? HIGH : LOW);
    }
  } else if (detectionState.burstDetected && !burstDismissed) {
    // Burst - Normal blinking until dismissed
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(buzzerPin, HIGH);
    
    if (currentMillis - previousMillis >= burstBlinkInterval) {
      previousMillis = currentMillis;
      redLEDBlinkState = !redLEDBlinkState;
      digitalWrite(redLEDPin, redLEDBlinkState ? HIGH : LOW);
    }
  } else if (detectionState.leakDetected && !detectionState.burstDetected) {
    // Leak - Solid red LED (not affected by dismiss button)
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(buzzerPin, LOW);
  } else {
    // Normal - Green LED
    digitalWrite(greenLEDPin, HIGH);
    digitalWrite(redLEDPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
  
  // Debug output with signal processing info
  Serial.print("Raw: ");
  Serial.print(sensors[0].processor.rawValue);
  Serial.print(" ");
  Serial.print(sensors[1].processor.rawValue);
  Serial.print(" ");
  Serial.print(sensors[2].processor.rawValue);
  Serial.print(" | Filtered: ");
  Serial.print(sensors[0].currentValue);
  Serial.print(" ");
  Serial.print(sensors[1].currentValue);
  Serial.print(" ");
  Serial.print(sensors[2].currentValue);
  Serial.print(" | Max: ");
  Serial.print(maxSensorValue);
  Serial.print(" | Status: ");
  Serial.print(detectionState.burstType);
  Serial.print(" | Noise: ");
  Serial.print(detectionState.environmentalNoise ? "YES" : "NO");
  Serial.print(" | Dismissed: ");
  Serial.print(burstDismissed ? "YES" : "NO");
  Serial.print(" | LED: ");
  if (detectionState.catastrophicDetected && !burstDismissed) {
    Serial.print("RED-FAST-BLINK");
  } else if (detectionState.burstDetected && !burstDismissed) {
    Serial.print("RED-BLINK");
  } else if (detectionState.leakDetected && !detectionState.burstDetected) {
    Serial.print("RED-SOLID");
  } else {
    Serial.print("GREEN");
  }
  Serial.println();
  
  // Send data to backend
  if (WiFi.status() == WL_CONNECTED && (currentMillis - lastHttpSend >= httpInterval)) {
    lastHttpSend = currentMillis;
    
    HTTPClient http;
    http.setTimeout(1500);
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    
    String jsonData = "{"
      "\"sensor1\": " + String(sensors[0].currentValue) + ","
      "\"sensor2\": " + String(sensors[1].currentValue) + ","
      "\"sensor3\": " + String(sensors[2].currentValue) + ","
      "\"leak_confirmed\": " + String(detectionState.leakDetected ? 1 : 0) + ","
      "\"burst_confirmed\": " + String(detectionState.burstDetected ? 1 : 0) + ","
      "\"leak_location\": \"" + detectionState.location + "\","
      "\"confidence\": " + String(detectionState.confidence) + ","
      "\"correlation_score\": " + String(detectionState.correlationScore) + ","
      "\"environmental_noise\": " + String(detectionState.environmentalNoise ? 1 : 0) + ","
      "\"active_sensors\": " + String(activeSensorCount) + ","
      "\"burst_type\": \"" + detectionState.burstType + "\","
      "\"burst_intensity\": " + String(detectionState.burstIntensity) + ","
      "\"burst_dismissed\": " + String(burstDismissed ? 1 : 0) + ","
      "\"timestamp\": " + String(currentMillis) +
    "}";
    
    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  
  delay(50); // 50ms delay for responsive detection
}
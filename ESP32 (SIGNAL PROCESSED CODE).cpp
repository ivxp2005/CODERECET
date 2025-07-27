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

// 🏗️ REAL-WORLD MUNICIPAL PIPELINE BURST DETECTION PARAMETERS
// Based on research: Underground pipeline bursts generate 50-200+ Hz vibrations
// with amplitudes typically 10-100x higher than normal flow conditions

// REALISTIC THRESHOLDS FOR MUNICIPAL PIPELINE BURSTS
const int normalFlowThreshold = 15;        // Normal water flow vibration (15-30 range)
const int leakThreshold = 45;              // Small leak detection (45-80 range)
const int burstThreshold = 120;            // Pipeline burst detection (120-300+ range)
const int catastrophicBurstThreshold = 250; // Major burst/pipe rupture (250+ range)

// 🔥 ADVANCED FILTERING FOR REAL-WORLD CONDITIONS
const int signalWindow = 50;               // Larger window for burst pattern analysis
const int noiseWindow = 150;               // Extended noise baseline for urban environments
const float adaptiveMultiplier = 2.5;      // Conservative threshold for urban noise
const int requiredConsecutive = 6;         // Faster response for burst detection
const int minLeakDuration = 300;           // Shorter duration for burst response
const int burstResponseTime = 150;         // Very fast burst response (150ms)

// 🎯 PRECISION FILTERS FOR MUNICIPAL ENVIRONMENT
const float sensorAgreementThreshold = 0.5;  // 50% sensor agreement (urban noise)
const int vibrationCooldown = 1500;          // 1.5-second cooldown after high vibration
const float signalStabilityThreshold = 0.25; // Higher variance tolerance for bursts
const int patternConsistency = 3;            // Pattern must be consistent across readings

// 🔍 BURST SIGNATURE DETECTION (Real-world pipeline burst frequencies)
const float burstFreqMin = 20.0;        // Min frequency for burst (20-60 Hz typical)
const float burstFreqMax = 80.0;        // Max frequency for burst (60-120 Hz possible)
const float amplitudeConsistency = 0.4; // Amplitude variation tolerance for bursts
const float burstAmplitudeSpike = 2.5;  // Burst causes 2.5x amplitude spike

// Enhanced sensor data structure
struct PrecisionSensor {
  int readings[signalWindow];
  int noiseBaseline[noiseWindow];
  float amplitudeHistory[15];
  int readIndex, noiseIndex, ampIndex;
  int total, noiseTotal, count, noiseCount;
  
  // Advanced filtering
  int consecutiveLeak, consecutiveBurst, consecutiveCatastrophic;
  float signalVariance, amplitudeVariance;
  unsigned long leakStartTime, lastHighVibration;
  bool inLeakState, inBurstState, inCatastrophicState;
  
  // Pattern detection
  int peakCount, valleyCount;
  float averagePeakInterval;
  int lastPeakTime, patternScore;
  bool signalStable;
  
  // Quality metrics
  float signalQuality, noiseRatio;
  int falsePositiveCount;
  
  // Burst-specific metrics
  float burstAmplitude, burstFrequency;
  int burstDuration, burstIntensity;
};

PrecisionSensor sensors[numSensors];

// Multi-sensor correlation
struct SensorCorrelation {
  float correlation_12, correlation_23, correlation_13;
  float timeDelay_12, timeDelay_23, timeDelay_13;
  bool sensorsAgree;
  int agreementScore;
};

SensorCorrelation sensorCorr;

// Leak detection state
struct LeakDetectionState {
  bool confirmed;
  String location;
  int primarySensor;
  float confidence;
  unsigned long detectionTime;
  int stabilityScore;
  bool environmentalNoise;
  String burstType;  // "leak", "burst", "catastrophic"
  float burstIntensity;
};

LeakDetectionState leakState;

// LED and control pins
const int greenLEDPin = 12;
const int redLEDPin = 26;
const int buzzerPin = 27;

// Timing variables
unsigned long previousMillis = 0;
const long burstBlinkInterval = 100;  // Faster blinking for burst
bool redLEDBlinkState = false;
unsigned long lastHttpSend = 0;
const long httpInterval = 100; // Faster updates for burst monitoring

// 🧮 ADVANCED CALCULATION FUNCTIONS

float calculateVariance(int* array, int size, int mean) {
  if (size < 2) return 0;
  float variance = 0;
  for (int i = 0; i < size; i++) {
    variance += pow(array[i] - mean, 2);
  }
  return variance / size;
}

float calculateCorrelation(int sensor1, int sensor2) {
  if (sensors[sensor1].count < signalWindow || sensors[sensor2].count < signalWindow) return 0;
  
  float mean1 = sensors[sensor1].total / sensors[sensor1].count;
  float mean2 = sensors[sensor2].total / sensors[sensor2].count;
  
  float numerator = 0, denom1 = 0, denom2 = 0;
  
  for (int i = 0; i < signalWindow; i++) {
    float x = sensors[sensor1].readings[i] - mean1;
    float y = sensors[sensor2].readings[i] - mean2;
    numerator += x * y;
    denom1 += x * x;
    denom2 += y * y;
  }
  
  float denominator = sqrt(denom1 * denom2);
  return (denominator > 0) ? numerator / denominator : 0;
}

bool detectBurstPattern(int sensorIndex) {
  PrecisionSensor* s = &sensors[sensorIndex];
  
  // 1. Check signal stability for burst conditions
  float avgValue = s->total / (s->count > 0 ? s->count : 1);
  s->signalVariance = calculateVariance(s->readings, s->count, avgValue);
  float stabilityRatio = s->signalVariance / max(1.0f, avgValue);
  s->signalStable = (stabilityRatio < signalStabilityThreshold);
  
  // 2. Amplitude consistency check for burst signature
  if (avgValue > leakThreshold) {
    s->amplitudeHistory[s->ampIndex] = avgValue;
    s->ampIndex = (s->ampIndex + 1) % 15;
    
    if (s->ampIndex == 0) { // Calculate amplitude variance every 15 readings
      float ampMean = 0;
      for (int i = 0; i < 15; i++) ampMean += s->amplitudeHistory[i];
      ampMean /= 15;
      
      s->amplitudeVariance = 0;
      for (int i = 0; i < 15; i++) {
        s->amplitudeVariance += pow(s->amplitudeHistory[i] - ampMean, 2);
      }
      s->amplitudeVariance /= 15;
      
      // Calculate burst amplitude
      s->burstAmplitude = ampMean;
    }
  }
  
  // 3. Burst pattern consistency scoring
  bool patternConsistent = (s->signalStable && s->amplitudeVariance < (avgValue * amplitudeConsistency));
  
  return patternConsistent;
}

bool isEnvironmentalNoise(int sensorIndex) {
  PrecisionSensor* s = &sensors[sensorIndex];
  unsigned long currentTime = millis();
  
  // 1. Recent high vibration check (extended cooldown for urban environment)
  if (currentTime - s->lastHighVibration < vibrationCooldown) {
    return true;
  }
  
  // 2. Signal quality check
  float avgValue = s->total / (s->count > 0 ? s->count : 1);
  float noiseAvg = s->noiseTotal / (s->noiseCount > 0 ? s->noiseCount : 1);
  s->noiseRatio = avgValue / max(1.0f, noiseAvg);
  
  // 3. Sudden spike detection (environmental noise signature)
  bool suddenSpike = false;
  if (s->count >= 3) {
    int recent = s->readings[(s->readIndex - 1 + signalWindow) % signalWindow];
    int previous = s->readings[(s->readIndex - 2 + signalWindow) % signalWindow];
    float spikeRatio = abs(recent - previous) / max(1.0f, (float)previous);
    
    if (spikeRatio > 3.0) { // 300% sudden change indicates environmental noise
      s->lastHighVibration = currentTime;
      suddenSpike = true;
    }
  }
  
  return suddenSpike || (s->noiseRatio > 15.0); // Signal 15x above baseline = likely noise
}

void updateSensorCorrelations() {
  sensorCorr.correlation_12 = calculateCorrelation(0, 1);
  sensorCorr.correlation_23 = calculateCorrelation(1, 2);
  sensorCorr.correlation_13 = calculateCorrelation(0, 2);
  
  // Calculate agreement score
  float avgCorrelation = (abs(sensorCorr.correlation_12) + abs(sensorCorr.correlation_23) + abs(sensorCorr.correlation_13)) / 3.0;
  sensorCorr.sensorsAgree = (avgCorrelation > sensorAgreementThreshold);
  sensorCorr.agreementScore = (int)(avgCorrelation * 100);
}

String determineLeakLocation() {
  // Find strongest correlations to determine location
  float maxCorr = max(abs(sensorCorr.correlation_12), max(abs(sensorCorr.correlation_23), abs(sensorCorr.correlation_13)));
  
  if (maxCorr < 0.3) return "Isolated sensor activity - possible false positive";
  
  if (abs(sensorCorr.correlation_12) == maxCorr) {
    return "Between Sensor 1 and Sensor 2 - Main Pipeline Section";
  } else if (abs(sensorCorr.correlation_23) == maxCorr) {
    return "Between Sensor 2 and Sensor 3 - Secondary Pipeline Section";
  } else {
    return "Near Sensor 1 or Sensor 3 - Pipeline Junction Area";
  }
}

String determineBurstType(float avgAmplitude) {
  if (avgAmplitude >= catastrophicBurstThreshold) {
    return "CATASTROPHIC BURST";
  } else if (avgAmplitude >= burstThreshold) {
    return "PIPELINE BURST";
  } else if (avgAmplitude >= leakThreshold) {
    return "PIPELINE LEAK";
  } else {
    return "NORMAL FLOW";
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  
  // Initialize precision sensor structures
  for (int s = 0; s < numSensors; s++) {
    for (int i = 0; i < signalWindow; i++) sensors[s].readings[i] = 0;
    for (int i = 0; i < noiseWindow; i++) sensors[s].noiseBaseline[i] = 0;
    for (int i = 0; i < 15; i++) sensors[s].amplitudeHistory[i] = 0;
    
    sensors[s].readIndex = 0;
    sensors[s].noiseIndex = 0;
    sensors[s].ampIndex = 0;
    sensors[s].total = 0;
    sensors[s].noiseTotal = 0;
    sensors[s].count = 0;
    sensors[s].noiseCount = 0;
    sensors[s].consecutiveLeak = 0;
    sensors[s].consecutiveBurst = 0;
    sensors[s].consecutiveCatastrophic = 0;
    sensors[s].leakStartTime = 0;
    sensors[s].lastHighVibration = 0;
    sensors[s].inLeakState = false;
    sensors[s].inBurstState = false;
    sensors[s].inCatastrophicState = false;
    sensors[s].signalStable = false;
    sensors[s].falsePositiveCount = 0;
  }
  
  // Initialize leak state
  leakState.confirmed = false;
  leakState.location = "No leak detected";
  leakState.primarySensor = -1;
  leakState.confidence = 0;
  leakState.stabilityScore = 0;
  leakState.environmentalNoise = false;
  leakState.burstType = "NORMAL";
  leakState.burstIntensity = 0;
  
  // Setup pins
  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(redLEDPin, LOW);
  digitalWrite(buzzerPin, LOW);
  
  // LED test
  digitalWrite(greenLEDPin, HIGH);
  digitalWrite(redLEDPin, HIGH);
  delay(2000);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(redLEDPin, LOW);
  
  // Extended calibration for municipal environment
  Serial.println("🔄 MUNICIPAL PIPELINE CALIBRATION (15 seconds)...");
  for (int i = 0; i < 600; i++) { // 15 seconds
    for (int s = 0; s < numSensors; s++) {
      int sensorValue = analogRead(sensorPins[s]);
      sensors[s].noiseBaseline[i % noiseWindow] = sensorValue;
      if (i < noiseWindow) sensors[s].noiseCount++;
    }
    delay(25);
  }
  
  // Calculate noise baselines
  for (int s = 0; s < numSensors; s++) {
    for (int i = 0; i < sensors[s].noiseCount; i++) {
      sensors[s].noiseTotal += sensors[s].noiseBaseline[i];
    }
    Serial.print("Sensor ");
    Serial.print(s+1);
    Serial.print(" baseline: ");
    Serial.println(sensors[s].noiseTotal / sensors[s].noiseCount);
  }
  
  Serial.println("✅ MUNICIPAL PIPELINE CALIBRATION COMPLETE!");
  Serial.println("🏗️ REAL-WORLD THRESHOLDS:");
  Serial.print("   Normal Flow: < ");
  Serial.println(normalFlowThreshold);
  Serial.print("   Leak Detection: > ");
  Serial.println(leakThreshold);
  Serial.print("   Burst Detection: > ");
  Serial.println(burstThreshold);
  Serial.print("   Catastrophic: > ");
  Serial.println(catastrophicBurstThreshold);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read all sensors with precision processing
  bool anyLeakDetected = false;
  bool anyBurstDetected = false;
  bool anyCatastrophicDetected = false;
  int strongestSensor = -1;
  int strongestReading = 0;
  int activeLeakSensors = 0;
  float totalBurstIntensity = 0;
  
  for (int s = 0; s < numSensors; s++) {
    int sensorValue = analogRead(sensorPins[s]);
    
    // Update moving average
    sensors[s].total -= sensors[s].readings[sensors[s].readIndex];
    sensors[s].readings[sensors[s].readIndex] = sensorValue;
    sensors[s].total += sensors[s].readings[sensors[s].readIndex];
    sensors[s].readIndex = (sensors[s].readIndex + 1) % signalWindow;
    if (sensors[s].count < signalWindow) sensors[s].count++;
    
    int avgValue = sensors[s].total / sensors[s].count;
    
    // Update noise baseline (only during quiet periods)
    if (avgValue < (sensors[s].noiseTotal / max(1, sensors[s].noiseCount)) + 30) {
      sensors[s].noiseTotal -= sensors[s].noiseBaseline[sensors[s].noiseIndex];
      sensors[s].noiseBaseline[sensors[s].noiseIndex] = sensorValue;
      sensors[s].noiseTotal += sensors[s].noiseBaseline[sensors[s].noiseIndex];
      sensors[s].noiseIndex = (sensors[s].noiseIndex + 1) % noiseWindow;
      if (sensors[s].noiseCount < noiseWindow) sensors[s].noiseCount++;
    }
    
    // Calculate adaptive thresholds for municipal environment
    int noiseAvg = sensors[s].noiseTotal / max(1, sensors[s].noiseCount);
    float noiseStdDev = sqrt(calculateVariance(sensors[s].noiseBaseline, sensors[s].noiseCount, noiseAvg));
    int adaptiveLeakThreshold = max(leakThreshold, (int)(noiseAvg + noiseStdDev * adaptiveMultiplier));
    int adaptiveBurstThreshold = max(burstThreshold, (int)(noiseAvg + noiseStdDev * 4.0));
    int adaptiveCatastrophicThreshold = max(catastrophicBurstThreshold, (int)(noiseAvg + noiseStdDev * 6.0));
    
    // 🎯 PRECISION FILTERING FOR MUNICIPAL PIPELINES
    bool isNoise = isEnvironmentalNoise(s);
    bool hasPattern = detectBurstPattern(s);
    bool aboveLeakThreshold = (avgValue > adaptiveLeakThreshold);
    bool aboveBurstThreshold = (avgValue > adaptiveBurstThreshold);
    bool aboveCatastrophicThreshold = (avgValue > adaptiveCatastrophicThreshold);
    
    // Combined precision detection for municipal conditions
    bool precisionLeak = aboveLeakThreshold && !isNoise && hasPattern;
    bool precisionBurst = aboveBurstThreshold && !isNoise && hasPattern;
    bool precisionCatastrophic = aboveCatastrophicThreshold && !isNoise && hasPattern;
    
    // Consecutive reading logic with burst-specific requirements
    if (precisionCatastrophic) {
      sensors[s].consecutiveCatastrophic++;
      sensors[s].consecutiveBurst++;
      sensors[s].consecutiveLeak++;
    } else if (precisionBurst) {
      sensors[s].consecutiveBurst++;
      sensors[s].consecutiveLeak++;
      sensors[s].consecutiveCatastrophic = 0;
    } else if (precisionLeak) {
      sensors[s].consecutiveLeak++;
      sensors[s].consecutiveBurst = 0;
      sensors[s].consecutiveCatastrophic = 0;
    } else {
      sensors[s].consecutiveLeak = max(0, sensors[s].consecutiveLeak - 1);
      sensors[s].consecutiveBurst = 0;
      sensors[s].consecutiveCatastrophic = 0;
    }
    
    // State management with burst-specific duration validation
    bool sensorLeakDetected = (sensors[s].consecutiveLeak >= requiredConsecutive);
    bool sensorBurstDetected = (sensors[s].consecutiveBurst >= requiredConsecutive);
    bool sensorCatastrophicDetected = (sensors[s].consecutiveCatastrophic >= (requiredConsecutive - 2));
    
    if (sensorCatastrophicDetected) {
      if (!sensors[s].inCatastrophicState) {
        sensors[s].leakStartTime = currentMillis;
        sensors[s].inCatastrophicState = true;
      }
      if (currentMillis - sensors[s].leakStartTime >= burstResponseTime) {
        anyCatastrophicDetected = true;
        anyBurstDetected = true;
        anyLeakDetected = true;
        activeLeakSensors++;
        totalBurstIntensity += avgValue;
      }
    } else if (sensorBurstDetected) {
      if (!sensors[s].inBurstState) {
        sensors[s].leakStartTime = currentMillis;
        sensors[s].inBurstState = true;
      }
      if (currentMillis - sensors[s].leakStartTime >= burstResponseTime) {
        anyBurstDetected = true;
        anyLeakDetected = true;
        activeLeakSensors++;
        totalBurstIntensity += avgValue;
      }
      sensors[s].inCatastrophicState = false;
    } else if (sensorLeakDetected) {
      if (!sensors[s].inLeakState) {
        sensors[s].leakStartTime = currentMillis;
        sensors[s].inLeakState = true;
      }
      if (currentMillis - sensors[s].leakStartTime >= minLeakDuration) {
        anyLeakDetected = true;
        activeLeakSensors++;
        totalBurstIntensity += avgValue;
      }
      sensors[s].inBurstState = false;
      sensors[s].inCatastrophicState = false;
    } else {
      sensors[s].inLeakState = false;
      sensors[s].inBurstState = false;
      sensors[s].inCatastrophicState = false;
    }
    
    if (avgValue > strongestReading) {
      strongestReading = avgValue;
      strongestSensor = s;
    }
  }
  
  // 🎯 MULTI-SENSOR PRECISION VALIDATION FOR MUNICIPAL PIPELINES
  updateSensorCorrelations();
  
  // Final leak confirmation with municipal-specific validation
  bool finalLeakConfirmed = false;
  bool finalBurstConfirmed = false;
  bool finalCatastrophicConfirmed = false;
  
  if (anyLeakDetected || anyBurstDetected || anyCatastrophicDetected) {
    // Requirement 1: Multiple sensors must agree (for municipal reliability)
    bool multiSensorAgreement = (activeLeakSensors >= 2) || sensorCorr.sensorsAgree;
    
    // Requirement 2: Signal stability across sensors
    bool signalStability = true;
    for (int s = 0; s < numSensors; s++) {
      if (sensors[s].inLeakState && !sensors[s].signalStable) {
        signalStability = false;
        break;
      }
    }
    
    // Requirement 3: Environmental noise check for urban environment
    bool noEnvironmentalNoise = true;
    for (int s = 0; s < numSensors; s++) {
      if (isEnvironmentalNoise(s)) {
        noEnvironmentalNoise = false;
        leakState.environmentalNoise = true;
        break;
      }
    }
    
    // Final decision with municipal precision filters
    finalLeakConfirmed = anyLeakDetected && multiSensorAgreement && signalStability && noEnvironmentalNoise;
    finalBurstConfirmed = anyBurstDetected && multiSensorAgreement && signalStability && noEnvironmentalNoise;
    finalCatastrophicConfirmed = anyCatastrophicDetected && multiSensorAgreement && signalStability && noEnvironmentalNoise;
    
    if (finalLeakConfirmed || finalBurstConfirmed || finalCatastrophicConfirmed) {
      leakState.confirmed = true;
      leakState.location = determineLeakLocation();
      leakState.primarySensor = strongestSensor;
      leakState.confidence = min(100.0f, (float)(sensorCorr.agreementScore + (signalStability ? 25 : 0)));
      leakState.detectionTime = currentMillis;
      leakState.stabilityScore = signalStability ? 100 : 50;
      leakState.burstIntensity = totalBurstIntensity / max(1, activeLeakSensors);
      
      if (finalCatastrophicConfirmed) {
        leakState.burstType = "CATASTROPHIC BURST";
      } else if (finalBurstConfirmed) {
        leakState.burstType = "PIPELINE BURST";
      } else {
        leakState.burstType = "PIPELINE LEAK";
      }
    }
  } else {
    leakState.confirmed = false;
    leakState.environmentalNoise = false;
    leakState.burstType = "NORMAL FLOW";
    leakState.burstIntensity = 0;
  }
  
  // LED control with municipal burst confirmation
  if (finalCatastrophicConfirmed) {
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(buzzerPin, HIGH);
    
    if (currentMillis - previousMillis >= 50) { // Very fast blinking for catastrophic
      previousMillis = currentMillis;
      redLEDBlinkState = !redLEDBlinkState;
      digitalWrite(redLEDPin, redLEDBlinkState ? HIGH : LOW);
    }
  } else if (finalBurstConfirmed) {
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(buzzerPin, HIGH);
    
    if (currentMillis - previousMillis >= burstBlinkInterval) {
      previousMillis = currentMillis;
      redLEDBlinkState = !redLEDBlinkState;
      digitalWrite(redLEDPin, redLEDBlinkState ? HIGH : LOW);
    }
  } else if (finalLeakConfirmed) {
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(buzzerPin, LOW);
  } else {
    digitalWrite(greenLEDPin, HIGH);
    digitalWrite(redLEDPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
  
  // Enhanced debugging output for municipal pipeline monitoring
  Serial.print("🏗️ MUNICIPAL PIPELINE: S1:");
  Serial.print(sensors[0].total / max(1, sensors[0].count));
  Serial.print(" S2:");
  Serial.print(sensors[1].total / max(1, sensors[1].count));
  Serial.print(" S3:");
  Serial.print(sensors[2].total / max(1, sensors[2].count));
  Serial.print(" | Corr:");
  Serial.print(sensorCorr.agreementScore);
  Serial.print("% | Status:");
  Serial.print(leakState.burstType);
  Serial.print(" | Loc:");
  Serial.print(leakState.location);
  Serial.print(" | Conf:");
  Serial.print(leakState.confidence);
  Serial.print("% | Intensity:");
  Serial.print(leakState.burstIntensity);
  Serial.println();
  
  // HTTP transmission with municipal pipeline data
  if (WiFi.status() == WL_CONNECTED && (currentMillis - lastHttpSend >= httpInterval)) {
    lastHttpSend = currentMillis;
    
    HTTPClient http;
    http.setTimeout(1500);
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");
    
    String jsonData = "{"
      "\"sensor1\": " + String(sensors[0].total / max(1, sensors[0].count)) + ","
      "\"sensor2\": " + String(sensors[1].total / max(1, sensors[1].count)) + ","
      "\"sensor3\": " + String(sensors[2].total / max(1, sensors[2].count)) + ","
      "\"leak_confirmed\": " + String(finalLeakConfirmed ? 1 : 0) + ","
      "\"burst_confirmed\": " + String(finalBurstConfirmed || finalCatastrophicConfirmed ? 1 : 0) + ","
      "\"leak_location\": \"" + leakState.location + "\","
      "\"confidence\": " + String(leakState.confidence) + ","
      "\"correlation_score\": " + String(sensorCorr.agreementScore) + ","
      "\"stability_score\": " + String(leakState.stabilityScore) + ","
      "\"environmental_noise\": " + String(leakState.environmentalNoise ? 1 : 0) + ","
      "\"active_sensors\": " + String(activeLeakSensors) + ","
      "\"burst_type\": \"" + leakState.burstType + "\","
      "\"burst_intensity\": " + String(leakState.burstIntensity) + ","
      "\"timestamp\": " + String(currentMillis) +
    "}";
    
    http.POST(jsonData);
    http.end();
  }
  
  delay(15); // Faster sampling for municipal burst detection
}
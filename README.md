# CODERECET

## Project Repository
*Commit and save your changes here*

### Team Name : 404FoundUs
### Team Members : Pardhiv Suresh M, R Sooryanarayan, Nazreen Shemeem, Liya Mary Mathew
### Project Description
Our project focuses on detecting underground water pipeline leaks using a piezoelectric sensor and an ESP32 microcontroller. When a pressurized pipe leaks, it generates small vibrations that are picked up by the piezoelectric sensor. These vibrations are then processed by the ESP32, which analyzes the signal to detect abnormal patterns that indicate a possible leak. When such a pattern is found, the system sends alerts in real time using Wi-Fi or other communication methods.
In the future, this system can be enhanced with advanced signal processing techniques or machine learning models to improve accuracy and reduce false detections caused by environmental noise. It can also be ruggedized for real-world underground deployment using weatherproof housings and solar or battery-powered operation. This project aims to offer a scalable and affordable solution to help reduce water loss and support smarter water infrastructure.

## Technical Details

### Technologies/Components Used

## For Software:

1. Languages Used

C++ - ESP32 microcontroller programming
JavaScript - Backend server and API
JSX/React - Frontend user interface
SQL - Database operations
HTML/CSS - Web styling

2. Frameworks Used

Node.js - Server runtime environment
Express.js - Backend web framework
React.js - Frontend UI framework
Arduino Framework - ESP32 development

3. Libraries Used

Express - Web server and API
SQLite3 - Database engine
CORS - Cross-origin middleware
React Router DOM - Navigation
Chart.js - Data visualization
WiFi.h - ESP32 wireless connectivity
HTTPClient.h - ESP32 HTTP communication

4. Tools Used

Arduino IDE - ESP32 development
Visual Studio Code - Code editing
npm - Package management
Git - Version control
React Scripts - Frontend build tools
SQLite Browser - Database management
Arduino Serial Monitor - ESP32 debugging


## For Hardware:

1. Main Components Used

Component List:

ESP32 Development Board (ESP32 DevKit V1 or similar)
3x Piezoelectric Sensors (vibration sensors)
2x LEDs (Green and Red)
3x 10kΩ Resistors (pull-down for piezo sensors)
2x 220Ω Resistors (current limiting for LEDs)
Breadboard and Jumper Wires
USB Cable (for programming and power)

2. Specifications

The ESP32 features built-in Wi-Fi and ADCs for analog input. The piezo sensors generate voltage in response to vibration. 1MΩ resistors stabilize the sensor signal, and 220Ω resistors protect the LEDs. Standard 5mm LEDs are used for indication.

3. Tools Required

We used a laptop with Arduino IDE for coding, a USB cable for uploading and power, and a multimeter for testing. In future versions, soldering tools, epoxy, and waterproof casings can be added for durable underground deployment.

## Implementation

## For Software:

### Installation

Installation Commands
---------------------

Backend Setup:
--------------
Run Commands:
cd backend
npm install

Frontend Setup:
---------------
Run commands:
cd frontend
npm install

ESP32 Setup:
------------
Install Arduino IDE
Add ESP32 board support in Arduino IDE
Install required libraries:
WiFi library (built-in)
HTTPClient library (built-in)

### Run
Run Commands:
-------------

Backend Server
Run:
cd backend
npm start

Frontend Application
Run:
cd frontend
npm start

### Project Documentation

### Screenshots (Add at least 3)

### Diagrams
Workflow:
1. Sensor Data Collection
- *3 Piezoelectric sensors* detect pipeline vibrations
- *Real-time analog readings* converted to digital values
- *Signal processing* filters environmental noise
- *Multi-sensor correlation* validates readings

2. ESP32 Processing
- *WiFi connection* to network
- *Advanced signal filtering* (moving average, noise detection)
- *Threshold comparison*:
  - Normal: < 45
  - Leak: 45-120
  - Burst: 120-250
  - Catastrophic: > 250
- *HTTP POST requests* send data to backend every 100ms

3. Backend Processing
- *Express.js server* receives sensor data
- *SQLite database* stores historical data
- *RESTful API endpoints*:
  - POST /api/data - Receive sensor data
  - GET /api/status - Latest status
  - GET /api/history - Historical data
  - POST /api/dismiss - Dismiss alerts
- *Data validation* and error handling

4. Frontend Dashboard
- *React.js application* displays real-time data
- *Chart.js visualization* shows sensor trends
- *Real-time updates* every 100ms
- *Alert system* with visual indicators
- *Responsive design* for all devices

5. Alert System
- *Visual LED indicators* on ESP32
- *Web dashboard alerts* 
- *telegram bot notifications* (expandable)
- *Historical logging* for analysis

Detection Algorithm Workflow


Sensor Reading → Noise Filter → Threshold Check → Multi-Sensor Validation → Alert Trigger


Signal Processing Pipeline

1. *Raw sensor data* collection
2. *Moving average filtering* (10-sample window)
3. *Environmental noise detection* (variance analysis)
4. *Adaptive threshold adjustment*
5. *Multi-sensor correlation* validation
6. *Pattern recognition* for burst signatures
7. *Confidence scoring* and location estimation

## 🔄 *Data Flow Diagram*


[Pipeline] → [Piezo Sensors] → [ESP32] → [WiFi] → [Backend API] → [Database] → [Frontend] → [User Dashboard]
                   ↓              ↓              ↓                    ↓                           ↓
              [Vibration]  [Signal Process]  [HTTP POST]         [Data Store]            [Real-time Display]


Real-Time Response Workflow

1. *Event Detection* (0-50ms)
2. *Signal Processing* (50-100ms)
3. *Data Transmission* (100-150ms)
4. *Backend Processing* (150-200ms)
5. *Dashboard Update* (200-250ms)
6. *Alert Display* (250-300ms)

Error Handling Workflow

- *Sensor failure* → Use remaining sensors
- *WiFi disconnection* → Local storage + retry
- *Backend failure* → ESP32 continues monitoring
- *Database errors* → Fallback to memory storage
- *Frontend errors* → Graceful degradation



## For Hardware:

### Schematic & Circuit

.
                    ESP32 DEVKIT V1
                ┌─────────────────────────┐
                │                         │
   5V Power ────┤ VIN                     │
                │                         │
   Ground ──────┤ GND                     │
                │                         │
Piezo Sensor 1 ─┤ GPIO 35                 │
                │                         │
Piezo Sensor 2 ─┤ GPIO 34                 │
                │                         │
Piezo Sensor 3 ─┤ VIN                     │
                │                         │
   Green LED ───┤ GPIO 12                 │
                │                         │
    Red LED ────┤ GPIO 26                 │
                │                         │
                |                         │
                │                         │
                └─────────────────────────┘
     

Build Photos:

Components(https://postimg.cc/gallery/B2yBd0Z) 
components shown:
ESP32 
piezoelectric sensors
jumberwires
resisters

Build(https://postimg.cc/gallery/jRBqtmV) 
Final(https://postimg.cc/gallery/G8QFfZ7) 

### Project Demo

### Video
[https://youtube.com/shorts/xG9N4DIJjbA?feature=shared] 

## Additional Demos
[Add any extra demo materials/links]

## Team Contributions
R Sooryanarayan: Iot hardware components, analyzing and collecting reports and data
Nazreen Shemeem: presentation and designing
Liya Mary Mathew : web dashboard, simulation software and alert automation

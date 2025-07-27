# Advanced 3-Sensor Leak Detection System

A comprehensive IoT-based leak detection system using ESP32 with 3 piezoelectric sensors, advanced signal processing, and a real-time web dashboard.

## 🚀 Features

### Hardware (ESP32)
- **3 Piezoelectric Sensors**: Multi-point monitoring for precise leak detection
- **Advanced Signal Processing**: 
  - Adaptive noise filtering
  - Multi-sensor correlation analysis
  - Pattern recognition for leak signatures
  - Environmental noise detection
- **Precision Detection**:
  - Leak threshold: 70 (configurable)
  - Burst threshold: 100 (configurable)
  - Signal window: 30 samples
  - Noise baseline: 100 samples
  - Required consecutive readings: 8
  - Minimum leak duration: 500ms

### Backend (Node.js + SQLite)
- **RESTful API** with endpoints:
  - `POST /api/data` - Receive sensor data from ESP32
  - `GET /api/status` - Get latest sensor status
  - `GET /api/history` - Get historical data for charts
  - `GET /api/sensors` - Get individual sensor data
- **Database Schema**:
  - 3 sensor values (sensor1, sensor2, sensor3)
  - Leak detection status (leak_confirmed, burst_confirmed)
  - Location information (leak_location)
  - Confidence metrics (confidence, correlation_score, stability_score)
  - Environmental data (environmental_noise, active_sensors)

### Frontend (React)
- **Real-time Dashboard** with:
  - Individual sensor value displays
  - Multi-sensor chart visualization
  - Advanced leak detection status
  - System health monitoring
  - Responsive design for mobile/desktop

## 📊 Data Structure

### ESP32 → Backend (JSON)
```json
{
  "sensor1": 150,
  "sensor2": 180,
  "sensor3": 120,
  "leak_confirmed": 0,
  "burst_confirmed": 0,
  "leak_location": "No leak detected",
  "confidence": 85.5,
  "correlation_score": 92,
  "stability_score": 88,
  "environmental_noise": 0,
  "active_sensors": 0,
  "timestamp": 1234567890
}
```

### Backend → Frontend (JSON)
```json
{
  "status": "Normal",
  "sensor1": 150,
  "sensor2": 180,
  "sensor3": 120,
  "leak_confirmed": false,
  "burst_confirmed": false,
  "leak_location": "No leak detected",
  "confidence": 85.5,
  "correlation_score": 92,
  "stability_score": 88,
  "environmental_noise": false,
  "active_sensors": 0,
  "timestamp": "2024-01-01T12:00:00.000Z"
}
```

## 🛠️ Installation & Setup

### Prerequisites
- Node.js (v14+)
- ESP32 development board
- 3 piezoelectric sensors
- WiFi network

### Backend Setup
```bash
cd backend
npm install
npm start
```

### Frontend Setup
```bash
cd frontend
npm install
npm start
```

### ESP32 Setup
1. Install Arduino IDE with ESP32 board support
2. Upload `code2.cpp` to your ESP32
3. Update WiFi credentials in the code
4. Update server IP address to match your backend

## 🔧 Configuration

### ESP32 Parameters (code2.cpp)
```cpp
// WiFi Configuration
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";
const char* serverName = "http://your_server_ip:5000/api/data";

// Sensor Pins
const int piezoPin1 = 35;
const int piezoPin2 = 34;
const int piezoPin3 = 39;

// Detection Thresholds
const int leakThreshold = 70;
const int burstThreshold = 100;

// Advanced Filtering
const int signalWindow = 30;
const int noiseWindow = 100;
const float adaptiveMultiplier = 3.0;
const int requiredConsecutive = 8;
const int minLeakDuration = 500;
```

### Backend Configuration (server.js)
```javascript
const PORT = 5000;
const dbPath = path.join(__dirname, 'data.db');
```

## 📈 API Endpoints

### POST /api/data
Receive sensor data from ESP32
```bash
curl -X POST http://localhost:5000/api/data \
  -H "Content-Type: application/json" \
  -d '{
    "sensor1": 150,
    "sensor2": 180,
    "sensor3": 120,
    "leak_confirmed": 0,
    "burst_confirmed": 0,
    "leak_location": "No leak detected",
    "confidence": 85.5,
    "correlation_score": 92,
    "stability_score": 88,
    "environmental_noise": 0,
    "active_sensors": 0
  }'
```

### GET /api/status
Get latest sensor status
```bash
curl http://localhost:5000/api/status
```

### GET /api/history
Get historical data for charts
```bash
curl http://localhost:5000/api/history
```

## 🧪 Testing

Run the API test script:
```bash
cd backend
node test_api.js
```

## 🎯 Detection Algorithm

1. **Signal Acquisition**: Read from 3 piezoelectric sensors
2. **Noise Filtering**: Adaptive baseline calculation
3. **Pattern Analysis**: Multi-sensor correlation
4. **Threshold Detection**: Adaptive thresholds based on noise
5. **Confirmation**: Multiple consecutive readings required
6. **Location Estimation**: Based on sensor correlation patterns
7. **Environmental Check**: Filter out environmental noise

## 📱 Dashboard Features

- **Real-time Monitoring**: Live sensor values and status
- **Multi-sensor Visualization**: Chart showing all 3 sensors
- **Advanced Detection Display**: Confidence scores and location
- **System Health**: Signal quality and environmental status
- **Responsive Design**: Works on mobile and desktop
- **Alert System**: Burst detection with visual alerts

## 🔒 Security Considerations

- Update default WiFi credentials
- Use HTTPS in production
- Implement authentication for API endpoints
- Secure database access
- Regular security updates

## 🚨 Troubleshooting

### Common Issues
1. **ESP32 not connecting**: Check WiFi credentials and network
2. **No data in dashboard**: Verify backend is running and accessible
3. **False positives**: Adjust thresholds in ESP32 code
4. **Database errors**: Check SQLite file permissions

### Debug Mode
Enable Serial output on ESP32 for debugging:
```cpp
Serial.begin(115200);
```

## 📄 License

This project is licensed under the MIT License.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review the ESP32 Serial output
3. Check backend logs
4. Create an issue with detailed information 
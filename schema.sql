CREATE DATABASE leak_detection;
USE leak_detection;
CREATE TABLE sensor_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  sensor1 INT NOT NULL,
  sensor2 INT NOT NULL,
  sensor3 INT NOT NULL,
  leak_confirmed BOOLEAN DEFAULT FALSE,
  burst_confirmed BOOLEAN DEFAULT FALSE,
  leak_location VARCHAR(255),
  confidence DECIMAL(5,2) DEFAULT 0.00,
  correlation_score INT DEFAULT 0,
  stability_score INT DEFAULT 0,
  environmental_noise BOOLEAN DEFAULT FALSE,
  active_sensors INT DEFAULT 0,
  timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
); 
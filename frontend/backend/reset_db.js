const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const dbPath = path.join(__dirname, 'data.db');

console.log('🗑️  Resetting database...');

// Delete the existing database file
const fs = require('fs');
if (fs.existsSync(dbPath)) {
  fs.unlinkSync(dbPath);
  console.log('✅ Old database file deleted');
} else {
  console.log('ℹ️  No existing database file found');
}

// Create new database with correct schema
const db = new sqlite3.Database(dbPath);

db.serialize(() => {
  db.run(`
    CREATE TABLE sensor_data (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      sensor1 INTEGER NOT NULL,
      sensor2 INTEGER NOT NULL,
      sensor3 INTEGER NOT NULL,
      leak_confirmed INTEGER DEFAULT 0,
      burst_confirmed INTEGER DEFAULT 0,
      leak_location TEXT,
      confidence REAL DEFAULT 0,
      correlation_score INTEGER DEFAULT 0,
      stability_score INTEGER DEFAULT 0,
      environmental_noise INTEGER DEFAULT 0,
      active_sensors INTEGER DEFAULT 0,
      timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `, (err) => {
    if (err) {
      console.error('❌ Error creating table:', err);
    } else {
      console.log('✅ New sensor_data table created successfully');
      
      // Insert some sample data
      db.run(`
        INSERT INTO sensor_data (
          sensor1, sensor2, sensor3, 
          leak_confirmed, burst_confirmed, 
          leak_location, confidence, 
          correlation_score, stability_score, 
          environmental_noise, active_sensors
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
      `, [
        150, 180, 120,
        0, 0,
        'No leak detected',
        85.5,
        92,
        88,
        0,
        0
      ], (err) => {
        if (err) {
          console.error('❌ Error inserting sample data:', err);
        } else {
          console.log('✅ Sample data inserted successfully');
        }
        
        db.close((err) => {
          if (err) {
            console.error('❌ Error closing database:', err);
          } else {
            console.log('✅ Database reset completed successfully!');
            console.log('🚀 You can now start the server with: node server.js');
          }
        });
      });
    }
  });
}); 
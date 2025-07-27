const express = require('express');
const cors = require('cors');
const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const app = express();
const PORT = 5000;

// Middleware
app.use(cors());
app.use(express.json());

// SQLite DB setup
const dbPath = path.join(__dirname, 'data.db');
const db = new sqlite3.Database(dbPath);

db.serialize(() => {
  // Check if table exists and has the correct schema
  db.get("PRAGMA table_info(sensor_data)", (err, rows) => {
    if (err) {
      console.log('Table does not exist, creating new table...');
      createNewTable();
    } else {
      // Check if table has the correct schema with all required columns
      db.all("PRAGMA table_info(sensor_data)", (err, columns) => {
        if (err) {
          console.log('Error checking table schema:', err);
          return;
        }
        
        const hasOldSchema = columns.some(col => col.name === 'value') && 
                           !columns.some(col => col.name === 'sensor1');
        
        const hasBurstDismissed = columns.some(col => col.name === 'burst_dismissed');
        
        if (hasOldSchema) {
          console.log('Detected old schema, migrating to new 3-sensor schema...');
          migrateTable();
        } else if (!hasBurstDismissed) {
          console.log('Table missing burst_dismissed column, adding it...');
          addBurstDismissedColumn();
        } else if (columns.length === 0) {
          console.log('Table is empty, creating new table...');
          createNewTable();
        } else {
          console.log('Table already has correct schema, continuing...');
        }
      });
    }
  });
});

function createNewTable() {
  db.run(`
    CREATE TABLE IF NOT EXISTS sensor_data (
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
      burst_type TEXT DEFAULT 'NORMAL FLOW',
      burst_intensity REAL DEFAULT 0,
      burst_dismissed INTEGER DEFAULT 0,
      timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `, (err) => {
    if (err) {
      console.error('Error creating table:', err);
    } else {
      console.log('✅ New sensor_data table created successfully');
    }
  });
}

function migrateTable() {
  // Drop the old table and create new one
  db.run('DROP TABLE IF EXISTS sensor_data', (err) => {
    if (err) {
      console.error('Error dropping old table:', err);
      return;
    }
    console.log('Old table dropped, creating new table...');
    createNewTable();
  });
}

function addBurstDismissedColumn() {
  db.run('ALTER TABLE sensor_data ADD COLUMN burst_dismissed INTEGER DEFAULT 0', (err) => {
    if (err) {
      console.error('Error adding burst_dismissed column:', err);
    } else {
      console.log('✅ burst_dismissed column added successfully');
    }
  });
}

// POST /api/data - receive sensor data from ESP32
app.post('/api/data', (req, res) => {
  const { 
    sensor1, sensor2, sensor3, 
    leak_confirmed, burst_confirmed, 
    leak_location, confidence, 
    correlation_score, stability_score, 
    environmental_noise, active_sensors,
    burst_type, burst_intensity, burst_dismissed
  } = req.body;
  
  // Validate required fields
  if (typeof sensor1 !== 'number' || typeof sensor2 !== 'number' || typeof sensor3 !== 'number') {
    return res.status(400).json({ error: 'Invalid sensor values' });
  }
  
  db.run(
    `INSERT INTO sensor_data (
      sensor1, sensor2, sensor3, 
      leak_confirmed, burst_confirmed, 
      leak_location, confidence, 
      correlation_score, stability_score, 
      environmental_noise, active_sensors,
      burst_type, burst_intensity, burst_dismissed
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
    [
      sensor1, sensor2, sensor3,
      leak_confirmed || 0, burst_confirmed || 0,
      leak_location || 'Unknown',
      confidence || 0,
      correlation_score || 0,
      stability_score || 0,
      environmental_noise || 0,
      active_sensors || 0,
      burst_type || 'NORMAL FLOW',
      burst_intensity || 0,
      burst_dismissed || 0
    ],
    function (err) {
      if (err) {
        console.error('DB Insert Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      res.json({ success: true, id: this.lastID });
    }
  );
});

// GET /api/data - get last 10 readings
app.get('/api/data', (req, res) => {
  db.all(
    'SELECT * FROM sensor_data ORDER BY id DESC LIMIT 10',
    [],
    (err, rows) => {
      if (err) {
        console.error('DB Query Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      // Return in chronological order
      res.json(rows.reverse());
    }
  );
});

// GET /api/status - get latest sensor data and status
app.get('/api/status', (req, res) => {
  db.get(
    'SELECT * FROM sensor_data ORDER BY id DESC LIMIT 1',
    [],
    (err, row) => {
      if (err) {
        console.error('DB Query Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      if (!row) {
        return res.json({ 
          status: 'No Data', 
          sensor1: null, 
          sensor2: null, 
          sensor3: null,
          leak_confirmed: false,
          burst_confirmed: false,
          leak_location: null,
          confidence: 0,
          correlation_score: 0,
          stability_score: 0,
          environmental_noise: false,
          active_sensors: 0,
          burst_type: 'NORMAL FLOW',
          burst_intensity: 0,
          timestamp: null 
        });
      }
      
      // Convert timestamp to ISO 8601 (UTC)
      let isoTimestamp = null;
      if (row.timestamp) {
        isoTimestamp = new Date(row.timestamp + 'Z').toISOString();
      }
      
      res.json({
        status: row.leak_confirmed ? (row.burst_confirmed ? 'Burst' : 'Leak') : 'Normal',
        sensor1: row.sensor1,
        sensor2: row.sensor2,
        sensor3: row.sensor3,
        leak_confirmed: Boolean(row.leak_confirmed),
        burst_confirmed: Boolean(row.burst_confirmed),
        leak_location: row.leak_location,
        confidence: row.confidence,
        correlation_score: row.correlation_score,
        stability_score: row.stability_score,
        environmental_noise: Boolean(row.environmental_noise),
        active_sensors: row.active_sensors,
        burst_type: row.burst_type || 'NORMAL FLOW',
        burst_intensity: row.burst_intensity || 0,
        burst_dismissed: Boolean(row.burst_dismissed),
        timestamp: isoTimestamp
      });
    }
  );
});

// GET /api/history - get last 20 sensor readings for chart
app.get('/api/history', (req, res) => {
  db.all(
    'SELECT sensor1, sensor2, sensor3, leak_confirmed, burst_confirmed, leak_location, confidence, burst_type, burst_intensity, timestamp FROM sensor_data ORDER BY id DESC LIMIT 20',
    [],
    (err, rows) => {
      if (err) {
        console.error('DB Query Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      // Convert all timestamps to ISO 8601 (UTC)
      const result = rows.reverse().map(item => ({
        ...item,
        leak_confirmed: Boolean(item.leak_confirmed),
        burst_confirmed: Boolean(item.burst_confirmed),
        burst_type: item.burst_type || 'NORMAL FLOW',
        burst_intensity: item.burst_intensity || 0,
        timestamp: item.timestamp ? new Date(item.timestamp + 'Z').toISOString() : null
      }));
      res.json(result);
    }
  );
});

// GET /api/sensors - get individual sensor data
app.get('/api/sensors', (req, res) => {
  db.all(
    'SELECT sensor1, sensor2, sensor3, timestamp FROM sensor_data ORDER BY id DESC LIMIT 50',
    [],
    (err, rows) => {
      if (err) {
        console.error('DB Query Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      // Convert all timestamps to ISO 8601 (UTC)
      const result = rows.reverse().map(item => ({
        ...item,
        timestamp: item.timestamp ? new Date(item.timestamp + 'Z').toISOString() : null
      }));
      res.json(result);
    }
  );
});

// POST /api/dismiss - dismiss burst alert
app.post('/api/dismiss', (req, res) => {
  db.run(
    'UPDATE sensor_data SET burst_dismissed = 1 WHERE id = (SELECT MAX(id) FROM sensor_data)',
    [],
    function (err) {
      if (err) {
        console.error('DB Update Error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      console.log('✅ Burst alert dismissed');
      res.json({ success: true, dismissed: true });
    }
  );
});

// Serve frontend
app.use(express.static(path.join(__dirname, '../frontend')));

app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
}); 
// Underground Water Leak Detection Backend
require('dotenv').config();
  console.log('DB_USER:', process.env.DB_USER);
const express = require('express');
const cors = require('cors');
const mysql = require('mysql2/promise');

const app = express();
const PORT = 5000;

// Middleware
app.use(cors());
app.use(express.json());

// MySQL connection pool
const pool = mysql.createPool({
  host: process.env.DB_HOST,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

// POST /api/update - Receive sensor data
app.post('/api/update', async (req, res) => {
  const { status, sensor_value } = req.body;
  if (!status || typeof sensor_value !== 'number') {
    return res.status(400).json({ error: 'Invalid payload' });
  }
  try {
    const [result] = await pool.execute(
      'INSERT INTO sensor_data (status, sensor_value) VALUES (?, ?)',
      [status, sensor_value]
    );
    res.json({ success: true, id: result.insertId });
  } catch (err) {
    console.error('DB Insert Error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// GET /api/status - Latest sensor status
app.get('/api/status', async (req, res) => {
  try {
    const [rows] = await pool.execute(
      'SELECT status, sensor_value, timestamp FROM sensor_data ORDER BY id DESC LIMIT 1'
    );
    if (rows.length === 0) {
      return res.json({ status: 'No Data', sensor_value: null, timestamp: null });
    }
    res.json(rows[0]);
  } catch (err) {
    console.error('DB Query Error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// GET /api/history - Last 20 sensor values
app.get('/api/history', async (req, res) => {
  try {
    const [rows] = await pool.execute(
      'SELECT sensor_value, timestamp FROM sensor_data ORDER BY id DESC LIMIT 20'
    );
    // Return in chronological order for charting
    res.json(rows.reverse());
  } catch (err) {
    console.error('DB Query Error:', err);
    res.status(500).json({ error: 'Database error' });
  }
});

// Start server
app.listen(PORT, () => {
  console.log(`Backend server running on http://localhost:${PORT}`);
}); 
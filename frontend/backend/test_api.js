const fetch = require('node-fetch');

const API_BASE = 'http://localhost:5000/api';

async function testAPI() {
  console.log('🧪 Testing 3-Sensor Leak Detection API...\n');

  // Test 1: POST sensor data
  console.log('1. Testing POST /api/data...');
  try {
    const testData = {
      sensor1: 150,
      sensor2: 180,
      sensor3: 120,
      leak_confirmed: 0,
      burst_confirmed: 0,
      leak_location: "No leak detected",
      confidence: 85.5,
      correlation_score: 92,
      stability_score: 88,
      environmental_noise: 0,
      active_sensors: 0
    };

    const response = await fetch(`${API_BASE}/data`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(testData)
    });

    const result = await response.json();
    console.log('✅ POST successful:', result);
  } catch (error) {
    console.log('❌ POST failed:', error.message);
  }

  // Test 2: GET status
  console.log('\n2. Testing GET /api/status...');
  try {
    const response = await fetch(`${API_BASE}/status`);
    const status = await response.json();
    console.log('✅ Status retrieved:', {
      status: status.status,
      sensor1: status.sensor1,
      sensor2: status.sensor2,
      sensor3: status.sensor3,
      leak_confirmed: status.leak_confirmed,
      leak_location: status.leak_location,
      confidence: status.confidence
    });
  } catch (error) {
    console.log('❌ Status failed:', error.message);
  }

  // Test 3: GET history
  console.log('\n3. Testing GET /api/history...');
  try {
    const response = await fetch(`${API_BASE}/history`);
    const history = await response.json();
    console.log(`✅ History retrieved: ${history.length} records`);
    if (history.length > 0) {
      console.log('Latest record:', {
        sensor1: history[history.length - 1].sensor1,
        sensor2: history[history.length - 1].sensor2,
        sensor3: history[history.length - 1].sensor3,
        leak_confirmed: history[history.length - 1].leak_confirmed
      });
    }
  } catch (error) {
    console.log('❌ History failed:', error.message);
  }

  // Test 4: POST leak detection data
  console.log('\n4. Testing POST leak detection data...');
  try {
    const leakData = {
      sensor1: 450,
      sensor2: 520,
      sensor3: 380,
      leak_confirmed: 1,
      burst_confirmed: 0,
      leak_location: "Between Sensor 1 and Sensor 2",
      confidence: 92.5,
      correlation_score: 95,
      stability_score: 90,
      environmental_noise: 0,
      active_sensors: 2
    };

    const response = await fetch(`${API_BASE}/data`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(leakData)
    });

    const result = await response.json();
    console.log('✅ Leak data posted:', result);
  } catch (error) {
    console.log('❌ Leak data failed:', error.message);
  }

  console.log('\n🎉 API testing completed!');
}

// Run the test
testAPI().catch(console.error); 
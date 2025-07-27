import React, { useEffect, useState, useRef } from 'react';
import { Line } from 'react-chartjs-2';
import { useNavigate } from 'react-router-dom';
import 'chart.js/auto';
import './App.css';

const API_BASE = 'http://localhost:5000/api';

function App() {
  const navigate = useNavigate();
  const [sensorData, setSensorData] = useState({
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
    burst_intensity: 0
  });
  const [history, setHistory] = useState([]);
  const [lastUpdated, setLastUpdated] = useState(null);
  const [burstBanner, setBurstBanner] = useState(false);
  const [frozenBurstData, setFrozenBurstData] = useState(null);
  const prevBurstRef = useRef(false);
  const firstFetchRef = useRef(true);
  const intervalRef = useRef();

  // Fetch latest sensor data and history
  const fetchData = async () => {
    try {
      const statusRes = await fetch(`${API_BASE}/status`);
      const statusData = await statusRes.json();
      setSensorData(statusData);
      setLastUpdated(statusData.timestamp);

      const historyRes = await fetch(`${API_BASE}/history`);
      const historyData = await historyRes.json();
      setHistory(historyData);

      // Show banner only when there's an actual burst (not NORMAL FLOW)
      const hasActualBurst = statusData.burst_confirmed && statusData.burst_type !== 'NORMAL FLOW';
      
      if (firstFetchRef.current) {
        if (hasActualBurst) {
          setBurstBanner(true);
          setFrozenBurstData({
            burst_type: statusData.burst_type,
            leak_location: statusData.leak_location,
            confidence: statusData.confidence,
            burst_intensity: statusData.burst_intensity
          });
        }
        firstFetchRef.current = false;
      } else {
        // Show banner only on rising edge (false -> true) for actual bursts
        if (!prevBurstRef.current && hasActualBurst) {
          setBurstBanner(true);
          setFrozenBurstData({
            burst_type: statusData.burst_type,
            leak_location: statusData.leak_location,
            confidence: statusData.confidence,
            burst_intensity: statusData.burst_intensity
          });
        } else if (burstBanner && hasActualBurst) {
          // Allow escalation: burst -> catastrophic, but never de-escalation
          const currentType = frozenBurstData?.burst_type;
          const newType = statusData.burst_type;
          
          if (currentType === 'PIPELINE BURST' && newType === 'CATASTROPHIC BURST') {
            setFrozenBurstData({
              burst_type: statusData.burst_type,
              leak_location: statusData.leak_location,
              confidence: statusData.confidence,
              burst_intensity: statusData.burst_intensity
            });
          }
          // If current is catastrophic, don't change it even if new data shows burst
        }
      }
      prevBurstRef.current = hasActualBurst;
    } catch (err) {
      console.error('Error fetching data:', err);
      setSensorData({
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
        active_sensors: 0
      });
    }
  };

  useEffect(() => {
    fetchData();
    intervalRef.current = setInterval(fetchData, 2000);
    return () => clearInterval(intervalRef.current);
  }, []);

  // Prepare data for Chart.js - show all 3 sensors
  const filteredHistory = history.filter(item => item.sensor1 >= 0 && item.sensor2 >= 0 && item.sensor3 >= 0);
  const chartData = {
    labels: filteredHistory.map(item => new Date(item.timestamp).toLocaleTimeString()),
    datasets: [
      {
        label: 'Sensor 1',
        data: filteredHistory.map(item => item.sensor1),
        fill: false,
        borderColor: '#00fff7',
        backgroundColor: 'rgba(0,255,247,0.1)',
        tension: 0.4,
        pointRadius: 2,
      },
      {
        label: 'Sensor 2',
        data: filteredHistory.map(item => item.sensor2),
        fill: false,
        borderColor: '#ffd700',
        backgroundColor: 'rgba(255,215,0,0.1)',
        tension: 0.4,
        pointRadius: 2,
      },
      {
        label: 'Sensor 3',
        data: filteredHistory.map(item => item.sensor3),
        fill: false,
        borderColor: '#ff8c00',
        backgroundColor: 'rgba(255,140,0,0.1)',
        tension: 0.4,
        pointRadius: 2,
      },
    ],
  };

  const chartOptions = {
    responsive: true,
    plugins: {
      legend: { 
        display: true,
        labels: {
          color: '#00fff7',
          font: {
            size: 12
          }
        }
      },
    },
    scales: {
      x: {
        title: { display: true, text: 'Time', color: '#00fff7' },
        ticks: { color: '#b0eaff' },
        grid: { color: 'rgba(0,255,247,0.1)' },
      },
      y: {
        title: { display: true, text: 'Sensor Values', color: '#00fff7' },
        ticks: { color: '#b0eaff' },
        grid: { color: 'rgba(0,255,247,0.1)' },
      },
    },
  };

  const leakDetected = sensorData.leak_confirmed;
  const burstDetected = sensorData.burst_confirmed;
  const catastrophicDetected = sensorData.burst_type === 'CATASTROPHIC BURST';

  const handleDismissBurst = async () => {
    try {
      const response = await fetch(`${API_BASE}/dismiss`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
      });
      
      if (response.ok) {
        console.log('✅ Burst alert dismissed successfully');
        setBurstBanner(false);
        setFrozenBurstData(null);
      } else {
        console.error('Failed to dismiss burst alert');
      }
    } catch (error) {
      console.error('Error dismissing burst alert:', error);
    }
  };

  // Calculate average sensor value
  const avgSensorValue = sensorData.sensor1 !== null && sensorData.sensor2 !== null && sensorData.sensor3 !== null 
    ? Math.round((sensorData.sensor1 + sensorData.sensor2 + sensorData.sensor3) / 3)
    : null;

  // Get status color based on burst type
  const getStatusColor = () => {
    if (catastrophicDetected) return 'status-catastrophic';
    if (burstDetected) return 'status-alert';
    if (leakDetected) return 'status-warning';
    return 'status-normal';
  };

  // Get status text based on burst type
  const getStatusText = () => {
    if (catastrophicDetected) return '�� CATASTROPHIC BURST DETECTED! 🚨';
    if (burstDetected) return '⚠️ PIPELINE BURST';
    if (leakDetected) return '💧 PIPELINE LEAK';
    return '✅ NORMAL FLOW';
  };

  // Only show the banner if burstBanner is true (do not depend on burstDetected)
  return (
    <div className="dashboard-bg">
      {burstBanner && frozenBurstData && (
        <div className="burst-banner-overlay">
          <div className={`burst-banner ${frozenBurstData.burst_type === 'CATASTROPHIC BURST' ? 'catastrophic-banner' : ''}`}>
            <h2>{frozenBurstData.burst_type === 'CATASTROPHIC BURST' ? '🚨 CATASTROPHIC BURST DETECTED! 🚨' : '🚨 BURST DETECTED! 🚨'}</h2>
            <p><strong>Type:</strong> {frozenBurstData.burst_type}</p>
            <p><strong>Location:</strong> {frozenBurstData.leak_location || 'Unknown'}</p>
            <p><strong>Confidence:</strong> {frozenBurstData.confidence.toFixed(1)}%</p>
            <p><strong>Intensity:</strong> {frozenBurstData.burst_intensity.toFixed(1)}</p>
            <button className="burst-banner-dismiss" onClick={handleDismissBurst}>Dismiss</button>
          </div>
        </div>
      )}
      <header className="dashboard-header">
        <h1 className="dashboard-heading">Municipal Pipeline Burst Detection System</h1>
      </header>
      <div className="dashboard-ludo-grid">
        {/* Box 1: Individual Sensor Values */}
        <div className="ludo-card sensor-values-card">
          <h2 className="sensor-title">Pipeline Sensor Values</h2>
          <div className="sensor-grid">
            <div className="sensor-item">
              <div className="sensor-label">Sensor 1</div>
              <div className="sensor-value neon-cyan">{sensorData.sensor1 !== null ? sensorData.sensor1 : '--'}</div>
            </div>
            <div className="sensor-item">
              <div className="sensor-label">Sensor 2</div>
              <div className="sensor-value neon-yellow">{sensorData.sensor2 !== null ? sensorData.sensor2 : '--'}</div>
            </div>
            <div className="sensor-item">
              <div className="sensor-label">Sensor 3</div>
              <div className="sensor-value neon-orange">{sensorData.sensor3 !== null ? sensorData.sensor3 : '--'}</div>
            </div>
          </div>
          <div className="avg-sensor">
            <div className="avg-label">Average</div>
            <div className="avg-value neon-yellow">{avgSensorValue !== null ? avgSensorValue : '--'}</div>
          </div>
          {lastUpdated && (
            <div className="last-updated">Last updated: {new Date(lastUpdated).toLocaleString(undefined, { hour12: false })}</div>
          )}
        </div>

        {/* Box 2: Multi-Sensor Graph */}
        <div className="ludo-card graph-card" style={{ cursor: 'pointer' }} onClick={() => navigate('/analytics')}>
          <h2 className="chart-title">Pipeline Vibration Monitoring (Last 20)</h2>
          <Line data={chartData} options={chartOptions} />
        </div>

        {/* Box 3: Pipeline Status */}
        <div className={`ludo-card leak-detection-card ${getStatusColor()}`}>
          <h2 className="leak-title">Pipeline Status</h2>
          <div className={`leak-status ${getStatusColor()}`}>
            {getStatusText()}
          </div>
          
          <div className="leak-details">
            <div className="detail-item">
              <span className="detail-label">Current Status:</span>
              <span className="detail-value">{sensorData.burst_type}</span>
            </div>
            <div className="detail-item">
              <span className="detail-label">Active Sensors:</span>
              <span className="detail-value">{sensorData.active_sensors}/3</span>
            </div>
            <div className="detail-item">
              <span className="detail-label">Signal Quality:</span>
              <span className="detail-value">{sensorData.correlation_score > 70 ? 'Good' : 'Poor'}</span>
            </div>
            <div className="detail-item">
              <span className="detail-label">Environmental:</span>
              <span className="detail-value">{sensorData.environmental_noise ? 'Noise Detected' : 'Clean'}</span>
            </div>
          </div>
        </div>

        {/* Box 4: System Status */}
        <div className="ludo-card system-status-card">
          <h2 className="system-title">Municipal System Status</h2>
          <div className="status-grid">
            <div className="status-item">
              <div className="status-label">Pipeline Status</div>
              <div className={`status-value ${getStatusColor()}`}>
                {catastrophicDetected ? 'CATASTROPHIC' : burstDetected ? 'BURST' : leakDetected ? 'LEAK' : 'NORMAL'}
              </div>
            </div>
            <div className="status-item">
              <div className="status-label">Signal Quality</div>
              <div className={`status-value ${sensorData.correlation_score > 70 ? 'status-good' : 'status-warning'}`}>
                {sensorData.correlation_score > 70 ? 'GOOD' : 'POOR'}
              </div>
            </div>
            <div className="status-item">
              <div className="status-label">Environmental</div>
              <div className={`status-value ${sensorData.environmental_noise ? 'status-warning' : 'status-good'}`}>
                {sensorData.environmental_noise ? 'NOISE' : 'CLEAN'}
              </div>
            </div>
            <div className="status-item">
              <div className="status-label">Burst Intensity</div>
              <div className={`status-value ${sensorData.burst_intensity > 200 ? 'status-catastrophic' : sensorData.burst_intensity > 120 ? 'status-alert' : 'status-normal'}`}>
                {sensorData.burst_intensity > 200 ? 'HIGH' : sensorData.burst_intensity > 120 ? 'MEDIUM' : 'LOW'}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App; 
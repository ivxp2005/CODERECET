import React, { useState, useEffect, useRef } from 'react';
import { Line, Doughnut, Bar } from 'react-chartjs-2';
import { useNavigate } from 'react-router-dom';
import 'chart.js/auto';
import './App.css';

const API_BASE = 'http://localhost:5000/api';

export default function Analytics() {
  const navigate = useNavigate();
  const [range, setRange] = useState('24h');
  const [sensorData, setSensorData] = useState({
    sensor1: null,
    sensor2: null,
    sensor3: null,
    leak_confirmed: false,
    burst_confirmed: false,
    leak_location: null,
    confidence: 0,
    burst_type: 'NORMAL FLOW',
    burst_intensity: 0
  });
  const [history, setHistory] = useState([]);
  const [lastUpdated, setLastUpdated] = useState(null);
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
    } catch (err) {
      console.error('Error fetching analytics data:', err);
    }
  };

  useEffect(() => {
    fetchData();
    intervalRef.current = setInterval(fetchData, 2000);
    return () => clearInterval(intervalRef.current);
  }, []);

  // Calculate statistics from real data
  const calculateStats = () => {
    if (history.length === 0) return { avg: 0, peak: 0, alertRate: 0, total: 0 };
    
    const allValues = history.flatMap(item => [item.sensor1, item.sensor2, item.sensor3]).filter(val => val >= 0);
    const avg = Math.round(allValues.reduce((sum, val) => sum + val, 0) / allValues.length);
    const peak = Math.max(...allValues);
    const alertCount = history.filter(item => item.leak_confirmed || item.burst_confirmed).length;
    const alertRate = Math.round((alertCount / history.length) * 100);
    
    return { avg, peak, alertRate, total: history.length };
  };

  const stats = calculateStats();

  // Prepare status distribution from real data
  const getStatusDistribution = () => {
    if (history.length === 0) return { labels: ['Normal'], datasets: [{ data: [1], backgroundColor: ['#00ffb3'], borderWidth: 0 }] };
    
    const normal = history.filter(item => !item.leak_confirmed && !item.burst_confirmed).length;
    const leak = history.filter(item => item.leak_confirmed && !item.burst_confirmed).length;
    const burst = history.filter(item => item.burst_confirmed).length;
    
    return {
      labels: ['Normal', 'Leak Detected', 'Major Burst'],
      datasets: [{
        data: [normal, leak, burst],
        backgroundColor: ['#00ffb3', '#ffd700', '#ff005c'],
        borderWidth: 0,
      }],
    };
  };

  // Prepare hourly averages from real data
  const getHourlyAverages = () => {
    if (history.length === 0) return { labels: [], datasets: [{ label: 'Avg Value', data: [], backgroundColor: '#00fff7', borderRadius: 8 }] };
    
    const hourlyData = {};
    history.forEach(item => {
      const hour = new Date(item.timestamp).getHours();
      if (!hourlyData[hour]) hourlyData[hour] = [];
      hourlyData[hour].push(item.sensor1, item.sensor2, item.sensor3);
    });
    
    const labels = Object.keys(hourlyData).sort((a, b) => a - b).map(hour => `${hour}:00`);
    const data = Object.keys(hourlyData).sort((a, b) => a - b).map(hour => {
      const values = hourlyData[hour].filter(val => val >= 0);
      return values.length > 0 ? Math.round(values.reduce((sum, val) => sum + val, 0) / values.length) : 0;
    });
    
    return {
      labels,
      datasets: [{
        label: 'Avg Value',
        data,
        backgroundColor: '#00fff7',
        borderRadius: 8,
      }],
    };
  };

  // Get alert history from real data
  const getAlertHistory = () => {
    return history
      .filter(item => item.leak_confirmed || item.burst_confirmed)
      .map(item => ({
        type: item.burst_confirmed ? 'Major Burst' : 'Leak Detected',
        value: Math.max(item.sensor1, item.sensor2, item.sensor3),
        time: new Date(item.timestamp).toLocaleTimeString(),
        level: item.burst_confirmed ? 'HIGH' : 'MEDIUM',
      }))
      .slice(-5); // Last 5 alerts
  };

  const lineData = {
    labels: history.slice(-100).map(item => new Date(item.timestamp).toLocaleTimeString()),
    datasets: [
      {
        label: 'Sensor 1',
        data: history.slice(-100).map(item => item.sensor1),
        borderColor: '#00fff7',
        backgroundColor: 'rgba(0,255,247,0.1)',
        pointBackgroundColor: history.slice(-100).map(item => item.burst_confirmed ? '#ff005c' : '#00fff7'),
        pointRadius: history.slice(-100).map(item => item.burst_confirmed ? 5 : 2),
        tension: 0.2,
      },
      {
        label: 'Sensor 2',
        data: history.slice(-100).map(item => item.sensor2),
        borderColor: '#ffd700',
        backgroundColor: 'rgba(255,215,0,0.1)',
        pointBackgroundColor: history.slice(-100).map(item => item.burst_confirmed ? '#ff005c' : '#ffd700'),
        pointRadius: history.slice(-100).map(item => item.burst_confirmed ? 5 : 2),
        tension: 0.2,
      },
      {
        label: 'Sensor 3',
        data: history.slice(-100).map(item => item.sensor3),
        borderColor: '#ff8c00',
        backgroundColor: 'rgba(255,140,0,0.1)',
        pointBackgroundColor: history.slice(-100).map(item => item.burst_confirmed ? '#ff005c' : '#ff8c00'),
        pointRadius: history.slice(-100).map(item => item.burst_confirmed ? 5 : 2),
        tension: 0.2,
      },
    ],
  };

  return (
    <div className="analytics-bg">
      <div className="analytics-header">
        <button className="back-btn" onClick={() => navigate('/')}>&larr; Back to Dashboard</button>
        <h1 className="analytics-title">Detailed Analytics</h1>
        <div className="analytics-actions">
          <button className="analytics-btn" onClick={fetchData}>Refresh</button>
          <button className="analytics-btn">Export</button>
        </div>
      </div>
      <div className="analytics-controls">
        <select value={range} onChange={e => setRange(e.target.value)} className="analytics-select">
          <option value="24h">Last 24 Hours</option>
          <option value="7d">Last 7 Days</option>
          <option value="30d">Last 30 Days</option>
        </select>
        <span className="analytics-meta">{stats.total} readings - Last update: {lastUpdated ? new Date(lastUpdated).toLocaleString() : 'Loading...'}</span>
      </div>
      <div className="analytics-stats-row">
        <div className="analytics-stat-card">
          <div className="stat-value">{stats.avg}</div>
          <div className="stat-label">Average Value</div>
        </div>
        <div className="analytics-stat-card">
          <div className="stat-value">{stats.peak}</div>
          <div className="stat-label">Peak Value</div>
        </div>
        <div className="analytics-stat-card">
          <div className="stat-value">{stats.alertRate}%</div>
          <div className="stat-label">Alert Rate</div>
        </div>
        <div className="analytics-stat-card">
          <div className="stat-value">{stats.total}</div>
          <div className="stat-label">Total Readings</div>
        </div>
      </div>
      <div className="analytics-main-graph">
        <div className="analytics-graph-title">Sensor Value Timeline</div>
        <Line data={lineData} options={{
          responsive: true,
          plugins: { legend: { labels: { color: '#00fff7' } } },
          scales: {
            x: { title: { display: true, text: 'Time', color: '#00fff7' }, ticks: { color: '#b0eaff' }, grid: { color: 'rgba(0,255,247,0.1)' } },
            y: { title: { display: true, text: 'Sensor Value', color: '#00fff7' }, ticks: { color: '#b0eaff' }, grid: { color: 'rgba(0,255,247,0.1)' } },
          },
        }} />
      </div>
      <div className="analytics-bottom-row">
        <div className="analytics-pie">
          <div className="analytics-graph-title">Status Distribution</div>
          <Doughnut data={getStatusDistribution()} options={{ plugins: { legend: { labels: { color: '#00fff7', font: { size: 14 } } } } }} />
        </div>
        <div className="analytics-bar">
          <div className="analytics-graph-title">Hourly Average Values</div>
          <Bar data={getHourlyAverages()} options={{
            plugins: { legend: { display: false } },
            scales: {
              x: { title: { display: true, text: 'Hour of Day', color: '#00fff7' }, ticks: { color: '#b0eaff' }, grid: { color: 'rgba(0,255,247,0.1)' } },
              y: { title: { display: true, text: 'Avg Value', color: '#00fff7' }, ticks: { color: '#b0eaff' }, grid: { color: 'rgba(0,255,247,0.1)' } },
            },
          }} />
        </div>
      </div>
      <div className="analytics-alert-history">
        <div className="analytics-graph-title">Alert History</div>
        {getAlertHistory().map((alert, i) => (
          <div className="analytics-alert-card" key={i}>
            <span className="alert-type">{alert.type}</span>
            <span className="alert-value">Value: {alert.value} - {alert.time}</span>
            <span className={`alert-level ${alert.level.toLowerCase()}`}>{alert.level}</span>
          </div>
        ))}
        {getAlertHistory().length === 0 && (
          <div className="analytics-alert-card">
            <span className="alert-type">No alerts yet</span>
            <span className="alert-value">System monitoring normally</span>
          </div>
        )}
      </div>
    </div>
  );
} 
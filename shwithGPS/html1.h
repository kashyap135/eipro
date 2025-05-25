const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>ESP32 Sensor Dashboard</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f8fafc;
      margin: 0;
      padding: 16px;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
      
    }
    h1 {
      text-align: center;
      color: #2563eb;
      margin-bottom: 24px;
    }
    .card {
      background: #fff;
      border-radius: 8px;
      padding: 16px;
      margin-bottom: 16px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .reading {
      display: flex;
      align-items: center;
      margin: 8px 0;
    }
    .label {
      flex: 1;
      color: #64748b;
    }
    .value {
      font-weight: bold;
      color: #2563eb;
      min-width: 80px;
      text-align: right;
    }
    .bar-container {
      flex: 1;
      height: 8px;
      background: #e2e8f0;
      border-radius: 4px;
      margin: 0 16px;
      overflow: hidden;
    }
    .bar {
      height: 100%;
      width: 0%;
      transition: width 0.3s, background-color 0.3s;
    }
    .normal { background: #22c55e; }
    .warning { background: #eab308; }
    .danger { background: #ef4444; }
    .fall-alert {
      display: none;
      background: #fee2e2;
      border: 1px solid #ef4444;
      color: #ef4444;
      padding: 8px;
      border-radius: 4px;
      margin-top: 8px;
      text-align: center;
      animation: blink 1s infinite;
    }
    @keyframes blink {
      50% { opacity: 0.5; }
    }
    .status { margin-left: 8px; }
    .status.normal { color: #22c55e; }
    .status.fall { color: #ef4444; }
    #map {
      height: 300px;
      border-radius: 8px;
      margin-top: 8px;
    }
  </style>
</head>
<body>
  <div class="container">
  <div style="text-align: right; margin-bottom: 12px;">
  <button onclick="resetFall()" style="
    background-color: #2563eb;
    color: white;
    padding: 8px 16px;
    border: none;
    border-radius: 6px;
    font-size: 14px;
    cursor: pointer;
  ">🔄 Reset Fall</button>
  </div>

    <h1>ESP32 Sensor Dashboard</h1>

    <div class="card">
      <h2>ADXL345 Data</h2>
      <div class="reading">
        <span class="label">X-axis (Roll)</span>
        <div class="bar-container"><div class="bar" id="roll-bar"></div></div>
        <span class="value" id="roll">...</span>
      </div>
      <div class="reading">
        <span class="label">Y-axis (Pitch)</span>
        <div class="bar-container"><div class="bar" id="pitch-bar"></div></div>
        <span class="value" id="pitch">...</span>
      </div>
      <div class="reading">
        <span class="label">Z-axis (Yaw)</span>
        <div class="bar-container"><div class="bar" id="yaw-bar"></div></div>
        <span class="value" id="yaw">...</span>
      </div>
      <div class="reading">
        <span class="label">Fall Status</span>
        <span class="value status normal" id="status">Normal</span>
      </div>
      <div class="fall-alert" id="fallAlert">⚠️ FALL DETECTED - CHECK IMMEDIATELY!</div>
    </div>

    <div class="card">
      <h2>Device Location</h2>
      <div id="map">Loading map...</div>
    </div>
  </div>

  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <script>
    function updateBar(id, value, max) {
      const bar = document.getElementById(id);
      let percent = (Math.abs(value) / max) * 100;
      bar.style.width = Math.min(100, Math.max(0, percent)) + '%';
      if (percent < 33) bar.className = 'bar normal';
      else if (percent < 66) bar.className = 'bar warning';
      else bar.className = 'bar danger';
    }

    function updateValue(id, value, unit = '') {
      const el = document.getElementById(id);
      el.textContent = (+value).toFixed(1) + unit;
    }

    function updateFallStatus(status) {
      const statusEl = document.getElementById('status');
      const alertEl = document.getElementById('fallAlert');
      if (status === 'Fall Detected') {
        statusEl.textContent = status;
        statusEl.className = 'value status fall';
        alertEl.style.display = 'block';
      } else {
        statusEl.textContent = 'Normal';
        statusEl.className = 'value status normal';
        alertEl.style.display = 'none';
      }
    }

    function fetchData() {
      fetch('/readADXL345')
        .then(r => r.json())
        .then(data => {
          updateValue('roll', data[0], '°');
          updateValue('pitch', data[1], '°');
          updateValue('yaw', data[2], '°');
          updateFallStatus(data[3]);
          updateBar('roll-bar', data[0], 180);
          updateBar('pitch-bar', data[1], 180);
          updateBar('yaw-bar', data[2], 180);
        });
    }
    function resetFall() {
      fetch('/resetFall')
        .then(res => {
        if (res.ok) {
        alert('✅ Fall status reset.');
        updateFallStatus('Normal');
      } else {
        alert('❌ Failed to reset fall status.');
      }
        });
    }


    let map = L.map('map').setView([0, 0], 2);
    let marker = L.marker([0, 0]).addTo(map);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '&copy; OpenStreetMap contributors'
    }).addTo(map);

    function updateMap(lat, lng) {
      map.setView([lat, lng], 16);
      marker.setLatLng([lat, lng]).bindPopup("ESP32 Location").openPopup();
    }

    function fetchLocation() {
      fetch('/getLocation')
        .then(res => res.json())
        .then(data => {
          updateMap(data.lat, data.lng);
        });
    }
    

    fetchData();
    fetchLocation();
    setInterval(fetchData, 2000);
    setInterval(fetchLocation, 5000);
  </script>
</body>
</html>
)rawliteral";

#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Польовик – Керування сервоприводом</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        html, body {
            margin:0; padding:0; height:100%;
            font-family: Arial, sans-serif;
            background: #000;
            color: #eee;
            overflow: hidden;
        }
        #loginScreen {
            position: fixed;
            inset: 0;
            background: rgba(0,0,0,0.9);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            z-index: 1000;
        }
        #mainContent {
            display: none;
            padding: 20px;
            max-width: 600px;
            margin: 0 auto;
        }
        h3 { margin: 10px 0; text-align: center; }
        .buttonContainer {
            display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin: 15px 0;
        }
        button {
            padding: 12px 20px;
            font-size: 18px;
            background: #4247b7;
            color: white;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            min-width: 60px;
        }
        button:hover { background: #ff494d; }
        button:active { background: #d32f2f; }
        #slider-container { margin: 20px 0; text-align: center; }
        #position-slider {
            width: 90%; height: 12px; background: linear-gradient(to right, #333, #4247b7, #333);
            border-radius: 10px; appearance: none;
        }
        #position-slider::-webkit-slider-thumb {
            width: 40px; height: 40px; background: #ff494d; border-radius: 50%;
            border: 4px solid white; cursor: pointer;
        }
        #current-pos { font-size: 20px; margin: 10px 0; text-align: center; }
        #STSValue { font-size: 16px; text-align: center; margin: 10px 0; padding: 10px; background: #111; border-radius: 8px; }
        #map { height: 300px; border: 2px solid #4247b7; border-radius: 10px; margin-top: 20px; }
        input[type="text"], input[type="password"] {
            width: 280px; padding: 12px; margin: 10px; font-size: 16px; border-radius: 6px; border: 1px solid #555; background: #222; color: white;
        }
        #loginBtn { padding: 14px 40px; font-size: 18px; background: #28a745; border: none; border-radius: 8px; color: white; cursor: pointer; }
        #loginBtn:hover { background: #218838; }
        #loginError { color: #ff4444; margin-top: 10px; min-height: 20px; }
    </style>
</head>
<body>

<div id="loginScreen">
    <h2>Польовик – Авторизація</h2>
    <input type="text" id="username" placeholder="Ім'я користувача (admin)" autocomplete="off">
    <input type="password" id="password" placeholder="Пароль" autocomplete="off">
    <button id="loginBtn" onclick="tryLogin()">Увійти</button>
    <div id="loginError"></div>
</div>

<div id="mainContent">
    <h3>Польовик</h3>
    <div class="StsValue" id="STSValue">Очікування даних сервоприводу...</div>

    <div class="buttonContainer">
        <button onmousedown="toggleCheckbox(1,6,0,0)" ontouchstart="toggleCheckbox(1,6,0,0)" onmouseup="toggleCheckbox(1,2,0,0)" ontouchend="toggleCheckbox(1,2,0,0)"><</button>
        <button onclick="toggleCheckbox(1,1,0,0)">0°</button>
        <button onmousedown="toggleCheckbox(1,5,0,0)" ontouchstart="toggleCheckbox(1,5,0,0)" onmouseup="toggleCheckbox(1,2,0,0)" ontouchend="toggleCheckbox(1,2,0,0)">></button>
        <button onclick="toggleCheckbox(1,7,0,0)">+</button>
        <button onclick="toggleCheckbox(1,8,0,0)">-</button>
    </div>

    <div id="slider-container">
        <input type="range" id="position-slider" min="0" max="4095" value="2047" step="any">
    </div>
    <input type="number" id="position-input" min="0" max="4095" value="2047" style="display:none;">
    <p id="current-pos">Поточна позиція: 0°</p>

    <div id="map"></div>
</div>

<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script>
// ── Auth & Token Logic ───────────────────────────────────────
let authToken = localStorage.getItem('servoAuthToken') || '';

// If token exists → show main content immediately
if (authToken) {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('mainContent').style.display = 'block';
}

async function tryLogin() {
    const user = document.getElementById('username').value.trim();
    const pass = document.getElementById('password').value.trim();
    const errorEl = document.getElementById('loginError');

    if (!user || !pass) {
        errorEl.textContent = 'Введіть ім\'я та пароль';
        return;
    }

    errorEl.textContent = 'Авторизація...';

    try {
        const res = await fetch('/auth', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username: user, password: pass })
        });

        const data = await res.json();

        if (data.status === 'success') {
            authToken = data.token;
            localStorage.setItem('servoAuthToken', authToken);
            document.getElementById('loginScreen').style.display = 'none';
            document.getElementById('mainContent').style.display = 'block';
            errorEl.textContent = '';
            // Optional: start data polling
            setInterval(getData, 300);
        } else {
            errorEl.textContent = 'Неправильний логін або пароль';
        }
    } catch (err) {
        errorEl.textContent = 'Помилка з\'єднання з пристроєм';
        console.error(err);
    }
}

// ── Command function with token ──────────────────────────────
function toggleCheckbox(inputT, inputI, inputA, inputB) {
    if (!authToken) {
        alert('Спочатку авторизуйтесь');
        return;
    }

    const url = `cmd?inputT=${inputT}&inputI=${inputI}&inputA=${inputA}&inputB=${inputB}&token=${encodeURIComponent(authToken)}`;

    fetch(url)
        .then(() => setTimeout(getData, 150))
        .catch(err => console.error('Command failed:', err));
}

// ── Status polling ───────────────────────────────────────────
function getData() {
    if (!authToken) return;

    fetch(`readSTS?token=${encodeURIComponent(authToken)}`)
        .then(res => res.text())
        .then(text => {
            document.getElementById("STSValue").innerHTML = text;

            const posMatch = text.match(/Raw:\s*([-]?\d+)/);
            if (posMatch) {
                const rawPos = parseInt(posMatch[1]);
                const deg = Math.round((rawPos - 2047) * 360 / 4095);
                document.getElementById("current-pos").textContent = `Поточна позиція: ${deg}°`;
                updateMapDirection(deg);

                // Sync slider if not dragging
                if (!isDragging) {
                    document.getElementById("position-slider").value = rawPos;
                    document.getElementById("position-input").value = rawPos;
                }
            }
        })
        .catch(err => console.error('Status fetch failed:', err));
}

setInterval(getData, 1000); // Poll every 1s

// ── Slider logic (with token) ────────────────────────────────
let currentPos = 2047;
let isDragging = false;
let lastSendTime = 0;
const throttleTime = 80;

const slider = document.getElementById('position-slider');
slider.addEventListener('input', function() {
    const now = Date.now();
    if (now - lastSendTime > throttleTime) {
        const newPos = parseInt(this.value);
        toggleCheckbox(1, 22, newPos, 0);
        lastSendTime = now;
    }
});

// ── Map init ─────────────────────────────────────────────────
let map, marker, arrow;

function initMap() {
    map = L.map('map').setView([48.904339, 37.605560], 10);
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; OpenStreetMap contributors'
    }).addTo(map);

    marker = L.marker([48.904339, 37.605560]).addTo(map)
        .bindPopup('Польовик').openPopup();

    arrow = L.polyline([], {color: 'red', weight: 8, opacity: 0.8}).addTo(map);
    updateMapDirection(0);
}

function updateMapDirection(deg) {
    if (!map || !marker) return;
    const center = marker.getLatLng();
    const distance = 0.18;
    const bearing = deg;
    const rad = bearing * Math.PI / 180;
    const dLat = distance * Math.cos(rad);
    const dLng = distance * Math.sin(rad) / Math.cos(center.lat * Math.PI / 180);
    const endLat = center.lat + dLat;
    const endLng = center.lng + dLng;
    arrow.setLatLngs([[center.lat, center.lng], [endLat, endLng]]);
}

window.onload = function() {
    document.getElementById("position-slider").value = 2047;
    document.getElementById("position-input").value = 2047;
    document.getElementById("current-pos").textContent = 'Поточна позиція: 0°';
    initMap();

    // If already logged in → start polling
    if (authToken) getData();
};
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
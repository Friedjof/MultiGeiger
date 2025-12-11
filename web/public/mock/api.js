const BASE_STATUS = {
  counts: 1500,
  cpm: 40.0,
  dose_uSvh: 0.116,
  hv_error: false,
  temperature: 22.8,
  humidity: 48.2,
  pressure: 1014.2,
  sensor: 'BME280',
  version: 'v1.0.0-mock',
  wifi_connected: true,
  wifi_mode: 'STA', // 'AP' or 'STA'
  wifi_ssid: 'MyNetwork',
  wifi_ip: '192.168.1.42',
  wifi_rssi: -65,
};

const DEFAULT_CONFIG = {
  thingName: 'ESP32-MultiGeiger',
  apPassword: 'multigeiger',
  wifiSsid: 'MyNetwork',
  wifiPassword: '',
  speakerTick: false,
  ledTick: true,
  showDisplay: true,
  sendToBle: false,
  sendToMqtt: false,
  mqttHost: 'mqtt.local',
  mqttPort: 1883,
  mqttUseTls: false,
  mqttRetain: false,
  mqttUsername: '',
  mqttPassword: '',
  mqttBaseTopic: 'multigeiger',
  hasLora: false,
  sendToLora: false,
  devaddr: '',
  nwkskey: '',
  appskey: '',
  soundLocalAlarm: false,
  localAlarmThreshold: 0.5,
  localAlarmFactor: 10,
  httpAuthUser: 'admin',
  httpAuthPassword: '',
};

const POLL_STEP_SECONDS = 2;
const DEFAULT_AUTH = {
  username: 'admin',
  password: 'admin',
};

function sleep(ms = 250) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function loadFromStorage(key, fallback) {
  try {
    const raw = localStorage.getItem(key);
    if (!raw) return fallback;
    return JSON.parse(raw);
  } catch (_) {
    return fallback;
  }
}

function saveToStorage(key, value) {
  try {
    localStorage.setItem(key, JSON.stringify(value));
  } catch (_) {
    /* ignore */
  }
}

export class MockAPI {
  constructor() {
    this.status = { ...BASE_STATUS };
    this.startedAt = Date.now();
    this.baseCounts = this.status.counts;
    this.cps = (this.status.cpm || 0) / 60;
    this.config = loadFromStorage('multigeiger:config', { ...DEFAULT_CONFIG });
    this.auth = { ...DEFAULT_AUTH };
    this.loggedIn = false;
  }

  unauthorized(message = 'Unauthorized') {
    const err = new Error(message);
    err.status = 401;
    throw err;
  }

  requireAuth() {
    if (!this.loggedIn) {
      this.unauthorized();
    }
  }

  async login(username, password) {
    await sleep(100);
    if (username === this.auth.username && password === this.auth.password) {
      this.loggedIn = true;
      return { status: 'ok' };
    }
    this.unauthorized('Invalid credentials');
  }

  async getStatus() {
    const elapsed = Math.max(0, Math.floor((Date.now() - this.startedAt) / 1000));
    const counts = this.baseCounts + Math.round(this.cps * elapsed);
    const cpm = this.cps * 60;
    const dose = cpm * 0.0029;
    await sleep();

    // Mock timestamps for last send times (3 minutes ago)
    const recentTimestamp = Date.now() - 180000;

    // Determine WiFi mode - use AP mode if no client SSID configured
    const wifiMode = this.config.wifiSsid ? 'STA' : 'AP';
    const isAPMode = wifiMode === 'AP';

    return {
      ...this.status,
      counts,
      cpm: Number(cpm.toFixed(1)),
      dose_uSvh: Number(dose.toFixed(3)),
      uptime_s: elapsed,
      wifi_mode: wifiMode,
      wifi_ssid: isAPMode ? this.config.thingName : this.config.wifiSsid,
      mqtt_enabled: this.config.sendToMqtt,
      mqtt_connected: this.config.sendToMqtt && !isAPMode,
      mqtt_last_publish: (this.config.sendToMqtt && !isAPMode) ? recentTimestamp : null,
      lora_enabled: this.config.sendToLora,
      lora_last_send: this.config.sendToLora ? recentTimestamp : null,
      ble_enabled: this.config.sendToBle,
      ble_connections: this.config.sendToBle ? 0 : 0,
    };
  }

  async getTime() {
    await sleep(50);
    return {
      epoch_ms: Date.now(),
      tz_offset_min: 0,
    };
  }

  async setTime(data) {
    await sleep(50);
    // In mock, we don't actually set time, just acknowledge
    console.log('Mock API: Time set request received', data);
    return { status: 'ok', epoch_ms: data.epoch_ms };
  }

  async getConfig() {
    this.requireAuth();
    await sleep();
    return { ...this.config, httpAuthPassword: '' };
  }

  async saveConfig(data) {
    this.requireAuth();
    const next = { ...data };
    if (!next.httpAuthPassword) {
      delete next.httpAuthPassword;
    } else {
      this.auth.password = next.httpAuthPassword;
      next.httpAuthPassword = '';
    }
    if (next.httpAuthUser) {
      this.auth.username = next.httpAuthUser;
    }

    this.config = { ...this.config, ...next };
    saveToStorage('multigeiger:config', this.config);
    await sleep();
    return { status: 'ok' };
  }

  async ping() {
    this.requireAuth();
    await sleep(100);
    return { status: 'ok' };
  }

  async uploadFirmware() {
    this.requireAuth();
    await sleep(600);
    return { status: 'ok' };
  }
}

window.MockAPI = MockAPI;
window.mockAPI = new MockAPI();

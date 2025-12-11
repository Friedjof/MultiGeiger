const DEFAULT_STATUS = {
  counts: 1200,
  cpm: 42.5,
  dose_uSvh: 0.12,
  uptime_s: 3600,
  hv_error: false,
  temperature: 22.8,
  humidity: 48.2,
  pressure: 1014.2,
  has_thp: true,
  version: 'v1.0.0-mock',
};

const DEFAULT_CONFIG = {
  thingName: 'ESP32-MultiGeiger',
  apPassword: 'multigeiger',
  wifiSsid: 'MyNetwork',
  wifiPassword: '',
  startSound: true,
  speakerTick: false,
  ledTick: true,
  showDisplay: true,
  sendToCommunity: true,
  sendToMadavi: true,
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
};

const POLL_STEP_SECONDS = 2;

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
    this.status = { ...DEFAULT_STATUS };
    this.config = loadFromStorage('multigeiger:config', { ...DEFAULT_CONFIG });
  }

  async getStatus() {
    // Simulate slight changes
    this.status.counts += Math.floor(Math.random() * 4);
    this.status.cpm = this.status.cpm + (Math.random() - 0.5);
    this.status.dose_uSvh = this.status.cpm * 0.0029;
    this.status.uptime_s += POLL_STEP_SECONDS;
    await sleep();
    return { ...this.status };
  }

  async getConfig() {
    await sleep();
    return { ...this.config };
  }

  async saveConfig(data) {
    this.config = { ...this.config, ...(data || {}) };
    saveToStorage('multigeiger:config', this.config);
    await sleep();
    return { status: 'ok' };
  }

  async ping() {
    await sleep(100);
    return { status: 'ok' };
  }

  async uploadFirmware() {
    await sleep(600);
    return { status: 'ok' };
  }
}

window.MockAPI = MockAPI;
window.mockAPI = new MockAPI();

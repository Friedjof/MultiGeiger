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
    this.status = { ...BASE_STATUS };
    this.startedAt = Date.now();
    this.baseCounts = this.status.counts;
    this.cps = (this.status.cpm || 0) / 60;
    this.config = loadFromStorage('multigeiger:config', { ...DEFAULT_CONFIG });
  }

  async getStatus() {
    const elapsed = Math.max(0, Math.floor((Date.now() - this.startedAt) / 1000));
    const counts = this.baseCounts + Math.round(this.cps * elapsed);
    const cpm = this.cps * 60;
    const dose = cpm * 0.0029;
    await sleep();
    return {
      ...this.status,
      counts,
      cpm: Number(cpm.toFixed(1)),
      dose_uSvh: Number(dose.toFixed(3)),
      uptime_s: elapsed,
    };
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

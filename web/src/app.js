const POLL_MS = 2000;
const HEARTBEAT_MS = 2000;

function shouldUseMockFromQuery() {
  const params = new URLSearchParams(window.location.search);
  if (params.has('mock')) {
    return params.get('mock') !== '0';
  }
  if (params.get('useMock') === '1') {
    return true;
  }

  // Default to mock when running locally without a device
  const host = window.location.hostname;
  if (host === 'localhost' || host === '127.0.0.1' || window.location.protocol === 'file:') {
    return true;
  }

  return false;
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const secs = Math.floor(seconds % 60);

  if (days > 0) return `${days}d ${hours}h ${minutes}m`;
  if (hours > 0) return `${hours}h ${minutes}m`;
  if (minutes > 0) return `${minutes}m ${secs}s`;
  return `${secs}s`;
}

function clampDecimals(value, decimals = 2) {
  if (typeof value !== 'number' || Number.isNaN(value)) return '--';
  return value.toFixed(decimals);
}

function qs(id) {
  return document.getElementById(id);
}

export class MultiGeigerApp {
  constructor() {
    this.useMock = shouldUseMockFromQuery();
    this.mockApi = null;
    this.statusTimer = null;
    this.heartbeatTimer = null;
    this.lastStatusAt = null;
    this.currentView = this.getInitialView();
    this.statusBadge = qs('statusBadge');
    this.uiVersion = qs('uiVersion');
  }

  async init() {
    this.applyVersion();
    this.bindTabs();
    this.bindConfigForm();
    this.bindUploadForm();
    this.switchView(this.currentView, { skipHistory: true });

    if (this.useMock) {
      await this.ensureMockApi();
    }

    this.refreshStatus();
    this.startStatusPoll();
    this.loadConfig();
    this.startLastUpdateTicker();
  }

  getInitialView() {
    return 'dashboard';
  }

  applyVersion() {
    const version = __APP_VERSION__ || 'dev';
    const url = `https://github.com/Stride-Labs/MultiGeiger/releases/tag/${version}`;
    const footerLink = qs('uiVersionFooter');
    if (footerLink) {
      footerLink.textContent = version;
      footerLink.href = url;
    }
  }

  bindTabs() {
    const tabs = document.querySelectorAll('.toggle-btn');
    tabs.forEach((tab) => {
      tab.addEventListener('click', (e) => {
        const view = tab.dataset.target || tab.dataset.panel || tab.dataset.view;
        if (!view) return;
        this.switchView(view);
      });
    });
  }

  switchView(view, opts = {}) {
    this.currentView = view;
    const { skipHistory = false } = opts;
    document.querySelectorAll('.toggle-btn').forEach((tab) => {
      tab.classList.toggle('active', (tab.dataset.target || tab.dataset.panel) === view);
    });
    document.querySelectorAll('section[data-panel]').forEach((panel) => {
      const isActive = panel.dataset.panel === view;
      panel.hidden = !isActive;
    });

    if (!skipHistory) {
      // SPA only at "/"
      if (window.location.pathname !== '/') {
        window.history.replaceState({}, '', '/');
      }
    }

    if (view === 'settings') {
      this.startHeartbeat();
    } else {
      this.stopHeartbeat();
    }
  }

  async ensureMockApi() {
    if (this.mockApi) return;
    await this.loadMockScript();
    const MockAPI = window.MockAPI;
    if (!MockAPI) {
      throw new Error('Mock API not available');
    }
    this.mockApi = new MockAPI();
  }

  loadMockScript() {
    if (window.MockAPI) return Promise.resolve();
    const existing = document.querySelector('script[data-mock-api]');
    if (existing) {
      return new Promise((resolve, reject) => {
        existing.addEventListener('load', () => resolve());
        existing.addEventListener('error', () => reject(new Error('Mock script failed to load')));
      });
    }

    return new Promise((resolve, reject) => {
      const script = document.createElement('script');
      script.type = 'module';
      script.src = '/mock/api.js';
      script.dataset.mockApi = '1';
      script.onload = () => resolve();
      script.onerror = () => reject(new Error('Failed to load mock API script'));
      document.head.appendChild(script);
    });
  }

  async apiRequest(path, options = {}) {
    if (this.useMock) {
      return this.callMock(path, options);
    }

    const response = await fetch(path, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...(options.headers || {}),
      },
    });

    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || `HTTP ${response.status}`);
    }

    const contentType = response.headers.get('content-type') || '';
    if (contentType.includes('application/json')) {
      return response.json();
    }
    return response.text();
  }

  async callMock(path, options) {
    if (!this.mockApi) {
      await this.ensureMockApi();
    }

    const body = options.body ? JSON.parse(options.body) : undefined;

    switch (path) {
      case '/api/status':
        return this.mockApi.getStatus();
      case '/api/config':
        if (options.method === 'POST') {
          return this.mockApi.saveConfig(body);
        }
        return this.mockApi.getConfig();
      case '/api/config/ping':
        return this.mockApi.ping();
      case '/update':
        return this.mockApi.uploadFirmware(body?.file);
      default:
        throw new Error(`Mock route not implemented: ${path}`);
    }
  }

  async refreshStatus() {
    try {
      const data = await this.apiRequest('/api/status');
      this.lastStatusAt = Date.now();
      this.updateStatusUi(data);
      this.setConnectionState(true);
    } catch (err) {
      console.warn('Status fetch failed', err);
      this.setConnectionState(false);
    }
  }

  startStatusPoll() {
    clearInterval(this.statusTimer);
    this.statusTimer = setInterval(() => this.refreshStatus(), POLL_MS);
  }

  updateStatusUi(data) {
    this.updateText('doseRate', clampDecimals(Number(data.dose_uSvh), 3));
    this.updateText('cpm', clampDecimals(Number(data.cpm), 1));
    this.updateText('counts', data.counts ?? '--');
    this.updateText('uptime', formatUptime(Number(data.uptime_s || 0)));

    const hvWarning = qs('hvWarning');
    if (hvWarning) {
      hvWarning.classList.toggle('hidden', !data.hv_error);
    }

    // Environment fields
    const temp = data.temperature;
    const humidity = data.humidity;
    const pressure = data.pressure;
    const gas = data.gas ?? data.gas_resistance;
    const hasTemp = temp !== undefined && temp !== null;
    const hasHumidity = humidity !== undefined && humidity !== null;
    const hasPressure = pressure !== undefined && pressure !== null;
    const hasGas = gas !== undefined && gas !== null;
    const hasEnv = hasTemp || hasHumidity || hasPressure || hasGas || data.has_thp;

    const envCard = qs('envCard');
    if (envCard) {
      envCard.classList.toggle('hidden', !hasEnv);
    }

    if (hasTemp) {
      this.updateText('temperature', clampDecimals(Number(temp), 1));
      qs('temperature')?.parentElement?.classList.remove('hidden');
    } else {
      qs('temperature')?.parentElement?.classList.add('hidden');
    }

    if (hasHumidity) {
      this.updateText('humidity', clampDecimals(Number(humidity), 1));
      qs('humidity')?.parentElement?.classList.remove('hidden');
    } else {
      qs('humidity')?.parentElement?.classList.add('hidden');
    }

    if (hasPressure) {
      this.updateText('pressure', clampDecimals(Number(pressure), 1));
      qs('pressure')?.parentElement?.classList.remove('hidden');
    } else {
      qs('pressure')?.parentElement?.classList.add('hidden');
    }

    const gasItem = qs('gasItem');
    if (hasGas && gasItem) {
      gasItem.classList.remove('hidden');
      this.updateText('gasValue', clampDecimals(Number(gas), 0));
    } else if (gasItem) {
      gasItem.classList.add('hidden');
    }

    const sensorBadge = qs('sensorBadge');
    if (sensorBadge) {
      const sensorName = this.resolveSensorName({ hasGas, hasHumidity, hasPressure, hasTemp });
      if (sensorName) {
        sensorBadge.textContent = sensorName;
        sensorBadge.classList.remove('hidden');
      } else {
        sensorBadge.classList.add('hidden');
      }
    }

  }

  resolveSensorName({ hasGas, hasHumidity, hasPressure, hasTemp }) {
    if (hasGas) return 'BME680';
    if (hasHumidity && hasPressure && hasTemp) return 'BME280';
    if (hasPressure && hasTemp) return 'BMP280';
    if (hasTemp) return 'Temp sensor';
    return null;
  }

  setConnectionState(online) {
    const stateEl = qs('statusText');
    const dot = qs('connectionDot');
    if (stateEl) {
      stateEl.textContent = online ? 'Online' : 'Offline';
    }
    if (dot) {
      dot.classList.toggle('online', online);
    }
  }

  updateText(id, value) {
    const el = qs(id);
    if (el) {
      el.textContent = value;
    }
  }

  startLastUpdateTicker() {
    const target = qs('lastUpdate');
    if (!target) return;
    setInterval(() => {
      if (!this.lastStatusAt) {
        target.textContent = 'Waiting...';
        return;
      }
      const diff = Math.max(0, Math.round((Date.now() - this.lastStatusAt) / 1000));
      const text = diff === 0 ? 'Just updated' : `Updated ${diff}s ago`;
      target.textContent = text;
    }, 1000);
  }

  bindConfigForm() {
    const form = qs('configForm');
    if (!form) return;

    form.addEventListener('submit', (e) => {
      e.preventDefault();
      this.saveConfig();
    });

    const resetBtn = qs('resetConfig');
    if (resetBtn) {
      resetBtn.addEventListener('click', () => this.resetConfig());
    }
  }

  async loadConfig() {
    try {
      const data = await this.apiRequest('/api/config');
      this.populateConfigForm(data);
      this.showBanner('configStatus', 'Configuration loaded', 'success');
    } catch (err) {
      console.error('Config load failed', err);
      this.showBanner('configStatus', 'Failed to load configuration', 'error');
    }
  }

  populateConfigForm(cfg) {
    const setVal = (id, value) => {
      const input = qs(id);
      if (input && value !== undefined && value !== null) {
        input.value = value;
      }
    };
    const setCheckbox = (id, value) => {
      const input = qs(id);
      if (input) input.checked = !!value;
    };

    setVal('thingName', cfg.thingName);
    setVal('apPassword', cfg.apPassword);
    setVal('wifiSsid', cfg.wifiSsid);
    setVal('wifiPassword', cfg.wifiPassword);

    setCheckbox('startSound', cfg.startSound);
    setCheckbox('speakerTick', cfg.speakerTick);
    setCheckbox('ledTick', cfg.ledTick);
    setCheckbox('showDisplay', cfg.showDisplay);

    setCheckbox('sendToCommunity', cfg.sendToCommunity);
    setCheckbox('sendToMadavi', cfg.sendToMadavi);
    setCheckbox('sendToBle', cfg.sendToBle);

    setCheckbox('sendToMqtt', cfg.sendToMqtt);
    setVal('mqttHost', cfg.mqttHost);
    setVal('mqttPort', cfg.mqttPort);
    setCheckbox('mqttUseTls', cfg.mqttUseTls);
    setCheckbox('mqttRetain', cfg.mqttRetain);
    setVal('mqttUsername', cfg.mqttUsername);
    setVal('mqttPassword', cfg.mqttPassword);
    setVal('mqttBaseTopic', cfg.mqttBaseTopic);

    if (cfg.hasLora) {
      const loraGroup = qs('loraGroup');
      if (loraGroup) loraGroup.classList.remove('hidden');
      setCheckbox('sendToLora', cfg.sendToLora);
      setVal('devaddr', cfg.devaddr);
      setVal('nwkskey', cfg.nwkskey);
      setVal('appskey', cfg.appskey);
    }

    setCheckbox('soundLocalAlarm', cfg.soundLocalAlarm);
    setVal('localAlarmThreshold', cfg.localAlarmThreshold);
    setVal('localAlarmFactor', cfg.localAlarmFactor);

    this.originalConfig = cfg;
  }

  readConfigForm() {
    const val = (id) => (qs(id)?.value ?? '').trim();
    const num = (id, fallback) => {
      const parsed = Number(val(id));
      return Number.isFinite(parsed) ? parsed : fallback;
    };
    const checked = (id) => !!qs(id)?.checked;

    const data = {
      thingName: val('thingName'),
      apPassword: val('apPassword'),
      wifiSsid: val('wifiSsid'),
      wifiPassword: val('wifiPassword'),
      startSound: checked('startSound'),
      speakerTick: checked('speakerTick'),
      ledTick: checked('ledTick'),
      showDisplay: checked('showDisplay'),
      sendToCommunity: checked('sendToCommunity'),
      sendToMadavi: checked('sendToMadavi'),
      sendToBle: checked('sendToBle'),
      sendToMqtt: checked('sendToMqtt'),
      mqttHost: val('mqttHost'),
      mqttPort: num('mqttPort', 1883),
      mqttUseTls: checked('mqttUseTls'),
      mqttRetain: checked('mqttRetain'),
      mqttUsername: val('mqttUsername'),
      mqttPassword: val('mqttPassword'),
      mqttBaseTopic: val('mqttBaseTopic'),
      soundLocalAlarm: checked('soundLocalAlarm'),
      localAlarmThreshold: Number(val('localAlarmThreshold')) || 0.5,
      localAlarmFactor: Number(val('localAlarmFactor')) || 10,
    };

    const loraGroup = qs('loraGroup');
    if (loraGroup && !loraGroup.classList.contains('hidden')) {
      data.sendToLora = checked('sendToLora');
      data.devaddr = val('devaddr');
      data.nwkskey = val('nwkskey');
      data.appskey = val('appskey');
    }

    return data;
  }

  async saveConfig() {
    const form = qs('configForm');
    if (form && !form.reportValidity()) {
      this.showBanner('configStatus', 'Please fill all required fields', 'error');
      return;
    }

    const data = this.readConfigForm();
    const saveBtn = qs('saveConfig');
    if (saveBtn) saveBtn.disabled = true;

    try {
      await this.apiRequest('/api/config', {
        method: 'POST',
        body: JSON.stringify(data),
      });
      this.showBanner('configStatus', 'Configuration saved. Device may restart.', 'success');
    } catch (err) {
      console.error('Config save failed', err);
      this.showBanner('configStatus', err.message || 'Failed to save configuration', 'error');
    } finally {
      if (saveBtn) saveBtn.disabled = false;
    }
  }

  resetConfig() {
    if (!this.originalConfig) return;
    this.populateConfigForm(this.originalConfig);
    this.showBanner('configStatus', 'Form reset to last loaded config', 'success');
  }

  startHeartbeat() {
    if (this.useMock) return;
    this.stopHeartbeat();
    this.sendHeartbeat();
    this.heartbeatTimer = setInterval(() => this.sendHeartbeat(), HEARTBEAT_MS);
  }

  stopHeartbeat() {
    clearInterval(this.heartbeatTimer);
    this.heartbeatTimer = null;
  }

  async sendHeartbeat() {
    try {
      await this.apiRequest('/api/config/ping', { method: 'POST' });
    } catch (err) {
      console.warn('Heartbeat failed', err);
    }
  }

  bindUploadForm() {
    const form = qs('uploadForm');
    if (!form) return;
    form.addEventListener('submit', (e) => {
      e.preventDefault();
      this.handleUpload();
    });
  }

  async handleUpload() {
    const input = qs('firmwareFile');
    const button = qs('uploadButton');
    const bar = qs('uploadProgressBar');
    const progress = qs('uploadProgress');
    const label = qs('uploadProgressLabel');
    if (!input?.files?.length) {
      alert('Please select a .bin firmware file');
      return;
    }

    const file = input.files[0];
    if (!file.name.endsWith('.bin')) {
      alert('Please select a .bin file');
      return;
    }

    const formData = new FormData();
    formData.append('update', file);

    if (button) button.disabled = true;
    if (progress) progress.classList.remove('hidden');
    this.updateUploadProgress(0);

    try {
      if (this.useMock) {
        await this.callMock('/update', { method: 'POST', body: JSON.stringify({ file }) });
        this.updateUploadProgress(100);
        alert('Mock upload done. Device would restart.');
      } else {
        await this.uploadFirmware(formData, (percent) => this.updateUploadProgress(percent));
      }
    } catch (err) {
      console.error('Upload failed', err);
      alert(err.message || 'Upload failed');
      this.updateUploadProgress(0);
    } finally {
      if (button) button.disabled = false;
      if (progress) progress.classList.add('hidden');
      if (bar) bar.style.width = '0%';
      if (label) label.textContent = '0%';
      if (input) input.value = '';
    }
  }

  async uploadFirmware(formData, onProgress) {
    await new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update');

      xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable && typeof onProgress === 'function') {
          const percent = Math.round((e.loaded / e.total) * 100);
          onProgress(percent);
        }
      });

      xhr.onload = () => {
        if (xhr.status === 200) {
          onProgress?.(100);
          alert('Firmware uploaded. Device will restart.');
          resolve();
        } else {
          reject(new Error(`Upload failed (${xhr.status})`));
        }
      };

      xhr.onerror = () => reject(new Error('Upload error'));
      xhr.send(formData);
    });
  }

  updateUploadProgress(percent) {
    const bar = qs('uploadProgressBar');
    const label = qs('uploadProgressLabel');
    if (bar) bar.style.width = `${percent}%`;
    if (label) label.textContent = `${percent}%`;
  }

  showBanner(id, message, type = 'info') {
    const el = qs(id);
    if (!el) return;
    el.textContent = message;
    el.classList.remove('hidden', 'success', 'error');
    if (type === 'success') el.classList.add('success');
    if (type === 'error') el.classList.add('error');
  }
}

/**
 * MultiGeiger Configuration Manager
 * Handles loading and saving device configuration
 */

// API endpoints
const USE_MOCK_API = window.location.hostname === 'localhost' ||
                     window.location.hostname === '127.0.0.1' ||
                     window.location.protocol === 'file:';

const CONFIG_API = USE_MOCK_API ? '/mock-config.json' : '/api/config';
const STATUS_API = USE_MOCK_API ? '/mock-api.json' : '/api/status';

class ConfigManager {
    constructor() {
        this.form = document.getElementById('config-form');
        this.statusMessage = document.getElementById('status-message');
        this.resetBtn = document.getElementById('reset-btn');
        this.originalConfig = null;
        this.heartbeatInterval = null;

        this.init();
    }

    init() {
        console.log('MultiGeiger Config Manager initialized');

        // Initialize accordion
        this.initAccordion();

        // Load configuration
        this.loadConfig();

        // Setup event listeners
        this.form.addEventListener('submit', (e) => this.handleSubmit(e));
        this.resetBtn.addEventListener('click', () => this.resetForm());

        // Load device info
        this.loadDeviceInfo();

        // Start heartbeat to keep ticks disabled while on config page
        this.startHeartbeat();

        // Stop heartbeat when leaving page
        window.addEventListener('beforeunload', () => this.stopHeartbeat());
    }

    startHeartbeat() {
        // Send heartbeat every 2 seconds to keep ticks disabled
        if (!USE_MOCK_API) {
            this.heartbeatInterval = setInterval(() => {
                fetch('/api/config/ping', { method: 'POST' })
                    .catch(err => console.warn('Heartbeat failed:', err));
            }, 2000);
            console.log('Config page heartbeat started');
        }
    }

    stopHeartbeat() {
        if (this.heartbeatInterval) {
            clearInterval(this.heartbeatInterval);
            this.heartbeatInterval = null;
            console.log('Config page heartbeat stopped');
        }
    }

    initAccordion() {
        const accordions = document.querySelectorAll('.accordion');

        // Mark last visible accordion in form
        this.updateLastVisibleAccordion();

        // Open first accordion by default
        if (accordions.length > 0) {
            accordions[0].classList.add('active');
        }

        // Add click event to all accordion headers
        accordions.forEach((accordion) => {
            const header = accordion.querySelector('.accordion-header');
            if (header) {
                header.addEventListener('click', () => {
                    this.toggleAccordion(accordion);
                });
            }
        });
    }

    updateLastVisibleAccordion() {
        // Get all accordions inside the form
        const formAccordions = Array.from(document.querySelectorAll('form .accordion'));

        // Filter visible ones
        const visibleAccordions = formAccordions.filter(acc =>
            !acc.classList.contains('hidden') &&
            getComputedStyle(acc).display !== 'none'
        );

        // Remove last-visible class and inline styles from all
        formAccordions.forEach(acc => {
            acc.classList.remove('last-visible');
            acc.style.borderRadius = '';
        });

        // Add to the last visible one
        if (visibleAccordions.length > 0) {
            const lastVisible = visibleAccordions[visibleAccordions.length - 1];
            lastVisible.classList.add('last-visible');
            // Also set inline style as backup
            lastVisible.style.borderRadius = '0 0 16px 16px';
            console.log('Last visible accordion:', lastVisible.querySelector('h2')?.textContent);
            console.log('Border radius applied:', lastVisible.style.borderRadius);
        }
    }

    toggleAccordion(accordion) {
        const isActive = accordion.classList.contains('active');

        // Toggle the clicked accordion
        if (isActive) {
            accordion.classList.remove('active');
        } else {
            accordion.classList.add('active');
        }
    }

    async loadConfig() {
        try {
            const response = await fetch(CONFIG_API);

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const config = await response.json();
            this.originalConfig = { ...config };
            this.populateForm(config);

        } catch (error) {
            console.error('Failed to load config:', error);
            this.showStatus('Failed to load configuration', 'error');
        }
    }

    populateForm(config) {
        // WiFi settings
        this.setFieldValue('thingName', config.thingName);
        this.setFieldValue('apPassword', config.apPassword);
        this.setFieldValue('wifiSsid', config.wifiSsid);
        this.setFieldValue('wifiPassword', config.wifiPassword);

        // Misc settings
        this.setCheckbox('startSound', config.startSound);
        this.setCheckbox('speakerTick', config.speakerTick);
        this.setCheckbox('ledTick', config.ledTick);
        this.setCheckbox('showDisplay', config.showDisplay);

        // Transmission settings
        this.setCheckbox('sendToCommunity', config.sendToCommunity);
        this.setCheckbox('sendToMadavi', config.sendToMadavi);
        this.setCheckbox('sendToBle', config.sendToBle);

        // MQTT settings
        this.setCheckbox('sendToMqtt', config.sendToMqtt);
        this.setFieldValue('mqttHost', config.mqttHost);
        this.setFieldValue('mqttPort', config.mqttPort);
        this.setCheckbox('mqttUseTls', config.mqttUseTls);
        this.setCheckbox('mqttRetain', config.mqttRetain);
        this.setFieldValue('mqttUsername', config.mqttUsername);
        this.setFieldValue('mqttPassword', config.mqttPassword);
        this.setFieldValue('mqttBaseTopic', config.mqttBaseTopic);

        // LoRa settings (if available)
        if (config.hasLora) {
            document.getElementById('lora-section').classList.remove('hidden');
            this.setCheckbox('sendToLora', config.sendToLora);
            this.setFieldValue('devaddr', config.devaddr);
            this.setFieldValue('nwkskey', config.nwkskey);
            this.setFieldValue('appskey', config.appskey);
            // Update last visible accordion after showing LoRa section
            this.updateLastVisibleAccordion();
        }

        // Alarm settings
        this.setCheckbox('soundLocalAlarm', config.soundLocalAlarm);
        this.setFieldValue('localAlarmThreshold', config.localAlarmThreshold);
        this.setFieldValue('localAlarmFactor', config.localAlarmFactor);
    }

    setFieldValue(id, value) {
        const field = document.getElementById(id);
        if (field && value !== undefined && value !== null) {
            field.value = value;
        }
    }

    setCheckbox(id, checked) {
        const checkbox = document.getElementById(id);
        if (checkbox) {
            checkbox.checked = !!checked;
        }
    }

    getFormData() {
        const data = {
            // WiFi settings
            thingName: document.getElementById('thingName').value,
            apPassword: document.getElementById('apPassword').value,
            wifiSsid: document.getElementById('wifiSsid').value,
            wifiPassword: document.getElementById('wifiPassword').value,

            // Misc settings
            startSound: document.getElementById('startSound').checked,
            speakerTick: document.getElementById('speakerTick').checked,
            ledTick: document.getElementById('ledTick').checked,
            showDisplay: document.getElementById('showDisplay').checked,

            // Transmission settings
            sendToCommunity: document.getElementById('sendToCommunity').checked,
            sendToMadavi: document.getElementById('sendToMadavi').checked,
            sendToBle: document.getElementById('sendToBle').checked,

            // MQTT settings
            sendToMqtt: document.getElementById('sendToMqtt').checked,
            mqttHost: document.getElementById('mqttHost').value,
            mqttPort: parseInt(document.getElementById('mqttPort').value) || 1883,
            mqttUseTls: document.getElementById('mqttUseTls').checked,
            mqttRetain: document.getElementById('mqttRetain').checked,
            mqttUsername: document.getElementById('mqttUsername').value,
            mqttPassword: document.getElementById('mqttPassword').value,
            mqttBaseTopic: document.getElementById('mqttBaseTopic').value,

            // Alarm settings
            soundLocalAlarm: document.getElementById('soundLocalAlarm').checked,
            localAlarmThreshold: parseFloat(document.getElementById('localAlarmThreshold').value) || 0.5,
            localAlarmFactor: parseInt(document.getElementById('localAlarmFactor').value) || 10
        };

        // Add LoRa settings if section is visible
        const loraSection = document.getElementById('lora-section');
        if (!loraSection.classList.contains('hidden')) {
            data.sendToLora = document.getElementById('sendToLora').checked;
            data.devaddr = document.getElementById('devaddr').value;
            data.nwkskey = document.getElementById('nwkskey').value;
            data.appskey = document.getElementById('appskey').value;
        }

        return data;
    }

    async handleSubmit(event) {
        event.preventDefault();

        // Validate form
        if (!this.form.checkValidity()) {
            this.showStatus('Please fill in all required fields', 'error');
            return;
        }

        // Stop heartbeat - config is being saved
        this.stopHeartbeat();

        const formData = this.getFormData();

        try {
            this.form.classList.add('loading');

            const response = await fetch(CONFIG_API, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(formData)
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            this.showStatus('Configuration saved! Device will restart...', 'success');

            // Redirect to dashboard after 3 seconds
            setTimeout(() => {
                window.location.href = '/';
            }, 3000);

        } catch (error) {
            console.error('Failed to save config:', error);
            this.showStatus('Failed to save configuration', 'error');
            this.form.classList.remove('loading');
        }
    }

    resetForm() {
        if (confirm('Discard changes and reset form?')) {
            if (this.originalConfig) {
                this.populateForm(this.originalConfig);
                this.showStatus('Form reset', 'success');
                setTimeout(() => this.hideStatus(), 2000);
            } else {
                this.form.reset();
            }
        }
    }

    async loadDeviceInfo() {
        try {
            const response = await fetch(STATUS_API);
            if (response.ok) {
                const data = await response.json();
                // Try to get version from response if available
                if (data.version) {
                    document.getElementById('current-version').textContent = data.version;
                } else {
                    document.getElementById('current-version').textContent = 'Unknown';
                }
            }
        } catch (error) {
            console.error('Failed to load device info:', error);
            document.getElementById('current-version').textContent = 'Unknown';
        }
    }

    showStatus(message, type) {
        this.statusMessage.textContent = message;
        this.statusMessage.className = 'status-message ' + type;

        // Scroll to message
        this.statusMessage.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }

    hideStatus() {
        this.statusMessage.classList.add('hidden');
    }
}

// Initialize config manager when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new ConfigManager();
});

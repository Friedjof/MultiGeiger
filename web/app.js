/**
 * MultiGeiger Live Dashboard
 * Fetches and displays radiation and environmental data
 */

// Switch between mock and real API
const USE_MOCK_API = window.location.hostname === 'localhost' ||
                     window.location.hostname === '127.0.0.1' ||
                     window.location.protocol === 'file:';

const API_ENDPOINT = USE_MOCK_API ? '/mock-api.json' : '/api/status';
const UPDATE_INTERVAL = 2000; // 2 seconds

class Dashboard {
    constructor() {
        this.updateTimer = null;
        this.lastUpdate = null;
        this.isConnected = false;

        this.init();
    }

    init() {
        console.log('MultiGeiger Dashboard initialized');
        this.startAutoUpdate();

        // Initial fetch
        this.fetchData();
    }

    startAutoUpdate() {
        this.updateTimer = setInterval(() => {
            this.fetchData();
        }, UPDATE_INTERVAL);
    }

    async fetchData() {
        try {
            const response = await fetch(API_ENDPOINT);

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const data = await response.json();
            this.updateUI(data);
            this.setConnectionStatus(true);

        } catch (error) {
            console.error('Fetch error:', error);
            this.setConnectionStatus(false);
        }
    }

    updateUI(data) {
        // Radiation data
        this.updateElement('dose-rate', data.dose_uSvh, 3);
        this.updateElement('cpm', data.cpm, 1);
        this.updateElement('counts', data.counts, 0);

        // HV Error
        const hvError = document.getElementById('hv-error');
        if (data.hv_error) {
            hvError.classList.remove('hidden');
        } else {
            hvError.classList.add('hidden');
        }

        // Environmental data
        if (data.has_thp) {
            this.updateElement('temperature', data.temperature, 1);
            this.updateElement('humidity', data.humidity, 1);
            this.updateElement('pressure', data.pressure, 1);
            this.showElement('env-section');
        } else {
            this.hideElement('env-section');
        }

        // Uptime
        this.updateUptime(data.uptime_s);

        // Last update time
        this.lastUpdate = new Date();
        this.updateLastUpdateText();
    }

    updateElement(id, value, decimals = 0) {
        const element = document.getElementById(id);
        if (element) {
            const formatted = typeof value === 'number'
                ? value.toFixed(decimals)
                : value;
            element.textContent = formatted;

            // Visual feedback on update
            element.style.transform = 'scale(1.05)';
            setTimeout(() => {
                element.style.transform = 'scale(1)';
            }, 200);
        }
    }

    updateUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        const secs = seconds % 60;

        let uptimeStr = '';
        if (days > 0) {
            uptimeStr = `${days}d ${hours}h ${minutes}m`;
        } else if (hours > 0) {
            uptimeStr = `${hours}h ${minutes}m`;
        } else if (minutes > 0) {
            uptimeStr = `${minutes}m ${secs}s`;
        } else {
            uptimeStr = `${secs}s`;
        }

        this.updateElement('uptime', uptimeStr);
    }

    updateLastUpdateText() {
        const now = new Date();
        const diff = Math.floor((now - this.lastUpdate) / 1000);

        let text = 'Just updated';
        if (diff > 0) {
            text = `Updated ${diff}s ago`;
        }

        const element = document.getElementById('last-update');
        if (element) {
            element.textContent = text;
        }
    }

    setConnectionStatus(connected) {
        const statusElement = document.getElementById('connection-status');
        const dotElement = document.getElementById('update-dot');

        this.isConnected = connected;

        if (statusElement) {
            statusElement.textContent = connected ? 'Online' : 'Offline';
        }

        if (dotElement) {
            if (connected) {
                dotElement.classList.remove('offline');
            } else {
                dotElement.classList.add('offline');
            }
        }
    }

    showElement(id) {
        const element = document.getElementById(id);
        if (element) {
            element.style.display = '';
        }
    }

    hideElement(id) {
        const element = document.getElementById(id);
        if (element) {
            element.style.display = 'none';
        }
    }
}

// Initialize dashboard when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    new Dashboard();
});

// Update "last update" text every second
setInterval(() => {
    const lastUpdateElement = document.getElementById('last-update');
    if (lastUpdateElement && window.dashboard) {
        window.dashboard.updateLastUpdateText();
    }
}, 1000);

// Store dashboard instance globally
let dashboard;
document.addEventListener('DOMContentLoaded', () => {
    dashboard = new Dashboard();
    window.dashboard = dashboard;
});

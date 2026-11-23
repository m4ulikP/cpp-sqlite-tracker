/**
 * Student Expense & Task Tracker - API Client
 * Interacts with C++20 REST API on http://localhost:8080
 */

const API_BASE = (() => {
  // If served directly by the C++ backend on port 8080
  if (window.location.hostname === 'localhost' && window.location.port === '8080') {
    return '';
  }
  // If served via python static server (e.g. port 3000, 5000, 8000) or file://
  return 'http://localhost:8080';
})();

class ApiClient {
  constructor(baseUrl = API_BASE) {
    this.baseUrl = baseUrl;
  }

  async request(endpoint, options = {}) {
    const url = `${this.baseUrl}${endpoint}`;
    const defaultHeaders = {
      'Accept': 'application/json',
      'Content-Type': 'application/json'
    };

    try {
      const response = await fetch(url, {
        ...options,
        headers: {
          ...defaultHeaders,
          ...(options.headers || {})
        }
      });

      let jsonBody = null;
      const text = await response.text();
      if (text) {
        try {
          jsonBody = JSON.parse(text);
        } catch (e) {
          throw new Error(`Invalid JSON response: ${text.slice(0, 80)}`);
        }
      }

      if (!response.ok) {
        const errorMsg = jsonBody?.error?.message || `HTTP ${response.status}: ${response.statusText}`;
        const errorCode = jsonBody?.error?.code || 'HTTP_ERROR';
        const err = new Error(errorMsg);
        err.code = errorCode;
        err.status = response.status;
        throw err;
      }

      return jsonBody;
    } catch (err) {
      if (!err.status && err.name === 'TypeError') {
        err.message = 'Unable to reach backend server at http://localhost:8080. Please ensure tracker_server.exe is running.';
        err.code = 'NETWORK_ERROR';
      }
      throw err;
    }
  }

  async checkHealth() {
    return this.request('/api/health');
  }

  async getSummary() {
    return this.request('/api/items/summary');
  }

  async getItems(filters = {}) {
    const params = new URLSearchParams();
    if (filters.type) params.append('type', filters.type);
    if (filters.status) params.append('status', filters.status);
    if (filters.category) params.append('category', filters.category);
    if (filters.search) params.append('search', filters.search);

    const query = params.toString() ? `?${params.toString()}` : '';
    return this.request(`/api/items${query}`);
  }

  async getItem(id) {
    return this.request(`/api/items/${encodeURIComponent(id)}`);
  }

  async createItem(item) {
    return this.request('/api/items', {
      method: 'POST',
      body: JSON.stringify(item)
    });
  }

  async updateItem(id, item) {
    return this.request(`/api/items/${encodeURIComponent(id)}`, {
      method: 'PUT',
      body: JSON.stringify(item)
    });
  }

  async deleteItem(id) {
    return this.request(`/api/items/${encodeURIComponent(id)}`, {
      method: 'DELETE'
    });
  }
}

window.apiClient = new ApiClient();

/**
 * Student Expense & Task Tracker - UI Controller
 * Handles DOM interactions, state, modals, and reactive rendering
 */

document.addEventListener('DOMContentLoaded', () => {
  // App State
  const state = {
    items: [],
    summary: null,
    filters: {
      type: '',
      status: '',
      category: '',
      search: ''
    },
    editingItemId: null,
    deletingItemId: null,
    isOnline: false,
    searchDebounceTimer: null
  };

  // DOM Elements
  const el = {
    // Navbar
    connectionStatus: document.getElementById('connectionStatus'),
    btnOpenCreateModal: document.getElementById('btnOpenCreateModal'),
    errorBanner: document.getElementById('errorBanner'),
    errorBannerTitle: document.getElementById('errorBannerTitle'),
    errorBannerMessage: document.getElementById('errorBannerMessage'),
    btnRetryConnection: document.getElementById('btnRetryConnection'),

    // Summary Cards
    summaryTotalExpense: document.getElementById('summaryTotalExpense'),
    summaryExpenseCount: document.getElementById('summaryExpenseCount'),
    summaryPaidAmount: document.getElementById('summaryPaidAmount'),
    summaryPendingAmount: document.getElementById('summaryPendingAmount'),
    expenseProgressBar: document.getElementById('expenseProgressBar'),
    summaryTaskPercentage: document.getElementById('summaryTaskPercentage'),
    taskProgressBar: document.getElementById('taskProgressBar'),
    summaryTasksDone: document.getElementById('summaryTasksDone'),
    summaryTasksProgress: document.getElementById('summaryTasksProgress'),
    summaryTasksPending: document.getElementById('summaryTasksPending'),
    summaryTotalTasks: document.getElementById('summaryTotalTasks'),

    // Filters
    searchInput: document.getElementById('searchInput'),
    btnClearSearch: document.getElementById('btnClearSearch'),
    typeFilterControl: document.getElementById('typeFilterControl'),
    categoryFilter: document.getElementById('categoryFilter'),
    statusFilter: document.getElementById('statusFilter'),
    btnResetFilters: document.getElementById('btnResetFilters'),
    btnRefresh: document.getElementById('btnRefresh'),
    itemsCountLabel: document.getElementById('itemsCountLabel'),

    // Table & States
    loadingState: document.getElementById('loadingState'),
    emptyState: document.getElementById('emptyState'),
    btnEmptyStateAction: document.getElementById('btnEmptyStateAction'),
    itemsTable: document.getElementById('itemsTable'),
    itemsTableBody: document.getElementById('itemsTableBody'),

    // Create / Edit Modal
    itemModal: document.getElementById('itemModal'),
    modalTitle: document.getElementById('modalTitle'),
    btnCloseModal: document.getElementById('btnCloseModal'),
    btnCancelModal: document.getElementById('btnCancelModal'),
    itemForm: document.getElementById('itemForm'),
    modalFormError: document.getElementById('modalFormError'),
    formItemId: document.getElementById('formItemId'),
    formTitle: document.getElementById('formTitle'),
    formCategory: document.getElementById('formCategory'),
    formAmount: document.getElementById('formAmount'),
    labelFormAmount: document.getElementById('labelFormAmount'),
    formStatus: document.getElementById('formStatus'),
    formPriority: document.getElementById('formPriority'),
    formDueDate: document.getElementById('formDueDate'),
    formDescription: document.getElementById('formDescription'),
    btnSubmitForm: document.getElementById('btnSubmitForm'),
    btnSubmitText: document.getElementById('btnSubmitText'),

    // Delete Modal
    deleteModal: document.getElementById('deleteModal'),
    deleteItemTitle: document.getElementById('deleteItemTitle'),
    btnCloseDeleteModal: document.getElementById('btnCloseDeleteModal'),
    btnCancelDelete: document.getElementById('btnCancelDelete'),
    btnConfirmDelete: document.getElementById('btnConfirmDelete'),

    // Toasts
    toastContainer: document.getElementById('toastContainer')
  };

  // Helper: Escape HTML to prevent XSS
  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#039;');
  }

  // Format Currency
  function formatCurrency(amount) {
    const num = Number(amount) || 0;
    return `$${num.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
  }

  // Show Toast
  function showToast(message, type = 'success') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.textContent = message;

    el.toastContainer.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(8px)';
      toast.style.transition = 'all 0.25s ease';
      setTimeout(() => toast.remove(), 250);
    }, 3200);
  }

  // Connection Status Update
  function setStatus(online, message = '') {
    state.isOnline = online;
    if (online) {
      el.connectionStatus.className = 'status-pill online';
      el.connectionStatus.querySelector('.status-text').textContent = 'Backend Connected';
      el.errorBanner.classList.add('hidden');
    } else {
      el.connectionStatus.className = 'status-pill offline';
      el.connectionStatus.querySelector('.status-text').textContent = 'Backend Offline';
      el.errorBanner.classList.remove('hidden');
      if (message) el.errorBannerMessage.textContent = message;
    }
  }

  // Fetch Dashboard Summary
  async function loadSummary() {
    try {
      const resp = await window.apiClient.getSummary();
      if (!resp || !resp.success) return;
      state.summary = resp.data;
      renderSummary(resp.data);
    } catch (err) {
      console.warn('Failed to load summary stats:', err);
    }
  }

  function renderSummary(summary) {
    if (!summary) return;

    // Expenses
    const exp = summary.expenses || {};
    el.summaryTotalExpense.textContent = formatCurrency(exp.total_amount);
    el.summaryExpenseCount.textContent = `${exp.total_count || 0} recorded expense${exp.total_count === 1 ? '' : 's'}`;
    el.summaryPaidAmount.textContent = formatCurrency(exp.paid_amount);
    el.summaryPendingAmount.textContent = formatCurrency(exp.pending_amount);

    const paidPct = exp.total_amount > 0 ? (exp.paid_amount / exp.total_amount) * 100 : 0;
    el.expenseProgressBar.style.width = `${Math.min(100, Math.round(paidPct))}%`;

    // Tasks
    const t = summary.tasks || {};
    const taskPct = Math.round(t.completion_percentage || 0);
    el.summaryTaskPercentage.textContent = `${taskPct}%`;
    el.taskProgressBar.style.width = `${taskPct}%`;
    el.summaryTasksDone.textContent = t.completed_count || 0;
    el.summaryTasksProgress.textContent = t.in_progress_count || 0;
    el.summaryTasksPending.textContent = t.pending_count || 0;
    el.summaryTotalTasks.textContent = `${t.total_count || 0} total task${t.total_count === 1 ? '' : 's'}`;
  }

  // Fetch Items
  async function loadItems() {
    el.loadingState.classList.remove('hidden');
    el.itemsTable.classList.add('hidden');
    el.emptyState.classList.add('hidden');

    try {
      const resp = await window.apiClient.getItems(state.filters);
      setStatus(true);
      state.items = (resp && resp.data) ? resp.data : [];
      renderItems(state.items);
    } catch (err) {
      setStatus(false, err.message);
      el.loadingState.classList.add('hidden');
      state.items = [];
      renderItems([]);
    }
  }

  function renderItems(items) {
    el.loadingState.classList.add('hidden');

    if (!items || items.length === 0) {
      el.itemsTable.classList.add('hidden');
      el.emptyState.classList.remove('hidden');
      el.itemsCountLabel.textContent = '0 records found';
      return;
    }

    el.emptyState.classList.add('hidden');
    el.itemsTable.classList.remove('hidden');
    el.itemsCountLabel.textContent = `Showing ${items.length} record${items.length === 1 ? '' : 's'}`;

    el.itemsTableBody.innerHTML = '';
    const todayStr = new Date().toISOString().split('T')[0];

    items.forEach(item => {
      const tr = document.createElement('tr');
      const isDone = item.status === 'completed' || item.status === 'paid';
      const isOverdue = item.due_date && item.due_date < todayStr && !isDone;

      tr.innerHTML = `
        <td>
          <button class="btn-toggle-status ${isDone ? 'is-done' : ''}" data-id="${item.id}" title="${isDone ? 'Mark Pending' : 'Mark Completed/Paid'}">
            ${isDone ? '&#10003;' : ''}
          </button>
        </td>
        <td>
          <div class="item-title-text ${isDone ? 'text-muted' : ''}" style="${isDone ? 'text-decoration: line-through;' : ''}">
            ${escapeHtml(item.title)}
          </div>
          ${item.description ? `<div class="item-desc-text">${escapeHtml(item.description)}</div>` : ''}
        </td>
        <td>
          <span class="badge ${item.type === 'expense' ? 'badge-expense' : 'badge-task'}">
            ${escapeHtml(item.type)}
          </span>
        </td>
        <td>
          <span class="badge badge-cat">${escapeHtml(item.category)}</span>
        </td>
        <td>
          <span class="amount-text">${item.type === 'expense' || item.amount > 0 ? formatCurrency(item.amount) : '&mdash;'}</span>
        </td>
        <td>
          <span class="due-date-text ${isOverdue ? 'overdue' : ''}">
            ${item.due_date ? escapeHtml(item.due_date) : '&mdash;'}
            ${isOverdue ? '<span title="Overdue" style="font-size:0.7rem; margin-left:2px;">&#9888;</span>' : ''}
          </span>
        </td>
        <td>
          <span class="badge badge-prio-${escapeHtml(item.priority)}">${escapeHtml(item.priority)}</span>
        </td>
        <td>
          <span class="badge badge-status-${escapeHtml(item.status)}">${escapeHtml(item.status.replace('_', ' '))}</span>
        </td>
        <td>
          <div class="action-group">
            <button class="btn-icon edit-btn" data-id="${item.id}" title="Edit Item">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"></path>
                <path d="M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z"></path>
              </svg>
            </button>
            <button class="btn-icon delete delete-btn" data-id="${item.id}" data-title="${escapeHtml(item.title)}" title="Delete Item">
              <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <polyline points="3 6 5 6 21 6"></polyline>
                <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
              </svg>
            </button>
          </div>
        </td>
      `;

      // Event listener: Toggle status
      const btnToggle = tr.querySelector('.btn-toggle-status');
      btnToggle.addEventListener('click', () => quickToggleStatus(item));

      // Event listener: Edit
      const btnEdit = tr.querySelector('.edit-btn');
      btnEdit.addEventListener('click', () => openEditModal(item));

      // Event listener: Delete
      const btnDelete = tr.querySelector('.delete-btn');
      btnDelete.addEventListener('click', () => openDeleteModal(item.id, item.title));

      el.itemsTableBody.appendChild(tr);
    });
  }

  // Quick Toggle Status
  async function quickToggleStatus(item) {
    let nextStatus = 'pending';
    if (item.type === 'expense') {
      nextStatus = (item.status === 'paid') ? 'pending' : 'paid';
    } else {
      nextStatus = (item.status === 'completed') ? 'pending' : 'completed';
    }

    try {
      const updated = {
        title: item.title,
        type: item.type,
        category: item.category,
        amount: item.amount,
        status: nextStatus,
        priority: item.priority,
        due_date: item.due_date,
        description: item.description
      };
      await window.apiClient.updateItem(item.id, updated);
      showToast(`Status changed to ${nextStatus}`, 'success');
      loadSummary();
      loadItems();
    } catch (err) {
      showToast(`Failed to update status: ${err.message}`, 'error');
    }
  }

  // Open Modal: Create Mode
  function openCreateModal() {
    state.editingItemId = null;
    el.modalTitle.textContent = 'Add New Item';
    el.btnSubmitText.textContent = 'Create Item';
    el.modalFormError.classList.add('hidden');
    el.itemForm.reset();
    el.formItemId.value = '';
    el.formStatus.value = 'pending';
    el.formPriority.value = 'medium';
    el.formAmount.value = '0.00';

    // Sync radio with initial type
    const defaultRadio = document.querySelector('input[name="itemType"][value="expense"]');
    if (defaultRadio) defaultRadio.checked = true;
    onTypeChanged('expense');

    el.itemModal.classList.remove('hidden');
    el.formTitle.focus();
  }

  // Open Modal: Edit Mode
  function openEditModal(item) {
    state.editingItemId = item.id;
    el.modalTitle.textContent = `Edit Item #${item.id}`;
    el.btnSubmitText.textContent = 'Save Changes';
    el.modalFormError.classList.add('hidden');

    el.formItemId.value = item.id;
    el.formTitle.value = item.title || '';
    el.formCategory.value = item.category || 'Other';
    el.formAmount.value = Number(item.amount || 0).toFixed(2);
    el.formStatus.value = item.status || 'pending';
    el.formPriority.value = item.priority || 'medium';
    el.formDueDate.value = item.due_date || '';
    el.formDescription.value = item.description || '';

    const typeRadio = document.querySelector(`input[name="itemType"][value="${item.type}"]`);
    if (typeRadio) typeRadio.checked = true;
    onTypeChanged(item.type);

    el.itemModal.classList.remove('hidden');
    el.formTitle.focus();
  }

  function closeModal() {
    el.itemModal.classList.add('hidden');
    el.modalFormError.classList.add('hidden');
    state.editingItemId = null;
  }

  function onTypeChanged(type) {
    if (type === 'expense') {
      el.labelFormAmount.textContent = 'Amount ($) *';
      if (el.formCategory.value === 'Assignment' || el.formCategory.value === 'Exam') {
        el.formCategory.value = 'Food';
      }
    } else {
      el.labelFormAmount.textContent = 'Points / Est. Cost ($)';
      if (el.formCategory.value === 'Food' || el.formCategory.value === 'Housing') {
        el.formCategory.value = 'Assignment';
      }
    }
  }

  // Handle Form Submit
  async function handleFormSubmit(e) {
    e.preventDefault();
    el.modalFormError.classList.add('hidden');

    const selectedType = document.querySelector('input[name="itemType"]:checked')?.value || 'expense';
    const titleVal = el.formTitle.value.trim();
    const amountVal = parseFloat(el.formAmount.value) || 0.0;

    if (!titleVal) {
      el.modalFormError.textContent = 'Please enter an item title.';
      el.modalFormError.classList.remove('hidden');
      el.formTitle.focus();
      return;
    }

    if (amountVal < 0) {
      el.modalFormError.textContent = 'Amount cannot be negative.';
      el.modalFormError.classList.remove('hidden');
      el.formAmount.focus();
      return;
    }

    const payload = {
      title: titleVal,
      type: selectedType,
      category: el.formCategory.value || 'Other',
      amount: amountVal,
      status: el.formStatus.value || 'pending',
      priority: el.formPriority.value || 'medium',
      due_date: el.formDueDate.value || '',
      description: el.formDescription.value.trim()
    };

    el.btnSubmitForm.disabled = true;
    el.btnSubmitText.textContent = state.editingItemId ? 'Saving...' : 'Creating...';

    try {
      if (state.editingItemId) {
        await window.apiClient.updateItem(state.editingItemId, payload);
        showToast('Item updated successfully', 'success');
      } else {
        await window.apiClient.createItem(payload);
        showToast('Item created successfully', 'success');
      }

      closeModal();
      loadSummary();
      loadItems();
    } catch (err) {
      el.modalFormError.textContent = err.message || 'Operation failed.';
      el.modalFormError.classList.remove('hidden');
    } finally {
      el.btnSubmitForm.disabled = false;
      el.btnSubmitText.textContent = state.editingItemId ? 'Save Changes' : 'Create Item';
    }
  }

  // Open Delete Confirmation Modal
  function openDeleteModal(id, title) {
    state.deletingItemId = id;
    el.deleteItemTitle.textContent = `"${title}" (ID: ${id})`;
    el.deleteModal.classList.remove('hidden');
  }

  function closeDeleteModal() {
    el.deleteModal.classList.add('hidden');
    state.deletingItemId = null;
  }

  // Confirm Delete
  async function confirmDelete() {
    if (!state.deletingItemId) return;
    const id = state.deletingItemId;
    el.btnConfirmDelete.disabled = true;
    el.btnConfirmDelete.textContent = 'Deleting...';

    try {
      await window.apiClient.deleteItem(id);
      showToast(`Item #${id} deleted`, 'success');
      closeDeleteModal();
      loadSummary();
      loadItems();
    } catch (err) {
      showToast(`Delete failed: ${err.message}`, 'error');
    } finally {
      el.btnConfirmDelete.disabled = false;
      el.btnConfirmDelete.textContent = 'Delete Permanently';
    }
  }

  // Wire Event Listeners
  function setupEventListeners() {
    // Open create modal buttons
    el.btnOpenCreateModal.addEventListener('click', openCreateModal);
    el.btnEmptyStateAction.addEventListener('click', openCreateModal);

    // Close modal triggers
    el.btnCloseModal.addEventListener('click', closeModal);
    el.btnCancelModal.addEventListener('click', closeModal);
    el.btnCloseDeleteModal.addEventListener('click', closeDeleteModal);
    el.btnCancelDelete.addEventListener('click', closeDeleteModal);

    // Escape key closes modals
    window.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        if (!el.itemModal.classList.contains('hidden')) closeModal();
        if (!el.deleteModal.classList.contains('hidden')) closeDeleteModal();
      }
    });

    // Form submit & change
    el.itemForm.addEventListener('submit', handleFormSubmit);
    document.querySelectorAll('input[name="itemType"]').forEach(radio => {
      radio.addEventListener('change', (e) => onTypeChanged(e.target.value));
    });

    // Confirm Delete
    el.btnConfirmDelete.addEventListener('click', confirmDelete);

    // Search input with debounce
    el.searchInput.addEventListener('input', (e) => {
      const val = e.target.value.trim();
      if (val) {
        el.btnClearSearch.classList.remove('hidden');
      } else {
        el.btnClearSearch.classList.add('hidden');
      }

      clearTimeout(state.searchDebounceTimer);
      state.searchDebounceTimer = setTimeout(() => {
        state.filters.search = val;
        loadItems();
      }, 250);
    });

    el.btnClearSearch.addEventListener('click', () => {
      el.searchInput.value = '';
      el.btnClearSearch.classList.add('hidden');
      state.filters.search = '';
      loadItems();
    });

    // Type Filter Tabs
    el.typeFilterControl.querySelectorAll('.seg-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        el.typeFilterControl.querySelectorAll('.seg-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        state.filters.type = btn.dataset.type;
        loadItems();
      });
    });

    // Category Filter
    el.categoryFilter.addEventListener('change', (e) => {
      state.filters.category = e.target.value;
      loadItems();
    });

    // Status Filter
    el.statusFilter.addEventListener('change', (e) => {
      state.filters.status = e.target.value;
      loadItems();
    });

    // Reset Filters
    el.btnResetFilters.addEventListener('click', () => {
      el.searchInput.value = '';
      el.btnClearSearch.classList.add('hidden');
      el.categoryFilter.value = '';
      el.statusFilter.value = '';
      el.typeFilterControl.querySelectorAll('.seg-btn').forEach(b => b.classList.remove('active'));
      el.typeFilterControl.querySelector('[data-type=""]').classList.add('active');

      state.filters = { type: '', status: '', category: '', search: '' };
      loadItems();
    });

    // Manual Refresh
    el.btnRefresh.addEventListener('click', () => {
      loadSummary();
      loadItems();
    });

    // Retry connection from banner
    el.btnRetryConnection.addEventListener('click', () => {
      loadSummary();
      loadItems();
    });
  }

  // Initialization
  async function init() {
    setupEventListeners();
    await loadSummary();
    await loadItems();
  }

  init();
});

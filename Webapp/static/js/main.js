document.addEventListener('DOMContentLoaded', function() {
    // Flash message close functionality
    const flashCloseButtons = document.querySelectorAll('.flash-close');
    flashCloseButtons.forEach(button => {
        button.addEventListener('click', function() {
            this.parentElement.style.animation = 'slideUp 0.3s ease-out forwards';
            setTimeout(() => {
                this.parentElement.remove();
            }, 300);
        });
    });

    // Login form submission with loading state
    const loginForm = document.getElementById('loginForm');
    if (loginForm) {
        loginForm.addEventListener('submit', function(e) {
            const loginBtn = document.getElementById('loginButton');
            
            // Add loading state
            loginBtn.classList.add('btn-loading');
            loginBtn.textContent = '';
            loginBtn.disabled = true;
            
            // Form will submit normally, but we show loading state
            setTimeout(() => {
                if (loginBtn) {
                    loginBtn.classList.remove('btn-loading');
                    loginBtn.textContent = 'Sign In';
                    loginBtn.disabled = false;
                }
            }, 2000);
        });
    }

    // Add input focus effects
    const inputs = document.querySelectorAll('input[type="email"], input[type="password"]');
    inputs.forEach(input => {
        input.addEventListener('focus', function() {
            this.parentElement.style.transform = 'scale(1.02)';
        });
        
        input.addEventListener('blur', function() {
            this.parentElement.style.transform = 'scale(1)';
        });
    });

    // Add hover effects to container
    const container = document.querySelector('.login-container');
    if (container) {
        container.addEventListener('mouseenter', function() {
            this.style.transform = 'translateY(-5px)';
        });
        
        container.addEventListener('mouseleave', function() {
            this.style.transform = 'translateY(0)';
        });
    }

    // Auto-hide flash messages after 5 seconds
    const flashMessages = document.querySelectorAll('.flash-message');
    flashMessages.forEach(message => {
        setTimeout(() => {
            if (message.parentElement) {
                message.style.animation = 'slideUp 0.3s ease-out forwards';
                setTimeout(() => {
                    message.remove();
                }, 300);
            }
        }, 5000);
    });
});

// CSS animation for slide up
const style = document.createElement('style');
style.textContent = `
    @keyframes slideUp {
        from {
            opacity: 1;
            transform: translateY(0);
        }
        to {
            opacity: 0;
            transform: translateY(-10px);
        }
    }
`;
document.head.appendChild(style);


document.addEventListener('DOMContentLoaded', function() {
    // Password toggle functionality
    const passwordToggles = document.querySelectorAll('.password-toggle');
    passwordToggles.forEach(toggle => {
        toggle.addEventListener('click', function() {
            const targetId = this.getAttribute('data-target');
            const passwordInput = document.getElementById(targetId);
            const eyeIcon = this.querySelector('.eye-icon');
            const eyeOffIcon = this.querySelector('.eye-off-icon');
            
            if (passwordInput.type === 'password') {
                passwordInput.type = 'text';
                eyeIcon.classList.add('hidden');
                eyeOffIcon.classList.remove('hidden');
            } else {
                passwordInput.type = 'password';
                eyeIcon.classList.remove('hidden');
                eyeOffIcon.classList.add('hidden');
            }
        });
    });

    // Password strength checker
    const passwordInput = document.getElementById('password');
    const strengthIndicator = document.getElementById('passwordStrength');
    const strengthText = strengthIndicator.querySelector('.strength-text');

    if (passwordInput && strengthIndicator) {
        passwordInput.addEventListener('input', function() {
            const password = this.value;
            const strength = calculatePasswordStrength(password);
            
            // Remove all strength classes
            strengthIndicator.classList.remove('strength-weak', 'strength-fair', 'strength-good', 'strength-strong');
            
            if (password.length > 0) {
                strengthIndicator.classList.add(`strength-${strength.level}`);
                strengthText.textContent = strength.text;
            } else {
                strengthText.textContent = 'Password strength';
            }
        });
    }

    // Password confirmation checker
    const confirmPasswordInput = document.getElementById('confirm_password');
    const passwordMatch = document.getElementById('passwordMatch');

    if (confirmPasswordInput && passwordMatch) {
        function checkPasswordMatch() {
            const password = passwordInput.value;
            const confirmPassword = confirmPasswordInput.value;
            
            if (confirmPassword.length > 0) {
                if (password === confirmPassword) {
                    passwordMatch.textContent = '✓ Passwords match';
                    passwordMatch.className = 'password-match match';
                } else {
                    passwordMatch.textContent = '✗ Passwords do not match';
                    passwordMatch.className = 'password-match no-match';
                }
            } else {
                passwordMatch.textContent = '';
                passwordMatch.className = 'password-match';
            }
        }

        passwordInput.addEventListener('input', checkPasswordMatch);
        confirmPasswordInput.addEventListener('input', checkPasswordMatch);
    }

    // Form submission with validation
    const signupForm = document.getElementById('signupForm');
    if (signupForm) {
        signupForm.addEventListener('submit', function(e) {
            const signupBtn = document.getElementById('signupButton');
            const password = passwordInput.value;
            const confirmPassword = confirmPasswordInput.value;
            const termsCheckbox = document.getElementById('terms');

            // Check if passwords match
            if (password !== confirmPassword) {
                e.preventDefault();
                alert('Passwords do not match!');
                return;
            }

            // Check if terms are accepted
            if (!termsCheckbox.checked) {
                e.preventDefault();
                alert('Please accept the Terms of Service and Privacy Policy!');
                return;
            }

            // Add loading state
            signupBtn.classList.add('btn-loading');
            signupBtn.textContent = '';
            signupBtn.disabled = true;
        });
    }
});

// Password strength calculation function
function calculatePasswordStrength(password) {
    let score = 0;
    let feedback = [];

    if (password.length >= 8) score += 1;
    else feedback.push('at least 8 characters');

    if (/[a-z]/.test(password)) score += 1;
    else feedback.push('lowercase letters');

    if (/[A-Z]/.test(password)) score += 1;
    else feedback.push('uppercase letters');

    if (/[0-9]/.test(password)) score += 1;
    else feedback.push('numbers');

    if (/[^A-Za-z0-9]/.test(password)) score += 1;

    const levels = {
        0: { level: 'weak', text: 'Very weak password' },
        1: { level: 'weak', text: 'Weak password' },
        2: { level: 'fair', text: 'Fair password' },
        3: { level: 'good', text: 'Good password' },
        4: { level: 'strong', text: 'Strong password' },
        5: { level: 'strong', text: 'Very strong password' }
    };

    return levels[score] || levels[0];
}

// Username availability checker (mock function)
function checkUsernameAvailability(username) {
    // This would normally make an AJAX request to your server
    // For demo purposes, we'll simulate some taken usernames
    const takenUsernames = ['admin', 'user', 'test', 'demo'];
    return !takenUsernames.includes(username.toLowerCase());
}









document.addEventListener('DOMContentLoaded', function() {
    // Animate balance amount on page load
    const balanceAmount = document.getElementById('balanceAmount');
    if (balanceAmount) {
        const finalAmount = parseFloat(balanceAmount.textContent);
        let currentAmount = 0;
        const increment = finalAmount / 50;
        const timer = setInterval(() => {
            currentAmount += increment;
            if (currentAmount >= finalAmount) {
                currentAmount = finalAmount;
                clearInterval(timer);
            }
            balanceAmount.textContent = currentAmount.toFixed(2);
        }, 20);
    }

    // Add click handlers for balance action buttons
    const addFundsBtn = document.querySelector('.add-funds');
    const transferBtn = document.querySelector('.transfer-funds');

    if (addFundsBtn) {
        addFundsBtn.addEventListener('click', function() {
            alert('Add funds functionality would be implemented here!');
        });
    }

    if (transferBtn) {
        transferBtn.addEventListener('click', function() {
            alert('Transfer funds functionality would be implemented here!');
        });
    }

    // Add click handlers for other action cards
    const shopCard = document.querySelector('.shop-card');
    const profileCard = document.querySelector('.profile-card');
    const supportCard = document.querySelector('.support-card');

    if (shopCard) {
        shopCard.addEventListener('click', function() {
            alert('Browse products functionality would be implemented here!');
        });
    }

    if (profileCard) {
        profileCard.addEventListener('click', function() {
            alert('Account settings functionality would be implemented here!');
        });
    }

    if (supportCard) {
        supportCard.addEventListener('click', function() {
            alert('Help & support functionality would be implemented here!');
        });
    }

    // Add some interactive effects
    const actionCards = document.querySelectorAll('.action-card');
    actionCards.forEach(card => {
        card.addEventListener('mouseenter', function() {
            this.style.background = 'linear-gradient(135deg, rgba(255, 255, 255, 1), rgba(255, 255, 255, 0.9))';
        });
        
        card.addEventListener('mouseleave', function() {
            this.style.background = 'linear-gradient(135deg, rgba(255, 255, 255, 0.9), rgba(255, 255, 255, 0.7))';
        });
    });
});
















// Purchase History JavaScript
document.addEventListener('DOMContentLoaded', function() {
    // Elements
    const searchInput = document.getElementById('searchInput');
    const statusFilter = document.getElementById('statusFilter');
    const categoryFilter = document.getElementById('categoryFilter');
    const clearFiltersBtn = document.getElementById('clearFilters');
    const clearSearchBtn = document.getElementById('clearSearchBtn');
    const purchaseList = document.querySelector('.purchase-list');
    const emptyState = document.getElementById('emptyState');
    const purchaseModal = document.getElementById('purchaseModal');
    const modalBody = document.getElementById('modalBody');
    const closeModalBtn = document.getElementById('closeModal');
    
    // Get all purchase items
    const purchaseItems = document.querySelectorAll('.purchase-item');
    
    // Flash message handling
    const flashMessages = document.querySelectorAll('.flash-message');
    flashMessages.forEach(message => {
        const closeBtn = message.querySelector('.flash-close');
        if (closeBtn) {
            closeBtn.addEventListener('click', () => {
                message.style.animation = 'slideUp 0.3s ease-out forwards';
                setTimeout(() => message.remove(), 300);
            });
        }
        
        // Auto-hide flash messages after 5 seconds
        setTimeout(() => {
            if (message.parentNode) {
                message.style.animation = 'slideUp 0.3s ease-out forwards';
                setTimeout(() => message.remove(), 300);
            }
        }, 5000);
    });
    
    // Search and Filter Functions
    function filterPurchases() {
        const searchTerm = searchInput.value.toLowerCase().trim();
        const statusValue = statusFilter.value;
        const categoryValue = categoryFilter.value;
        
        let visibleCount = 0;
        
        purchaseItems.forEach(item => {
            const itemName = item.querySelector('.item-name').textContent.toLowerCase();
            const itemStatus = item.dataset.status;
            const itemCategory = item.dataset.category;
            
            // Check search term
            const matchesSearch = !searchTerm || itemName.includes(searchTerm);
            
            // Check status filter
            const matchesStatus = !statusValue || itemStatus === statusValue;
            
            // Check category filter
            const matchesCategory = !categoryValue || itemCategory === categoryValue;
            
            if (matchesSearch && matchesStatus && matchesCategory) {
                item.style.display = 'block';
                visibleCount++;
                
                // Add entrance animation
                item.style.animation = 'slideIn 0.5s ease-out';
            } else {
                item.style.display = 'none';
            }
        });
        
        // Show/hide empty state
        if (visibleCount === 0) {
            purchaseList.style.display = 'none';
            emptyState.style.display = 'block';
        } else {
            purchaseList.style.display = 'flex';
            emptyState.style.display = 'none';
        }
    }
    
    // Event Listeners for Search and Filter
    searchInput.addEventListener('input', debounce(filterPurchases, 300));
    statusFilter.addEventListener('change', filterPurchases);
    categoryFilter.addEventListener('change', filterPurchases);
    
    // Clear filters
    clearFiltersBtn.addEventListener('click', () => {
        searchInput.value = '';
        statusFilter.value = '';
        categoryFilter.value = '';
        filterPurchases();
        
        // Add button animation
        clearFiltersBtn.style.transform = 'scale(0.95)';
        setTimeout(() => {
            clearFiltersBtn.style.transform = 'scale(1)';
        }, 150);
    });
    
    // Clear search from empty state
    clearSearchBtn.addEventListener('click', () => {
        searchInput.value = '';
        statusFilter.value = '';
        categoryFilter.value = '';
        filterPurchases();
    });
    
    // Modal Functions
    function openModal(purchaseData) {
        const modalContent = `
            <div class="purchase-detail">
                <div class="detail-header">
                    <div class="detail-icon">
                        ${getIconForCategory(purchaseData.category)}
                    </div>
                    <div class="detail-title">
                        <h4>${purchaseData.item}</h4>
                        <p>Order #${purchaseData.id.toString().padStart(6, '0')}</p>
                    </div>
                </div>
                
                <div class="detail-section">
                    <h5>Order Information</h5>
                    <div class="detail-grid">
                        <div class="detail-item">
                            <span class="label">Status:</span>
                            <span class="status-badge status-${purchaseData.status.toLowerCase()}">${purchaseData.status}</span>
                        </div>
                        <div class="detail-item">
                            <span class="label">Category:</span>
                            <span class="value">${purchaseData.category}</span>
                        </div>
                        <div class="detail-item">
                            <span class="label">Order Date:</span>
                            <span class="value">${purchaseData.date}</span>
                        </div>
                        <div class="detail-item">
                            <span class="label">Amount:</span>
                            <span class="value amount-highlight">$${purchaseData.amount.toFixed(2)}</span>
                        </div>
                    </div>
                </div>
                
                <div class="detail-section">
                    <h5>Shipping Information</h5>
                    <div class="shipping-info">
                        <p><strong>Shipping Address:</strong> 123 Main St, City, State 12345</p>
                        <p><strong>Estimated Delivery:</strong> ${getEstimatedDelivery(purchaseData.status)}</p>
                        <p><strong>Tracking ID:</strong> TR${purchaseData.id}${Date.now().toString().slice(-6)}</p>
                    </div>
                </div>
                
                <div class="detail-actions">
                    <button class="action-btn track-btn">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/>
                            <circle cx="12" cy="10" r="3"/>
                        </svg>
                        Track Order
                    </button>
                    <button class="action-btn reorder-btn">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M3 3h2l.4 2M7 13h10l4-8H5.4m0 0L7 13m0 0-2.293 2.293c-.63.63-.184 1.707.707 1.707H19M7 13v4a2 2 0 0 0 2 2h2m8-2v2a2 2 0 0 1-2 2H9"/>
                        </svg>
                        Reorder
                    </button>
                    <button class="action-btn support-btn">
                        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>
                        </svg>
                        Contact Support
                    </button>
                </div>
            </div>
        `;
        
        modalBody.innerHTML = modalContent;
        purchaseModal.style.display = 'flex';
        document.body.style.overflow = 'hidden';
        
        // Add modal event listeners
        addModalEventListeners();
    }
    
    function closeModal() {
        purchaseModal.style.display = 'none';
        document.body.style.overflow = 'auto';
    }
    
    function addModalEventListeners() {
        const trackBtn = modalBody.querySelector('.track-btn');
        const reorderBtn = modalBody.querySelector('.reorder-btn');
        const supportBtn = modalBody.querySelector('.support-btn');
        
        if (trackBtn) {
            trackBtn.addEventListener('click', () => {
                showNotification('Tracking information sent to your email!', 'info');
                closeModal();
            });
        }
        
        if (reorderBtn) {
            reorderBtn.addEventListener('click', () => {
                showNotification('Item added to cart successfully!', 'success');
                closeModal();
            });
        }
        
        if (supportBtn) {
            supportBtn.addEventListener('click', () => {
                showNotification('Support ticket created. We\'ll contact you soon!', 'info');
                closeModal();
            });
        }
    }
    
    // Purchase item event listeners
    purchaseItems.forEach(item => {
        const viewBtn = item.querySelector('.view-btn');
        const moreBtn = item.querySelector('.more-btn');
        
        if (viewBtn) {
            viewBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                const purchaseData = extractPurchaseData(item);
                openModal(purchaseData);
            });
        }
        
        if (moreBtn) {
            moreBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                showOptionsMenu(e, item);
            });
        }
        
        // Click on item to view details
        item.addEventListener('click', () => {
            const purchaseData = extractPurchaseData(item);
            openModal(purchaseData);
        });
    });
    
    // Modal close event listeners
    closeModalBtn.addEventListener('click', closeModal);
    
    purchaseModal.addEventListener('click', (e) => {
        if (e.target === purchaseModal) {
            closeModal();
        }
    });
    
    // Keyboard events
    document.addEventListener('keydown', (e) => {
        if (e.key === 'Escape' && purchaseModal.style.display === 'flex') {
            closeModal();
        }
    });
    
    // Helper Functions
    function extractPurchaseData(item) {
        return {
            id: parseInt(item.querySelector('.view-btn').dataset.id),
            item: item.querySelector('.item-name').textContent,
            category: item.dataset.category,
            status: item.dataset.status,
            date: item.querySelector('.date').textContent,
            amount: parseFloat(item.querySelector('.amount').textContent.replace('$', ''))
        };
    }
    
    function getIconForCategory(category) {
        const icons = {
            'Electronics': '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"/><line x1="8" y1="21" x2="16" y2="21"/><line x1="12" y1="17" x2="12" y2="21"/></svg>',
            'Food & Beverages': '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 8h1a4 4 0 0 1 0 8h-1"/><path d="M2 8h16v9a4 4 0 0 1-4 4H6a4 4 0 0 1-4-4V8z"/><line x1="6" y1="1" x2="6" y2="4"/><line x1="10" y1="1" x2="10" y2="4"/><line x1="14" y1="1" x2="14" y2="4"/></svg>',
            'Health & Fitness': '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M22 12h-4l-3 9L9 3l-3 9H2"/></svg>',
            'Clothing': '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20.38 3.46 16 2a4 4 0 0 1-8 0L3.62 3.46a2 2 0 0 0-1.34 2.23l.58 3.47a1 1 0 0 0 .99.84H6v10c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V10h2.15a1 1 0 0 0 .99-.84l.58-3.47a2 2 0 0 0-1.34-2.23z"/></svg>',
            'Home & Kitchen': '<svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 9h18v10a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V9Z"/><path d="m3 9 2.45-4.9A2 2 0 0 1 7.24 3h9.52a2 2 0 0 1 1.8 1.1L21 9"/><path d="M12 3v6"/></svg>'
        };
        return icons[category] || icons['Home & Kitchen'];
    }
    
    function getEstimatedDelivery(status) {
        const now = new Date();
        const deliveryDays = {
            'Processing': 5,
            'Shipped': 3,
            'Delivered': 0,
            'Cancelled': 0
        };
        
        if (status === 'Delivered') return 'Delivered';
        if (status === 'Cancelled') return 'Cancelled';
        
        const deliveryDate = new Date(now.getTime() + (deliveryDays[status] * 24 * 60 * 60 * 1000));
        return deliveryDate.toLocaleDateString('en-US', { 
            weekday: 'long', 
            year: 'numeric', 
            month: 'long', 
            day: 'numeric' 
        });
    }
    
    function showOptionsMenu(event, item) {
        // Remove existing menus
        document.querySelectorAll('.options-menu').forEach(menu => menu.remove());
        
        const menu = document.createElement('div');
        menu.className = 'options-menu';
        menu.innerHTML = `
            <div class="menu-item" data-action="view">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                    <circle cx="12" cy="12" r="3"/>
                </svg>
                View Details
            </div>
            <div class="menu-item" data-action="reorder">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M3 3h2l.4 2M7 13h10l4-8H5.4m0 0L7 13m0 0-2.293 2.293c-.63.63-.184 1.707.707 1.707H19M7 13v4a2 2 0 0 0 2 2h2m8-2v2a2 2 0 0 1-2 2H9"/>
                </svg>
                Reorder Item
            </div>
            <div class="menu-item" data-action="track">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/>
                    <circle cx="12" cy="10" r="3"/>
                </svg>
                Track Order
            </div>
            <div class="menu-item" data-action="support">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>
                </svg>
                Contact Support
            </div>
        `;
        
        // Position menu
        const rect = event.target.getBoundingClientRect();
        menu.style.position = 'fixed';
        menu.style.top = `${rect.bottom + 5}px`;
        menu.style.right = `${window.innerWidth - rect.right}px`;
        menu.style.zIndex = '1000';
        
        document.body.appendChild(menu);
        
        // Add event listeners to menu items
        menu.querySelectorAll('.menu-item').forEach(menuItem => {
            menuItem.addEventListener('click', (e) => {
                const action = e.currentTarget.dataset.action;
                const purchaseData = extractPurchaseData(item);
                
                switch (action) {
                    case 'view':
                        openModal(purchaseData);
                        break;
                    case 'reorder':
                        showNotification('Item added to cart successfully!', 'success');
                        break;
                    case 'track':
                        showNotification('Tracking information sent to your email!', 'info');
                        break;
                    case 'support':
                        showNotification('Support ticket created. We\'ll contact you soon!', 'info');
                        break;
                }
                
                menu.remove();
            });
        });
        
        // Close menu when clicking outside
        setTimeout(() => {
            document.addEventListener('click', function closeMenu(e) {
                if (!menu.contains(e.target)) {
                    menu.remove();
                    document.removeEventListener('click', closeMenu);
                }
            });
        }, 0);
    }
    
    function showNotification(message, type = 'info') {
        const notification = document.createElement('div');
        notification.className = `flash-message flash-${type}`;
        notification.innerHTML = `
            ${message}
            <button class="flash-close">&times;</button>
        `;
        
        // Insert at the top of the page
        const container = document.querySelector('.purchase-history-container');
        const firstChild = container.firstElementChild;
        container.insertBefore(notification, firstChild);
        
        // Add close functionality
        const closeBtn = notification.querySelector('.flash-close');
        closeBtn.addEventListener('click', () => {
            notification.style.animation = 'slideUp 0.3s ease-out forwards';
            setTimeout(() => notification.remove(), 300);
        });
        
        // Auto-hide after 3 seconds
        setTimeout(() => {
            if (notification.parentNode) {
                notification.style.animation = 'slideUp 0.3s ease-out forwards';
                setTimeout(() => notification.remove(), 300);
            }
        }, 3000);
    }
    
    function debounce(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    }
    
    // Add hover effects to purchase items
    purchaseItems.forEach(item => {
        item.addEventListener('mouseenter', () => {
            item.style.transform = 'translateY(-2px)';
        });
        
        item.addEventListener('mouseleave', () => {
            item.style.transform = 'translateY(0)';
        });
    });
    
    // Smooth scrolling for better UX
    function smoothScrollTo(element) {
        element.scrollIntoView({
            behavior: 'smooth',
            block: 'center'
        });
    }
    
    // Initialize page
    console.log('Purchase History page initialized successfully');
    console.log(`Total purchases loaded: ${purchaseItems.length}`);
});

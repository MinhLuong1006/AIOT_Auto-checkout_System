import os
from werkzeug.security import check_password_hash
from flask import Flask, render_template, request, redirect, url_for, flash
from flask_login import LoginManager, UserMixin, login_user, logout_user, login_required, current_user
from flask_mail import Mail, Message
from itsdangerous import URLSafeTimedSerializer, SignatureExpired
from flask import Flask, render_template, request, redirect, url_for, flash, jsonify, session, current_app
from flask_login import LoginManager, UserMixin, login_user, logout_user, login_required, current_user
from flask_mail import Mail, Message
from itsdangerous import URLSafeTimedSerializer, SignatureExpired
import firebase_admin
from firebase_admin import credentials, db
import time
from flask_cors import CORS
import bcrypt
import threading
import re
import uuid
from werkzeug.security import generate_password_hash, check_password_hash
from collections import defaultdict
app = Flask(__name__)

app.config['SECRET_KEY'] = 'iotproject'
CORS(app)  # Enable CORS for all routes

# Initialize Firebase Admin SDK
cred = credentials.Certificate("auto-checkout-b3ea1-firebase-adminsdk-fbsvc-132b543750.json") #change this to your Firebase Admin SDK JSON file path
firebase_admin.initialize_app(cred, {
    "databaseURL": "https://auto-checkout-b3ea1-default-rtdb.asia-southeast1.firebasedatabase.app/" #change this to your database URL
})

# Flask-Mail configuration
app.config.update(
    MAIL_SERVER='smtp.gmail.com',
    MAIL_PORT=587,
    MAIL_USE_TLS=True,
    MAIL_USE_SSL=False,
    MAIL_USERNAME='YOUR_EMAIL',
    MAIL_PASSWORD='YOUR_EMAIL_APP_PASSWORD', 
    MAIL_DEFAULT_SENDER='YOUR_EMAIL',
    MAIL_MAX_EMAILS=None,
    MAIL_SUPPRESS_SEND=False,  # Set to True to suppress sending in development
    MAIL_ASCII_ATTACHMENTS=False
)

mail = Mail(app)
s = URLSafeTimedSerializer(app.config['SECRET_KEY'])  # Token serializer

def validate_email(email):
    """Validate email format"""
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.match(pattern, email) is not None


def validate_password(password):
    """Validate password strength"""
    if len(password) < 8:
        return False, "Password must be at least 8 characters long"
    if not re.search(r'[A-Z]', password):
        return False, "Password must contain at least one uppercase letter"
    if not re.search(r'[a-z]', password):
        return False, "Password must contain at least one lowercase letter"
    if not re.search(r'[0-9]', password):
        return False, "Password must contain at least one number"
    return True, "Password is valid"

def sanitize_email(email):
    # Replace '.' with ',' or another safe character (',' is readable and allowed)
    return email.replace('.', ',')

def categorize_item(item_name):
    """Automatically categorize items based on keywords in the name"""
    item_lower = item_name.lower()
    
    # Electronics keywords
    if any(keyword in item_lower for keyword in ['phone', 'laptop', 'computer', 'headphone', 'earbuds', 'tablet', 'watch', 'tv', 'camera', 'speaker', 'charger', 'cable']):
        return 'Electronics'
    
    # Food & Beverages keywords
    elif any(keyword in item_lower for keyword in ['coffee', 'tea', 'juice', 'water', 'snack', 'bread', 'milk', 'food', 'drink', 'beverage', 'beer', 'wine']):
        return 'Food & Beverages'
    
    # Health & Fitness keywords
    elif any(keyword in item_lower for keyword in ['vitamin', 'supplement', 'protein', 'fitness', 'gym', 'yoga', 'exercise', 'health', 'medicine', 'medical']):
        return 'Health & Fitness'
    
    # Clothing keywords
    elif any(keyword in item_lower for keyword in ['shirt', 'pants', 'dress', 'shoe', 'jacket', 'hat', 'clothing', 'apparel', 'fashion', 'wear']):
        return 'Clothing'
    
    # Home & Kitchen keywords
    elif any(keyword in item_lower for keyword in ['kitchen', 'home', 'furniture', 'decor', 'appliance', 'cookware', 'utensil', 'plate', 'cup', 'bowl']):
        return 'Home & Kitchen'
    
    else:
        return 'General'

def get_user_purchase_history(user_id):
    """Get purchase history for a specific user from Firebase"""
    try:
        # Get the history data from Firebase
        history_ref = db.reference(f'users/{user_id}/history')
        history_data = history_ref.get()
        
        purchases = []
        
        if history_data:
            for purchase_id, purchase_info in history_data.items():
                # Extract the required fields
                purchase = {
                    'id': purchase_id,
                    'item': purchase_info.get('item', 'Unknown Item'),
                    'amount': purchase_info.get('amount', 1),  # quantity
                    'price': purchase_info.get('price', 0.0),  # total price (no need to multiply)
                    'date': purchase_info.get('date', ''),
                    'category': categorize_item(purchase_info.get('item', ''))
                }
                purchases.append(purchase)
        
        # Sort purchases by date (newest first)
        purchases.sort(key=lambda x: x['date'], reverse=True)
        
        return purchases
        
    except Exception as e:
        print(f"Error fetching purchase history: {e}")
        return []

@app.route('/')
def home():
    return redirect(url_for('login'))

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        email = request.form.get('email')
        password = request.form.get('password')
        remember = request.form.get('remember')

        if not email or not password:
            flash('Please enter both email and password!', 'error')
            return render_template('login.html')

        try:
            # Get the specific user using the sanitized email
            sanitized_email = sanitize_email(email)
            user_ref = db.reference(f'users/{sanitized_email}')
            user_data = user_ref.get()

            if user_data and check_password_hash(user_data['password'], password):
                # Login success
                session['user_id'] = sanitized_email
                session['username'] = user_data['username']
                flash('Login successful!', 'success')
                return redirect(url_for('dashboard'))  # Change this to your actual home/dashboard route
            else:
                flash('Invalid email or password.', 'error')

        except Exception as e:
            flash(f'Login error: {str(e)}', 'error')
            
    return render_template('login.html')

@app.route('/signup', methods=['GET', 'POST'])
def signup():
    if request.method == 'POST':
        email = request.form.get('email')
        username = request.form.get('username')
        password = request.form.get('password')
        confirm_password = request.form.get('confirm_password')
        
        # Basic validation
        if not all([email, username, password, confirm_password]):
            flash('Please fill in all fields!', 'error')
        elif not validate_email(email):
            flash('Please enter a valid email address!', 'error')
        elif len(username) < 3:
            flash('Username must be at least 3 characters long!', 'error')
        elif password != confirm_password:
            flash('Passwords do not match!', 'error')
        else:
            is_valid, message = validate_password(password)
            if not is_valid:
                flash(message, 'error')
            else:
                try:
                    sanitized_email = sanitize_email(email)
                    user_ref = db.reference(f'users/{sanitized_email}')

                    # Check if user already exists
                    if user_ref.get():
                        flash('An account with this email already exists.', 'error')
                        return render_template('signup.html')

                    # Save user data
                    hashed_password = generate_password_hash(password)
                    user_ref.set({
                        'email': email,
                        'username': username,
                        'password': hashed_password,
                        'balance': 100
                    })

                    flash('Account created successfully! You can now log in.', 'success')
                    return redirect(url_for('login'))

                except Exception as e:
                    flash(f'Error saving to Firebase: {e}', 'error')
    
    return render_template('signup.html')

@app.route('/reset_password', methods=['GET', 'POST'])
def reset_password():
    if request.method == 'POST':
        email = request.form['email']

        # Validate email format
        if not validate_email(email):
            flash('Please enter a valid email address!', 'error')
            return render_template('reset_password.html')

        # Check if email exists in Firebase using sanitized email
        sanitized_email = sanitize_email(email)
        user_ref = db.reference(f'users/{sanitized_email}')
        user_data = user_ref.get()

        if not user_data:
            flash('Email not found. Please try again.', 'error')
            return render_template('reset_password.html')

        # Generate password reset token
        token = s.dumps(email, salt='password-reset')

        # Create reset URL
        reset_url = url_for('reset_with_token', token=token, _external=True)

        # Send email
        try:
            msg = Message("Password Reset Request", recipients=[email])
            msg.body = f"Click the link to reset your password: {reset_url}"
            mail.send(msg)
            flash('Password reset link sent to your email.', 'success')
        except Exception as e:
            flash(f'Error sending email: {str(e)}', 'error')

        return redirect(url_for('login'))

    return render_template('reset_password.html')

@app.route('/reset_password/<token>', methods=['GET', 'POST'])
def reset_with_token(token):
    try:
        email = s.loads(token, salt="password-reset", max_age=3600)  # Valid for 1 hour
    except SignatureExpired:
        flash("The token is expired! Please request a new one.", 'error')
        return redirect(url_for('reset_password'))
    except:
        flash("Invalid token!", 'error')
        return redirect(url_for('reset_password'))

    # Check if user exists using sanitized email
    sanitized_email = sanitize_email(email)
    user_ref = db.reference(f'users/{sanitized_email}')
    user_data = user_ref.get()

    if not user_data:
        flash("Invalid token!", 'error')
        return redirect(url_for('reset_password'))

    if request.method == 'POST':
        new_password = request.form['password']
        confirm_password = request.form['confirm_password']

        if new_password != confirm_password:
            flash("Passwords do not match, please try again.", 'error')
            return render_template("reset_with_token.html", token=token)

        # Validate password strength
        is_valid, message = validate_password(new_password)
        if not is_valid:
            flash(message, 'error')
            return render_template("reset_with_token.html", token=token)

        # Update password in Firebase using Werkzeug's generate_password_hash
        try:
            hashed_password = generate_password_hash(new_password)
            user_ref.update({
                'password': hashed_password
            })
            
            flash('Password reset successfully! You can now log in with your new password.', 'success')
            return redirect(url_for("login"))
            
        except Exception as e:
            flash(f"Error updating password: {str(e)}", 'error')
            return render_template("reset_with_token.html", token=token)

    return render_template("reset_with_token.html", token=token)

def is_admin(email):
    """Check if the user is an admin"""
    return email.lower() == 'admin@gmail.com'

@app.route('/dashboard')
def dashboard():
    if 'user_id' not in session:
        flash("Please log in first.", "error")
        return redirect(url_for('login'))

    try:
        user_id = session['user_id']
        user_ref = db.reference(f'users/{user_id}')
        user_data = user_ref.get()

        if not user_data:
            flash("User data not found.", "error")
            return redirect(url_for('login'))

        # Check if user is admin
        user_email = user_data.get('email', '')
        if is_admin(user_email):
            # Admin dashboard - show storage item counts
            item_counts = get_storage_items()
            return render_template('admin_dashboard.html', 
                                 item_counts=item_counts, 
                                 username=user_data.get('username', 'Admin'))
        else:
            # Regular user dashboard
            balance = user_data.get('balance', 0)
            username = user_data.get('username', 'User')
            return render_template('dashboard.html', balance=balance, username=username)

    except Exception as e:
        flash(f"Error loading dashboard: {e}", "error")
        return redirect(url_for('login'))
        
@app.route('/api/admin/items')
def api_admin_items():
    """API endpoint for admin to get storage item counts"""
    if 'user_id' not in session:
        return jsonify({'error': 'Not authenticated'}), 401
    
    try:
        user_id = session['user_id']
        user_ref = db.reference(f'users/{user_id}')
        user_data = user_ref.get()
        
        if not user_data or not is_admin(user_data.get('email', '')):
            return jsonify({'error': 'Access denied'}), 403
        
        item_counts = get_storage_items()
        total_items = sum(item_counts.values()) if item_counts else 0
        
        return jsonify({
            'items': item_counts,
            'total_items': total_items,
            'unique_items': len(item_counts)
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500
@app.route('/api/admin/update-item', methods=['POST'])
def update_item_count():
    """API endpoint for admin to update item counts in storage"""
    if 'user_id' not in session:
        return jsonify({'success': False, 'message': 'Not authenticated'}), 401
    
    try:
        user_id = session['user_id']
        user_ref = db.reference(f'users/{user_id}')
        user_data = user_ref.get()
        
        if not user_data or not is_admin(user_data.get('email', '')):
            return jsonify({'success': False, 'message': 'Access denied'}), 403
        
        # Get the JSON data from request
        data = request.get_json()
        if not data:
            return jsonify({'success': False, 'message': 'No data provided'}), 400
        
        item_name = data.get('item')
        new_count = data.get('count')
        
        # Validate input
        if not item_name or new_count is None:
            return jsonify({'success': False, 'message': 'Item name and count are required'}), 400
        
        if not isinstance(new_count, int) or new_count < 0:
            return jsonify({'success': False, 'message': 'Count must be a non-negative integer'}), 400
        
        # Update item count in Firebase storage
        storage_ref = db.reference('storage')
        
        # Check if item exists
        current_items = storage_ref.get() or {}
        
        if new_count == 0:
            # Remove item if count is 0
            if item_name in current_items:
                storage_ref.child(item_name).delete()
                return jsonify({
                    'success': True, 
                    'message': f'{item_name} removed from storage',
                    'action': 'deleted'
                })
            else:
                return jsonify({'success': False, 'message': 'Item not found'}), 404
        else:
            # Update or create item with new count
            storage_ref.child(item_name).set(new_count)
            
            action = 'updated' if item_name in current_items else 'created'
            return jsonify({
                'success': True, 
                'message': f'{item_name} {action} successfully',
                'action': action,
                'new_count': new_count
            })
        
    except Exception as e:
        print(f"Error updating item count: {e}")
        return jsonify({'success': False, 'message': f'Server error: {str(e)}'}), 500
def get_storage_items():
    """Get item counts from Firebase storage node"""
    try:
        storage_ref = db.reference('storage')
        storage_data = storage_ref.get()
        
        if storage_data:
            # Convert to regular dict and sort by count (descending)
            sorted_items = dict(sorted(storage_data.items(), key=lambda x: x[1], reverse=True))
            return sorted_items
        else:
            return {}
        
    except Exception as e:
        print(f"Error fetching storage items: {e}")
        return {}
    
@app.route('/purchase-history')
def purchase_history():
    if 'user_id' not in session:
        flash('Please log in to access your account.', 'error')
        return redirect(url_for('login'))
    
    # Get purchase history from Firebase
    user_id = session['user_id']
    purchases = get_user_purchase_history(user_id)
    
    # Auto-categorize items that don't have categories
    for purchase in purchases:
        if purchase.get('category') == 'General' or not purchase.get('category'):
            purchase['category'] = categorize_item(purchase['item'])
    
    return render_template('purchase_history.html', 
                         purchases=purchases,
                         username=session.get('username', 'User'))

@app.route('/api/purchase-history')
def api_purchase_history():
    """API endpoint to get purchase history as JSON"""
    if 'user_id' not in session:
        return jsonify({'error': 'Not authenticated'}), 401
    
    try:
        user_id = session['user_id']
        purchases = get_user_purchase_history(user_id)
        
        # Auto-categorize items
        for purchase in purchases:
            if purchase.get('category') == 'General' or not purchase.get('category'):
                purchase['category'] = categorize_item(purchase['item'])
        
        # Calculate total spent using the 'price' field (which is already the total price)
        total_spent = sum(purchase['price'] for purchase in purchases)
        
        return jsonify({
            'purchases': purchases,
            'total_orders': len(purchases),
            'total_spent': total_spent
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/balance')
def get_balance():
    if 'user_id' not in session:
        return jsonify({'error': 'Not authenticated'}), 401
    
    try:
        user_id = session['user_id']
        user_ref = db.reference(f'users/{user_id}')
        user_data = user_ref.get()
        
        if not user_data:
            return jsonify({'error': 'User not found'}), 404
            
        balance = user_data.get('balance', 0)
        return jsonify({'balance': balance})
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/logout')
def logout():
    session.clear()
    flash('You have been logged out successfully.', 'success')
    return redirect(url_for('login'))
    
#kiosk
# Checkout Display Routes
@app.route('/checkout')
def checkout_display():
    """Main checkout display page"""
    return render_template('checkout.html')

@app.route('/checkout/fullscreen')
def checkout_fullscreen():
    """Fullscreen checkout display (useful for kiosk mode)"""
    return render_template('checkout.html')

# API routes for checkout system
@app.route('/api/checkout-data')
def get_checkout_data():
    """API endpoint to get current checkout data"""
    try:
        ref = db.reference('display')
        checkout_data = ref.get()
        
        over_ref = db.reference('over')
        over_status = over_ref.get()
        
        response_data = {
            'display': checkout_data,
            'over': over_status,
            'status': 'success'
        }
        
        return jsonify(response_data)
    
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/api/checkout-items')
def get_checkout_items():
    """API endpoint to get current scanned items"""
    try:
        ref = db.reference('display/items')
        items = ref.get()
        
        return jsonify({
            'items': items or {},
            'status': 'success'
        })
    
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/api/checkout-total')
def get_checkout_total():
    """API endpoint to get current total"""
    try:
        ref = db.reference('display/total')
        total = ref.get()
        
        return jsonify({
            'total': total or 0,
            'status': 'success'
        })
    
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/api/checkout-status')
def get_checkout_status():
    """API endpoint to get payment status"""
    try:
        ref = db.reference('over')
        over_status = ref.get()
        
        status_message = "Scan your card here to pay" if over_status == 1 else "Scanning in progress..."
        
        return jsonify({
            'over': over_status,
            'message': status_message,
            'status': 'success'
        })
    
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500
if __name__ == '__main__':
    app.run(debug=True)
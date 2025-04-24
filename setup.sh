#!/bin/bash
# setup.sh - Setup script for Systems Software Continuous Assessment 2
#
# This script performs the following setup operations:
# - Creates user groups for Manufacturing and Distribution departments
# - Creates test user accounts and assigns them to groups
# - Creates and configures directories with appropriate permissions
# - Sets up the necessary environment for the file transfer system

# Exit on any error
set -e

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root" 
    exit 1
fi

echo "Setting up environment for file transfer system..."

# Create groups
echo "Creating groups..."
groupadd -f manufacturing
groupadd -f distribution

# Create users for manufacturing department
echo "Creating users for Manufacturing department..."
useradd -m -G manufacturing mfg_user1 2>/dev/null || true
useradd -m -G manufacturing mfg_user2 2>/dev/null || true

# Create users for distribution department
echo "Creating users for Distribution department..."
useradd -m -G distribution dist_user1 2>/dev/null || true

# Create user with access to both departments (for testing)
echo "Creating user with access to both departments..."
useradd -m -G manufacturing,distribution admin_user 2>/dev/null || true

# Set passwords for the users (for testing purposes only)
echo "Setting user passwords..."
echo "mfg_user1:password1" | chpasswd
echo "mfg_user2:password2" | chpasswd
echo "dist_user1:password3" | chpasswd
echo "admin_user:password4" | chpasswd

# Create directories if they don't exist
echo "Creating directories..."
mkdir -p Manufacturing
mkdir -p Distribution

# Set directory permissions
echo "Setting directory permissions..."
chown root:manufacturing Manufacturing
chown root:distribution Distribution

# Set directory permissions to 770 (rwxrwx---)
# This allows users in the group to read, write, and execute
chmod 770 Manufacturing
chmod 770 Distribution

echo "Setup completed successfully!"
echo "Created users:"
echo "- mfg_user1 (Manufacturing)"
echo "- mfg_user2 (Manufacturing)"
echo "- dist_user1 (Distribution)"
echo "- admin_user (Manufacturing and Distribution)"
echo ""
echo "Created directories:"
echo "- Manufacturing (owned by group: manufacturing)"
echo "- Distribution (owned by group: distribution)"
echo ""
echo "You can now compile and run the application using 'make all'"
echo "To clean up use 'make uninstall' before running this script again"
# Dinari Blockchain - Complete Setup & Testing Guide

**Version:** 1.0.0-alpha
**Date:** 2025-10-30
**Status:** Development/Testnet Ready

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Local Development Setup](#local-development-setup)
3. [Docker Deployment](#docker-deployment)
4. [Azure Cloud Deployment](#azure-cloud-deployment)
5. [Testing with Postman](#testing-with-postman)
6. [Systemd Service Setup](#systemd-service-setup)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

- **Git**: Version 2.30+
- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: Version 3.15+
- **OpenSSL**: Version 1.1.1+ or 3.0+
- **Docker** (for containerized deployment): Version 20.10+
- **Azure CLI** (for Azure deployment): Latest version

### System Requirements

**Minimum:**
- CPU: 2 cores
- RAM: 4 GB
- Disk: 20 GB free space
- Network: Broadband internet connection

**Recommended:**
- CPU: 4+ cores
- RAM: 8+ GB
- Disk: 100+ GB SSD
- Network: Stable high-speed connection

---

## 1. Local Development Setup

### Step 1: Clone the Repository

```bash
# Clone from GitHub
git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git
cd dinari-blockchain-hub

# Checkout the correct branch
git checkout claude/dinari-blockchain-cpp-implementation-011CUdKvaxLVvH9HpLbvx2Tq
```

### Step 2: Install Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    pkg-config \
    git
```

#### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl pkg-config
```

#### Windows (Visual Studio 2022)

1. Install Visual Studio 2022 with C++ development tools
2. Install CMake from https://cmake.org/download/
3. Install OpenSSL from https://slproweb.com/products/Win32OpenSSL.html

### Step 3: Build the Project

```bash
# Create build directory
mkdir build
cd build

# Configure (Release mode)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with all CPU cores
cmake --build . -j$(nproc)

# The binary will be at: build/dinarid
```

### Step 4: Configure

```bash
# Create data directory
mkdir -p ~/.dinari/mainnet

# Copy and edit configuration
cp config/mainnet.conf ~/.dinari/mainnet.conf

# Edit the config file
nano ~/.dinari/mainnet.conf
```

**Important:** Change default RPC credentials!

```ini
# CHANGE THESE!
rpcuser=YOUR_CUSTOM_USERNAME
rpcpassword=YOUR_SECURE_PASSWORD

# Network settings
testnet=0
port=9333
rpcport=9334

# Data directory
datadir=~/.dinari/mainnet

# Logging
loglevel=info
printtoconsole=1
```

### Step 5: Run the Node

```bash
# Run in foreground (for testing)
./build/dinarid --config=~/.dinari/mainnet.conf

# Run in background
./build/dinarid --config=~/.dinari/mainnet.conf --daemon

# Enable mining
./build/dinarid --config=~/.dinari/mainnet.conf --mining --miningthreads=4

# Testnet
./build/dinarid --testnet --config=~/.dinari/testnet.conf
```

### Step 6: Verify It's Running

```bash
# Check logs
tail -f ~/.dinari/mainnet/debug.log

# Test RPC (in another terminal)
curl --user YOUR_USERNAME:YOUR_PASSWORD \
     --data-binary '{"jsonrpc":"2.0","id":"test","method":"getblockcount","params":[]}' \
     -H 'content-type: text/plain;' \
     http://127.0.0.1:9334/
```

Expected response:
```json
{
    "jsonrpc": "2.0",
    "result": 0,
    "id": "test"
}
```

---

## 2. Docker Deployment

### Step 1: Install Docker

#### Ubuntu/Debian

```bash
# Remove old versions
sudo apt-get remove docker docker-engine docker.io containerd runc

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add your user to docker group
sudo usermod -aG docker $USER

# Log out and back in for group changes to take effect
```

#### macOS/Windows

Download and install Docker Desktop from https://www.docker.com/products/docker-desktop

### Step 2: Configure Environment

```bash
# Copy example environment file
cp .env.example .env

# Edit configuration
nano .env
```

Example `.env`:

```bash
# RPC Authentication
RPC_USER=yourusername
RPC_PASSWORD=YourSecurePassword123!

# Mining (if using mining profile)
MINING_ADDRESS=D1YourDinariAddressHere
MINING_THREADS=4

# Network
DINARI_NETWORK=mainnet
```

### Step 3: Build Docker Image

```bash
# Build the image
docker build -t dinari-blockchain:latest .

# Verify image was created
docker images | grep dinari
```

### Step 4: Run with Docker Compose

#### Basic Node (Mainnet)

```bash
# Start the node
docker-compose up -d dinari-node

# View logs
docker-compose logs -f dinari-node

# Stop the node
docker-compose down
```

#### Testnet Node

```bash
# Start testnet
docker-compose --profile testnet up -d dinari-testnet

# View logs
docker-compose logs -f dinari-testnet
```

#### Mining Node

```bash
# Make sure MINING_ADDRESS is set in .env first!

# Start mining
docker-compose --profile mining up -d dinari-miner

# View logs
docker-compose logs -f dinari-miner

# Check mining stats
docker exec dinari-blockchain cat /data/dinari/debug.log | grep "Mining Hashrate"
```

#### All Services

```bash
# Start everything (mainnet + testnet + mining)
docker-compose --profile testnet --profile mining up -d

# View all logs
docker-compose logs -f
```

### Step 5: Verify Docker Deployment

```bash
# Check container status
docker ps

# Test RPC (with proper auth from .env)
curl --user yourusername:YourSecurePassword123! \
     --data-binary '{"jsonrpc":"2.0","id":"test","method":"getblockchaininfo","params":[]}' \
     -H 'content-type: text/plain;' \
     http://127.0.0.1:9334/

# Enter container shell
docker exec -it dinari-blockchain /bin/bash

# View blockchain data
docker exec dinari-blockchain ls -lh /data/dinari/
```

### Step 6: Backup and Persistence

```bash
# Backup blockchain data
docker run --rm -v dinari-data:/data -v $(pwd):/backup \
    alpine tar czf /backup/dinari-blockchain-backup.tar.gz /data

# Restore from backup
docker run --rm -v dinari-data:/data -v $(pwd):/backup \
    alpine sh -c "cd /data && tar xzf /backup/dinari-blockchain-backup.tar.gz --strip 1"
```

---

## 3. Azure Cloud Deployment

### Method 1: Azure Virtual Machine (Recommended)

#### Step 1: Prerequisites

```bash
# Install Azure CLI (if not already installed)
curl -sL https://aka.ms/InstallAzureCLIDeb | sudo bash

# Login to Azure
az login

# Set default subscription (if you have multiple)
az account set --subscription "YOUR_SUBSCRIPTION_NAME"
```

#### Step 2: Run Deployment Script

```bash
# Make script executable
chmod +x azure/deploy.sh

# Set environment variables (optional)
export RESOURCE_GROUP="dinari-blockchain-rg"
export LOCATION="eastus"
export VM_NAME="dinari-node-01"
export VM_SIZE="Standard_D4s_v3"

# Run deployment
./azure/deploy.sh
```

The script will:
1. Create resource group
2. Create virtual network
3. Configure firewall (NSG)
4. Create public IP
5. Create VM with 128GB OS disk + 512GB data disk
6. Display connection information

#### Step 3: Connect and Setup

```bash
# SSH into the VM (IP from deployment output)
ssh dinari@YOUR_PUBLIC_IP

# Install Docker
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER

# Logout and login again
exit
ssh dinari@YOUR_PUBLIC_IP

# Clone repository
git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git
cd dinari-blockchain-hub

# Configure environment
cp .env.example .env
nano .env  # Edit with your settings

# Start node
docker-compose up -d

# Verify
docker ps
docker-compose logs -f
```

#### Step 4: Configure Domain (Optional)

```bash
# Create DNS record pointing to your public IP

# Update NSG to allow HTTPS if needed
az network nsg rule create \
    --resource-group dinari-blockchain-rg \
    --nsg-name dinari-nsg \
    --name AllowHTTPS \
    --priority 1003 \
    --destination-port-ranges 443 \
    --protocol Tcp \
    --access Allow
```

### Method 2: Azure Container Instances (Simpler, Lower Cost)

#### Step 1: Prepare Configuration

```bash
# Edit azure/container-deploy.yml
nano azure/container-deploy.yml

# Update these values:
# - YOUR_RPC_USER
# - YOUR_RPC_PASSWORD
# - YOUR_STORAGE_ACCOUNT
# - YOUR_STORAGE_KEY
```

#### Step 2: Create Storage Account

```bash
# Create storage account for persistent data
az storage account create \
    --name dinariblockchainstore \
    --resource-group dinari-blockchain-rg \
    --location eastus \
    --sku Standard_LRS

# Create file share
az storage share create \
    --name dinari-blockchain-data \
    --account-name dinariblockchainstore

# Get storage key
az storage account keys list \
    --account-name dinariblockchainstore \
    --query "[0].value" \
    --output tsv
```

#### Step 3: Deploy Container

```bash
# Deploy using YAML file
az container create \
    --resource-group dinari-blockchain-rg \
    --file azure/container-deploy.yml

# Check status
az container show \
    --resource-group dinari-blockchain-rg \
    --name dinari-blockchain-container \
    --query instanceView.state

# Get public IP
az container show \
    --resource-group dinari-blockchain-rg \
    --name dinari-blockchain-container \
    --query ipAddress.ip \
    --output tsv

# View logs
az container logs \
    --resource-group dinari-blockchain-rg \
    --name dinari-blockchain-container
```

---

## 4. Testing with Postman

### Step 1: Import Collection

1. Open Postman
2. Click **Import** button
3. Select `postman/Dinari_Blockchain_API.postman_collection.json`
4. Collection will appear in left sidebar

### Step 2: Configure Authentication

1. Click on the **Dinari Blockchain API** collection
2. Go to **Authorization** tab
3. Type: **Basic Auth**
4. Username: Your RPC username (from .env or config)
5. Password: Your RPC password
6. Click **Save**

### Step 3: Update Variables

1. Click on collection → **Variables** tab
2. Update these variables:
   - `baseUrl`: `http://localhost:9334` (or your server IP)
   - `rpcUser`: Your username
   - `rpcPassword`: Your password
3. Click **Save**

### Step 4: Test Requests

#### Test 1: Get Block Count

1. Expand **Blockchain Queries** → **Get Block Count**
2. Click **Send**
3. Expected response:

```json
{
    "jsonrpc": "2.0",
    "result": 0,
    "id": 1
}
```

#### Test 2: Get Blockchain Info

1. Select **Get Blockchain Info**
2. Click **Send**
3. Expected response:

```json
{
    "jsonrpc": "2.0",
    "result": {
        "chain": "main",
        "blocks": 0,
        "headers": 0,
        "bestblockhash": "...",
        "difficulty": 486604799,
        "chainwork": "..."
    },
    "id": 1
}
```

#### Test 3: List Blocks (Blockchain Explorer)

1. Expand **Blockchain Explorer** → **List Blocks**
2. Edit request body to specify range:

```json
{
    "jsonrpc": "2.0",
    "method": "listblocks",
    "params": [0, 10],
    "id": 1
}
```

3. Click **Send**
4. Expected response: Array of blocks with full details

#### Test 4: Get Transaction (Blockchain Explorer)

1. Select **Get Transaction by Hash**
2. Replace `your_transaction_hash_here` with actual TX hash
3. Click **Send**
4. Expected: Full transaction details with inputs, outputs, confirmations

#### Test 5: Wallet Operations

```json
// Get new address
{
    "jsonrpc": "2.0",
    "method": "getnewaddress",
    "params": ["default"],
    "id": 1
}

// Get balance
{
    "jsonrpc": "2.0",
    "method": "getbalance",
    "params": [],
    "id": 1
}

// Send transaction
{
    "jsonrpc": "2.0",
    "method": "sendtoaddress",
    "params": ["D1RecipientAddressHere", 100000000],
    "id": 1
}
```

### Common Postman Errors

**401 Unauthorized**
- Check username/password in Authorization tab
- Verify RPC credentials in .env or config file
- Restart node after changing credentials

**Connection Refused**
- Check if node is running: `docker ps` or `ps aux | grep dinarid`
- Verify port: Default RPC port is 9334 (mainnet) or 19334 (testnet)
- Check firewall rules

**Method not found**
- Check method name spelling
- Ensure you're using correct API version
- Some methods require wallet to be enabled

---

## 5. Systemd Service Setup

For production deployment on Linux servers.

### Step 1: Create Service File

```bash
sudo nano /etc/systemd/system/dinari.service
```

### Step 2: Add Configuration

```ini
[Unit]
Description=Dinari Blockchain Node
After=network.target

[Service]
Type=simple
User=dinari
Group=dinari
WorkingDirectory=/opt/dinari-blockchain
ExecStart=/opt/dinari-blockchain/dinarid \
    --config=/etc/dinari/mainnet.conf \
    --datadir=/var/lib/dinari \
    --daemon=0

# Restart policy
Restart=on-failure
RestartSec=10
StartLimitInterval=200
StartLimitBurst=10

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/dinari

[Install]
WantedBy=multi-user.target
```

### Step 3: Create User and Directories

```bash
# Create dinari user
sudo useradd -r -s /bin/false dinari

# Create directories
sudo mkdir -p /opt/dinari-blockchain
sudo mkdir -p /var/lib/dinari
sudo mkdir -p /etc/dinari

# Copy binary
sudo cp build/dinarid /opt/dinari-blockchain/

# Copy config
sudo cp config/mainnet.conf /etc/dinari/

# Set permissions
sudo chown -R dinari:dinari /var/lib/dinari
sudo chown -R dinari:dinari /opt/dinari-blockchain
sudo chmod 755 /opt/dinari-blockchain/dinarid
```

### Step 4: Enable and Start Service

```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service to start on boot
sudo systemctl enable dinari

# Start service
sudo systemctl start dinari

# Check status
sudo systemctl status dinari
```

### Step 5: Manage Service

```bash
# Start
sudo systemctl start dinari

# Stop
sudo systemctl stop dinari

# Restart
sudo systemctl restart dinari

# View logs
sudo journalctl -u dinari -f

# View recent logs
sudo journalctl -u dinari --since "1 hour ago"
```

---

## 6. Troubleshooting

### Issue: Node Won't Start

**Symptoms:**
- Binary exits immediately
- "Fatal error" in logs

**Solutions:**

```bash
# 1. Check dependencies
ldd build/dinarid

# 2. Check config file syntax
cat ~/.dinari/mainnet.conf

# 3. Check data directory permissions
ls -l ~/.dinari/

# 4. Check if port is already in use
sudo netstat -tlnp | grep 9334

# 5. Run with debug logging
./build/dinarid --loglevel=debug
```

### Issue: Cannot Connect to RPC

**Symptoms:**
- "Connection refused"
- 401 Unauthorized

**Solutions:**

```bash
# 1. Verify node is running
ps aux | grep dinarid

# 2. Check RPC is enabled
grep "server" ~/.dinari/mainnet.conf

# 3. Test locally first
curl --user user:pass http://127.0.0.1:9334/

# 4. Check firewall
sudo ufw status
sudo iptables -L

# 5. Verify credentials
cat ~/.dinari/mainnet.conf | grep rpc
```

### Issue: High CPU Usage

**Symptoms:**
- 100% CPU usage
- System slowdown

**Solutions:**

```bash
# 1. Reduce mining threads
# Edit config: miningthreads=2

# 2. Lower mining priority
nice -n 19 ./build/dinarid --mining

# 3. Disable mining temporarily
# Remove --mining flag

# 4. Check for runaway processes
top -p $(pgrep dinarid)
```

### Issue: Blockchain Sync Slow

**Symptoms:**
- Block count not increasing
- No peer connections

**Solutions:**

```bash
# 1. Check peer connections
# In RPC:
{"method":"getblockchaininfo"}

# 2. Check network connectivity
ping 8.8.8.8

# 3. Check if P2P port is open
sudo netstat -tlnp | grep 9333

# 4. Add manual peers (if needed)
# Edit config: addnode=peer_ip:9333

# 5. Restart with fresh peer database
rm ~/.dinari/mainnet/peers.dat
```

### Issue: Out of Memory

**Symptoms:**
- Node crashes
- "Cannot allocate memory" errors

**Solutions:**

```bash
# 1. Check memory usage
free -h

# 2. Add swap space
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 3. Reduce mempool size
# Edit config: maxmempool=100

# 4. Upgrade to machine with more RAM
```

### Issue: Docker Container Exits

**Symptoms:**
- Container status: Exited
- Repeated restarts

**Solutions:**

```bash
# 1. Check logs
docker logs dinari-blockchain

# 2. Check exit code
docker inspect dinari-blockchain | grep ExitCode

# 3. Run interactively to debug
docker run -it --rm dinari-blockchain:latest /bin/bash

# 4. Check volume permissions
docker exec dinari-blockchain ls -l /data/dinari

# 5. Recreate container
docker-compose down -v
docker-compose up -d
```

### Getting Help

If you're still stuck:

1. Check logs:
   - Local: `~/.dinari/mainnet/debug.log`
   - Docker: `docker logs dinari-blockchain`
   - Systemd: `sudo journalctl -u dinari`

2. GitHub Issues:
   - https://github.com/EmekaIwuagwu/dinari-blockchain-hub/issues

3. Security Issues:
   - Email: security@dinariblockchain.com (if available)

---

## Security Reminders

⚠️ **CRITICAL SECURITY WARNINGS** ⚠️

1. **Change Default Credentials:**
   - Default RPC credentials are `dinariuser:dinaripass`
   - MUST be changed in production!

2. **RPC Authentication:**
   - Current implementation has bypass (see Security Audit)
   - Do NOT expose RPC to internet until fixed
   - Use firewall to restrict RPC access

3. **Wallet Security:**
   - Encrypt wallet with strong passphrase
   - Backup wallet regularly
   - Never share private keys

4. **Network Security:**
   - Keep software updated
   - Use firewall
   - Monitor access logs

5. **Production Deployment:**
   - Complete security audit first
   - Fix all CRITICAL and HIGH issues
   - Use TLS/SSL for RPC
   - Implement proper monitoring

**See:** `SECURITY_AUDIT_SUMMARY.md` for full security assessment

---

## Next Steps

After successful setup:

1. **Test Everything:**
   - Mine some test blocks
   - Create transactions
   - Test all RPC commands
   - Verify backups work

2. **Monitor:**
   - Set up monitoring (Prometheus/Grafana)
   - Configure alerts
   - Monitor disk space
   - Track sync status

3. **Secure:**
   - Review security audit
   - Fix critical issues
   - Implement proper authentication
   - Add TLS/SSL

4. **Scale:**
   - Add more nodes
   - Set up load balancer
   - Configure backup nodes
   - Plan for growth

---

## Appendix: Quick Command Reference

```bash
# Build
cmake --build . -j$(nproc)

# Run mainnet
./dinarid --config=mainnet.conf

# Run testnet
./dinarid --testnet

# Run with mining
./dinarid --mining --miningthreads=4

# Docker build
docker build -t dinari:latest .

# Docker run
docker-compose up -d

# Check status
docker ps
systemctl status dinari

# View logs
tail -f ~/.dinari/mainnet/debug.log
docker logs -f dinari-blockchain
journalctl -u dinari -f

# RPC test
curl --user user:pass \
  --data-binary '{"method":"getblockcount"}' \
  http://localhost:9334/

# Backup
tar czf dinari-backup.tar.gz ~/.dinari/
docker run --rm -v dinari-data:/data -v $(pwd):/backup alpine tar czf /backup/backup.tar.gz /data

# Restore
tar xzf dinari-backup.tar.gz -C ~/.dinari/
```

---

**End of Setup Guide**

For questions or issues, please refer to:
- Security Audit: `SECURITY_AUDIT_SUMMARY.md`
- Detailed Audit: `docs/COMPREHENSIVE_SECURITY_AUDIT_REPORT.md`
- GitHub Issues: https://github.com/EmekaIwuagwu/dinari-blockchain-hub/issues

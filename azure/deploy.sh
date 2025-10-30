#!/bin/bash
# Dinari Blockchain - Azure Deployment Script

set -e

echo "========================================="
echo "Dinari Blockchain - Azure Deployment"
echo "========================================="

# Configuration
RESOURCE_GROUP="${RESOURCE_GROUP:-dinari-blockchain-rg}"
LOCATION="${LOCATION:-eastus}"
VM_NAME="${VM_NAME:-dinari-node}"
VM_SIZE="${VM_SIZE:-Standard_D4s_v3}"
IMAGE="Ubuntu2204"
ADMIN_USERNAME="dinari"

# Check Azure CLI
if ! command -v az &> /dev/null; then
    echo "Error: Azure CLI not found. Please install it first."
    exit 1
fi

# Login check
echo "Checking Azure login status..."
if ! az account show &> /dev/null; then
    echo "Please login to Azure:"
    az login
fi

# Create Resource Group
echo "Creating resource group: $RESOURCE_GROUP in $LOCATION..."
az group create --name $RESOURCE_GROUP --location $LOCATION

# Create Virtual Network
echo "Creating virtual network..."
az network vnet create \
    --resource-group $RESOURCE_GROUP \
    --name dinari-vnet \
    --address-prefix 10.0.0.0/16 \
    --subnet-name dinari-subnet \
    --subnet-prefix 10.0.1.0/24

# Create Network Security Group
echo "Creating network security group..."
az network nsg create \
    --resource-group $RESOURCE_GROUP \
    --name dinari-nsg

# Add NSG rules
echo "Adding firewall rules..."
# SSH
az network nsg rule create \
    --resource-group $RESOURCE_GROUP \
    --nsg-name dinari-nsg \
    --name AllowSSH \
    --priority 1000 \
    --source-address-prefixes '*' \
    --destination-port-ranges 22 \
    --protocol Tcp \
    --access Allow

# P2P Port
az network nsg rule create \
    --resource-group $RESOURCE_GROUP \
    --nsg-name dinari-nsg \
    --name AllowP2P \
    --priority 1001 \
    --source-address-prefixes '*' \
    --destination-port-ranges 9333 \
    --protocol Tcp \
    --access Allow

# RPC Port (restrict to your IP for security)
YOUR_IP=$(curl -s ifconfig.me)
az network nsg rule create \
    --resource-group $RESOURCE_GROUP \
    --nsg-name dinari-nsg \
    --name AllowRPC \
    --priority 1002 \
    --source-address-prefixes "$YOUR_IP/32" \
    --destination-port-ranges 9334 \
    --protocol Tcp \
    --access Allow

# Create Public IP
echo "Creating public IP..."
az network public-ip create \
    --resource-group $RESOURCE_GROUP \
    --name dinari-public-ip \
    --allocation-method Static \
    --sku Standard

# Create Network Interface
echo "Creating network interface..."
az network nic create \
    --resource-group $RESOURCE_GROUP \
    --name dinari-nic \
    --vnet-name dinari-vnet \
    --subnet dinari-subnet \
    --network-security-group dinari-nsg \
    --public-ip-address dinari-public-ip

# Create VM
echo "Creating virtual machine..."
az vm create \
    --resource-group $RESOURCE_GROUP \
    --name $VM_NAME \
    --location $LOCATION \
    --nics dinari-nic \
    --image $IMAGE \
    --size $VM_SIZE \
    --admin-username $ADMIN_USERNAME \
    --generate-ssh-keys \
    --storage-sku Premium_LRS \
    --os-disk-size-gb 128 \
    --data-disk-sizes-gb 512

# Get Public IP
PUBLIC_IP=$(az network public-ip show \
    --resource-group $RESOURCE_GROUP \
    --name dinari-public-ip \
    --query ipAddress \
    --output tsv)

echo ""
echo "========================================="
echo "Deployment Complete!"
echo "========================================="
echo "VM Name: $VM_NAME"
echo "Public IP: $PUBLIC_IP"
echo "SSH Command: ssh $ADMIN_USERNAME@$PUBLIC_IP"
echo ""
echo "Next steps:"
echo "1. SSH into the VM"
echo "2. Install Docker: curl -fsSL https://get.docker.com | sh"
echo "3. Clone repository: git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git"
echo "4. Configure .env file"
echo "5. Run: docker-compose up -d"
echo "========================================="

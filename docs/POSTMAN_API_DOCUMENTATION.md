# Dinari Blockchain - Complete API Documentation for Postman

**Version:** 1.0.0
**Last Updated:** 2025-10-30
**API Type:** JSON-RPC 2.0
**Authentication:** HTTP Basic Auth

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Authentication](#authentication)
3. [API Structure](#api-structure)
4. [Blockchain Queries](#blockchain-queries)
5. [Blockchain Explorer](#blockchain-explorer)
6. [Wallet Operations](#wallet-operations)
7. [Mempool Operations](#mempool-operations)
8. [Mining Operations](#mining-operations)
9. [Network Information](#network-information)
10. [Error Codes](#error-codes)
11. [Best Practices](#best-practices)

---

## Quick Start

### Import Collection

1. Open Postman
2. Click **Import** → **File**
3. Select `postman/Dinari_Blockchain_API.postman_collection.json`
4. Collection appears in left sidebar

### Configure Environment

1. Click **Dinari Blockchain API** collection
2. Go to **Variables** tab
3. Set variables:
   - `baseUrl`: `http://localhost:9334` (or your server)
   - `rpcUser`: Your RPC username (default: `dinariuser`)
   - `rpcPassword`: Your RPC password (default: `dinaripass`)

⚠️ **SECURITY WARNING**: Change default credentials before deployment!

### First Test

Send **Get Block Count** request:

```json
POST {{baseUrl}}/
Authorization: Basic {{rpcUser}}:{{rpcPassword}}
Content-Type: application/json

{
    "jsonrpc": "2.0",
    "method": "getblockcount",
    "params": [],
    "id": 1
}
```

Expected Response:
```json
{
    "jsonrpc": "2.0",
    "result": 0,
    "id": 1
}
```

---

## Authentication

### HTTP Basic Authentication

All API requests require HTTP Basic Authentication.

**Header Format:**
```
Authorization: Basic base64(username:password)
```

**Postman Configuration:**

1. Select collection or request
2. Go to **Authorization** tab
3. Type: **Basic Auth**
4. Username: Your RPC username
5. Password: Your RPC password
6. Click **Save**

**Example:**

```bash
# Command line (curl)
curl --user dinariuser:dinaripass \
     --data-binary '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}' \
     -H 'content-type: application/json' \
     http://localhost:9334/
```

### Security Considerations

- ✅ Use HTTPS in production (TLS/SSL)
- ✅ Change default credentials
- ✅ Restrict RPC access by IP
- ✅ Use firewall rules
- ✅ Enable rate limiting
- ❌ Never expose RPC to public internet without security

---

## API Structure

### JSON-RPC 2.0 Format

**Request:**
```json
{
    "jsonrpc": "2.0",        // Protocol version (always "2.0")
    "method": "method_name", // RPC method name
    "params": [],            // Parameters array
    "id": 1                  // Request ID (can be number or string)
}
```

**Success Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {},            // Result object or value
    "id": 1                  // Matches request ID
}
```

**Error Response:**
```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32600,
        "message": "Invalid Request"
    },
    "id": 1
}
```

---

## Blockchain Queries

### 1. Get Block Count

Returns the current blockchain height.

**Endpoint:** `POST /`

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getblockcount",
    "params": [],
    "id": "blockcount"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": 12345,
    "id": "blockcount"
}
```

**Parameters:** None

**Returns:** `number` - Current block height

**Use Cases:**
- Check blockchain sync status
- Determine network height
- Monitor block production

---

### 2. Get Block Hash

Returns the block hash at a specific height.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getblockhash",
    "params": [100],
    "id": "blockhash"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f",
    "id": "blockhash"
}
```

**Parameters:**
- `height` (number, required): Block height

**Returns:** `string` - Block hash (hex)

**Errors:**
- `-8`: Block height out of range

---

### 3. Get Block

Returns detailed information about a block.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getblock",
    "params": ["000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f", true],
    "id": "block"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "hash": "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f",
        "height": 0,
        "version": 1,
        "merkleroot": "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b",
        "time": 1231006505,
        "bits": 486604799,
        "nonce": 2083236893,
        "tx_count": 1,
        "size": 285,
        "chainwork": "0000000000000000000000000000000000000000000000000000000100010001"
    },
    "id": "block"
}
```

**Parameters:**
- `blockhash` (string, required): Block hash (hex)
- `verbose` (boolean, optional): Return JSON object (true) or hex string (false). Default: true

**Returns:** `object` - Block details

**Fields:**
- `hash`: Block hash
- `height`: Block height
- `version`: Block version
- `merkleroot`: Merkle root of transactions
- `time`: Block timestamp (Unix time)
- `bits`: Difficulty target
- `nonce`: Proof-of-work nonce
- `tx_count`: Number of transactions
- `size`: Block size in bytes
- `chainwork`: Total chain work

---

### 4. Get Best Block Hash

Returns the hash of the best (tip) block.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getbestblockhash",
    "params": [],
    "id": "bestblock"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "00000000000000000001a0e7d6e0c5c1e8e9f2f3d4c5b6a7e8f9d0c1b2a3e4f5",
    "id": "bestblock"
}
```

**Parameters:** None

**Returns:** `string` - Best block hash

---

### 5. Get Difficulty

Returns the current proof-of-work difficulty.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getdifficulty",
    "params": [],
    "id": "difficulty"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": 486604799,
    "id": "difficulty"
}
```

**Parameters:** None

**Returns:** `number` - Current difficulty

---

### 6. Get Blockchain Info

Returns comprehensive blockchain information.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getblockchaininfo",
    "params": [],
    "id": "chaininfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "chain": "main",
        "blocks": 12345,
        "headers": 12345,
        "bestblockhash": "00000000000000000001...",
        "difficulty": 486604799,
        "chainwork": "000000000000000000000..."
    },
    "id": "chaininfo"
}
```

**Parameters:** None

**Returns:** `object` - Blockchain info

---

### 7. Get TX Out

Returns details about an unspent transaction output (UTXO).

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "gettxout",
    "params": ["abc123def456...", 0],
    "id": "txout"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "bestblock": "000...",
        "confirmations": 100,
        "value": 5000000000,
        "height": 12245,
        "coinbase": false
    },
    "id": "txout"
}
```

**Parameters:**
- `txid` (string, required): Transaction ID
- `n` (number, required): Output index

**Returns:** `object` - UTXO details or `null` if spent

---

## Blockchain Explorer

### 8. Get Raw Transaction

Returns transaction details by hash. **NEW EXPLORER API**

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getrawtransaction",
    "params": ["abc123def456789...", true],
    "id": "rawtx"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "txid": "abc123def456789...",
        "version": 1,
        "locktime": 0,
        "size": 225,
        "coinbase": false,
        "vin": [
            {
                "prevout": {
                    "hash": "def456...",
                    "n": 0
                },
                "scriptSig": "...",
                "sequence": 4294967295
            }
        ],
        "vout": [
            {
                "value": 1000000000,
                "n": 0,
                "scriptPubKey": "..."
            }
        ],
        "blockhash": "000...",
        "blockheight": 100,
        "confirmations": 50,
        "blocktime": 1735567200
    },
    "id": "rawtx"
}
```

**Parameters:**
- `txid` (string, required): Transaction hash
- `verbose` (boolean, optional): Return JSON (true) or hex (false). Default: true

**Returns:** `object` - Transaction details

**Fields:**
- `txid`: Transaction ID
- `version`: Transaction version
- `locktime`: Lock time
- `size`: Transaction size in bytes
- `coinbase`: Is coinbase transaction
- `vin`: Array of inputs
- `vout`: Array of outputs
- `blockhash`: Block containing this transaction
- `blockheight`: Block height
- `confirmations`: Number of confirmations
- `blocktime`: Block timestamp

**Use Cases:**
- Blockchain explorers
- Transaction tracking
- Payment verification
- Audit trails

---

### 9. List Blocks

Returns a list of blocks with full details. **NEW EXPLORER API**

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "listblocks",
    "params": [0, 10],
    "id": "listblocks"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": [
        {
            "height": 0,
            "hash": "000000000019d668...",
            "timestamp": 1231006505,
            "time": "2009-01-03T18:15:05Z",
            "tx_count": 1,
            "size": 285,
            "bits": 486604799,
            "nonce": 2083236893,
            "merkleroot": "4a5e1e4baab...",
            "transactions": [
                "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"
            ],
            "miner": "D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6",
            "confirmations": 12346
        },
        {
            "height": 1,
            "...": "..."
        }
    ],
    "id": "listblocks"
}
```

**Parameters:**
- `start_height` (number, optional): Starting block height. Default: 0
- `count` (number, optional): Number of blocks to return (1-100). Default: 10

**Returns:** `array` - Array of block objects

**Fields (per block):**
- `height`: Block height
- `hash`: Block hash
- `timestamp`: Unix timestamp
- `time`: ISO 8601 formatted time
- `tx_count`: Number of transactions
- `size`: Block size in bytes
- `bits`: Difficulty target
- `nonce`: PoW nonce
- `merkleroot`: Merkle root
- `transactions`: Array of transaction hashes
- `miner`: Miner's address
- `confirmations`: Number of confirmations

**Use Cases:**
- Block explorers
- Network monitoring
- Chain analysis
- Historical data retrieval

**Rate Limits:**
- Maximum 100 blocks per request
- Consider pagination for large ranges

---

## Wallet Operations

### 10. Get New Address

Generates a new receiving address.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getnewaddress",
    "params": ["default"],
    "id": "newaddr"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6P7Q8r9S0",
    "id": "newaddr"
}
```

**Parameters:**
- `label` (string, optional): Address label. Default: ""

**Returns:** `string` - New Dinari address (starts with 'D')

---

### 11. Get Balance

Returns the wallet balance.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getbalance",
    "params": [],
    "id": "balance"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "confirmed": 10000000000,
        "unconfirmed": 500000000,
        "available": 10000000000
    },
    "id": "balance"
}
```

**Parameters:** None

**Returns:** `object` - Balance information

**Fields:**
- `confirmed`: Confirmed balance (satoshis)
- `unconfirmed`: Unconfirmed balance (satoshis)
- `available`: Available for spending (satoshis)

**Note:** 1 DNT = 100,000,000 satoshis

---

### 12. Send To Address

Sends DNT to an address.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "sendtoaddress",
    "params": ["D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6P7Q8r9S0", 100000000],
    "id": "send"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "abc123def456789012345678901234567890123456789012345678901234abcd",
    "id": "send"
}
```

**Parameters:**
- `address` (string, required): Destination Dinari address
- `amount` (number, required): Amount in satoshis

**Returns:** `string` - Transaction ID

**Errors:**
- `-6`: Insufficient funds
- `-5`: Invalid address
- `-13`: Wallet is locked

⚠️ **Requires unlocked wallet**

---

### 13. List Addresses

Lists all wallet addresses.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "listaddresses",
    "params": [],
    "id": "listaddr"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": [
        {
            "address": "D1...",
            "label": "default",
            "balance": 5000000000
        },
        {
            "address": "D2...",
            "label": "savings",
            "balance": 3000000000
        }
    ],
    "id": "listaddr"
}
```

**Parameters:** None

**Returns:** `array` - Array of address objects

---

### 14. List Transactions

Lists recent wallet transactions.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "listtransactions",
    "params": [10],
    "id": "listtx"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": [
        {
            "txid": "abc123...",
            "category": "receive",
            "amount": 100000000,
            "confirmations": 6,
            "time": 1735567200
        }
    ],
    "id": "listtx"
}
```

**Parameters:**
- `count` (number, optional): Number of transactions. Default: 10

**Returns:** `array` - Array of transaction objects

---

### 15. List Unspent

Lists unspent transaction outputs.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "listunspent",
    "params": [],
    "id": "listunspent"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": [
        {
            "txid": "abc123...",
            "vout": 0,
            "address": "D1...",
            "amount": 100000000,
            "confirmations": 10
        }
    ],
    "id": "listunspent"
}
```

**Parameters:** None

**Returns:** `array` - Array of UTXO objects

---

### 16. Get Wallet Info

Returns wallet state information.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getwalletinfo",
    "params": [],
    "id": "walletinfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "encrypted": true,
        "unlocked": false,
        "tx_count": 25,
        "balance": 10000000000
    },
    "id": "walletinfo"
}
```

---

### 17. Encrypt Wallet

Encrypts the wallet with a passphrase.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "encryptwallet",
    "params": ["MySecurePassphrase123!"],
    "id": "encrypt"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Wallet encrypted. Restart required.",
    "id": "encrypt"
}
```

**Parameters:**
- `passphrase` (string, required): Encryption passphrase (min 8 chars)

⚠️ **Warning:** Cannot be undone. Backup wallet first!

---

### 18. Wallet Lock

Locks the encrypted wallet.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "walletlock",
    "params": [],
    "id": "lock"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Wallet locked",
    "id": "lock"
}
```

---

### 19. Wallet Passphrase

Unlocks the wallet for a specified time.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "walletpassphrase",
    "params": ["MySecurePassphrase123!", 300],
    "id": "unlock"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Wallet unlocked for 300 seconds",
    "id": "unlock"
}
```

**Parameters:**
- `passphrase` (string, required): Wallet passphrase
- `timeout` (number, required): Unlock time in seconds

**Auto-Lock:** Wallet automatically locks after timeout

---

### 20. Wallet Passphrase Change

Changes the wallet encryption passphrase.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "walletpassphrasechange",
    "params": ["OldPassphrase123!", "NewPassphrase456!"],
    "id": "changepass"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Passphrase changed successfully",
    "id": "changepass"
}
```

**Parameters:**
- `oldpassphrase` (string, required): Current passphrase
- `newpassphrase` (string, required): New passphrase (min 8 chars)

---

### 21. Import Mnemonic

Imports HD wallet from BIP39 mnemonic.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "importmnemonic",
    "params": ["word1 word2 word3 ... word24", "MyPassphrase123!"],
    "id": "importmnemonic"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Mnemonic imported successfully",
    "id": "importmnemonic"
}
```

**Parameters:**
- `mnemonic` (string, required): 12, 15, 18, 21, or 24 words
- `passphrase` (string, optional): Optional mnemonic passphrase

---

### 22. Import Private Key

Imports a private key.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "importprivkey",
    "params": ["L1a2B3c4D5e6F7g8H9i0J1k2L3m4N5o6P7q8R9s0T1u2V3w4X5y6Z7"],
    "id": "importkey"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": "Private key imported",
    "id": "importkey"
}
```

**Parameters:**
- `privkey` (string, required): WIF-encoded private key

---

### 23. Validate Address

Validates a Dinari address.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "validateaddress",
    "params": ["D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6P7Q8r9S0"],
    "id": "validateaddr"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "isvalid": true,
        "address": "D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6P7Q8r9S0",
        "ismine": false
    },
    "id": "validateaddr"
}
```

---

### 24. Get Address Info

Gets detailed address information.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getaddressinfo",
    "params": ["D1A2b3C4d5E6f7G8h9I0j1K2L3M4n5O6P7Q8r9S0"],
    "id": "addrinfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "address": "D1...",
        "ismine": true,
        "iswatchonly": false,
        "isscript": false,
        "pubkey": "02abc123...",
        "label": "default"
    },
    "id": "addrinfo"
}
```

---

## Mempool Operations

### 25. Get Mempool Info

Returns mempool statistics.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getmempoolinfo",
    "params": [],
    "id": "mempoolinfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "size": 145,
        "bytes": 234567,
        "usage": 512000,
        "maxmempool": 314572800
    },
    "id": "mempoolinfo"
}
```

**Parameters:** None

**Returns:** `object` - Mempool statistics

**Fields:**
- `size`: Number of transactions
- `bytes`: Total size in bytes
- `usage`: Memory usage
- `maxmempool`: Maximum mempool size

---

### 26. Get Raw Mempool

Lists all mempool transactions.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getrawmempool",
    "params": [false],
    "id": "rawmempool"
}
```

**Response (verbose=false):**
```json
{
    "jsonrpc": "2.0",
    "result": [
        "abc123...",
        "def456...",
        "ghi789..."
    ],
    "id": "rawmempool"
}
```

**Response (verbose=true):**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "abc123...": {
            "size": 225,
            "fee": 1000,
            "time": 1735567200
        }
    },
    "id": "rawmempool"
}
```

**Parameters:**
- `verbose` (boolean, optional): Detailed info (true) or txid list (false). Default: false

---

## Mining Operations

### 27. Get Mining Info

Returns mining-related information.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getmininginfo",
    "params": [],
    "id": "mininginfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "blocks": 12345,
        "difficulty": 486604799,
        "networkhashps": 1234567890,
        "pooledtx": 45,
        "chain": "main"
    },
    "id": "mininginfo"
}
```

---

## Network Information

### 28. Get Network Info

Returns network information.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getnetworkinfo",
    "params": [],
    "id": "netinfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": {
        "version": 100000,
        "protocolversion": 70001,
        "connections": 8,
        "networks": ["ipv4"],
        "localservices": "0000000000000001"
    },
    "id": "netinfo"
}
```

---

### 29. Get Peer Info

Returns information about connected peers.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getpeerinfo",
    "params": [],
    "id": "peerinfo"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": [
        {
            "id": 1,
            "addr": "192.168.1.100:9333",
            "services": "0000000000000001",
            "version": 70001,
            "subver": "/Dinari:1.0.0/",
            "inbound": false,
            "startingheight": 12340,
            "synced_headers": 12345,
            "synced_blocks": 12345
        }
    ],
    "id": "peerinfo"
}
```

---

### 30. Get Connection Count

Returns the number of connections.

**Request:**
```json
{
    "jsonrpc": "2.0",
    "method": "getconnectioncount",
    "params": [],
    "id": "conncount"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "result": 8,
    "id": "conncount"
}
```

---

## Error Codes

### JSON-RPC Standard Errors

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON |
| -32600 | Invalid Request | Missing required fields |
| -32601 | Method not found | Unknown method |
| -32602 | Invalid params | Wrong parameter types/count |
| -32603 | Internal error | Server error |

### Application-Specific Errors

| Code | Constant | Description |
|------|----------|-------------|
| -1 | RPC_MISC_ERROR | General error |
| -3 | RPC_TYPE_ERROR | Type mismatch |
| -4 | RPC_WALLET_ERROR | Wallet error |
| -5 | RPC_INVALID_ADDRESS_OR_KEY | Invalid address or key |
| -6 | RPC_WALLET_INSUFFICIENT_FUNDS | Insufficient funds |
| -7 | RPC_OUT_OF_MEMORY | Out of memory |
| -8 | RPC_INVALID_PARAMETER | Invalid parameter |
| -13 | RPC_WALLET_UNLOCK_NEEDED | Wallet is locked |
| -14 | RPC_WALLET_PASSPHRASE_INCORRECT | Wrong passphrase |
| -20 | RPC_DATABASE_ERROR | Database error |
| -22 | RPC_DESERIALIZATION_ERROR | Deserialization failed |
| -25 | RPC_VERIFY_ERROR | Verification error |

### Example Error Response

```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -6,
        "message": "Insufficient funds"
    },
    "id": "send"
}
```

---

## Best Practices

### 1. Error Handling

Always check for errors in responses:

```javascript
if (response.error) {
    console.error(`Error ${response.error.code}: ${response.error.message}`);
    return;
}
```

### 2. Request IDs

Use descriptive request IDs:

```json
{
    "id": "get-block-100"  // Good
}
```

Not:
```json
{
    "id": 1  // Less descriptive
}
```

### 3. Timeout Handling

Set appropriate timeouts:
- Normal requests: 30 seconds
- Block sync operations: 60 seconds
- Large data queries: 120 seconds

### 4. Rate Limiting

Respect rate limits:
- Max 10 requests/second per IP
- Use exponential backoff on errors
- Implement client-side throttling

### 5. Pagination

For large datasets, use pagination:

```json
// Get blocks 0-99
{"method": "listblocks", "params": [0, 100]}

// Get blocks 100-199
{"method": "listblocks", "params": [100, 100]}
```

### 6. Security

- ✅ Always use HTTPS in production
- ✅ Store credentials securely (use environment variables)
- ✅ Never commit credentials to version control
- ✅ Rotate credentials regularly
- ✅ Use IP whitelisting when possible
- ✅ Monitor for unusual activity

### 7. Testing

Create test cases for:
- ✅ Successful requests
- ✅ Error scenarios
- ✅ Edge cases (empty params, max values)
- ✅ Rate limiting
- ✅ Authentication failures

### 8. Monitoring

Track:
- Request success/failure rates
- Response times
- Error frequency by type
- API usage patterns

---

## Complete Example Workflow

### Scenario: Send DNT Payment

```javascript
// 1. Check wallet is unlocked
POST {{baseUrl}}/
{
    "method": "getwalletinfo",
    "params": [],
    "id": "check-wallet"
}

// 2. If locked, unlock it
POST {{baseUrl}}/
{
    "method": "walletpassphrase",
    "params": ["MyPassword", 300],
    "id": "unlock-wallet"
}

// 3. Check balance
POST {{baseUrl}}/
{
    "method": "getbalance",
    "params": [],
    "id": "check-balance"
}

// 4. Send payment
POST {{baseUrl}}/
{
    "method": "sendtoaddress",
    "params": ["D1RecipientAddress...", 100000000],
    "id": "send-payment"
}

// 5. Get transaction details
POST {{baseUrl}}/
{
    "method": "getrawtransaction",
    "params": ["<txid-from-step-4>", true],
    "id": "verify-tx"
}

// 6. Lock wallet for security
POST {{baseUrl}}/
{
    "method": "walletlock",
    "params": [],
    "id": "lock-wallet"
}
```

---

## Support

### Resources

- **API Collection:** `postman/Dinari_Blockchain_API.postman_collection.json`
- **Setup Guide:** `docs/SETUP_GUIDE.md`
- **Security Audit:** `SECURITY_AUDIT_SUMMARY.md`
- **GitHub:** https://github.com/EmekaIwuagwu/dinari-blockchain-hub

### Getting Help

1. Check this documentation
2. Review setup guide
3. Check GitHub issues
4. Review security audit for known limitations

---

## Appendix: Quick Reference

### All Methods

| Method | Category | Description |
|--------|----------|-------------|
| getblockcount | Blockchain | Get blockchain height |
| getblockhash | Blockchain | Get block hash by height |
| getblock | Blockchain | Get block details |
| getbestblockhash | Blockchain | Get tip block hash |
| getdifficulty | Blockchain | Get PoW difficulty |
| getblockchaininfo | Blockchain | Get chain info |
| gettxout | Blockchain | Get UTXO info |
| **getrawtransaction** | **Explorer** | **Get TX by hash** |
| **listblocks** | **Explorer** | **List blocks with details** |
| getnewaddress | Wallet | Generate address |
| getbalance | Wallet | Get balance |
| sendtoaddress | Wallet | Send DNT |
| listaddresses | Wallet | List addresses |
| listtransactions | Wallet | List TXs |
| listunspent | Wallet | List UTXOs |
| getwalletinfo | Wallet | Get wallet info |
| encryptwallet | Wallet | Encrypt wallet |
| walletlock | Wallet | Lock wallet |
| walletpassphrase | Wallet | Unlock wallet |
| walletpassphrasechange | Wallet | Change passphrase |
| importmnemonic | Wallet | Import mnemonic |
| importprivkey | Wallet | Import private key |
| validateaddress | Wallet | Validate address |
| getaddressinfo | Wallet | Get address info |
| getmempoolinfo | Mempool | Get mempool stats |
| getrawmempool | Mempool | List mempool TXs |

---

**End of Postman API Documentation**

For questions or issues, refer to the comprehensive setup guide or GitHub repository.

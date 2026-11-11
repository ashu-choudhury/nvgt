# NVGT Web Proxy Bridge (WebSocket to UDP)

Web browsers running WebAssembly cannot open direct UDP or raw TCP sockets due to browser security sandboxes.
The **NVGT Web Proxy Bridge** solves this challenge by acting as a transparent relay between browser WebSocket clients and standard UDP game servers (such as ENet multiplayer audio game servers).

---

## How It Works

1. The browser WebAssembly client connects to the proxy using a standard WebSocket (`ws://` or `wss://`).
2. The proxy opens a dedicated UDP socket for that client session.
3. Outgoing packets from the browser are forwarded as native UDP datagrams to the target game server.
4. Incoming UDP datagrams from the game server are sent back to the browser client over the WebSocket.
5. The game server requires zero modifications: it perceives the packets as coming from a normal UDP player.

---

## Quick Start (Python)

### Requirements
```bash
pip install websockets
```

### Running the Proxy
```bash
python nvgt_udp_proxy.py --port 8080 --target-host 127.0.0.1 --target-port 1234
```

### Options
- `--host`: Bind host (default: `0.0.0.0`).
- `--port`: WebSocket listen port (default: `8080`).
- `--target-host`: Game server UDP address (default: `127.0.0.1`).
- `--target-port`: Game server UDP port (default: `1234`).
- `--allow-custom-targets`: Allows clients to dynamically route using `wss://proxy/?target=server.com:port`.
- `--ssl-cert`, `--ssl-key`: Enables secure WebSocket (`wss://`) for HTTPS hosting.

---

## Quick Start (Node.js)

### Requirements
```bash
npm install ws
```

### Running the Proxy
```bash
node nvgt_udp_proxy.js --port 8080 --target-host 127.0.0.1 --target-port 1234
```

---

## Configuring Your NVGT Game

You can set the proxy URL in three ways:

1. **In `config.ini` / `.nvgtrc`**:
   ```ini
   [build]
   web_proxy = wss://proxy.mygame.com:8080
   ```

2. **At compile time via CLI**:
   ```bash
   nvgt -c -p web mygame.nvgt -s build.web_proxy=wss://proxy.mygame.com:8080
   ```

3. **In-game via the Web Player UI**:
   Players can click the 🌐 icon in the top header of the web page to inspect or update their proxy connection URL.

#!/usr/bin/env node
/**
 * nvgt_udp_proxy.js - Node.js WebSocket to UDP Proxy Bridge for NVGT Web Games
 *
 * Requirements:
 *   npm install ws
 *
 * Usage:
 *   node nvgt_udp_proxy.js --port 8080 --target-host 127.0.0.1 --target-port 1234
 */

const dgram = require('dgram');
const http = require('http');
const https = require('https');
const fs = require('fs');
const { WebSocketServer } = require('ws');

// Parse CLI arguments
const args = process.argv.slice(2);
function getArg(flag, defaultValue) {
  const idx = args.indexOf(flag);
  return idx !== -1 && idx + 1 < args.length ? args[idx + 1] : defaultValue;
}

const PORT = parseInt(getArg('--port', '8080'), 10);
const HOST = getArg('--host', '0.0.0.0');
const TARGET_HOST = getArg('--target-host', '127.0.0.1');
const TARGET_PORT = parseInt(getArg('--target-port', '1234'), 10);
const SSL_CERT = getArg('--ssl-cert', null);
const SSL_KEY = getArg('--ssl-key', null);

let server;
if (SSL_CERT && SSL_KEY) {
  server = https.createServer({
    cert: fs.readFileSync(SSL_CERT),
    key: fs.readFileSync(SSL_KEY)
  });
} else {
  server = http.createServer();
}

const wss = new WebSocketServer({ server });

wss.on('connection', (ws, req) => {
  let targetHost = TARGET_HOST;
  let targetPort = TARGET_PORT;

  // Support ?target=host:port
  if (req.url && req.url.includes('target=')) {
    try {
      const match = req.url.match(/target=([^:&]+):(\d+)/);
      if (match) {
        targetHost = match[1];
        targetPort = parseInt(match[2], 10);
      }
    } catch (_) {}
  }

  const clientIp = req.socket.remoteAddress;
  console.log(`[Proxy] Client ${clientIp} connected -> UDP ${targetHost}:${targetPort}`);

  // Create dedicated UDP socket for this client
  const udp = dgram.createSocket('udp4');

  udp.on('message', (msg) => {
    if (ws.readyState === ws.OPEN) {
      ws.send(msg);
    }
  });

  udp.on('error', (err) => {
    console.error(`[UDP Error] ${err.message}`);
  });

  ws.on('message', (data) => {
    const buf = Buffer.isBuffer(data) ? data : Buffer.from(data);
    udp.send(buf, targetPort, targetHost, (err) => {
      if (err) console.error(`[Send Error] ${err.message}`);
    });
  });

  ws.on('close', () => {
    console.log(`[Proxy] Client ${clientIp} disconnected.`);
    udp.close();
  });

  ws.on('error', (err) => {
    console.error(`[WS Error] ${err.message}`);
    udp.close();
  });
});

server.listen(PORT, HOST, () => {
  const proto = SSL_CERT ? 'wss' : 'ws';
  console.log(`[NVGT Proxy] Listening on ${proto}://${HOST}:${PORT}`);
  console.log(`[NVGT Proxy] Forwarding traffic to UDP ${TARGET_HOST}:${TARGET_PORT}`);
});

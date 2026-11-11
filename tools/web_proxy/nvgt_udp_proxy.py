#!/usr/bin/env python3
"""
nvgt_udp_proxy.py - Zero-Dependency WebSocket to UDP Proxy Bridge for NVGT Games
Relays network traffic between web browsers (over WebSockets) and native game servers (over UDP / ENet).
Requires ZERO external packages — uses only Python 3 standard library (asyncio, hashlib, base64, socket).

Usage:
  python nvgt_udp_proxy.py --port 8080 --target-host 127.0.0.1 --target-port 1234
"""

import argparse
import asyncio
import base64
import hashlib
import logging
import socket
import struct
import sys
from typing import Tuple

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")

WS_MAGIC = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def compute_accept_key(sec_key: str) -> str:
    combined = (sec_key.strip() + WS_MAGIC).encode('utf-8')
    return base64.b64encode(hashlib.sha1(combined).digest()).decode('utf-8')


def build_ws_frame(data: bytes, opcode: int = 0x02) -> bytes:
    """Builds an unmasked WebSocket frame from server to client."""
    length = len(data)
    first_byte = 0x80 | (opcode & 0x0F)  # FIN = 1
    if length <= 125:
        header = struct.pack("!BB", first_byte, length)
    elif length <= 65535:
        header = struct.pack("!BBH", first_byte, 126, length)
    else:
        header = struct.pack("!BBQ", first_byte, 127, length)
    return header + data


class UDPClientRelay(asyncio.DatagramProtocol):
    def __init__(self, ws_writer, client_addr):
        self.ws_writer = ws_writer
        self.client_addr = client_addr
        self.transport = None

    def connection_made(self, transport):
        self.transport = transport

    def datagram_received(self, data: bytes, addr: Tuple[str, int]):
        # Forward incoming UDP packet from game server to web client via WebSocket frame
        try:
            frame = build_ws_frame(data, opcode=0x02)
            self.ws_writer.write(frame)
        except Exception as e:
            logging.debug(f"Failed to forward UDP to WS for {self.client_addr}: {e}")

    def error_received(self, exc):
        logging.warning(f"UDP socket error for {self.client_addr}: {exc}")

    def connection_lost(self, exc):
        pass


class WSClientSession:
    def __init__(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, target_host: str, target_port: int, allow_custom: bool):
        self.reader = reader
        self.writer = writer
        self.default_host = target_host
        self.default_port = target_port
        self.allow_custom = allow_custom
        self.target_addr = (target_host, target_port)
        self.client_addr = writer.get_extra_info('peername')
        self.udp_transport = None
        self.udp_protocol = None

    async def handle_handshake(self) -> bool:
        headers = {}
        request_line = await self.reader.readline()
        if not request_line:
            return False
        
        parts = request_line.decode('utf-8', errors='ignore').split()
        if len(parts) >= 2:
            path = parts[1]
            if self.allow_custom and '?' in path:
                query = path.split('?', 1)[1]
                for p in query.split('&'):
                    if p.startswith('target='):
                        dest = p.split('=', 1)[1]
                        if ':' in dest:
                            h, port_str = dest.split(':', 1)
                            self.target_addr = (h, int(port_str))

        while True:
            line = await self.reader.readline()
            if not line or line == b'\r\n' or line == b'\n':
                break
            line_str = line.decode('utf-8', errors='ignore').strip()
            if ':' in line_str:
                k, v = line_str.split(':', 1)
                headers[k.strip().lower()] = v.strip()

        sec_key = headers.get('sec-websocket-key')
        if not sec_key:
            self.writer.write(b"HTTP/1.1 400 Bad Request\r\n\r\n")
            await self.writer.drain()
            return False

        accept_val = compute_accept_key(sec_key)
        response = (
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Accept: {accept_val}\r\n"
            "\r\n"
        )
        self.writer.write(response.encode('utf-8'))
        await self.writer.drain()
        return True

    async def run(self):
        try:
            if not await self.handle_handshake():
                return

            logging.info(f"Client {self.client_addr} connected -> Relaying to UDP {self.target_addr}")

            loop = asyncio.get_running_loop()
            self.udp_transport, self.udp_protocol = await loop.create_datagram_endpoint(
                lambda: UDPClientRelay(self.writer, self.client_addr),
                local_addr=('0.0.0.0', 0),
                family=socket.AF_INET
            )

            # Read WebSocket frames from client
            while True:
                header = await self.reader.readexactly(2)
                b1, b2 = header[0], header[1]
                opcode = b1 & 0x0F
                masked = (b2 & 0x80) != 0
                payload_len = b2 & 0x7F

                if opcode == 0x08:  # Connection close
                    break

                if payload_len == 126:
                    ext = await self.reader.readexactly(2)
                    payload_len = struct.unpack("!H", ext)[0]
                elif payload_len == 127:
                    ext = await self.reader.readexactly(8)
                    payload_len = struct.unpack("!Q", ext)[0]

                if masked:
                    mask = await self.reader.readexactly(4)
                else:
                    mask = None

                data = await self.reader.readexactly(payload_len)
                if masked:
                    data = bytes(b ^ mask[i % 4] for i, b in enumerate(data))

                if opcode == 0x09:  # Ping -> Pong
                    self.writer.write(build_ws_frame(data, opcode=0x0A))
                    await self.writer.drain()
                elif opcode in (0x01, 0x02):  # Text or Binary
                    # Forward payload as UDP datagram to target server
                    if self.udp_transport:
                        self.udp_transport.sendto(data, self.target_addr)

        except (asyncio.IncompleteReadError, ConnectionResetError):
            pass
        except Exception as e:
            logging.error(f"Error for client {self.client_addr}: {e}")
        finally:
            logging.info(f"Client {self.client_addr} disconnected.")
            if self.udp_transport:
                self.udp_transport.close()
            try:
                self.writer.close()
                await self.writer.wait_closed()
            except Exception:
                pass


async def main():
    parser = argparse.ArgumentParser(description="Zero-Dependency NVGT WebSocket-to-UDP Proxy Bridge")
    parser.add_argument("--host", default="0.0.0.0", help="Host address to bind WebSocket server (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8080, help="Port to listen for WebSocket connections (default: 8080)")
    parser.add_argument("--target-host", default="127.0.0.1", help="Default target UDP game server host (default: 127.0.0.1)")
    parser.add_argument("--target-port", type=int, default=1234, help="Default target UDP game server port (default: 1234)")
    parser.add_argument("--allow-custom-targets", action="store_true", help="Allow web clients to specify ?target=host:port")
    parser.add_argument("--ssl-cert", help="Path to SSL certificate (PEM) for wss://")
    parser.add_argument("--ssl-key", help="Path to SSL private key (PEM) for wss://")

    args = parser.parse_args()

    ssl_context = None
    if args.ssl_cert and args.ssl_key:
        import ssl
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ssl_context.load_cert_chain(args.ssl_cert, keyfile=args.ssl_key)
        proto = "wss"
    else:
        proto = "ws"

    async def client_connected(reader, writer):
        session = WSClientSession(reader, writer, args.target_host, args.target_port, args.allow_custom_targets)
        await session.run()

    server = await asyncio.start_server(client_connected, args.host, args.port, ssl=ssl_context)
    logging.info(f"NVGT UDP Proxy Bridge listening on {proto}://{args.host}:{args.port}")
    logging.info(f"Target UDP game server: {args.target_host}:{args.target_port}")

    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Proxy server stopped by user.")

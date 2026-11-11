#!/usr/bin/env python3
"""
Simple HTTP server for testing NVGT WebAssembly builds locally.
Serves static files with correct MIME types, WASM streaming support,
and Cross-Origin isolation headers (COOP/COEP) for high-resolution timers.
"""

import os
import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler

class NVGTServerHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # Enable Cross-Origin Isolation for performance.now() and SharedArrayBuffer if needed
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()

    def guess_type(self, path):
        if path.endswith('.wasm'):
            return 'application/wasm'
        if path.endswith('.js'):
            return 'application/javascript'
        if path.endswith('.ogg'):
            return 'audio/ogg'
        if path.endswith('.wav'):
            return 'audio/wav'
        if path.endswith('.bin'):
            return 'application/octet-stream'
        return super().guess_type(path)

def run(port=8000, serve_dir=None):
    if serve_dir:
        os.chdir(serve_dir)
    server_address = ('', port)
    httpd = HTTPServer(server_address, NVGTServerHandler)
    print(f"NVGT Web Server serving '{os.getcwd()}' at http://localhost:{port}/")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 and sys.argv[1].isdigit() else 8000
    target_dir = sys.argv[2] if len(sys.argv) > 2 else "examples/sound_hunter/sound_hunter_web"
    run(port, target_dir)

#!/usr/bin/env python3
"""
serve.py - Local Web Server for Testing NVGT Web Games
Serves the generated sound_hunter_web/ folder and opens it in your default browser.
"""

import http.server
import socketserver
import webbrowser
from pathlib import Path

PORT = 8000
DIRECTORY = Path(__file__).resolve().parent / "sound_hunter_web"

class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "application/javascript",
        ".bin": "application/octet-stream",
    }
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DIRECTORY), **kwargs)

    def do_GET(self):
        if self.path == '/favicon.ico':
            self.send_response(204)
            self.end_headers()
            return
        super().do_GET()

    def end_headers(self):
        # Add CORS and standard caching headers
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        super().end_headers()

def main():
    if not DIRECTORY.exists():
        print(f"Error: Directory {DIRECTORY} does not exist. Please compile the game first.")
        return

    url = f"http://localhost:{PORT}"
    print("=" * 60)
    print(f"  NVGT Web Game Server Running!")
    print(f"  URL: {url}")
    print(f"  Serving directory: {DIRECTORY}")
    print("  Controls in Game:")
    print("    - Left Arrow / A : Move Left")
    print("    - Right Arrow / D : Move Right")
    print("    - Space / Enter   : Collect Item")
    print("    - Up Arrow / C    : Announce Position & Score")
    print("    - Escape          : Quit Game")
    print("=" * 60)
    print("Press Ctrl+C to stop the server.\n")

    webbrowser.open(url)

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")

if __name__ == "__main__":
    main()

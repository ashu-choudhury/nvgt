#!/usr/bin/env python3
"""
makestub_web.py - Packages WebAssembly templates and runtime into release/stub/nvgt_web.bin
NVGT - NonVisual Gaming Toolkit (https://nvgt.dev)
"""

import os
import shutil
import sys
import zipfile
from pathlib import Path

repo_root = Path(__file__).resolve().parents[1]
template_dir = repo_root / "template" / "web"
stub_out_dir = repo_root / "release" / "stub"
stub_out_file = stub_out_dir / "nvgt_web.bin"

def package_web_stub(wasm_path: Path = None, js_path: Path = None):
    stub_out_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Packaging web stub from {template_dir} into {stub_out_file}...")
    with zipfile.ZipFile(stub_out_file, "w", zipfile.ZIP_DEFLATED) as z:
        # Add web templates
        for file in ["index.html", "style.css", "nvgt_bridge.js"]:
            fpath = template_dir / file
            if fpath.exists():
                z.write(fpath, file)
                print(f"  Added {file}")
            else:
                print(f"  Warning: {fpath} not found")
        
        # Add wasm and js if provided or if found in release / build
        search_dirs = [
            repo_root / "build" / "web",
            repo_root / "release" / "web",
            template_dir
        ]
        
        target_wasm = wasm_path
        target_js = js_path
        
        if not target_wasm:
            for d in search_dirs:
                if (d / "nvgt.wasm").exists():
                    target_wasm = d / "nvgt.wasm"
                    break
                    
        if not target_js:
            for d in search_dirs:
                if (d / "nvgt.js").exists():
                    target_js = d / "nvgt.js"
                    break
                    
        if target_wasm and target_wasm.exists():
            z.write(target_wasm, "nvgt.wasm")
            print(f"  Added nvgt.wasm from {target_wasm}")
            
        if target_js and target_js.exists():
            z.write(target_js, "nvgt.js")
            print(f"  Added nvgt.js from {target_js}")

    print(f"Successfully generated {stub_out_file} ({stub_out_file.stat().st_size} bytes)")

if __name__ == "__main__":
    wasm = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    js = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    package_web_stub(wasm, js)

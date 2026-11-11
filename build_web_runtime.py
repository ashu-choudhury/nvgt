import os, sys, glob, shutil, subprocess

repo_root = os.path.dirname(os.path.abspath(__file__))
as_root = os.path.join(repo_root, "build", "angelscript")
as_src = os.path.join(as_root, "sdk", "angelscript", "source")
as_inc = os.path.join(as_root, "sdk", "angelscript", "include")

if not os.path.isdir(as_src):
    print("Fetching AngelScript SDK for WebAssembly build...")
    subprocess.run(["git", "clone", "--depth", "1", "https://github.com/codecat/angelscript-mirror.git", as_root], check=True)

addon_src = os.path.join(repo_root, "ASAddon", "src")
addon_inc = os.path.join(repo_root, "ASAddon", "include")
out_dir = os.path.join(repo_root, "build", "web")
os.makedirs(out_dir, exist_ok=True)

# Detect emcc executable
emcc_path = shutil.which("emcc")
emcc_cmd = [emcc_path] if emcc_path else []

if not emcc_cmd:
    # Check Scoop / Windows locations
    scoop_emcc = r"E:\Scoop\persist\emscripten\upstream\emscripten\emcc.py"
    scoop_config = r"E:\Scoop\apps\emscripten\current\.emscripten"
    if os.path.isfile(scoop_emcc):
        emcc_cmd = ["python", scoop_emcc, "--em-config", scoop_config]
    else:
        print("Error: emcc not found in PATH or standard locations.")
        sys.exit(1)

# AngelScript sources
sources = glob.glob(os.path.join(as_src, "*.cpp"))
as_filtered = [
    s for s in sources 
    if not os.path.basename(s).startswith("as_callfunc_") 
    or os.path.basename(s) == "as_callfunc_wasm.cpp" 
    or os.path.basename(s) == "as_callfunc.cpp"
]

# ASAddon sources
addon_sources = [
    os.path.join(addon_src, "scriptstdstring.cpp"),
    os.path.join(addon_src, "scriptarray.cpp"),
    os.path.join(addon_src, "scriptdictionary.cpp")
]

# Web runtime source
runtime_source = os.path.join(repo_root, "build", "web", "nvgt_web_runtime.cpp")
aes_source = os.path.join(repo_root, "build", "web", "aes.c")

all_sources = as_filtered + addon_sources + [runtime_source, aes_source]
print(f"Compiling NVGT Web Runtime with {len(all_sources)} source files...")

out_js = os.path.join(out_dir, "nvgt.js")
cmd = emcc_cmd + [
    "-O2",
    f"-I{as_src}", f"-I{as_inc}", f"-I{addon_inc}", f"-I{out_dir}",
    "-sDEFAULT_TO_CXX",
    "-sUSE_ZLIB=1",
    "-sASYNCIFY=1",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sEMULATE_FUNCTION_POINTER_CASTS=1",
    "-sEXPORTED_FUNCTIONS=['_main','_nvgt_run_game']",
    "-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap']",
] + all_sources + ["-o", out_js]

res = subprocess.run(cmd, capture_output=True, text=True)
print("Return code:", res.returncode)
if res.stderr:
    with open("build_web_err.txt", "w", encoding="utf-8") as f:
        f.write(res.stderr)
    print("Stderr written to build_web_err.txt")

if res.returncode == 0:
    out_wasm = os.path.join(out_dir, "nvgt.wasm")
    print(f"Generated {out_js} ({os.path.getsize(out_js)} bytes)")
    print(f"Generated {out_wasm} ({os.path.getsize(out_wasm)} bytes)")

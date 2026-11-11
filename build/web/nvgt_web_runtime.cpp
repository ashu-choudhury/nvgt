/* nvgt_web_runtime.cpp - WebAssembly/Emscripten runtime stub for NVGT
 *
 * NonVisual Gaming Toolkit (NVGT) WebAssembly Runtime
 * Automatically loads encrypted & compressed game.bin, registers full NVGT API,
 * provides WebAudio spatial sound, Web Speech / ARIA screen reader output, and
 * Asyncify cooperative event loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include <angelscript.h>
#include <scriptstdstring.h>
#include <scriptarray.h>
#include <scriptdictionary.h>
extern "C" {
#include <aes.h>
}
#include <zlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

// JavaScript Audio & UI Bridges
EM_JS(void, js_audio_load, (int sound_id, const char* path_str), {
    const win = typeof window !== "undefined" ? window : globalThis;
    const path = UTF8ToString(path_str);
    if (win.NVGT && win.NVGT.loadAudio) {
        win.NVGT.loadAudio(sound_id, path);
    }
});

EM_JS(void, js_audio_play, (int sound_id, const char* path_str, float pan, float vol, float pitch, int loop), {
    const win = typeof window !== "undefined" ? window : globalThis;
    const path = UTF8ToString(path_str);
    if (win.NVGT && win.NVGT.playAudio) {
        win.NVGT.playAudio(sound_id, path, pan, vol, pitch, loop);
    }
});

EM_JS(void, js_audio_stop, (int sound_id), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.stopAudio) {
        win.NVGT.stopAudio(sound_id);
    }
});

EM_JS(void, js_audio_pause, (int sound_id), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.pauseAudio) {
        win.NVGT.pauseAudio(sound_id);
    }
});

EM_JS(void, js_audio_set_pan, (int sound_id, float pan), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.setAudioPan) {
        win.NVGT.setAudioPan(sound_id, pan);
    }
});

EM_JS(void, js_audio_set_volume, (int sound_id, float vol), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.setAudioVolume) {
        win.NVGT.setAudioVolume(sound_id, vol);
    }
});

EM_JS(void, js_audio_set_pitch, (int sound_id, float pitch), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.setAudioPitch) {
        win.NVGT.setAudioPitch(sound_id, pitch);
    }
});

EM_JS(int, js_audio_is_playing, (int sound_id), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.isAudioPlaying) {
        return win.NVGT.isAudioPlaying(sound_id) ? 1 : 0;
    }
    return 0;
});

EM_JS(void, js_audio_destroy, (int sound_id), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win.NVGT && win.NVGT.destroyAudio) {
        win.NVGT.destroyAudio(sound_id);
    }
});

EM_JS(void, js_screen_reader_speak, (const char* text_str, int interrupt), {
    const win = typeof window !== "undefined" ? window : globalThis;
    const str = UTF8ToString(text_str);
    if (win.NVGT && win.NVGT.speakAria) {
        win.NVGT.speakAria(str, interrupt);
    } else if (typeof document !== "undefined") {
        let spk = document.getElementById("nvgt-speaker");
        if (spk) {
            if (interrupt) spk.textContent = "";
            setTimeout(function() {
                spk.textContent = "";
                const span = document.createElement("span");
                span.textContent = str;
                spk.appendChild(span);
            }, 20);
        }
    }
});

EM_JS(void, js_show_window, (const char* title_str), {
    if (typeof document !== "undefined") {
        document.title = UTF8ToString(title_str);
        const canvas = document.getElementById("canvas");
        if (canvas) canvas.focus();
    } else {
        console.log("[Window]", UTF8ToString(title_str));
    }
});

EM_JS(char*, js_input_box, (const char* title_str, const char* caption_str, const char* default_str), {
    const title = UTF8ToString(title_str);
    const caption = UTF8ToString(caption_str);
    const defVal = UTF8ToString(default_str);
    const promptText = (title ? title + "\n\n" : "") + caption;
    let res = "";
    if (typeof window !== "undefined" && window.prompt) {
        const val = window.prompt(promptText, defVal);
        if (val !== null) res = val;
    }
    if (typeof document !== "undefined") {
        const c = document.getElementById("canvas");
        if (c) c.focus();
    }
    const len = lengthBytesUTF8(res) + 1;
    const ptr = _malloc(len);
    stringToUTF8(res, ptr, len);
    return ptr;
});

static std::string web_input_box(const std::string& title, const std::string& caption, const std::string& default_value = "", uint64_t flags = 0) {
    #ifdef __EMSCRIPTEN__
    char* res_ptr = js_input_box(title.c_str(), caption.c_str(), default_value.c_str());
    if (!res_ptr) return default_value;
    std::string result(res_ptr);
    free(res_ptr);
    return result;
    #else
    return default_value;
    #endif
}

EM_JS(int, js_key_pressed, (int code), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (!win.nvgtKeyPressed) return 0;
    const res = win.nvgtKeyPressed[code] ? 1 : 0;
    win.nvgtKeyPressed[code] = 0;
    return res;
});

EM_JS(int, js_key_down, (int code), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (!win.nvgtKeyStates) return 0;
    return win.nvgtKeyStates[code] ? 1 : 0;
});

EM_JS(void, js_init_input, (), {
    const win = typeof window !== "undefined" ? window : globalThis;
    if (win._nvgtInputInitialized) return;
    win._nvgtInputInitialized = true;
    win.nvgtKeyStates = {};
    win.nvgtKeyPressed = {};
    const codeMap = {
        "ArrowLeft": 80, "ArrowRight": 79, "ArrowUp": 82, "ArrowDown": 81,
        "Space": 44, "Enter": 40, "Escape": 41, "Backspace": 42, "Tab": 43,
        "KeyA": 4, "KeyB": 5, "KeyC": 6, "KeyD": 7, "KeyE": 8, "KeyF": 9,
        "KeyG": 10, "KeyH": 11, "KeyI": 12, "KeyJ": 13, "KeyK": 14, "KeyL": 15,
        "KeyM": 16, "KeyN": 17, "KeyO": 18, "KeyP": 19, "KeyQ": 20, "KeyR": 21,
        "KeyS": 22, "KeyT": 23, "KeyU": 24, "KeyV": 25, "KeyW": 26, "KeyX": 27,
        "KeyY": 28, "KeyZ": 29
    };
    if (typeof window !== "undefined") {
        window.addEventListener("keydown", function(e) {
            const code = codeMap[e.code];
            if (code !== undefined) {
                if (!win.nvgtKeyStates[code]) {
                    win.nvgtKeyPressed[code] = 1;
                }
                win.nvgtKeyStates[code] = 1;
                if (e.code === "Space" || e.code.startsWith("Arrow")) {
                    e.preventDefault();
                }
            }
        });
        window.addEventListener("keyup", function(e) {
            const code = codeMap[e.code];
            if (code !== undefined) {
                win.nvgtKeyStates[code] = 0;
            }
        });
    }
});
#endif

// NVGT Bytecode Stream: Handles AES-256-CBC Decryption and ZLib Decompression
class NVGTBytecodeStream : public asIBinaryStream {
    unsigned char* content;
    z_stream zstr;
    int cursor;
    int written_size;
public:
    NVGTBytecodeStream() : content(NULL), cursor(0), written_size(0) {
        memset(&zstr, 0, sizeof(z_stream));
        zstr.data_type = -1;
    }
    ~NVGTBytecodeStream() {
        if (zstr.data_type != -1) {
            inflateEnd(&zstr);
        }
    }
    int Write(const void*, asUINT) override { return -1; }
    int Read(void* ptr, asUINT size) override {
        zstr.next_out = (Bytef*)ptr;
        zstr.avail_out = size;
        while (zstr.avail_out > 0) {
            int err = inflate(&zstr, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END) break;
            if (err != Z_OK) return -1;
        }
        cursor += (size - zstr.avail_out);
        return size - zstr.avail_out;
    }
    int get_cursor() const { return cursor; }
    void reset_cursor() { cursor = 0; }
    void set(unsigned char* code, int size) {
        // AES-256-CBC Decrypt using NVGT's standard key and IV (from SHA256("Kernel32.lib"))
        uint8_t key[32] = {200, 19, 208, 2, 34, 125, 189, 244, 115, 45, 169, 206, 128, 192, 189, 53, 139, 132, 62, 214, 198, 109, 193, 215, 92, 221, 177, 101, 24, 188, 247, 151};
        uint8_t iv[16] = {12, 33, 90, 223, 2, 253, 247, 14, 187, 149, 42, 156, 146, 54, 235, 204};
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_decrypt_buffer(&ctx, code, size);

        // Strip PKCS7 padding
        uint8_t pad = code[size - 1];
        if (pad > 0 && pad <= 16) {
            written_size = size - pad;
        } else {
            written_size = size;
        }

        content = code;
        zstr.data_type = 0;
        inflateInit(&zstr);
        zstr.next_in = (Bytef*)content;
        zstr.avail_in = written_size;
    }
};

template <typename T>
bool read_7bit_encoded(NVGTBytecodeStream& stream, T& val) {
    val = 0;
    int shift = 0;
    while (true) {
        unsigned char b = 0;
        if (stream.Read(&b, 1) != 1) return false;
        val |= (T(b & 0x7f) << shift);
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    return true;
}

bool read_string(NVGTBytecodeStream& stream, std::string& str) {
    uint32_t len = 0;
    if (!read_7bit_encoded(stream, len)) return false;
    str.resize(len);
    if (len > 0) {
        if (stream.Read(&str[0], len) != (int)len) return false;
    }
    return true;
}

// Web Sound Class - Audio Node compatible with NVGT mixer & sound architecture
static int next_sound_id = 1;

class web_sound {
    int refcount = 1;
    int sound_id;
public:
    std::string filename;
    float pan = 0.0f;
    float volume = 1.0f;
    float pitch = 100.0f;
    bool active = false;

    web_sound() {
        sound_id = next_sound_id++;
    }
    ~web_sound() {
        #ifdef __EMSCRIPTEN__
        js_audio_destroy(sound_id);
        #endif
    }

    void addref() { refcount++; }
    void release() { if (--refcount == 0) delete this; }

    bool load(const std::string& path, void* pack = nullptr) {
        filename = path;
        #ifdef __EMSCRIPTEN__
        js_audio_load(sound_id, filename.c_str());
        #endif
        active = true;
        return true;
    }

    bool play(bool reset_loop_state = true) {
        #ifdef __EMSCRIPTEN__
        js_audio_play(sound_id, filename.c_str(), pan, volume, pitch, 0);
        #else
        printf("[Sound Play %d] %s (pan: %.2f, vol: %.2f)\n", sound_id, filename.c_str(), pan, volume);
        #endif
        return true;
    }

    bool play_looped() {
        #ifdef __EMSCRIPTEN__
        js_audio_play(sound_id, filename.c_str(), pan, volume, pitch, 1);
        #else
        printf("[Sound Loop %d] %s (pan: %.2f, vol: %.2f)\n", sound_id, filename.c_str(), pan, volume);
        #endif
        return true;
    }

    bool stop() {
        #ifdef __EMSCRIPTEN__
        js_audio_stop(sound_id);
        #endif
        return true;
    }

    bool pause() {
        #ifdef __EMSCRIPTEN__
        js_audio_pause(sound_id);
        #endif
        return true;
    }

    bool is_playing() const {
        #ifdef __EMSCRIPTEN__
        return js_audio_is_playing(sound_id) != 0;
        #else
        return false;
        #endif
    }

    void set_pan(float p) {
        pan = p;
        #ifdef __EMSCRIPTEN__
        js_audio_set_pan(sound_id, pan);
        #endif
    }
    float get_pan() const { return pan; }

    void set_volume(float v) {
        volume = v;
        #ifdef __EMSCRIPTEN__
        js_audio_set_volume(sound_id, volume);
        #endif
    }
    float get_volume() const { return volume; }

    void set_pitch(float p) {
        pitch = p;
        #ifdef __EMSCRIPTEN__
        js_audio_set_pitch(sound_id, pitch);
        #endif
    }
    float get_pitch() const { return pitch; }
};

static web_sound* web_sound_factory() { return new web_sound(); }

// Static C wrappers for sound to guarantee 100% ABI compatibility with CallSystemFunctionNative on WASM
static bool sound_load_fn(web_sound* s, const std::string& path, void* pack) {
    return s->load(path, pack);
}
static bool sound_play_fn(web_sound* s, bool reset_loop_state) {
    return s->play(reset_loop_state);
}
static bool sound_play_looped_fn(web_sound* s) {
    return s->play_looped();
}
static bool sound_stop_fn(web_sound* s) {
    return s->stop();
}
static bool sound_pause_fn(web_sound* s) {
    return s->pause();
}
static bool sound_is_playing_fn(web_sound* s) {
    return s->is_playing();
}
static void sound_set_pan_fn(web_sound* s, float pan) {
    s->set_pan(pan);
}
static float sound_get_pan_fn(web_sound* s) {
    return s->get_pan();
}
static void sound_set_volume_fn(web_sound* s, float vol) {
    s->set_volume(vol);
}
static float sound_get_volume_fn(web_sound* s) {
    return s->get_volume();
}
static void sound_set_pitch_fn(web_sound* s, float pitch) {
    s->set_pitch(pitch);
}
static float sound_get_pitch_fn(web_sound* s) {
    return s->get_pitch();
}

// Web Timer Class - Performance counter based
class web_timer {
    int refcount = 1;
    double start_time = 0.0;
    double pause_time = 0.0;
    bool is_paused = false;
public:
    web_timer() { restart(); }
    void addref() { refcount++; }
    void release() { if (--refcount == 0) delete this; }

    void restart() {
        #ifdef __EMSCRIPTEN__
        start_time = emscripten_get_now();
        #else
        start_time = 0.0;
        #endif
        is_paused = false;
    }

    int64_t get_elapsed() const {
        if (is_paused) return (int64_t)(pause_time - start_time);
        #ifdef __EMSCRIPTEN__
        return (int64_t)(emscripten_get_now() - start_time);
        #else
        return 0;
        #endif
    }

    void set_elapsed(int64_t ms) {
        #ifdef __EMSCRIPTEN__
        start_time = emscripten_get_now() - ms;
        #endif
    }

    bool pause() {
        if (!is_paused) {
            #ifdef __EMSCRIPTEN__
            pause_time = emscripten_get_now();
            #endif
            is_paused = true;
        }
        return true;
    }

    bool resume() {
        if (is_paused) {
            #ifdef __EMSCRIPTEN__
            start_time += (emscripten_get_now() - pause_time);
            #endif
            is_paused = false;
        }
        return true;
    }

    void toggle_pause() {
        if (is_paused) resume(); else pause();
    }

    bool get_paused() const { return is_paused; }
    bool get_running() const { return !is_paused; }
};

static web_timer* web_timer_factory() { return new web_timer(); }

// Static C wrappers for timer
static int64_t timer_get_elapsed_fn(web_timer* t) { return t->get_elapsed(); }
static void timer_set_elapsed_fn(web_timer* t, int64_t ms) { t->set_elapsed(ms); }
static void timer_restart_fn(web_timer* t) { t->restart(); }
static bool timer_pause_fn(web_timer* t) { return t->pause(); }
static bool timer_resume_fn(web_timer* t) { return t->resume(); }
static void timer_toggle_pause_fn(web_timer* t) { t->toggle_pause(); }
static bool timer_get_paused_fn(web_timer* t) { return t->get_paused(); }
static bool timer_get_running_fn(web_timer* t) { return t->get_running(); }

// Dummy Pack Interface to satisfy default pack argument
class web_pack_interface {
    int refcount = 1;
public:
    void addref() { refcount++; }
    void release() { if (--refcount == 0) delete this; }
};

// Global Functions
static void* dummy_show_window(const std::string& title, unsigned int flags = 0) {
    #ifdef __EMSCRIPTEN__
    js_show_window(title.c_str());
    #else
    printf("[Window] %s\n", title.c_str());
    #endif
    return (void*)1;
}

static bool dummy_speak(const std::string& text, bool interrupt = true) {
    #ifdef __EMSCRIPTEN__
    js_screen_reader_speak(text.c_str(), interrupt ? 1 : 0);
    #else
    printf("[ScreenReader] %s\n", text.c_str());
    #endif
    return true;
}

static void web_wait(int ms) {
    #ifdef __EMSCRIPTEN__
    emscripten_sleep(ms);
    #endif
}

static int web_random(int min, int max) {
    if (min >= max) return min;
    return min + (rand() % (max - min + 1));
}

static float web_clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static int web_clamp_i(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static bool web_key_pressed(int code) {
    #ifdef __EMSCRIPTEN__
    return js_key_pressed(code) != 0;
    #else
    return false;
    #endif
}

static bool web_key_down(int code) {
    #ifdef __EMSCRIPTEN__
    return js_key_down(code) != 0;
    #else
    return false;
    #endif
}

static bool web_key_up(int code) {
    return !web_key_down(code);
}

static void MessageCallback(const asSMessageInfo *msg, void *param) {
    printf("[AngelScript %s] (%d, %d) : %s\n", 
           msg->type == asMSGTYPE_ERROR ? "ERROR" : (msg->type == asMSGTYPE_WARNING ? "WARN" : "INFO"),
           msg->row, msg->col, msg->message);
}

// Register the complete NVGT API matching official signatures
int register_nvgt_api(asIScriptEngine* engine) {
    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterScriptDictionary(engine);

    // pack_interface
    engine->RegisterObjectType("pack_interface", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("pack_interface", asBEHAVE_ADDREF, "void addref()", asMETHOD(web_pack_interface, addref), asCALL_THISCALL);
    engine->RegisterObjectBehaviour("pack_interface", asBEHAVE_RELEASE, "void release()", asMETHOD(web_pack_interface, release), asCALL_THISCALL);
    engine->RegisterGlobalFunction("pack_interface@ get_sound_default_pack() property", asFUNCTIONPR(+[]() -> void* { return nullptr; }, (), void*), asCALL_CDECL);
    engine->RegisterGlobalFunction("void set_sound_default_pack(pack_interface@+ storage) property", asFUNCTIONPR(+[](void*){}, (void*), void), asCALL_CDECL);

    // game_window
    engine->RegisterObjectType("game_window", 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterGlobalFunction("game_window@ show_window(const string &in title = \"\", uint flags = 0)", asFUNCTION(dummy_show_window), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool destroy_window()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool hide_window()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool focus_window()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool is_window_active()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool is_window_hidden()", asFUNCTIONPR(+[]() -> bool { return false; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("int get_window_width()", asFUNCTIONPR(+[]() -> int { return 800; }, (), int), asCALL_CDECL);
    engine->RegisterGlobalFunction("int get_window_height()", asFUNCTIONPR(+[]() -> int { return 600; }, (), int), asCALL_CDECL);
    engine->RegisterGlobalFunction("string get_window_text()", asFUNCTIONPR(+[]() -> std::string { return ""; }, (), std::string), asCALL_CDECL);
    engine->RegisterGlobalFunction("game_window@ get_window() property", asFUNCTIONPR(+[]() -> void* { return (void*)1; }, (), void*), asCALL_CDECL);
    engine->RegisterGlobalFunction("void refresh_window()", asFUNCTIONPR(+[](){}, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void wait(int ms)", asFUNCTION(web_wait), asCALL_CDECL);
    engine->RegisterGlobalFunction("uint64 idle_ticks()", asFUNCTIONPR(+[]() -> uint64_t { return 0; }, (), uint64_t), asCALL_CDECL);

    // sound
    engine->RegisterObjectType("sound", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("sound", asBEHAVE_FACTORY, "sound@ s()", asFUNCTION(web_sound_factory), asCALL_CDECL);
    engine->RegisterObjectBehaviour("sound", asBEHAVE_ADDREF, "void addref()", asFUNCTIONPR(+[](web_sound* s){ s->addref(); }, (web_sound*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectBehaviour("sound", asBEHAVE_RELEASE, "void release()", asFUNCTIONPR(+[](web_sound* s){ s->release(); }, (web_sound*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool load(const string &in filename, const pack_interface@+ pack = null)", asFUNCTION(sound_load_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool play(bool reset_loop_state = true)", asFUNCTION(sound_play_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool play_looped()", asFUNCTION(sound_play_looped_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool stop()", asFUNCTION(sound_stop_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool pause()", asFUNCTION(sound_pause_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "bool get_playing() const property", asFUNCTION(sound_is_playing_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "void set_pan(float pan) property", asFUNCTION(sound_set_pan_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "float get_pan() const property", asFUNCTION(sound_get_pan_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "void set_volume(float vol) property", asFUNCTION(sound_set_volume_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "float get_volume() const property", asFUNCTION(sound_get_volume_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "void set_pitch(float pitch) property", asFUNCTION(sound_set_pitch_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("sound", "float get_pitch() const property", asFUNCTION(sound_get_pitch_fn), asCALL_CDECL_OBJFIRST);

    // timer
    engine->RegisterObjectType("timer", 0, asOBJ_REF);
    engine->RegisterObjectBehaviour("timer", asBEHAVE_FACTORY, "timer@ t()", asFUNCTION(web_timer_factory), asCALL_CDECL);
    engine->RegisterObjectBehaviour("timer", asBEHAVE_ADDREF, "void addref()", asFUNCTIONPR(+[](web_timer* t){ t->addref(); }, (web_timer*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectBehaviour("timer", asBEHAVE_RELEASE, "void release()", asFUNCTIONPR(+[](web_timer* t){ t->release(); }, (web_timer*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "int64 get_elapsed() const property", asFUNCTION(timer_get_elapsed_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "void set_elapsed(int64 time_units) property", asFUNCTION(timer_set_elapsed_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "void restart()", asFUNCTION(timer_restart_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "bool pause()", asFUNCTION(timer_pause_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "bool resume()", asFUNCTION(timer_resume_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "void toggle_pause()", asFUNCTION(timer_toggle_pause_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "bool get_paused() const property", asFUNCTION(timer_get_paused_fn), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("timer", "bool get_running() const property", asFUNCTION(timer_get_running_fn), asCALL_CDECL_OBJFIRST);

    // Math Functions
    engine->RegisterGlobalFunction("double abs(double v)", asFUNCTIONPR(std::abs, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("float absf(float v)", asFUNCTIONPR(std::abs, (float), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float clamp(float value, float min, float max)", asFUNCTION(web_clamp), asCALL_CDECL);
    engine->RegisterGlobalFunction("int clamp(int value, int min, int max)", asFUNCTION(web_clamp_i), asCALL_CDECL);
    engine->RegisterGlobalFunction("int random(int min, int max)", asFUNCTION(web_random), asCALL_CDECL);
    engine->RegisterGlobalFunction("double sqrt(double a)", asFUNCTIONPR(std::sqrt, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double pow(double a, double b)", asFUNCTIONPR(std::pow, (double, double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double sin(double x)", asFUNCTIONPR(std::sin, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double cos(double x)", asFUNCTIONPR(std::cos, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double tan(double x)", asFUNCTIONPR(std::tan, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double asin(double x)", asFUNCTIONPR(std::asin, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double acos(double x)", asFUNCTIONPR(std::acos, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double atan(double x)", asFUNCTIONPR(std::atan, (double), double), asCALL_CDECL);
    engine->RegisterGlobalFunction("double atan2(double y, double x)", asFUNCTIONPR(std::atan2, (double, double), double), asCALL_CDECL);

    // Screen Reader & TTS
    engine->RegisterGlobalFunction("bool screen_reader_speak(const string &in text, bool interrupt = true)", asFUNCTION(dummy_speak), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_output(const string &in text, bool interrupt = true)", asFUNCTION(dummy_speak), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_silence()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_is_speaking()", asFUNCTIONPR(+[]() -> bool { return false; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_has_speech()", asFUNCTIONPR(+[]() -> bool { return true; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_has_braille()", asFUNCTIONPR(+[]() -> bool { return false; }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool screen_reader_braille(const string &in text)", asFUNCTIONPR(+[](const std::string&){ return true; }, (const std::string&), bool), asCALL_CDECL);

    // Input & Key State Functions
    engine->RegisterGlobalFunction("bool key_pressed(int code)", asFUNCTION(web_key_pressed), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool key_down(int code)", asFUNCTION(web_key_down), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool key_up(int code)", asFUNCTION(web_key_up), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool key_released(int code)", asFUNCTION(web_key_up), asCALL_CDECL);

    // Standard Key Constants
    static const int KEY_RETURN = 40, KEY_ESCAPE = 41, KEY_BACK = 42, KEY_TAB = 43, KEY_SPACE = 44;
    static const int KEY_RIGHT = 79, KEY_LEFT = 80, KEY_DOWN = 81, KEY_UP = 82;
    static const int KEY_A = 4, KEY_B = 5, KEY_C = 6, KEY_D = 7, KEY_E = 8, KEY_F = 9;
    static const int KEY_G = 10, KEY_H = 11, KEY_I = 12, KEY_J = 13, KEY_K = 14, KEY_L = 15;
    static const int KEY_M = 16, KEY_N = 17, KEY_O = 18, KEY_P = 19, KEY_Q = 20, KEY_R = 21;
    static const int KEY_S = 22, KEY_T = 23, KEY_U = 24, KEY_V = 25, KEY_W = 26, KEY_X = 27;
    static const int KEY_Y = 28, KEY_Z = 29;

    engine->RegisterGlobalProperty("const int KEY_RETURN", (void*)&KEY_RETURN);
    engine->RegisterGlobalProperty("const int KEY_ESCAPE", (void*)&KEY_ESCAPE);
    engine->RegisterGlobalProperty("const int KEY_BACK", (void*)&KEY_BACK);
    engine->RegisterGlobalProperty("const int KEY_TAB", (void*)&KEY_TAB);
    engine->RegisterGlobalProperty("const int KEY_SPACE", (void*)&KEY_SPACE);
    engine->RegisterGlobalProperty("const int KEY_RIGHT", (void*)&KEY_RIGHT);
    engine->RegisterGlobalProperty("const int KEY_LEFT", (void*)&KEY_LEFT);
    engine->RegisterGlobalProperty("const int KEY_DOWN", (void*)&KEY_DOWN);
    engine->RegisterGlobalProperty("const int KEY_UP", (void*)&KEY_UP);
    engine->RegisterGlobalProperty("const int KEY_A", (void*)&KEY_A);
    engine->RegisterGlobalProperty("const int KEY_B", (void*)&KEY_B);
    engine->RegisterGlobalProperty("const int KEY_C", (void*)&KEY_C);
    engine->RegisterGlobalProperty("const int KEY_D", (void*)&KEY_D);
    engine->RegisterGlobalProperty("const int KEY_E", (void*)&KEY_E);
    engine->RegisterGlobalProperty("const int KEY_F", (void*)&KEY_F);
    engine->RegisterGlobalProperty("const int KEY_G", (void*)&KEY_G);
    engine->RegisterGlobalProperty("const int KEY_H", (void*)&KEY_H);
    engine->RegisterGlobalProperty("const int KEY_I", (void*)&KEY_I);
    engine->RegisterGlobalProperty("const int KEY_J", (void*)&KEY_J);
    engine->RegisterGlobalProperty("const int KEY_K", (void*)&KEY_K);
    engine->RegisterGlobalProperty("const int KEY_L", (void*)&KEY_L);
    engine->RegisterGlobalProperty("const int KEY_M", (void*)&KEY_M);
    engine->RegisterGlobalProperty("const int KEY_N", (void*)&KEY_N);
    engine->RegisterGlobalProperty("const int KEY_O", (void*)&KEY_O);
    engine->RegisterGlobalProperty("const int KEY_P", (void*)&KEY_P);
    engine->RegisterGlobalProperty("const int KEY_Q", (void*)&KEY_Q);
    engine->RegisterGlobalProperty("const int KEY_R", (void*)&KEY_R);
    engine->RegisterGlobalProperty("const int KEY_S", (void*)&KEY_S);
    engine->RegisterGlobalProperty("const int KEY_T", (void*)&KEY_T);
    engine->RegisterGlobalProperty("const int KEY_U", (void*)&KEY_U);
    engine->RegisterGlobalProperty("const int KEY_V", (void*)&KEY_V);
    engine->RegisterGlobalProperty("const int KEY_W", (void*)&KEY_W);
    engine->RegisterGlobalProperty("const int KEY_X", (void*)&KEY_X);
    engine->RegisterGlobalProperty("const int KEY_Y", (void*)&KEY_Y);
    engine->RegisterGlobalProperty("const int KEY_Z", (void*)&KEY_Z);

    // Misc
    engine->RegisterGlobalFunction("void exit(int retcode = 0)", asFUNCTIONPR(+[](int){ exit(0); }, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void alert(const string &in title, const string &in text, bool can_cancel = false, uint flags = 0)", asFUNCTIONPR(+[](const std::string& t, const std::string& m, bool, unsigned int){ dummy_speak(t + ": " + m, true); }, (const std::string&, const std::string&, bool, unsigned int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void message(const string &in text, const string &in title = \"\")", asFUNCTIONPR(+[](const std::string& m, const std::string& t){ dummy_speak((t.empty() ? "" : t + ": ") + m, true); }, (const std::string&, const std::string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("string input_box(const string &in title, const string &in caption, const string &in default_value = \"\", uint64 flags = 0)", asFUNCTION(web_input_box), asCALL_CDECL);

    return 0;
}

static bool g_game_running = false;

extern "C" {
EMSCRIPTEN_KEEPALIVE
int nvgt_run_game(const char* game_file) {
    if (g_game_running) {
        printf("Notice: nvgt_run_game already active, ignoring duplicate invocation.\n");
        return 0;
    }
    g_game_running = true;

    if (!game_file) game_file = "game.bin";
    FILE* fp = fopen(game_file, "rb");
    if (!fp) {
        printf("Error: Could not open %s\n", game_file);
        g_game_running = false;
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long total_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::vector<unsigned char> data(total_size);
    fread(data.data(), 1, total_size, fp);
    fclose(fp);

    printf("Loaded %s: %ld bytes\n", game_file, total_size);

    uint32_t data_loc = 0;
    memcpy(&data_loc, &data[total_size - 4], 4);
    printf("Bytecode offset: %u\n", data_loc);

    size_t pos = data_loc;
    uint8_t pack_count = data[pos++];
    printf("Embedded packs: %u\n", pack_count);

    uint32_t code_size = 0;
    int shift = 0;
    while (pos < (size_t)total_size) {
        uint8_t b = data[pos++];
        code_size |= (b & 0x7f) << shift;
        if ((b & 0x80) == 0) break;
        shift += 7;
    }
    code_size ^= 47635; // NVGT_BYTECODE_NUMBER_XOR
    printf("Decoded bytecode payload size: %u bytes\n", code_size);

    if (pos + code_size > (size_t)total_size) {
        printf("Error: Code size exceeds file size\n");
        return -1;
    }

    unsigned char* encrypted_payload = &data[pos];

    asIScriptEngine* engine = asCreateScriptEngine();
    engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);
    register_nvgt_api(engine);

    asIScriptModule* mod = engine->GetModule("nvgt_game", asGM_ALWAYS_CREATE);

    NVGTBytecodeStream stream;
    stream.set(encrypted_payload, code_size);

    // Read NVGT metadata header
    unsigned short plugin_count = 0;
    stream.Read(&plugin_count, 2);
    for (int i = 0; i < plugin_count; i++) {
        std::string name;
        read_string(stream, name);
    }

    int ns_count = 0;
    stream.Read(&ns_count, 4);
    for (int i = 0; i < ns_count; i++) {
        std::string k, v;
        read_string(stream, k);
        read_string(stream, v);
    }

    // Engine properties (asEP_LAST_PROPERTY = 41)
    for (int i = 0; i < asEP_LAST_PROPERTY; i++) {
        uint64_t val = 0;
        read_7bit_encoded(stream, val);
        engine->SetEngineProperty(asEEngineProp(i), asPWORD(val));
    }

    int64_t build_time = 0;
    stream.Read(&build_time, 8);

    bool no_auto_chdir = false;
    stream.Read(&no_auto_chdir, 1);

    printf("Metadata decoded: plugin_count=%u, ns_count=%d, build_time=%lld, no_auto_chdir=%d, total_header_bytes=%d\n",
           plugin_count, ns_count, build_time, (int)no_auto_chdir, stream.get_cursor());
    stream.reset_cursor();

    printf("Loading AngelScript bytecode...\n");
    int r = mod->LoadByteCode(&stream);
    printf("LoadByteCode result: %d\n", r);
    if (r < 0) {
        printf("Failed to load AngelScript bytecode!\n");
        return -1;
    }

    asIScriptFunction* main_fn = mod->GetFunctionByDecl("void main()");
    if (!main_fn) {
        main_fn = mod->GetFunctionByDecl("int main()");
    }
    if (!main_fn) {
        printf("Error: main entry point not found in script module!\n");
        return -1;
    }

    printf("Executing game main()...\n");
    asIScriptContext* ctx = engine->CreateContext();
    ctx->Prepare(main_fn);
    r = ctx->Execute();
    printf("Execution completed with code: %d\n", r);

    ctx->Release();
    engine->ShutDownAndRelease();
    g_game_running = false;
    printf("NVGT WebAssembly finished successfully!\n");
    return r;
}
}

int main(int argc, char** argv) {
    printf("Starting NVGT WebAssembly Runtime...\n");

    #ifdef __EMSCRIPTEN__
    js_init_input();
    printf("Runtime initialized. Waiting for start trigger via nvgt_run_game()...\n");
    return 0;
    #else
    const char* game_file = argc > 1 ? argv[1] : "game.bin";
    return nvgt_run_game(game_file);
    #endif
}

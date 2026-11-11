/* web.cpp - module containing WebAssembly and Emscripten platform functions
 *
 * NVGT - NonVisual Gaming Toolkit
 * Copyright (c) 2022-2026 Sam Tupy
 * https://nvgt.dev
 * This software is provided "as-is", without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "web.h"
#include "xplatform.h"
#include <string>
#include <vector>

static std::string g_web_proxy_url;

void web_set_proxy_url(const std::string& url) {
	g_web_proxy_url = url;
}

std::string web_get_proxy_url() {
	return g_web_proxy_url;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS(void, nvgt_js_speak_aria, (const char* text, int interrupt), {
	let spk = document.getElementById('nvgt-speaker');
	if (!spk) {
		spk = document.createElement('div');
		spk.id = 'nvgt-speaker';
		spk.setAttribute('aria-live', 'assertive');
		spk.setAttribute('aria-atomic', 'true');
		spk.setAttribute('role', 'alert');
		spk.style.position = 'absolute';
		spk.style.left = '-9999px';
		spk.style.width = '1px';
		spk.style.height = '1px';
		spk.style.overflow = 'hidden';
		document.body.appendChild(spk);
	}
	const str = UTF8ToString(text);
	if (interrupt) {
		spk.textContent = '';
	}
	// Small timeout ensures screen readers register rapid sequential updates
	setTimeout(() => {
		spk.textContent = str;
	}, 10);
});

EM_JS(void, nvgt_js_speak_synth, (const char* text, int interrupt, float rate, float pitch, float volume, int voice_idx), {
	if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
		if (interrupt) {
			window.speechSynthesis.cancel();
		}
		const u = new SpeechSynthesisUtterance(UTF8ToString(text));
		u.rate = rate;
		u.pitch = pitch;
		u.volume = volume;
		const voices = window.speechSynthesis.getVoices();
		if (voice_idx >= 0 && voice_idx < voices.length) {
			u.voice = voices[voice_idx];
		}
		window.speechSynthesis.speak(u);
	}
});

EM_JS(void, nvgt_js_silence, (), {
	let spk = document.getElementById('nvgt-speaker');
	if (spk) spk.textContent = '';
	if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
		window.speechSynthesis.cancel();
	}
});

EM_JS(int, nvgt_js_is_speaking, (), {
	if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
		return window.speechSynthesis.speaking ? 1 : 0;
	}
	return 0;
});

EM_JS(int, nvgt_js_get_voice_count, (), {
	if (typeof window !== 'undefined' && 'speechSynthesis' in window) {
		return window.speechSynthesis.getVoices().length;
	}
	return 0;
});

bool web_screen_reader_speak(const std::string& text, bool interrupt) {
	nvgt_js_speak_aria(text.c_str(), interrupt ? 1 : 0);
	return true;
}

bool web_screen_reader_silence() {
	nvgt_js_silence();
	return true;
}

std::string web_screen_reader_detect() {
	return "ARIA Live / Web Speech";
}

bool web_is_screen_reader_active() {
	return true;
}

web_speech_engine::web_speech_engine() : tts_engine_impl("Web") {}

bool web_speech_engine::is_available() {
	return true;
}

bool web_speech_engine::speak(const std::string &text, bool interrupt, bool blocking) {
	nvgt_js_speak_synth(text.c_str(), interrupt ? 1 : 0, rate, pitch, volume, voice_index);
	if (blocking) {
		while (is_speaking()) {
			emscripten_sleep(10);
		}
	}
	return true;
}

bool web_speech_engine::is_speaking() {
	return nvgt_js_is_speaking() != 0;
}

bool web_speech_engine::stop() {
	nvgt_js_silence();
	return true;
}

int web_speech_engine::get_voice_count() {
	return nvgt_js_get_voice_count();
}

std::string web_speech_engine::get_voice_name(int index) {
	return "Web Voice " + std::to_string(index);
}

std::string web_speech_engine::get_voice_language(int index) {
	return "en-US";
}

bool web_speech_engine::set_voice(int index) {
	voice_index = index;
	return true;
}

void register_native_tts() {
	tts_engine_register("Web", []() -> std::shared_ptr<tts_engine> {
		return std::make_shared<web_speech_engine>();
	});
}

#else

// Desktop fallback stubs when web.cpp is referenced outside Emscripten
bool web_screen_reader_speak(const std::string& text, bool interrupt) { return false; }
bool web_screen_reader_silence() { return false; }
std::string web_screen_reader_detect() { return ""; }
bool web_is_screen_reader_active() { return false; }

web_speech_engine::web_speech_engine() : tts_engine_impl("Web") {}
bool web_speech_engine::is_available() { return false; }
bool web_speech_engine::speak(const std::string &text, bool interrupt, bool blocking) { return false; }
bool web_speech_engine::is_speaking() { return false; }
bool web_speech_engine::stop() { return false; }
int web_speech_engine::get_voice_count() { return 0; }
std::string web_speech_engine::get_voice_name(int index) { return ""; }
std::string web_speech_engine::get_voice_language(int index) { return ""; }
bool web_speech_engine::set_voice(int index) { return false; }

#endif

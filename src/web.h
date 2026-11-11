/* web.h - header containing functions only applicable to the WebAssembly/Emscripten platform
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

#pragma once
#include <string>
#include <vector>
#include "tts.h"

// Screen reader integration via ARIA Live Region & Web Speech API
bool web_screen_reader_speak(const std::string& text, bool interrupt = false);
bool web_screen_reader_silence();
std::string web_screen_reader_detect();
bool web_is_screen_reader_active();

// Web TTS engine leveraging HTML5 SpeechSynthesis
class web_speech_engine : public tts_engine_impl {
	float rate = 1.0f;
	float pitch = 1.0f;
	float volume = 1.0f;
	int voice_index = 0;
public:
	web_speech_engine();
	virtual ~web_speech_engine() override = default;
	virtual bool is_available() override;
	virtual bool speak(const std::string &text, bool interrupt = false, bool blocking = false) override;
	virtual bool is_speaking() override;
	virtual bool stop() override;
	virtual float get_rate() override { return rate; }
	virtual float get_pitch() override { return pitch; }
	virtual float get_volume() override { return volume; }
	virtual void set_rate(float r) override { rate = r; }
	virtual void set_pitch(float p) override { pitch = p; }
	virtual void set_volume(float v) override { volume = v; }
	virtual bool get_rate_range(float& minimum, float& midpoint, float& maximum) override { minimum = 0.1f; midpoint = 1.0f; maximum = 10.0f; return true; }
	virtual bool get_pitch_range(float& minimum, float& midpoint, float& maximum) override { minimum = 0.0f; midpoint = 1.0f; maximum = 2.0f; return true; }
	virtual bool get_volume_range(float& minimum, float& midpoint, float& maximum) override { minimum = 0.0f; midpoint = 0.5f; maximum = 1.0f; return true; }
	virtual int get_voice_count() override;
	virtual std::string get_voice_name(int index) override;
	virtual std::string get_voice_language(int index) override;
	virtual bool set_voice(int index) override;
	virtual int get_current_voice() override { return voice_index; }
};

// Web Proxy configuration for proxying UDP/TCP networking
void web_set_proxy_url(const std::string& url);
std::string web_get_proxy_url();

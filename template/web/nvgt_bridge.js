/**
 * nvgt_bridge.js - NVGT Web Player Runtime Bridge
 * Handles WebAudio unlocking, AudioBuffer decoding & spatial playback,
 * screen reader ARIA speech (NVDA/JAWS/Narrator), keyboard trapping,
 * touch inputs, and WebSocket proxy networking for WebAssembly.
 */

(function () {
  'use strict';

  // State
  let audioUnlocked = false;
  let webProxyUrl = '';
  let activeProxySocket = null;
  let gameBinData = null;
  let gameStarted = false;

  // Web Audio System
  const audioBuffers = {};       // url/path -> AudioBuffer
  const pendingDecodes = {};     // url/path -> Array of { resolve, reject }
  const soundInstances = {};     // soundId -> instance state

  function getAudioContext() {
    if (!window.nvgtAudioContext) {
      const AudioCtxClass = window.AudioContext || window.webkitAudioContext;
      if (AudioCtxClass) {
        window.nvgtAudioContext = new AudioCtxClass();
      }
    }
    if (window.nvgtAudioContext && window.nvgtAudioContext.state === 'suspended') {
      window.nvgtAudioContext.resume().catch(() => {});
    }
    return window.nvgtAudioContext;
  }

  function loadAudio(soundId, path) {
    if (!path || typeof path !== 'string' || !path.trim()) {
      return Promise.resolve(null);
    }

    if (soundId) {
      soundInstances[soundId] = soundInstances[soundId] || {
        path: path,
        source: null,
        gainNode: null,
        pannerNode: null,
        isPlaying: false,
        loop: false,
        volume: 1.0,
        pan: 0.0,
        pitch: 100.0
      };
      soundInstances[soundId].path = path;
    }

    if (audioBuffers[path]) {
      return Promise.resolve(audioBuffers[path]);
    }

    if (pendingDecodes[path]) {
      return new Promise((resolve) => {
        pendingDecodes[path].push(resolve);
      });
    }

    pendingDecodes[path] = [];

    const decodeData = (arrayBuffer) => {
      const ctx = getAudioContext();
      if (!ctx) {
        console.warn('[NVGT Audio] AudioContext unavailable to decode:', path);
        return Promise.resolve(null);
      }
      return ctx.decodeAudioData(arrayBuffer.slice(0))
        .then((decodedBuffer) => {
          audioBuffers[path] = decodedBuffer;
          console.log('[NVGT Audio] Decoded ' + path + ' (' + decodedBuffer.duration.toFixed(2) + 's)');
          const waiters = pendingDecodes[path] || [];
          delete pendingDecodes[path];
          waiters.forEach((resolve) => resolve(decodedBuffer));
          return decodedBuffer;
        })
        .catch((err) => {
          console.warn('[NVGT Audio] decodeAudioData failed for ' + path, err);
          const waiters = pendingDecodes[path] || [];
          delete pendingDecodes[path];
          waiters.forEach((resolve) => resolve(null));
          return null;
        });
    };

    // Check Emscripten Virtual FS first
    const fs = (typeof FS !== 'undefined') ? FS : (window.Module && window.Module.FS ? window.Module.FS : null);
    if (fs) {
      try {
        const fileBytes = fs.readFile(path);
        if (fileBytes && fileBytes.buffer) {
          return decodeData(fileBytes.buffer.slice(fileBytes.byteOffset, fileBytes.byteOffset + fileBytes.byteLength));
        }
      } catch (_) {}
    }

    // Otherwise fetch via HTTP
    return fetch(path)
      .then((res) => {
        if (!res.ok) throw new Error('HTTP ' + res.status);
        return res.arrayBuffer();
      })
      .then((buf) => decodeData(buf))
      .catch((err) => {
        console.warn('[NVGT Audio] Fetch failed for ' + path, err);
        const waiters = pendingDecodes[path] || [];
        delete pendingDecodes[path];
        waiters.forEach((resolve) => resolve(null));
        return null;
      });
  }

  function playAudio(soundId, path, pan, vol, pitch, loop) {
    if (!path || typeof path !== 'string' || !path.trim()) {
      return;
    }
    const ctx = getAudioContext();
    if (!ctx) return;
    if (ctx.state === 'suspended') {
      ctx.resume().catch(() => {});
    }

    let inst = soundInstances[soundId];
    if (!inst) {
      inst = {
        path: path,
        source: null,
        gainNode: null,
        pannerNode: null,
        isPlaying: false,
        loop: !!loop,
        volume: vol,
        pan: pan,
        pitch: pitch
      };
      soundInstances[soundId] = inst;
    }
    inst.path = path;
    inst.loop = !!loop;
    inst.volume = vol;
    inst.pan = pan;
    inst.pitch = pitch;

    const startNode = (buffer) => {
      if (!buffer) return;
      try {
        if (inst.source) {
          try { inst.source.stop(); } catch (_) {}
          try { inst.source.disconnect(); } catch (_) {}
          inst.source = null;
        }

        const source = ctx.createBufferSource();
        source.buffer = buffer;
        source.loop = !!inst.loop;
        source.playbackRate.value = Math.max(0.1, Math.min(4.0, (inst.pitch || 100.0) / 100.0));

        const gainNode = ctx.createGain();
        gainNode.gain.value = Math.max(0.0, Math.min(2.0, inst.volume !== undefined ? inst.volume : 1.0));

        let tail = source;
        let pannerNode = null;
        if (ctx.createStereoPanner) {
          pannerNode = ctx.createStereoPanner();
          const normPan = Math.max(-1.0, Math.min(1.0, (inst.pan || 0.0) / 100.0));
          pannerNode.pan.value = normPan;
          tail.connect(pannerNode);
          tail = pannerNode;
        }

        tail.connect(gainNode);
        gainNode.connect(ctx.destination);

        inst.source = source;
        inst.gainNode = gainNode;
        inst.pannerNode = pannerNode;
        inst.isPlaying = true;

        source.onended = () => {
          if (inst.source === source) {
            inst.isPlaying = false;
            inst.source = null;
          }
        };

        source.start(0);
      } catch (e) {
        console.warn('[NVGT Audio] Play error:', e);
      }
    };

    if (audioBuffers[path]) {
      startNode(audioBuffers[path]);
    } else {
      loadAudio(soundId, path).then((buf) => {
        if (buf && inst.path === path) {
          startNode(buf);
        }
      });
    }
  }

  function stopAudio(soundId) {
    const inst = soundInstances[soundId];
    if (inst) {
      if (inst.source) {
        try { inst.source.stop(); } catch (_) {}
        try { inst.source.disconnect(); } catch (_) {}
        inst.source = null;
      }
      inst.isPlaying = false;
    }
  }

  function pauseAudio(soundId) {
    stopAudio(soundId);
  }

  function setAudioPan(soundId, pan) {
    const inst = soundInstances[soundId];
    if (inst) {
      inst.pan = pan;
      if (inst.pannerNode) {
        inst.pannerNode.pan.value = Math.max(-1.0, Math.min(1.0, pan / 100.0));
      }
    }
  }

  function setAudioVolume(soundId, vol) {
    const inst = soundInstances[soundId];
    if (inst) {
      inst.volume = vol;
      if (inst.gainNode) {
        inst.gainNode.gain.value = Math.max(0.0, Math.min(2.0, vol));
      }
    }
  }

  function setAudioPitch(soundId, pitch) {
    const inst = soundInstances[soundId];
    if (inst) {
      inst.pitch = pitch;
      if (inst.source) {
        inst.source.playbackRate.value = Math.max(0.1, Math.min(4.0, pitch / 100.0));
      }
    }
  }

  function isAudioPlaying(soundId) {
    const inst = soundInstances[soundId];
    return inst ? !!inst.isPlaying : false;
  }

  function destroyAudio(soundId) {
    stopAudio(soundId);
    delete soundInstances[soundId];
  }

  // Screen Reader ARIA Speech (Never uses Web Speech API to avoid collision with NVDA/JAWS)
  function speakAria(text, interrupt) {
    let spk = document.getElementById('nvgt-speaker');
    if (!spk) {
      spk = document.createElement('div');
      spk.id = 'nvgt-speaker';
      spk.className = 'sr-only';
      spk.setAttribute('aria-live', 'assertive');
      spk.setAttribute('aria-atomic', 'true');
      spk.setAttribute('role', 'alert');
      document.body.appendChild(spk);
    }
    if (interrupt) {
      spk.textContent = '';
    }
    // Timeout gives screen reader DOM mutation observers time to catch new content
    setTimeout(() => {
      spk.textContent = '';
      const span = document.createElement('span');
      span.textContent = text;
      spk.appendChild(span);
    }, 20);
  }

  // Read proxy URL from metadata
  const proxyMeta = document.querySelector('meta[name="nvgt-web-proxy"]');
  if (proxyMeta && proxyMeta.content && !proxyMeta.content.startsWith('%')) {
    webProxyUrl = proxyMeta.content.trim();
  }

  // Pre-fetch game.bin immediately on load so it is available before or as WASM boots
  fetch('game.bin')
    .then((res) => {
      if (!res.ok) throw new Error('HTTP ' + res.status);
      return res.arrayBuffer();
    })
    .then((buf) => {
      gameBinData = new Uint8Array(buf);
      console.log('game.bin preloaded (' + gameBinData.length + ' bytes)');
      saveGameBinToFS();
    })
    .catch((err) => {
      console.warn('game.bin fetch notice:', err);
    });

  function saveGameBinToFS() {
    if (!gameBinData) return;
    const fs = (typeof FS !== 'undefined') ? FS : (window.Module && window.Module.FS ? window.Module.FS : null);
    if (fs) {
      try {
        fs.writeFile('game.bin', gameBinData);
        console.log('game.bin written to Emscripten virtual filesystem.');
      } catch (e) {
        // Already exists or written
      }
    }
  }

  // Set up Emscripten Module object
  window.Module = window.Module || {};
  Module.canvas = document.getElementById('canvas');

  Module.setStatus = function (text) {
    const statusText = document.getElementById('status-text');
    if (statusText) {
      if (text) {
        statusText.textContent = text;
        document.getElementById('status-bar').style.display = 'block';
      } else {
        document.getElementById('status-bar').style.display = 'none';
      }
    }
  };

  Module.onRuntimeInitialized = function () {
    Module.setStatus('');
    console.log('NVGT WebAssembly Engine initialized successfully.');
    saveGameBinToFS();
    if (audioUnlocked && !gameStarted) {
      launchGame();
    }
  };

  function launchGame() {
    if (gameStarted) return;
    saveGameBinToFS();
    if (typeof Module !== 'undefined' && Module.ccall) {
      gameStarted = true;
      try {
        console.log('Launching NVGT game via nvgt_run_game()...');
        Module.ccall('nvgt_run_game', 'number', ['string'], ['game.bin'], { async: true });
      } catch (err) {
        console.warn('nvgt_run_game notice:', err);
      }
    }
  }

  // Audio Context Unlocker
  function unlockAudio() {
    if (audioUnlocked) return;
    audioUnlocked = true;

    // Hide splash overlay
    const overlay = document.getElementById('splash-overlay');
    if (overlay) {
      overlay.classList.add('hidden');
    }

    // Focus game canvas
    const canvas = document.getElementById('canvas');
    if (canvas) {
      canvas.focus();
    }

    // Initialize and resume WebAudio AudioContext on user gesture
    getAudioContext();

    // Announce start to screen readers
    speakAria('Game started.', true);

    // Launch game
    launchGame();
  }

  // Bind unlock events
  window.addEventListener('DOMContentLoaded', () => {
    const overlay = document.getElementById('splash-overlay');

    if (overlay) {
      overlay.focus();
      overlay.addEventListener('click', unlockAudio);
    }

    // Automatically grab focus and announce prompt to screen reader on page load
    setTimeout(() => {
      if (!audioUnlocked) {
        speakAria('Please enter to start the game.', true);
        if (overlay) overlay.focus();
      }
    }, 150);

    window.addEventListener('keydown', (e) => {
      if (!audioUnlocked && (e.key === 'Enter' || e.key === ' ' || e.code === 'Space')) {
        unlockAudio();
      }
    });

    // Keyboard Focus & Key Trap for Game Controls
    const gameKeys = new Set([
      'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
      'Space', ' ', 'Enter', 'Escape', 'Tab', 'Backspace'
    ]);

    window.addEventListener('keydown', (e) => {
      if (audioUnlocked && gameKeys.has(e.key)) {
        // Prevent browser scrolling and default tab navigation while playing
        if (e.key !== 'Escape') {
          e.preventDefault();
        }
      }
    }, { passive: false });

    // Fullscreen Toggle
    const fsBtn = document.getElementById('fullscreen-btn');
    if (fsBtn) {
      fsBtn.addEventListener('click', () => {
        if (!document.fullscreenElement) {
          document.documentElement.requestFullscreen().catch(() => {});
        } else {
          document.exitFullscreen().catch(() => {});
        }
      });
    }

    // Touch Virtual Controls Dispatcher
    const touchButtons = document.querySelectorAll('.dpad-btn, .action-btn');
    touchButtons.forEach((btn) => {
      const key = btn.dataset.key;
      if (!key) return;

      const sendKeyEvent = (type) => {
        const ev = new KeyboardEvent(type, {
          key: key,
          code: key,
          bubbles: true,
          cancelable: true
        });
        document.dispatchEvent(ev);
        if (Module.canvas) {
          Module.canvas.dispatchEvent(ev);
        }
      };

      btn.addEventListener('touchstart', (e) => {
        e.preventDefault();
        sendKeyEvent('keydown');
      });

      btn.addEventListener('touchend', (e) => {
        e.preventDefault();
        sendKeyEvent('keyup');
      });

      btn.addEventListener('mousedown', (e) => {
        sendKeyEvent('keydown');
      });

      btn.addEventListener('mouseup', (e) => {
        sendKeyEvent('keyup');
      });
    });

    // Proxy Status indicator
    const proxyBtn = document.getElementById('proxy-status-btn');
    if (proxyBtn) {
      if (webProxyUrl) {
        proxyBtn.title = 'Web Proxy configured: ' + webProxyUrl;
        proxyBtn.style.color = '#10b981'; // Green
      } else {
        proxyBtn.title = 'Web Proxy: Not configured (Local/Singleplayer mode)';
        proxyBtn.style.color = '#9ca3af'; // Gray
      }
      proxyBtn.addEventListener('click', () => {
        const current = webProxyUrl || 'None';
        const newUrl = prompt('Enter Web Proxy URL for online multiplayer (e.g., wss://proxy.mygame.com:8080):', current);
        if (newUrl !== null) {
          webProxyUrl = newUrl.trim();
          if (webProxyUrl) {
            proxyBtn.style.color = '#10b981';
            proxyBtn.title = 'Web Proxy configured: ' + webProxyUrl;
            if (typeof Module.ccall !== 'undefined') {
              try {
                Module.ccall('web_set_proxy_url', 'void', ['string'], [webProxyUrl]);
              } catch (_) {}
            }
          } else {
            proxyBtn.style.color = '#9ca3af';
            proxyBtn.title = 'Web Proxy: None';
          }
        }
      });
    }
  });

  // Global NVGT Web API
  window.NVGT = {
    unlockAudio: unlockAudio,
    getAudioContext: getAudioContext,
    loadAudio: loadAudio,
    playAudio: playAudio,
    stopAudio: stopAudio,
    pauseAudio: pauseAudio,
    setAudioPan: setAudioPan,
    setAudioVolume: setAudioVolume,
    setAudioPitch: setAudioPitch,
    isAudioPlaying: isAudioPlaying,
    destroyAudio: destroyAudio,
    speakAria: speakAria,
    setWebProxy: function (url) {
      webProxyUrl = url;
    },
    getWebProxy: function () {
      return webProxyUrl;
    }
  };
})();

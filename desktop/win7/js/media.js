/* ==========================================================================
   MosaicOS 0.50 "Aero" — Media Player
   Synthesised demo tracks + local audio playback (WebAudio, no assets)
   ========================================================================== */
'use strict';

const MediaPlayer = {
  ctx: null, master: null, analyser: null, timer: null, step: 0, elapsed: 0,
  startedAt: 0, offset: 0, playing: false, current: 0, volume: 0.8,
  ui: null, local: null, localUrl: null
};

const TRACKS = [
  {
    title: 'Aero Breeze', artist: 'Mosaic Sessions', duration: 168, tempo: 92, type: 'sine',
    chords: [[60, 64, 67, 71], [57, 60, 64, 67], [53, 57, 60, 64], [55, 59, 62, 67]],
    melody: [72, 0, 74, 76, 0, 79, 76, 74, 72, 0, 71, 72, 74, 0, 0, 0, 69, 0, 72, 74, 0, 76, 74, 72, 71, 0, 69, 67, 0, 0, 0, 0],
    bass: [48, 45, 41, 43]
  },
  {
    title: 'Mosaic Nights', artist: 'Mosaic Sessions', duration: 186, tempo: 76, type: 'triangle',
    chords: [[57, 60, 64], [53, 57, 60], [48, 52, 55], [55, 59, 62]],
    melody: [69, 0, 72, 0, 76, 0, 74, 72, 69, 0, 67, 0, 69, 0, 0, 0, 65, 0, 69, 0, 72, 0, 69, 67, 65, 0, 64, 0, 62, 0, 0, 0],
    bass: [45, 41, 36, 43]
  },
  {
    title: 'Harmony Piano', artist: 'Mosaic Sessions', duration: 154, tempo: 64, type: 'sine',
    chords: [[60, 64, 67], [59, 62, 67], [57, 60, 64], [55, 59, 62]],
    melody: [72, 76, 79, 76, 74, 72, 71, 67, 72, 74, 76, 79, 81, 79, 76, 72, 71, 74, 76, 79, 76, 74, 72, 67, 69, 72, 74, 71, 69, 67, 64, 60],
    bass: [48, 47, 45, 43]
  }
];

function midiFreq(m) { return 440 * Math.pow(2, (m - 69) / 12); }

function initMediaAudio() {
  if (MediaPlayer.ctx) return MediaPlayer.ctx;
  const AC = window.AudioContext || window.webkitAudioContext;
  if (!AC) return null;
  const ctx = new AC();
  const master = ctx.createGain();
  const analyser = ctx.createAnalyser();
  analyser.fftSize = 128;
  master.gain.value = (Settings.data.muted ? 0 : (Settings.data.volume == null ? 80 : Settings.data.volume) / 100) * 0.5;
  master.connect(analyser);
  analyser.connect(ctx.destination);
  MediaPlayer.ctx = ctx; MediaPlayer.master = master; MediaPlayer.analyser = analyser;
  MediaPlayer.volume = Settings.data.volume == null ? 0.8 : Settings.data.volume / 100;
  return ctx;
}

MediaPlayer.setVolume = function (v) {
  MediaPlayer.volume = clamp(v, 0, 1);
  Settings.save({ volume: Math.round(v * 100), muted: false });
  if (MediaPlayer.master) MediaPlayer.master.gain.value = MediaPlayer.volume * 0.5;
  if (MediaPlayer.local) MediaPlayer.local.volume = MediaPlayer.volume;
};

MediaPlayer.note = function (freq, dur, type = 'sine', gain = 0.05) {
  const ctx = MediaPlayer.ctx;
  if (!ctx) return;
  const t = ctx.currentTime;
  const o = ctx.createOscillator(), g = ctx.createGain();
  o.type = type; o.frequency.value = freq;
  g.gain.setValueAtTime(0.0001, t);
  g.gain.exponentialRampToValueAtTime(gain, t + 0.02);
  g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
  o.connect(g).connect(MediaPlayer.master);
  o.start(t); o.stop(t + dur + 0.05);
};

MediaPlayer.playStep = function () {
  const t = TRACKS[MediaPlayer.current];
  const step = MediaPlayer.step;
  const chordIndex = Math.floor(step / 8) % t.chords.length;
  const beat = step % 8;
  if (beat === 0) {
    t.chords[chordIndex].forEach((n, i) => MediaPlayer.note(midiFreq(n), 2.1, t.type, 0.022 - i * 0.002));
    MediaPlayer.note(midiFreq(t.bass[chordIndex] - 12), 0.7, 'sine', 0.05);
  }
  if (beat === 4) MediaPlayer.note(midiFreq(t.bass[chordIndex]), 0.45, 'triangle', 0.04);
  const mel = t.melody[step % t.melody.length];
  if (mel) MediaPlayer.note(midiFreq(mel), 0.32, t.type, 0.035);
  MediaPlayer.step = (step + 1) % 32;
  if (MediaPlayer.ui) updateMediaProgress();
};

function updateMediaProgress() {
  if (!MediaPlayer.ui) return;
  const t = TRACKS[MediaPlayer.current];
  let el = MediaPlayer.elapsed;
  if (MediaPlayer.playing && !MediaPlayer.local) el = MediaPlayer.offset + (MediaPlayer.ctx.currentTime - MediaPlayer.startedAt);
  if (MediaPlayer.playing && MediaPlayer.local) el = MediaPlayer.local.currentTime;
  el = clamp(el, 0, t.duration);
  const seek = MediaPlayer.ui.querySelector('#mp-seek');
  const now = MediaPlayer.ui.querySelector('#mp-now');
  if (seek) seek.value = String(Math.round(el / t.duration * 1000));
  if (now) now.textContent = `${fmtClock(el)} / ${fmtClock(t.duration)}`;
  if (MediaPlayer.playing && !MediaPlayer.local && el >= t.duration) mediaNext(true);
}

function fmtClock(sec) {
  sec = Math.max(0, Math.floor(sec || 0));
  return `${Math.floor(sec / 60)}:${pad2(sec % 60)}`;
}

function startMediaScheduler() {
  if (MediaPlayer.timer) clearInterval(MediaPlayer.timer);
  const t = TRACKS[MediaPlayer.current];
  const ms = Math.max(80, 60000 / t.tempo / 2);
  MediaPlayer.timer = setInterval(() => { if (MediaPlayer.playing && !MediaPlayer.local) MediaPlayer.playStep(); }, ms);
}

function mediaPlay() {
  const ctx = initMediaAudio();
  if (!ctx) { toast('Reproductor', 'El audio no está disponible en este navegador.'); return; }
  if (ctx.state === 'suspended') ctx.resume();
  if (MediaPlayer.local) { MediaPlayer.local.play(); MediaPlayer.playing = true; updateMediaButtons(); return; }
  if (!MediaPlayer.playing) {
    MediaPlayer.step = Math.floor(MediaPlayer.offset / (60 / TRACKS[MediaPlayer.current].tempo / 2)) % 32;
    MediaPlayer.startedAt = ctx.currentTime;
    MediaPlayer.playing = true;
    MediaPlayer.note(midiFreq(60), 0.3, 'sine', 0.04);
    startMediaScheduler();
    updateMediaButtons();
  }
}
function mediaPause() {
  if (!MediaPlayer.playing) return;
  if (MediaPlayer.local) MediaPlayer.local.pause();
  else {
    MediaPlayer.offset = Math.min(TRACKS[MediaPlayer.current].duration, MediaPlayer.offset + (MediaPlayer.ctx.currentTime - MediaPlayer.startedAt));
    if (MediaPlayer.timer) clearInterval(MediaPlayer.timer);
    MediaPlayer.timer = null;
  }
  MediaPlayer.playing = false;
  updateMediaButtons();
}
function mediaNext(auto = false) {
  mediaStop();
  MediaPlayer.current = (MediaPlayer.current + 1) % TRACKS.length;
  MediaPlayer.offset = 0; MediaPlayer.step = 0; MediaPlayer.elapsed = 0;
  renderMediaTrack();
  if (!auto) mediaPlay();
}
function mediaPrev() {
  mediaStop();
  MediaPlayer.current = (MediaPlayer.current - 1 + TRACKS.length) % TRACKS.length;
  MediaPlayer.offset = 0; MediaPlayer.step = 0; MediaPlayer.elapsed = 0;
  renderMediaTrack();
}
function mediaStop() {
  if (MediaPlayer.timer) clearInterval(MediaPlayer.timer);
  MediaPlayer.timer = null; MediaPlayer.playing = false;
  if (MediaPlayer.local) { MediaPlayer.local.pause(); }
  updateMediaButtons();
}
function renderMediaTrack() {
  if (!MediaPlayer.ui) return;
  const t = TRACKS[MediaPlayer.current];
  const title = MediaPlayer.ui.querySelector('#mp-title');
  const artist = MediaPlayer.ui.querySelector('#mp-artist');
  if (title) title.textContent = MediaPlayer.local ? (MediaPlayer.localName || 'Archivo local') : t.title;
  if (artist) artist.textContent = MediaPlayer.local ? 'Reproducción local' : t.artist + ' · Pista ' + (MediaPlayer.current + 1) + ' de ' + TRACKS.length;
  $$('.mp-item', MediaPlayer.ui).forEach(el => el.classList.toggle('active', !MediaPlayer.local && +el.dataset.idx === MediaPlayer.current));
  updateMediaButtons();
  updateMediaProgress();
}
function updateMediaButtons() {
  if (!MediaPlayer.ui) return;
  const b = MediaPlayer.ui.querySelector('#mp-play');
  if (b) b.textContent = MediaPlayer.playing ? '❚❚' : '▶';
}

Apps.media = {
  title: 'Reproductor multimedia', icon: 'media', width: 720, height: 590, minW: 520, minH: 430,
  mount(body, win, args = {}) {
    body.classList.add('media');
    body.innerHTML = `
      <div class="mp-body">
        <div class="mp-visual">
          <canvas id="mp-canvas"></canvas>
          <div class="mp-title"><b id="mp-title">Aero Breeze</b><span id="mp-artist">Mosaic Sessions · Pista 1 de 3</span></div>
        </div>
        <div class="mp-controls">
          <div class="mp-seek"><span id="mp-now">0:00 / 2:48</span><input type="range" id="mp-seek" min="0" max="1000" value="0"><span id="mp-total">2:48</span></div>
          <div class="mp-buttons">
            <button id="mp-prev" title="Anterior">⏮</button>
            <button id="mp-play" class="mp-play" title="Reproducir / Pausar">▶</button>
            <button id="mp-next" title="Siguiente">⏭</button>
            <button id="mp-stop" title="Detener">■</button>
            <div class="mp-vol" style="margin-left:14px"><svg class="ico" style="width:15px;height:15px"><use href="#i-volume"></use></svg><input type="range" id="mp-vol" min="0" max="100" value="${Settings.data.volume == null ? 80 : Settings.data.volume}"></div>
            <button id="mp-open" title="Abrir archivo de audio" style="width:auto;border-radius:3px;padding:0 9px;font-size:10.5px">Abrir…</button>
          </div>
        </div>
        <div class="mp-list" id="mp-list">
          ${TRACKS.map((t, i) => `<div class="mp-item ${i === MediaPlayer.current ? 'active' : ''}" data-idx="${i}">${iconSvg('music')}<div><div>${t.title}</div><small style="color:#7f9db2">${t.artist}</small></div><small>${fmtClock(t.duration)}</small></div>`).join('')}
          <div class="mp-item" data-local="1" style="display:none">${iconSvg('music')}<div><div id="mp-local-name">Archivo local</div><small style="color:#7f9db2">Reproducción local</small></div></div>
        </div>
      </div>`;
    MediaPlayer.ui = body;
    const canvas = $('#mp-canvas', body), ctx = canvas.getContext('2d');
    let raf = null, visualPhase = 0;

    function drawViz() {
      const w = canvas.width = canvas.clientWidth * (window.devicePixelRatio || 1);
      const h = canvas.height = canvas.clientHeight * (window.devicePixelRatio || 1);
      const g = ctx.createLinearGradient(0, h, 0, 0);
      g.addColorStop(0, 'rgba(50,150,220,.15)'); g.addColorStop(.55, 'rgba(90,200,255,.55)'); g.addColorStop(1, 'rgba(200,245,255,.9)');
      ctx.clearRect(0, 0, w, h);
      const data = new Uint8Array(MediaPlayer.analyser ? MediaPlayer.analyser.frequencyBinCount : 64);
      if (MediaPlayer.analyser) MediaPlayer.analyser.getByteFrequencyData(data);
      const bars = 48;
      visualPhase += .03;
      for (let i = 0; i < bars; i++) {
        let v;
        if (MediaPlayer.playing && MediaPlayer.analyser) v = data[Math.floor(i * data.length / bars)] / 255;
        else v = (Math.sin(visualPhase + i * .45) * .5 + .5) * .22;
        const bh = Math.max(2, v * h * .72);
        const bw = w / bars * .58;
        const x = i * (w / bars) + (w / bars - bw) / 2;
        ctx.fillStyle = g;
        ctx.beginPath();
        const r = Math.min(4, bw / 2);
        const y = h - bh - h * .06;
        ctx.roundRect ? ctx.roundRect(x, y, bw, bh, r) : ctx.rect(x, y, bw, bh);
        ctx.fill();
      }
      raf = requestAnimationFrame(drawViz);
    }
    drawViz();

    $('#mp-play', body).onclick = () => { AudioKit.click(); MediaPlayer.playing ? mediaPause() : mediaPlay(); };
    $('#mp-next', body).onclick = () => mediaNext();
    $('#mp-prev', body).onclick = () => mediaPrev();
    $('#mp-stop', body).onclick = () => { mediaStop(); MediaPlayer.offset = 0; MediaPlayer.elapsed = 0; if (MediaPlayer.local) MediaPlayer.local.currentTime = 0; updateMediaProgress(); };
    $('#mp-seek', body).oninput = e => {
      const t = TRACKS[MediaPlayer.current];
      const target = +e.target.value / 1000 * t.duration;
      if (MediaPlayer.local) MediaPlayer.local.currentTime = target;
      else {
        MediaPlayer.offset = target; MediaPlayer.step = Math.floor(target / (60 / t.tempo / 2)) % 32;
        if (MediaPlayer.playing) { MediaPlayer.startedAt = MediaPlayer.ctx.currentTime; }
      }
      updateMediaProgress();
    };
    $('#mp-vol', body).oninput = e => { MediaPlayer.setVolume(+e.target.value / 100); };
    async function openLocal(p) {
      if (!p) return;
      try {
        let src;
        if (FS.native) {
          src = 'api/fs/raw?path=' + encodeURIComponent(p);
        } else {
          const data = await FS.read(p);
          src = data;
          if (!/^data:/.test(src)) {
            const ext = extOf(p) || 'mp3';
            src = `data:audio/${ext === 'mp3' ? 'mpeg' : ext};base64,` + btoa(unescape(encodeURIComponent(data)));
          }
        }
        mediaStop();
        if (MediaPlayer.localUrl) URL.revokeObjectURL(MediaPlayer.localUrl);
        MediaPlayer.local = new Audio(src);
        MediaPlayer.local.volume = MediaPlayer.volume;
        MediaPlayer.localName = baseName(p);
        MediaPlayer.local.addEventListener('ended', () => mediaNext(true));
        MediaPlayer.local.addEventListener('timeupdate', updateMediaProgress);
        MediaPlayer.local.addEventListener('loadedmetadata', () => {
          const total = $('#mp-total', body);
          if (total && isFinite(MediaPlayer.local.duration)) total.textContent = fmtClock(MediaPlayer.local.duration);
        });
        const localItem = $('[data-local]', body);
        if (localItem) { localItem.style.display = 'flex'; $('#mp-local-name', body).textContent = MediaPlayer.localName; }
        renderMediaTrack();
        MediaPlayer.local.play().then(() => { MediaPlayer.playing = true; updateMediaButtons(); }).catch(() => {});
        toast('Reproductor', 'Cargado ' + MediaPlayer.localName);
      } catch (e) { Dialog.error('No se pudo abrir el audio', e.message); }
    }
    $('#mp-open', body).onclick = async () => {
      const p = await FileDialog.show({ mode: 'open', title: 'Abrir archivo de audio', initial: joinPath(FS.home, 'Music'), filter: ['mp3', 'wav', 'ogg', 'm4a', 'flac'] });
      openLocal(p);
    };
    $$('.mp-item[data-idx]', body).forEach(el => {
      el.onclick = () => {
        mediaStop();
        MediaPlayer.local = null;
        MediaPlayer.current = +el.dataset.idx;
        MediaPlayer.offset = 0; MediaPlayer.elapsed = 0; MediaPlayer.step = 0;
        renderMediaTrack();
      };
      el.ondblclick = () => { el.click(); mediaPlay(); };
    });
    win.onClosed = () => {
      mediaStop();
      cancelAnimationFrame(raf);
      if (MediaPlayer.ui === body) MediaPlayer.ui = null;
      if (MediaPlayer.local) { MediaPlayer.local.pause(); MediaPlayer.local = null; }
    };
    renderMediaTrack();
    if (args.path) openLocal(args.path);
  }
};

/* Keep the player reachable for the Control Panel, tray and integration tests. */
window.MediaPlayer = MediaPlayer;

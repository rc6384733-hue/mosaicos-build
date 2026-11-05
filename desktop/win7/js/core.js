/* ==========================================================================
   MosaicOS 0.50 "Aero" — core utilities, settings, virtual file system,
   dialogs and system sounds.  No external dependencies.
   ========================================================================== */
'use strict';

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
const clamp = (n, a, b) => Math.max(a, Math.min(b, n));
const uid = (p = 'id') => p + '-' + Math.random().toString(36).slice(2, 9);
const esc = (s) => String(s == null ? '' : s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const pad2 = n => String(n).padStart(2, '0');

function fmtSize(bytes) {
  if (bytes == null) return '';
  if (bytes < 1024) return bytes + ' bytes';
  if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / 1048576).toFixed(1) + ' MB';
}
function fmtDate(ts) {
  const d = new Date(ts || Date.now());
  return `${pad2(d.getDate())}/${pad2(d.getMonth() + 1)}/${d.getFullYear()} ${pad2(d.getHours())}:${pad2(d.getMinutes())}`;
}
function fmtTime(d = new Date(), seconds = false) {
  let h = d.getHours(), m = pad2(d.getMinutes()), s = pad2(d.getSeconds());
  const ap = h >= 12 ? 'PM' : 'AM';
  h = h % 12 || 12;
  return `${h}:${m}${seconds ? ':' + s : ''} ${ap}`;
}
function extOf(name) {
  const i = name.lastIndexOf('.');
  return i > 0 ? name.slice(i + 1).toLowerCase() : '';
}
function baseName(p) { return p.replace(/[\\/]+$/, '').split(/[\\/]/).pop() || p; }
function dirName(p) { const q = p.replace(/[\\/]+$/, ''); const i = q.lastIndexOf('/'); return i <= 0 ? '/' : q.slice(0, i); }
function joinPath(a, b) {
  if (!b) return a;
  if (b.startsWith('/')) return b;
  return (a === '/' ? '' : a.replace(/\/+$/, '')) + '/' + b;
}
function iconForName(name) {
  const e = extOf(name);
  if (['png', 'jpg', 'jpeg', 'gif', 'bmp', 'svg', 'webp'].includes(e)) return 'image';
  if (['mp3', 'wav', 'ogg', 'flac', 'm4a'].includes(e)) return 'music';
  if (['mp4', 'avi', 'mkv', 'webm', 'mov'].includes(e)) return 'video';
  if (['txt', 'log', 'md', 'ini', 'conf', 'csv'].includes(e)) return 'text';
  if (['exe', 'msi', 'bat', 'sh', 'bin'].includes(e)) return 'program';
  if (['zip', '7z', 'rar', 'tar', 'gz'].includes(e)) return 'archive';
  return 'file';
}
function iconSvg(name, cls = 'ico') {
  return `<svg class="${cls}" aria-hidden="true"><use href="#i-${name}"></use></svg>`;
}

/* ------------------------------------------------------------------ */
/* Settings                                                           */
/* ------------------------------------------------------------------ */
const DEFAULT_SETTINGS = {
  version: 1,
  userName: 'Mosaic',
  wallpaper: 'harmony',
  accent: '#2f7fbc',
  sounds: true,
  taskbarGlass: true,
  showGadgets: true,
  iconSize: 'medium',
  firstRun: true
};
const Settings = {
  data: { ...DEFAULT_SETTINGS },
  load() {
    try {
      const raw = localStorage.getItem('mosaic.settings.v1');
      if (raw) this.data = { ...DEFAULT_SETTINGS, ...JSON.parse(raw) };
    } catch (e) { /* private mode */ }
    return this.data;
  },
  save(patch = {}) {
    Object.assign(this.data, patch);
    try { localStorage.setItem('mosaic.settings.v1', JSON.stringify(this.data)); } catch (e) {}
    window.dispatchEvent(new CustomEvent('mosaic:settings', { detail: this.data }));
  },
  reset() {
    try { localStorage.removeItem('mosaic.settings.v1'); } catch (e) {}
    this.data = { ...DEFAULT_SETTINGS };
  }
};
Settings.load();

const WALLPAPERS = {
  harmony: { name: 'Harmony', css: 'url("assets/wallpaper.svg") center / cover no-repeat #06233c' },
  aurora: { name: 'Aurora', css: 'radial-gradient(ellipse at 30% 20%, #2a7f6e, transparent 55%), radial-gradient(ellipse at 75% 70%, #123a6b, transparent 60%), linear-gradient(140deg, #03141f, #0b4d5a 50%, #061a2c)' },
  ocean: { name: 'Océano', css: 'radial-gradient(ellipse at 50% 0%, #2e9fd8, transparent 60%), linear-gradient(180deg, #0b5c8c, #07293f 70%, #03101a)' },
  sunset: { name: 'Atardecer', css: 'radial-gradient(circle at 72% 30%, #ffd27a 0 7%, transparent 8%), linear-gradient(180deg, #35234f, #8a3c5b 45%, #d97745 78%, #f2b35c)' },
  mint: { name: 'Menta', css: 'radial-gradient(ellipse at 25% 25%, #b9f0d0, transparent 55%), linear-gradient(135deg, #0f6b58, #0a3a38 60%, #06201f)' },
  graphite: { name: 'Grafito', css: 'radial-gradient(ellipse at 60% 20%, #5c6670, transparent 55%), linear-gradient(150deg, #1b2127, #0c1014 65%, #05070a)' },
  bloom: { name: 'Bloom', css: 'radial-gradient(circle at 50% 42%, rgba(150,225,255,.28), transparent 42%), radial-gradient(circle at 30% 75%, rgba(255,150,190,.18), transparent 45%), linear-gradient(150deg, #04263d, #0a4f78 55%, #041c2d)' }
};
const ACCENTS = [
  { name: 'Aero', color: '#2f7fbc' }, { name: 'Cielo', color: '#1f9bd1' },
  { name: 'Esmeralda', color: '#2e8b57' }, { name: 'Ámbar', color: '#c27c12' },
  { name: 'Rubí', color: '#b03a3a' }, { name: 'Violeta', color: '#6d4ca8' },
  { name: 'Grafito', color: '#56626d' }
];

/* ------------------------------------------------------------------ */
/* Small persistent store                                             */
/* ------------------------------------------------------------------ */
const Store = {
  get(k, d) {
    try { const v = localStorage.getItem('mosaic.' + k); return v == null ? d : JSON.parse(v); }
    catch (e) { return d; }
  },
  set(k, v) {
    try { localStorage.setItem('mosaic.' + k, JSON.stringify(v)); } catch (e) {}
  }
};

/* ------------------------------------------------------------------ */
/* File system: virtual (localStorage) or native (Python bridge)      */
/* ------------------------------------------------------------------ */
const FS = {
  native: false,
  home: '/Users/Mosaic',
  system: { os: 'MosaicOS 0.50', kernel: 'web-shell', browser: navigator.userAgent },
  _root: null,

  async init() {
    if (location.protocol === 'http:' || location.protocol === 'https:') {
      try {
        const r = await fetch('api/system', { cache: 'no-store' });
        if (r.ok) {
          const data = await r.json();
          if (data && data.ok) {
            this.native = true;
            this.home = data.home || '/home/mosaic';
            this.system = data;
            return;
          }
        }
      } catch (e) { /* demo mode */ }
    }
    this._loadVirtual();
  },

  _defaultTree() {
    const now = Date.now();
    const f = (name, content = '', ts = now) => ({ type: 'file', name, content, modified: ts, size: content.length });
    const d = (name, children = [], ts = now) => ({ type: 'dir', name, children, modified: ts });
    return d('', [
      d('Users', [
        d('Mosaic', [
          d('Desktop', [f('Bienvenido.txt', 'Bienvenido a MosaicOS 0.50 "Aero".\r\n\r\nEste escritorio es completamente interactivo:\r\n- Doble clic en los iconos para abrir programas.\r\n- Boton derecho para personalizar.\r\n- Tecla Windows para abrir el menu Inicio.\r\n- Arrastra ventanas a los bordes para organizarlas (Aero Snap).\r\n\r\nMosaicOS Project, 2026.')]),
          d('Documents', [
            f('Léeme.txt', 'MosaicOS 0.50 "Aero"\r\n=====================\r\n\r\nSistema de escritorio con interfaz clásica de Windows 7.\r\n\r\nAplicaciones incluidas:\r\n  Explorador, Notepad, Paint, Calculadora, Terminal,\r\n  Panel de control, Reproductor, Navegador, Minesweeper,\r\n  Solitaire, Snake, Notas rápidas, Administrador de tareas y más.\r\n\r\nLos archivos de esta carpeta se guardan en el navegador.\r\nEn la edición live se guardan en tu carpeta personal real.'),
            f('Notas.txt', 'Lista de tareas\r\n[ ] Probar todas las aplicaciones\r\n[ ] Cambiar el fondo del escritorio\r\n[ ] Jugar una partida de Solitaire\r\n[ ] Abrir el Panel de control'),
            d('Recetas', [f('Tortilla.txt', 'Ingredientes:\r\n- 4 huevos\r\n- 2 patatas\r\n- Aceite de oliva\r\n- Sal\r\n\r\nBatir, pochar la patata y cuajar a fuego medio.')], now - 86400000)
          ]),
          d('Downloads', [f('MosaicOS-0.50.txt', 'MosaicOS 0.50 "Aero" — notas de la versión.\r\nInterfaz Windows 7, 16 aplicaciones, shell web integrado.')]),
          d('Pictures', []),
          d('Music', []),
          d('Videos', []),
          d('Favorites', [])
        ])
      ]),
      d('Program Files', [
        d('Mosaic', [f('version.txt', 'MosaicOS 0.50 Aero')]),
        d('Internet Explorer', [f('iexplore.exe', 'Mosaic Web shell application')]),
        d('Windows Media Player', [f('wmplayer.exe', 'Mosaic Media application')])
      ]),
      d('Windows', [
        d('System32', [f('cmd.exe', 'Mosaic Terminal'), f('notepad.exe', 'Mosaic Notepad'), f('mspaint.exe', 'Mosaic Paint'), f('calc.exe', 'Mosaic Calculator')]),
        d('Web', [f('shell.html', 'MosaicOS Aero shell')])
      ]),
      d('Recycle Bin', []),
      d('MosaicOS', [f('LICENSE.txt', 'MosaicOS Project — software libre. Consulte LICENSE en el código fuente.'), f('build.txt', 'Versión 0.50 "Aero"\nEdición: web shell + Debian live')])
    ]);
  },

  _loadVirtual() {
    let tree = Store.get('fs.v1', null);
    if (!tree || !tree.children) { tree = this._defaultTree(); Store.set('fs.v1', tree); }
    this._root = tree;
    this.home = '/Users/Mosaic';
  },
  _saveVirtual() { Store.set('fs.v1', this._root); },

  _find(path) {
    const parts = String(path).split('/').filter(Boolean);
    let node = this._root;
    for (const p of parts) {
      if (!node || node.type !== 'dir') return null;
      node = node.children.find(c => c.name.toLowerCase() === p.toLowerCase());
    }
    return node;
  },
  _findParent(path) {
    const parent = this._find(dirName(path));
    return parent && parent.type === 'dir' ? parent : null;
  },
  _pathOf(node, start = this._root, prefix = '') {
    if (node === start) return prefix || '/';
    if (!start.children) return null;
    for (const c of start.children) {
      const p = prefix + '/' + c.name;
      if (c === node) return p;
      const r = this._pathOf(node, c, p);
      if (r) return r;
    }
    return null;
  },

  async list(path) {
    if (this.native) {
      const r = await fetch('api/fs/list?path=' + encodeURIComponent(path));
      if (!r.ok) throw new Error('No se puede abrir ' + path);
      return (await r.json()).items;
    }
    const node = this._find(path);
    if (!node) throw new Error('Ruta no encontrada: ' + path);
    if (node.type === 'file') return [{ name: node.name, type: 'file', size: node.size, modified: node.modified, ext: extOf(node.name) }];
    return node.children.map(c => ({
      name: c.name, type: c.type, size: c.type === 'file' ? (c.content || '').length : null,
      modified: c.modified, ext: c.type === 'file' ? extOf(c.name) : '', original: c.original || null
    })).sort((a, b) => (a.type === b.type ? a.name.localeCompare(b.name, 'es') : a.type === 'dir' ? -1 : 1));
  },

  async read(path) {
    if (this.native) {
      const r = await fetch('api/fs/read?path=' + encodeURIComponent(path));
      if (!r.ok) throw new Error('No se puede leer ' + path);
      return await r.text();
    }
    const n = this._find(path);
    if (!n || n.type !== 'file') throw new Error('Archivo no encontrado: ' + path);
    return n.content || '';
  },

  async write(path, content) {
    if (this.native) {
      const r = await fetch('api/fs/write', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ path, content }) });
      if (!r.ok) throw new Error((await r.text()) || 'No se puede guardar');
      return true;
    }
    let n = this._find(path);
    if (n) {
      if (n.type !== 'file') throw new Error('La ruta es una carpeta');
      n.content = content; n.modified = Date.now();
    } else {
      const parent = this._findParent(path);
      if (!parent) throw new Error('Carpeta no encontrada: ' + dirName(path));
      n = { type: 'file', name: baseName(path), content, modified: Date.now() };
      parent.children.push(n);
    }
    this._saveVirtual();
    return true;
  },

  async mkdir(path) {
    if (this.native) {
      const r = await fetch('api/fs/mkdir', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ path }) });
      if (!r.ok) throw new Error((await r.text()) || 'No se puede crear la carpeta');
      return true;
    }
    if (this._find(path)) throw new Error('Ya existe un elemento con ese nombre');
    const parent = this._findParent(path);
    if (!parent) throw new Error('Carpeta no encontrada');
    parent.children.push({ type: 'dir', name: baseName(path), children: [], modified: Date.now() });
    this._saveVirtual();
    return true;
  },

  async remove(path) {
    if (this.native) {
      const r = await fetch('api/fs/delete', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ path }) });
      if (!r.ok) throw new Error((await r.text()) || 'No se puede eliminar');
      return true;
    }
    const n = this._find(path);
    if (!n) throw new Error('No encontrado');
    const parent = this._findParent(path);
    parent.children = parent.children.filter(c => c !== n);
    const bin = this._find('/Recycle Bin');
    if (bin && path !== '/Recycle Bin' && !path.startsWith('/Recycle Bin/')) {
      n.original = path;
      n.name = baseName(path) + (bin.children.some(c => c.name === baseName(path)) ? ' (' + Date.now().toString().slice(-4) + ')' : '');
      bin.children.push(n);
    }
    this._saveVirtual();
    return true;
  },

  async rename(path, newName) {
    newName = String(newName).replace(/[\\/:*?"<>|]/g, '').trim();
    if (!newName) throw new Error('Nombre no válido');
    if (this.native) {
      const r = await fetch('api/fs/rename', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ path, name: newName }) });
      if (!r.ok) throw new Error((await r.text()) || 'No se puede renombrar');
      return joinPath(dirName(path), newName);
    }
    const n = this._find(path);
    if (!n) throw new Error('No encontrado');
    const parent = this._findParent(path);
    if (parent.children.some(c => c !== n && c.name.toLowerCase() === newName.toLowerCase())) throw new Error('Ya existe ese nombre');
    n.name = newName; n.modified = Date.now();
    this._saveVirtual();
    return joinPath(dirName(path), newName);
  },

  async restore(path) {
    const n = this._find(path);
    if (!n || !n.original) return false;
    const target = n.original;
    const parent = this._findParent(target);
    if (parent) {
      n.name = baseName(target);
      n.original = null;
      parent.children.push(n);
      const bin = this._findParent(path);
      if (bin) bin.children = bin.children.filter(c => c !== n);
      this._saveVirtual();
      return true;
    }
    return false;
  },

  async emptyBin() {
    const bin = this._find('/Recycle Bin');
    if (bin) { bin.children = []; this._saveVirtual(); }
  },

  async sizeOf(path) {
    const n = this._find(path);
    if (!n) return 0;
    if (n.type === 'file') return (n.content || '').length;
    let total = 0;
    for (const c of n.children) total += await this.sizeOf(joinPath(path, c.name));
    return total;
  }
};

/* ------------------------------------------------------------------ */
/* Modal dialogs                                                      */
/* ------------------------------------------------------------------ */
const Dialog = {
  layer: null,
  ensure() { this.layer = $('#dialog-layer'); return this.layer; },
  show({ title = 'MosaicOS', body = '', buttons = [{ label: 'Aceptar', value: true, primary: true }], width = 420, icon = 'info', onOpen = null }) {
    this.ensure();
    return new Promise(resolve => {
      const wrap = document.createElement('div');
      wrap.className = 'dialog-backdrop';
      wrap.innerHTML = `
        <div class="dialog" style="width:${width}px" role="dialog" aria-modal="true">
          <div class="dialog-title"><span>${esc(title)}</span><button class="dlg-x" data-v="__x">×</button></div>
          <div class="dialog-body">
            <div class="dialog-icon">${iconSvg(icon, 'ico dlg-ico')}</div>
            <div class="dialog-content">${body}</div>
          </div>
          <div class="dialog-foot"></div>
        </div>`;
      const foot = $('.dialog-foot', wrap);
      buttons.forEach(b => {
        const btn = document.createElement('button');
        btn.className = 'btn' + (b.primary ? ' primary' : '');
        btn.textContent = b.label;
        btn.dataset.v = b.value === undefined ? b.label : b.value;
        foot.appendChild(btn);
      });
      const close = v => { wrap.remove(); resolve(v); };
      wrap.addEventListener('click', e => {
        const b = e.target.closest('[data-v]');
        if (b) { close(b.dataset.v === '__x' ? null : (b.dataset.v === 'true' ? true : b.dataset.v === 'false' ? false : b.dataset.v)); }
        else if (e.target === wrap) close(null);
      });
      wrap.addEventListener('keydown', e => { if (e.key === 'Escape') close(null); });
      const layer = this.ensure();
      layer.appendChild(wrap);
      const first = $('input, textarea, button.primary', wrap) || $('button', wrap);
      setTimeout(() => first && first.focus(), 30);
      if (onOpen) onOpen(wrap, close);
    });
  },
  alert(title, message, icon = 'info') {
    return this.show({ title, body: `<p>${esc(message).replace(/\n/g, '<br>')}</p>`, icon, width: 430 });
  },
  error(title, message) {
    return this.show({ title, body: `<p>${esc(message).replace(/\n/g, '<br>')}</p>`, icon: 'error', width: 430 });
  },
  confirm(title, message, danger = false) {
    return this.show({
      title, body: `<p>${esc(message).replace(/\n/g, '<br>')}</p>`, icon: danger ? 'warning' : 'question',
      buttons: [{ label: 'Sí', value: true, primary: !danger }, { label: 'No', value: false }]
    });
  },
  prompt(title, label, value = '') {
    let current = String(value);
    return this.show({
      title,
      body: `<label class="field-label">${esc(label)}</label><input class="text-input" id="dlg-input" value="${esc(value)}">`,
      buttons: [{ label: 'Aceptar', value: '__ok', primary: true }, { label: 'Cancelar', value: false }],
      onOpen: (wrap, close) => {
        const input = $('#dlg-input', wrap);
        input.focus(); input.select();
        input.addEventListener('input', () => { current = input.value; });
        input.addEventListener('keydown', e => { if (e.key === 'Enter') { current = input.value; close('__ok'); } });
      }
    }).then(v => (v === '__ok' ? (current.trim() || null) : null));
  }
};

/* ------------------------------------------------------------------ */
/* Sounds (WebAudio, synthesised — no external assets)                */
/* ------------------------------------------------------------------ */
const AudioKit = {
  ctx: null,
  enabled: true,
  init() {
    if (!this.ctx) {
      const AC = window.AudioContext || window.webkitAudioContext;
      if (AC) this.ctx = new AC();
    }
    if (this.ctx && this.ctx.state === 'suspended') this.ctx.resume();
    return this.ctx;
  },
  tone(freq, dur = 0.12, type = 'sine', gain = 0.06, when = 0) {
    if (!Settings.data.sounds) return;
    const ctx = this.init(); if (!ctx) return;
    const t = ctx.currentTime + when;
    const o = ctx.createOscillator(), g = ctx.createGain();
    o.type = type; o.frequency.value = freq;
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(gain, t + 0.012);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g).connect(ctx.destination);
    o.start(t); o.stop(t + dur + 0.03);
  },
  click() { this.tone(1200, 0.035, 'square', 0.015); },
  notify() { this.tone(880, 0.14, 'sine', 0.05); this.tone(1320, 0.2, 'sine', 0.04, 0.09); },
  error() { this.tone(220, 0.22, 'sawtooth', 0.035); this.tone(160, 0.28, 'sawtooth', 0.03, 0.14); },
  startup() {
    if (!Settings.data.sounds) return;
    [523.25, 659.25, 783.99, 1046.5].forEach((f, i) => this.tone(f, 1.1, 'sine', 0.055, i * 0.16));
    this.tone(392, 1.4, 'triangle', 0.035, 0.1);
  },
  shutdown() {
    [783.99, 659.25, 523.25, 392].forEach((f, i) => this.tone(f, 0.5, 'sine', 0.05, i * 0.12));
  }
};

/* ------------------------------------------------------------------ */
/* Aero notification ("toast")                                        */
/* ------------------------------------------------------------------ */
function toast(title, message = '', icon = 'info', ms = 5200) {
  const host = $('#toasts');
  if (!host) return;
  const t = document.createElement('div');
  t.className = 'toast';
  t.innerHTML = `<div class="toast-icon">${iconSvg(icon, 'ico')}</div><div class="toast-text"><strong>${esc(title)}</strong><span>${esc(message)}</span></div><button class="toast-x" aria-label="Cerrar">×</button>`;
  host.appendChild(t);
  requestAnimationFrame(() => t.classList.add('show'));
  const kill = () => { t.classList.remove('show'); setTimeout(() => t.remove(), 260); };
  $('.toast-x', t).onclick = kill;
  setTimeout(kill, ms);
  AudioKit.notify();
  return t;
}

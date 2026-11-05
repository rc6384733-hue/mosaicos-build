/* ==========================================================================
   MosaicOS 0.50 "Aero" — window manager and desktop shell
   Windows · taskbar · Start menu · Aero Snap · Alt+Tab · trays · lock screen
   ========================================================================== */
'use strict';

const TASKBAR_H = 40;

/* ------------------------------------------------------------------ */
/* Window manager                                                     */
/* ------------------------------------------------------------------ */
const WM = {
  wins: new Map(),
  activeId: null,
  z: 100,
  layer: null,
  cascade: 0,

  init() {
    this.layer = $('#windows-layer');
    window.addEventListener('resize', () => this.clampAll());
  },

  clampAll() {
    const maxW = innerWidth - 8;
    const maxH = (this.layer ? this.layer.clientHeight : innerHeight - TASKBAR_H) - 8;
    for (const w of this.wins.values()) {
      if (w.element.classList.contains('maximized')) continue;
      const r = w.element.getBoundingClientRect();
      const width = Math.min(r.width, maxW);
      const height = Math.min(r.height, maxH);
      let left = clamp(r.left, 0, Math.max(0, innerWidth - width));
      let top = clamp(r.top, 0, Math.max(0, maxH - height));
      w.element.style.width = width + 'px';
      w.element.style.height = height + 'px';
      w.element.style.left = left + 'px';
      w.element.style.top = top + 'px';
    }
  },

  create(appId, args = {}) {
    const app = Apps[appId];
    if (!app) {
      Dialog.error('MosaicOS', 'No se encuentra la aplicación «' + appId + '».');
      return null;
    }
    if (!this.layer) this.layer = $('#windows-layer');
    const node = $('#tpl-window').content.firstElementChild.cloneNode(true);
    const id = uid('win');
    node.dataset.id = id;

    const availH = (this.layer.clientHeight || innerHeight - TASKBAR_H);
    const width = Math.min(app.width || 760, Math.max(300, innerWidth - 48));
    const height = Math.min(app.height || 520, Math.max(200, availH - 48));
    const offset = (this.cascade++ % 8) * 26;
    const left = clamp(Math.round((innerWidth - width) / 2) + offset - 52, 6, Math.max(6, innerWidth - width - 6));
    const top = clamp(Math.round((availH - height) / 2) + offset - 40, 4, Math.max(4, availH - height - 4));
    node.style.width = width + 'px';
    node.style.height = height + 'px';
    node.style.left = left + 'px';
    node.style.top = top + 'px';
    node.style.minWidth = (app.minW || 260) + 'px';
    node.style.minHeight = (app.minH || 150) + 'px';
    node.style.zIndex = ++this.z;

    const body = $('.win-body', node);
    const icon = app.icon || 'program';
    $('.win-icon use', node).setAttribute('href', '#i-' + icon);
    $('.win-title', node).textContent = app.title || 'Aplicación';

    this.layer.appendChild(node);

    const win = {
      id,
      appId,
      icon,
      title: app.title || appId,
      body,
      element: node,
      beforeClose: null,
      onClosed: null,
      _restore: null,
      _closing: false,
      setTitle(t) {
        const text = String(t == null ? app.title : t);
        win.title = text;
        $('.win-title', node).textContent = text;
        const btn = $('#task-buttons .task-btn[data-id="' + id + '"]', document);
        if (btn) { btn.title = text; const s = $('span', btn); if (s) s.textContent = text; }
        if (Shell && Shell.activeId === id) document.title = text + ' — MosaicOS 0.50';
      },
      close() { return WM.close(id); },
      minimize() { WM.minimize(id); },
      maximize() { WM.maximize(id); },
      restore() { WM.restore(id); },
      focus() { WM.focus(id); },
      isMaximized() { return node.classList.contains('maximized'); },
      isMinimized() { return node.classList.contains('minimized'); }
    };
    this.wins.set(id, win);

    $('.wc-min', node).addEventListener('click', e => { e.stopPropagation(); this.minimize(id); });
    $('.wc-max', node).addEventListener('click', e => { e.stopPropagation(); this.toggleMaximize(id); });
    $('.wc-close', node).addEventListener('click', e => { e.stopPropagation(); win.close(); });
    $('.win-titlebar', node).addEventListener('dblclick', e => {
      if (!e.target.closest('.win-controls')) this.toggleMaximize(id);
    });
    node.addEventListener('pointerdown', () => this.focus(id), true);

    this._bindDrag(node, win);
    this._bindResize(node, win);

    this.focus(id);

    try {
      const r = app.mount(body, win, args || {});
      if (r && typeof r.catch === 'function') r.catch(err => this._mountError(body, err));
    } catch (err) {
      this._mountError(body, err);
    }
    Shell.onWindowsChanged();
    return win;
  },

  _mountError(body, err) {
    console.error(err);
    body.innerHTML = `<div style="padding:24px;font-size:12px;color:#8f2f24">
      <b>No se pudo iniciar la aplicación.</b>
      <p style="color:#557082">${esc(err && err.message ? err.message : String(err))}</p></div>`;
  },

  focus(id) {
    const win = this.wins.get(id);
    if (!win) return;
    if (win.element.classList.contains('minimized')) win.element.classList.remove('minimized');
    this.activeId = id;
    for (const [wid, w] of this.wins) w.element.classList.toggle('inactive', wid !== id);
    win.element.style.zIndex = ++this.z;
    if (Shell) {
      Shell.activeId = id;
      Shell.onWindowsChanged();
      document.title = win.title + ' — MosaicOS 0.50';
    }
  },

  topVisible(excludeId) {
    let best = null;
    for (const [id, w] of this.wins) {
      if (id === excludeId || w.element.classList.contains('minimized')) continue;
      if (!best || (+w.element.style.zIndex || 0) > (+best.element.style.zIndex || 0)) best = w;
    }
    return best;
  },

  minimize(id) {
    const win = this.wins.get(id);
    if (!win) return;
    win.element.classList.add('minimized');
    if (this.activeId === id) {
      const next = this.topVisible(id);
      this.activeId = next ? next.id : null;
      if (next) this.focus(next.id);
      else { if (Shell) { Shell.activeId = null; Shell.onWindowsChanged(); } }
    }
    if (Shell) Shell.onWindowsChanged();
  },

  restore(id) {
    const win = this.wins.get(id);
    if (!win) return;
    if (win.element.classList.contains('minimized')) win.element.classList.remove('minimized');
    this.focus(id);
  },

  toggleMaximize(id) {
    const win = this.wins.get(id);
    if (!win) return;
    if (win.element.classList.contains('maximized')) this.restoreFromMax(id);
    else this.maximize(id);
  },

  maximize(id) {
    const win = this.wins.get(id);
    if (!win || win.element.classList.contains('maximized')) return;
    const el = win.element;
    win._restore = { left: el.style.left, top: el.style.top, width: el.style.width, height: el.style.height };
    el.classList.add('maximized');
    el.style.left = '0px';
    el.style.top = '0px';
    el.style.width = '100%';
    el.style.height = '100%';
    this.focus(id);
  },

  restoreFromMax(id) {
    const win = this.wins.get(id);
    if (!win) return;
    const el = win.element;
    el.classList.remove('maximized');
    if (win._restore) {
      el.style.left = win._restore.left || '';
      el.style.top = win._restore.top || '';
      el.style.width = win._restore.width || '';
      el.style.height = win._restore.height || '';
    } else {
      el.style.left = '60px'; el.style.top = '40px';
      el.style.width = '760px'; el.style.height = '520px';
    }
    win._restore = null;
    this.focus(id);
  },

  snap(id, side) {
    const win = this.wins.get(id);
    if (!win) return;
    const el = win.element;
    const layerH = this.layer ? this.layer.clientHeight : innerHeight - TASKBAR_H;
    if (side === 'left' || side === 'right') {
      win._restore = null;
      el.classList.remove('maximized');
      el.style.top = '0px';
      el.style.height = layerH + 'px';
      el.style.width = Math.round(innerWidth / 2) + 'px';
      el.style.left = (side === 'left' ? 0 : Math.round(innerWidth / 2)) + 'px';
    } else if (side === 'top') {
      this.maximize(id);
      return;
    }
    this.focus(id);
  },

  async close(id) {
    const win = this.wins.get(id);
    if (!win || win._closing) return;
    win._closing = true;
    try {
      if (typeof win.beforeClose === 'function') {
        const ok = await win.beforeClose();
        if (ok === false) { win._closing = false; return; }
      }
    } catch (e) {
      console.error(e);
    }
    try { if (typeof win.onClosed === 'function') win.onClosed(); } catch (e) { console.error(e); }
    win.element.remove();
    this.wins.delete(id);
    if (this.activeId === id) {
      const next = this.topVisible();
      this.activeId = next ? next.id : null;
      if (next) {
        next.element.classList.remove('inactive');
        next.element.style.zIndex = ++this.z;
        if (Shell) { Shell.activeId = next.id; document.title = next.title + ' — MosaicOS 0.50'; }
      } else if (Shell) {
        Shell.activeId = null;
        document.title = 'MosaicOS 0.50 — Aero Desktop';
      }
    }
    if (Shell) Shell.onWindowsChanged();
  },

  closeAll() {
    for (const id of Array.from(this.wins.keys())) {
      const w = this.wins.get(id);
      w.beforeClose = null;
      this.close(id);
    }
  },

  _bindDrag(node, win) {
    const bar = $('.win-titlebar', node);
    bar.addEventListener('pointerdown', e => {
      if (e.button !== 0 || e.target.closest('.win-controls')) return;
      e.preventDefault();
      this.focus(win.id);
      const rect = node.getBoundingClientRect();
      const startX = e.clientX, startY = e.clientY;
      let originX = rect.left, originY = rect.top;
      let moved = false;
      let snapped = null;

      if (node.classList.contains('maximized')) {
        const ratio = (e.clientX - rect.left) / Math.max(1, rect.width);
        this.restoreFromMax(win.id);
        const nr = node.getBoundingClientRect();
        originX = clamp(e.clientX - nr.width * ratio, 0, innerWidth - nr.width);
        originY = Math.max(0, e.clientY - 16);
        node.style.left = originX + 'px';
        node.style.top = originY + 'px';
      }

      const preview = $('#snap-preview');
      const layerH = this.layer ? this.layer.clientHeight : innerHeight - TASKBAR_H;

      const onMove = ev => {
        const dx = ev.clientX - startX, dy = ev.clientY - startY;
        if (!moved && Math.abs(dx) + Math.abs(dy) < 3) return;
        moved = true;
        const w = node.offsetWidth, h = node.offsetHeight;
        const left = clamp(originX + dx, -w + 90, innerWidth - 90);
        const top = clamp(originY + dy, 0, Math.max(0, layerH - 36));
        node.style.left = left + 'px';
        node.style.top = top + 'px';

        const edge = 8;
        snapped = null;
        if (ev.clientY <= edge) snapped = 'top';
        else if (ev.clientX <= edge) snapped = 'left';
        else if (ev.clientX >= innerWidth - edge) snapped = 'right';
        if (snapped) {
          preview.classList.remove('hidden');
          if (snapped === 'top') {
            preview.style.left = '0px'; preview.style.top = '0px';
            preview.style.width = '100%'; preview.style.height = layerH + 'px';
          } else {
            preview.style.top = '0px'; preview.style.height = layerH + 'px';
            preview.style.width = Math.round(innerWidth / 2) + 'px';
            preview.style.left = (snapped === 'left' ? 0 : Math.round(innerWidth / 2)) + 'px';
          }
        } else {
          preview.classList.add('hidden');
        }
      };

      const onUp = () => {
        document.removeEventListener('pointermove', onMove);
        document.removeEventListener('pointerup', onUp);
        preview.classList.add('hidden');
        const layer = this.layer;
        if (snapped === 'top') {
          this.maximize(win.id);
        } else if (snapped === 'left' || snapped === 'right') {
          if (win._restore) win._restore = null;
          node.classList.remove('maximized');
          node.style.top = '0px';
          node.style.height = (layer ? layer.clientHeight : innerHeight - TASKBAR_H) + 'px';
          node.style.width = Math.round(innerWidth / 2) + 'px';
          node.style.left = (snapped === 'left' ? 0 : Math.round(innerWidth / 2)) + 'px';
          this.focus(win.id);
        } else if (moved) {
          this.clampAll();
        }
      };

      document.addEventListener('pointermove', onMove);
      document.addEventListener('pointerup', onUp, { once: true });
    });
  },

  _bindResize(node, win) {
    $$('.win-resize', node).forEach(handle => {
      handle.addEventListener('pointerdown', e => {
        if (e.button !== 0 || node.classList.contains('maximized')) return;
        e.preventDefault(); e.stopPropagation();
        this.focus(win.id);
        const dir = handle.dataset.dir;
        const rect = node.getBoundingClientRect();
        const sy = e.clientY, sx = e.clientX;
        const minW = parseInt(node.style.minWidth) || 260;
        const minH = parseInt(node.style.minHeight) || 150;
        const layerH = this.layer ? this.layer.clientHeight : innerHeight - TASKBAR_H;
        let x = rect.left, y = rect.top, w = rect.width, h = rect.height;
        const onMove = ev => {
          const dx = ev.clientX - sx, dy = ev.clientY - sy;
          if (dir.includes('e')) w = clamp(rect.width + dx, minW, innerWidth - x);
          if (dir.includes('s')) h = clamp(rect.height + dy, minH, layerH - y);
          if (dir.includes('w')) {
            const nw = clamp(rect.width - dx, minW, rect.left + rect.width);
            x = rect.left + rect.width - nw; w = nw;
          }
          if (dir.includes('n')) {
            const nh = clamp(rect.height - dy, minH, rect.top + rect.height);
            y = rect.top + rect.height - nh; h = nh;
          }
          node.style.left = x + 'px'; node.style.top = y + 'px';
          node.style.width = w + 'px'; node.style.height = h + 'px';
        };
        const onUp = () => {
          document.removeEventListener('pointermove', onMove);
          document.removeEventListener('pointerup', onUp);
        };
        document.addEventListener('pointermove', onMove);
        document.addEventListener('pointerup', onUp, { once: true });
      });
    });
  }
};

/* ------------------------------------------------------------------ */
/* Shell                                                              */
/* ------------------------------------------------------------------ */
const Shell = {
  activeId: null,
  flyMonth: new Date(),
  flyKind: null,
  _allProgramsOpen: false,
  _menuCleanup: null,
  _clockTimer: null,
  _alt: { open: false, index: 0, order: [] },
  _deskIcons: null,
  _showDesktop: false,
  _iconSize: null,

  init() {
    if (this._inited) return;
    this._inited = true;
    WM.init();
    this._deskIcons = $('#desktop-icons');
    this.buildQuickLaunch();
    this.buildStartLinks();
    this.renderStartPrograms();
    this.bindTaskbar();
    this.bindStartMenu();
    this.bindTray();
    this.bindDesktop();
    this.bindShortcuts();
    this.bindLockScreen();
    this.onWindowsChanged();
    this.applySettings();
    this.refreshDesktop();
    this.updateClock();
    if (this._clockTimer) clearInterval(this._clockTimer);
    this._clockTimer = setInterval(() => this.updateClock(), 1000);
    document.title = 'MosaicOS 0.50 — Aero Desktop';
  },

  /* ---------------- open applications ---------------- */
  openApp(appId, args = {}) {
    if (!appId) return null;
    const alias = {
      iexplore: 'browser', ie: 'browser', wmplayer: 'media', cmd: 'terminal',
      command: 'terminal', mspaint: 'paint', taskman: 'taskmgr', controlpanel: 'control',
      mycomputer: 'explorer', computer: 'explorer', files: 'explorer'
    };
    const id = Apps[appId] ? appId : alias[appId];
    if (!id || !Apps[id]) {
      Dialog.error('MosaicOS', 'No se encuentra la aplicación «' + appId + '».');
      return null;
    }
    this.closeStart();
    return WM.create(id, args);
  },

  openExternal(url) {
    const u = String(url || '').trim();
    if (!u) return;
    if (!/^https?:\/\//i.test(u)) { toast('MosaicOS', 'Dirección no compatible: ' + u); return; }
    if (FS.native) {
      fetch('api/open-url', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: u })
      }).then(r => { if (!r.ok) toast('MosaicOS', 'No se pudo abrir el navegador del sistema.'); })
        .catch(() => toast('MosaicOS', 'No se pudo abrir el navegador del sistema.'));
      return;
    }
    const w = window.open(u, '_blank', 'noopener,noreferrer');
    if (!w) toast('Mosaic Web', 'El navegador bloqueó la ventana emergente. Permita las ventanas emergentes para abrir enlaces externos.');
  },

  runDialog() {
    return Dialog.show({
      title: 'Ejecutar', icon: 'run', width: 430,
      body: `<p style="margin-bottom:9px;color:#557082">Escriba el nombre de un programa, una carpeta, un documento o una dirección de Internet.</p>
        <label class="field-label">Abrir:</label><input class="text-input" id="run-input" placeholder="notepad" value="">`,
      buttons: [{ label: 'Aceptar', value: '__ok', primary: true }, { label: 'Cancelar', value: false }],
      onOpen: (wrap, close) => {
        const input = $('#run-input', wrap);
        input.focus();
        input.addEventListener('keydown', e => { if (e.key === 'Enter') close('__ok'); });
      }
    }).then(v => {
      if (v !== '__ok') return;
      const value = ($('#run-input') ? $('#run-input').value : '').trim();
      if (!value) return;
      const aliases = {
        notepad: ['notepad', {}], calc: ['calc', {}], mspaint: ['paint', {}], paint: ['paint', {}],
        explorer: ['explorer', {}], control: ['control', {}], cmd: ['terminal', {}],
        terminal: ['terminal', {}], media: ['media', {}], wmplayer: ['media', {}],
        iexplore: ['browser', {}], browser: ['browser', {}], taskmgr: ['taskmgr', {}],
        welcome: ['welcome', {}], games: ['games', {}], sticky: ['sticky', {}]
      };
      const low = value.toLowerCase();
      if (aliases[low]) { this.openApp(aliases[low][0], aliases[low][1]); return; }
      if (/^https?:\/\//i.test(value)) { this.openApp('browser', { url: value }); return; }
      if (/^[\/~.]|^[a-zA-Z]:[\\/]/.test(value)) {
        const path = value === '~' ? FS.home : value.replace(/^[a-zA-Z]:[\\/]?/, '/').replace(/\\/g, '/');
        this.openApp('explorer', { path: path || '/' });
        return;
      }
      Dialog.error('Ejecutar', 'MosaicOS no puede encontrar «' + value + '». Compruebe que el nombre esté escrito correctamente e inténtelo de nuevo.');
    });
  },

  /* ---------------- taskbar ---------------- */
  buildQuickLaunch() {
    const host = $('#quick-launch');
    if (!host) return;
    const defaults = [
      ['explorer', 'explorer', 'Equipo'],
      ['browser', 'browser', 'Mosaic Web'],
      ['media', 'media', 'Reproductor multimedia'],
      ['notepad', 'notepad', 'Notepad']
    ];
    host.innerHTML = defaults.map(([app, icon, title]) =>
      `<button class="ql-btn" data-app="${app}" title="${esc(title)}" aria-label="${esc(title)}">${iconSvg(icon)}</button>`).join('');
    $$('.ql-btn', host).forEach(b => b.onclick = () => this.openApp(b.dataset.app, {}));
  },

  onWindowsChanged() {
    const host = $('#task-buttons');
    if (!host) return;
    const wins = Array.from(WM.wins.values());
    host.innerHTML = wins.map(w => {
      const minimized = w.element.classList.contains('minimized');
      const active = WM.activeId === w.id && !minimized;
      return `<button class="task-btn ${active ? 'active' : ''}" data-id="${w.id}" title="${esc(w.title)}">
        ${iconSvg(w.icon)}<span>${esc(w.title)}</span></button>`;
    }).join('');
    $$('.task-btn', host).forEach(btn => {
      const id = btn.dataset.id;
      btn.onclick = () => {
        const w = WM.wins.get(id);
        if (!w) return;
        if (w.element.classList.contains('minimized')) WM.restore(id);
        else if (WM.activeId === id) WM.minimize(id);
        else WM.focus(id);
      };
      btn.onauxclick = e => { if (e.button === 1) { e.preventDefault(); WM.close(id); } };
      btn.oncontextmenu = e => {
        e.preventDefault();
        const r = btn.getBoundingClientRect();
        const w = WM.wins.get(id);
        if (!w) return;
        const maxed = w.element.classList.contains('maximized');
        const min = w.element.classList.contains('minimized');
        this.menuAt(r.left, r.top - 8, [
          { label: 'Restaurar', icon: 'up', action: () => WM.restore(id) },
          { label: min ? 'Activar' : 'Minimizar', icon: 'image', action: () => min ? WM.restore(id) : WM.minimize(id) },
          { label: maxed ? 'Restaurar tamaño' : 'Maximizar', icon: 'image', action: () => WM.toggleMaximize(id) },
          { sep: true },
          { label: 'Cerrar ventana', icon: 'shutdown', key: 'Alt+F4', action: () => WM.close(id) }
        ]);
      };
    });
  },

  bindTaskbar() {
    $('#start-button').onclick = e => { e.stopPropagation(); this.toggleStart(); };
    $('#start-button').oncontextmenu = e => {
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Propiedades del menú Inicio', icon: 'control', action: () => this.openApp('control', {}) },
        { label: 'Abrir el Explorador', icon: 'explorer', action: () => this.openApp('explorer', {}) },
        { sep: true },
        { label: 'Administrador de tareas', icon: 'taskmgr', action: () => this.openApp('taskmgr', {}) },
        { label: 'Bloquear', icon: 'lock', action: () => this.lock() }
      ]);
    };
    $('#tray-show-desktop').onclick = () => this.showDesktop();
    $('#show-desktop').onclick = () => this.showDesktop();
    $('#tray-show-desktop').oncontextmenu = e => {
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Mostrar escritorio', action: () => this.showDesktop() },
        { label: 'Administrador de tareas', icon: 'taskmgr', action: () => this.openApp('taskmgr', {}) }
      ]);
    };
    $('#taskbar').oncontextmenu = e => {
      if (e.target.closest('button')) return;
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Administrador de tareas', icon: 'taskmgr', action: () => this.openApp('taskmgr', {}) },
        { sep: true },
        { label: 'Mostrar el escritorio', action: () => this.showDesktop() },
        { label: 'Propiedades', icon: 'control', action: () => this.openApp('control', {}) },
        { sep: true },
        { label: 'Bloquear', icon: 'lock', action: () => this.lock() }
      ]);
    };
    // close menus when clicking elsewhere
    document.addEventListener('pointerdown', e => {
      if (!e.target.closest('#start-menu') && !e.target.closest('#start-button')) this.closeStart();
      if (!e.target.closest('#tray-flyout') && !e.target.closest('#tray-clock') &&
          !e.target.closest('#tray-volume') && !e.target.closest('#tray-network')) this.closeFlyout();
    }, true);
  },

  /* ---------------- Start menu ---------------- */
  toggleStart(force) {
    const menu = $('#start-menu');
    const show = force == null ? menu.classList.contains('hidden') : force;
    if (show) {
      menu.classList.remove('hidden');
      $('#start-button').classList.add('open');
      this.closeFlyout(); this.hideContextMenu();
      const input = $('#sm-search');
      if (input) { input.value = ''; this.renderStartPrograms(); }
      this._allProgramsOpen = false;
      $('#all-programs').classList.add('hidden');
    } else {
      menu.classList.add('hidden');
      $('#start-button').classList.remove('open');
      this._allProgramsOpen = false;
      $('#all-programs').classList.add('hidden');
    }
  },

  closeStart() {
    if (!$('#start-menu')) return;
    if (!$('#start-menu').classList.contains('hidden')) this.toggleStart(false);
  },

  buildStartLinks() {
    const host = $('#sm-links');
    if (!host) return;
    const links = [
      ['Documentos', 'text', () => this.openApp('explorer', { path: joinPath(FS.home, 'Documents') })],
      ['Imágenes', 'image', () => this.openApp('explorer', { path: joinPath(FS.home, 'Pictures') })],
      ['Música', 'music', () => this.openApp('explorer', { path: joinPath(FS.home, 'Music') })],
      ['Equipo', 'computer', () => this.openApp('explorer', { path: '/' })],
      ['Panel de control', 'control', () => this.openApp('control', {})],
      ['Dispositivos e impresoras', 'drive', () => Dialog.alert('Dispositivos e impresoras', 'No hay dispositivos adicionales conectados.\nEl teclado, el ratón y la pantalla del equipo están listos para usarse.')],
      ['Programas predeterminados', 'program', () => Dialog.alert('Programas predeterminados', 'MosaicOS abre los archivos de texto con Notepad, las imágenes con el Visor de imágenes y el audio con el Reproductor multimedia.')],
      ['Ayuda y soporte técnico', 'help', () => this.openApp('browser', { page: 'help' })]
    ];
    host.innerHTML = links.map(([label, icon], i) =>
      `<div class="sm-link ${i === 4 ? 'sep' : ''}" data-link="${i}">${iconSvg(icon)}<span>${esc(label)}</span></div>`).join('');
    $$('.sm-link', host).forEach(el => {
      el.onclick = () => { links[+el.dataset.link][2](); this.closeStart(); };
    });
  },

  renderStartPrograms(filter = '') {
    const host = $('#sm-programs');
    if (!host) return;
    const q = String(filter || '').toLowerCase().trim();
    const pinned = ['welcome', 'browser', 'notepad', 'paint', 'calc', 'explorer', 'control', 'media', 'games', 'sticky', 'taskmgr', 'terminal'];
    let ids;
    if (q) {
      ids = Object.keys(Apps).filter(id => {
        const a = Apps[id];
        if (!a.title) return false;
        return a.title.toLowerCase().includes(q) || id.includes(q);
      });
    } else {
      ids = pinned.filter(id => Apps[id] && Apps[id].title);
    }
    if (!ids.length) {
      host.innerHTML = `<div style="padding:18px 10px;color:#7d8d9a;font-size:11.5px;text-align:center">No se encontraron programas.</div>`;
    } else {
      host.innerHTML = ids.map(id => {
        const a = Apps[id];
        return `<div class="sm-item ${['games', 'control'].includes(id) ? 'bold' : ''}" data-app="${id}" title="${esc(a.title)}">${iconSvg(a.icon)}<span>${esc(a.title)}</span></div>`;
      }).join('');
    }
    $$('.sm-item', host).forEach(el => el.onclick = () => this.openApp(el.dataset.app, {}));
  },

  bindStartMenu() {
    $('#sm-search').oninput = e => this.renderStartPrograms(e.target.value);
    $('#sm-search').onkeydown = e => {
      if (e.key === 'Enter') {
        const first = $('#sm-programs .sm-item');
        if (first) first.click();
      }
    };
    $('#sm-all').onclick = e => {
      e.stopPropagation();
      this._allProgramsOpen = !this._allProgramsOpen;
      const panel = $('#all-programs');
      panel.classList.toggle('hidden', !this._allProgramsOpen);
      if (this._allProgramsOpen) this.renderAllPrograms();
    };
    const user = $('#sm-user');
    if (user) user.onclick = () => { this.openApp('control', { page: 'users' }); this.closeStart(); };
    $('#sm-shutdown').onclick = () => this.shutdown();
    $('#sm-power-arrow').onclick = e => {
      e.stopPropagation();
      const r = e.currentTarget.getBoundingClientRect();
      this.menuAt(r.left - 150, r.top - 8, [
        { label: 'Reiniciar', icon: 'forward', action: () => this.restart() },
        { label: 'Suspender', icon: 'lock', action: () => { this.closeStart(); this.lock(); } },
        { sep: true },
        { label: 'Cerrar sesión', icon: 'user', action: () => this.logoff() },
        { label: 'Bloquear', icon: 'lock', action: () => this.lock() },
        { sep: true },
        { label: 'Apagar', icon: 'shutdown', action: () => this.shutdown() }
      ]);
    };
    $('#start-menu').addEventListener('pointerdown', e => e.stopPropagation());
  },

  renderAllPrograms() {
    const panel = $('#all-programs');
    panel.innerHTML = `<div class="side-title" style="margin:2px 6px 5px">Todos los programas</div>` +
      Object.keys(Apps).filter(id => Apps[id].title).sort((a, b) => Apps[a].title.localeCompare(Apps[b].title, 'es')).map(id => {
        const a = Apps[id];
        return `<div class="sm-item" data-app="${id}">${iconSvg(a.icon)}<span>${esc(a.title)}</span></div>`;
      }).join('');
    $$('.sm-item', panel).forEach(el => el.onclick = () => this.openApp(el.dataset.app, {}));
  },

  /* ---------------- desktop ---------------- */
  desktopPath() { return joinPath(FS.home, 'Desktop'); },

  refreshDesktop() {
    const host = this._deskIcons || $('#desktop-icons');
    if (!host) return;
    const items = [
      { label: 'Equipo', icon: 'computer', app: 'explorer', args: { path: '/' } },
      { label: 'Documentos', icon: 'folder', app: 'explorer', args: { path: joinPath(FS.home, 'Documents') } },
      { label: 'Papelera de reciclaje', icon: 'recycle', app: 'explorer', args: { path: '/Recycle Bin' } },
      { sep: true },
      { label: 'Mosaic Web', icon: 'browser', app: 'browser' },
      { label: 'Notepad', icon: 'notepad', app: 'notepad' },
      { label: 'Paint', icon: 'paint', app: 'paint' },
      { label: 'Calculadora', icon: 'calc', app: 'calc' },
      { label: 'Terminal', icon: 'terminal', app: 'terminal' },
      { label: 'Panel de control', icon: 'control', app: 'control' },
      { label: 'Juegos', icon: 'games', app: 'games' },
      { label: 'Bienvenido', icon: 'help', app: 'welcome' }
    ];
    const small = Settings.data.iconSize === 'small';
    host.innerHTML = items.map(it => {
      if (it.sep) return `<div class="desk-sep" aria-hidden="true"></div>`;
      return `<div class="desk-icon ${small ? 'small' : ''}" data-open="${it.app}" data-args="${esc(JSON.stringify(it.args || {}))}" title="${esc(it.label)}">
        ${iconSvg(it.icon)}<span>${esc(it.label)}</span></div>`;
    }).join('');
    this._bindDesktopIcons(host);
    // user files in the Desktop folder
    if (FS.home) {
      FS.list(this.desktopPath()).then(list => {
        if (!list || !list.length) return;
        list.forEach(it => {
          const el = document.createElement('div');
          el.className = 'desk-icon user-file' + (small ? ' small' : '');
          el.dataset.path = joinPath(this.desktopPath(), it.name);
          el.dataset.type = it.type;
          el.title = it.name;
          el.innerHTML = iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name)) + `<span>${esc(it.name)}</span>`;
          this._bindDesktopIcon(el, host);
          host.appendChild(el);
        });
        if (this._iconSize !== Settings.data.iconSize) this._iconSize = Settings.data.iconSize;
      }).catch(() => {});
    }
    this._iconSize = Settings.data.iconSize;
  },

  _bindDesktopIcons(host) {
    $$('.desk-icon', host).forEach(el => this._bindDesktopIcon(el, host));
  },

  _bindDesktopIcon(el, host) {
    el.onclick = e => {
      e.stopPropagation();
      if (e.ctrlKey) { el.classList.toggle('selected'); return; }
      $$('.desk-icon.selected', host).forEach(x => x.classList.remove('selected'));
      el.classList.add('selected');
      AudioKit.click();
    };
    el.ondblclick = e => { e.stopPropagation(); this.openDesktopIcon(el); };
    el.oncontextmenu = e => {
      e.preventDefault(); e.stopPropagation();
      $$('.desk-icon.selected', host).forEach(x => x.classList.remove('selected'));
      el.classList.add('selected');
      const isFile = !!el.dataset.path;
      const items = [{ label: 'Abrir', icon: isFile && el.dataset.type === 'dir' ? 'folder-open' : 'program', action: () => this.openDesktopIcon(el) }];
      if (isFile) {
        items.push({ label: 'Mostrar en el Explorador', icon: 'folder', action: () => this.openApp('explorer', { path: dirName(el.dataset.path) }) });
        items.push({ sep: true });
        items.push({ label: 'Cambiar nombre', icon: 'text', key: 'F2', action: () => this.renameDesktopItem(el.dataset.path, el) });
        items.push({ label: 'Eliminar', icon: 'recycle', key: 'Supr', action: () => this.deleteDesktopItem(el.dataset.path) });
        items.push({ sep: true });
        items.push({ label: 'Propiedades', icon: 'info', action: () => this.desktopProperties(el.dataset.path) });
      } else {
        const app = Apps[el.dataset.open];
        items.push({ sep: true });
        items.push({ label: 'Propiedades', icon: 'info', action: () => Dialog.alert('Propiedades', (app ? app.title : el.textContent.trim()) + '\nAplicación de MosaicOS 0.50.') });
      }
      this.menuAt(e.clientX, e.clientY, items);
    };
  },

  openDesktopIcon(el) {
    if (el.dataset.path) {
      const p = el.dataset.path;
      if (el.dataset.type === 'dir') { this.openApp('explorer', { path: p }); return; }
      const e = extOf(p);
      if (['txt', 'log', 'md', 'ini', 'conf', 'csv', 'json', 'xml', 'html', 'css', 'js', 'sh', 'bat'].includes(e)) return this.openApp('notepad', { path: p });
      if (['png', 'jpg', 'jpeg', 'gif', 'bmp', 'svg', 'webp'].includes(e)) return this.openApp('imgview', { path: p });
      if (['mp3', 'wav', 'ogg', 'flac', 'm4a'].includes(e)) return this.openApp('media', { path: p });
      this.openApp('explorer', { path: dirName(p) });
      return;
    }
    let args = {};
    try { args = JSON.parse(el.dataset.args || '{}'); } catch (err) {}
    this.openApp(el.dataset.open, args);
  },

  async renameDesktopItem(path, el) {
    const current = baseName(path);
    const name = await Dialog.prompt('Cambiar nombre', 'Nuevo nombre:', current);
    if (!name || name === current) return;
    try {
      await FS.rename(path, name);
      this.refreshDesktop();
      toast('Escritorio', 'Elemento renombrado.');
    } catch (e) { Dialog.error('No se pudo cambiar el nombre', e.message); }
  },

  async deleteDesktopItem(path) {
    const ok = await Dialog.confirm('Eliminar', `¿Mover «${baseName(path)}» a la Papelera de reciclaje?`, true);
    if (!ok) return;
    try { await FS.remove(path); this.refreshDesktop(); toast('Papelera', 'Elemento movido a la Papelera de reciclaje.'); }
    catch (e) { Dialog.error('No se pudo eliminar', e.message); }
  },

  async desktopProperties(path) {
    try {
      const st = await FS.list(path);
      const it = st[0] || { name: baseName(path), type: 'file', size: 0, modified: Date.now() };
      Dialog.show({
        title: baseName(path) + ' — Propiedades', icon: it.type === 'dir' ? 'folder' : iconForName(it.name), width: 430,
        body: `<div class="props"><div class="props-head">${iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name), 'ico props-ico')}<b>${esc(it.name)}</b></div>
          <table class="props-table"><tr><td>Tipo:</td><td>${it.type === 'dir' ? 'Carpeta de archivos' : 'Archivo'}</td></tr>
          <tr><td>Ubicación:</td><td>${esc(dirName(path))}</td></tr>
          <tr><td>Tamaño:</td><td>${fmtSize(it.size || 0)}</td></tr>
          <tr><td>Modificado:</td><td>${it.modified ? fmtDate(it.modified) : '—'}</td></tr></table></div>`,
        buttons: [{ label: 'Aceptar', value: true, primary: true }]
      });
    } catch (e) { Dialog.error('Propiedades', e.message); }
  },

  async newDesktopFolder() {
    const n = await Dialog.prompt('Nueva carpeta', 'Nombre de la carpeta:', 'Nueva carpeta');
    if (!n) return;
    try { await FS.mkdir(joinPath(this.desktopPath(), n)); this.refreshDesktop(); }
    catch (e) { Dialog.error('No se pudo crear la carpeta', e.message); }
  },

  async newDesktopText() {
    let n = await Dialog.prompt('Nuevo documento de texto', 'Nombre del archivo:', 'Nuevo documento de texto.txt');
    if (!n) return;
    if (!extOf(n)) n += '.txt';
    try { await FS.write(joinPath(this.desktopPath(), n), ''); this.refreshDesktop(); }
    catch (e) { Dialog.error('No se pudo crear el archivo', e.message); }
  },

  bindDesktop() {
    const host = $('#desktop-icons');
    if (host) {
      host.onclick = e => { if (e.target === host) $$('.desk-icon.selected', host).forEach(x => x.classList.remove('selected')); };
      host.oncontextmenu = e => {
        if (e.target.closest('.desk-icon')) return;
        e.preventDefault();
        this.desktopMenu(e.clientX, e.clientY);
      };
    }
    const wall = $('#wallpaper-layer');
    if (wall) {
      wall.oncontextmenu = e => {
        e.preventDefault();
        this.desktopMenu(e.clientX, e.clientY);
      };
      wall.onclick = () => $$('.desk-icon.selected').forEach(x => x.classList.remove('selected'));
    }
    const gadgets = $('#gadgets');
    if (gadgets) gadgets.oncontextmenu = e => e.preventDefault();
  },

  desktopMenu(x, y) {
    this.menuAt(x, y, [
      { label: 'Ver', icon: 'image', action: () => toast('Escritorio', 'Use el Panel de control para cambiar el tamaño de los iconos.') },
      { label: Settings.data.iconSize === 'small' ? 'Iconos medianos' : 'Iconos pequeños', icon: 'image', action: () => { Settings.save({ iconSize: Settings.data.iconSize === 'small' ? 'medium' : 'small' }); this.applySettings(); this.refreshDesktop(); } },
      { label: Settings.data.showGadgets ? 'Ocultar gadgets' : 'Mostrar gadgets', icon: 'clock', action: () => { Settings.save({ showGadgets: !Settings.data.showGadgets }); this.applySettings(); } },
      { label: 'Actualizar', icon: 'forward', action: () => this.refreshDesktop() },
      { sep: true },
      { label: 'Nueva carpeta', icon: 'folder', action: () => this.newDesktopFolder() },
      { label: 'Nuevo documento de texto', icon: 'text', action: () => this.newDesktopText() },
      { sep: true },
      { label: 'Abrir Terminal aquí', icon: 'terminal', action: () => this.openApp('terminal', { path: this.desktopPath() }) },
      { label: 'Personalizar', icon: 'wallpaper', action: () => this.openApp('control', { page: 'personalization' }) },
      { label: 'Propiedades de pantalla', icon: 'computer', action: () => this.openApp('control', { page: 'system' }) }
    ]);
  },

  /* ---------------- context menu ---------------- */
  menuAt(x, y, items) {
    const menu = $('#context-menu');
    if (!menu) return;
    this.hideContextMenu();
    menu.innerHTML = items.map((it, i) => {
      if (it.sep) return `<div class="cm-sep"></div>`;
      return `<div class="cm-item ${it.disabled ? 'disabled' : ''}" data-i="${i}">
        ${it.icon ? iconSvg(it.icon) : '<span style="width:17px"></span>'}
        <span>${esc(it.label)}</span>${it.key ? `<span class="cm-key">${esc(it.key)}</span>` : ''}</div>`;
    }).join('');
    menu.classList.remove('hidden');
    menu.style.visibility = 'hidden';
    menu.style.left = '0px'; menu.style.top = '0px';
    const r = menu.getBoundingClientRect();
    const left = clamp(x, 4, Math.max(4, innerWidth - r.width - 4));
    const top = clamp(y, 4, Math.max(4, innerHeight - r.height - 4));
    menu.style.left = left + 'px';
    menu.style.top = top + 'px';
    menu.style.visibility = '';
    $$('.cm-item', menu).forEach(el => {
      const it = items[+el.dataset.i];
      if (!it || it.disabled) return;
      el.onclick = ev => {
        ev.stopPropagation();
        this.hideContextMenu();
        AudioKit.click();
        try {
          const res = it.action && it.action();
          if (res && typeof res.catch === 'function') res.catch(err => Dialog.error('MosaicOS', err.message || String(err)));
        } catch (err) { Dialog.error('MosaicOS', err.message || String(err)); }
      };
    });
    const cleanup = ev => {
      if (!menu.contains(ev.target)) this.hideContextMenu();
    };
    setTimeout(() => document.addEventListener('pointerdown', cleanup, true), 0);
    this._menuCleanup = cleanup;
  },

  hideContextMenu() {
    const menu = $('#context-menu');
    if (menu) { menu.classList.add('hidden'); menu.innerHTML = ''; }
    if (this._menuCleanup) {
      document.removeEventListener('pointerdown', this._menuCleanup, true);
      this._menuCleanup = null;
    }
  },

  /* ---------------- tray ---------------- */
  bindTray() {
    $('#tray-clock').onclick = () => this.toggleFlyout('clock');
    $('#tray-volume').onclick = () => this.toggleFlyout('volume');
    $('#tray-network').onclick = () => this.toggleFlyout('network');
    $('#tray-clock').oncontextmenu = e => {
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Ajustar fecha y hora', icon: 'clock', action: () => this.openApp('control', { page: 'datetime' }) },
        { label: 'Propiedades del reloj', icon: 'control', action: () => this.openApp('control', { page: 'datetime' }) }
      ]);
    };
    $('#tray-volume').oncontextmenu = e => {
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Abrir mezclador de volumen', icon: 'volume', action: () => this.openApp('control', { page: 'sound' }) },
        { label: Settings.data.muted ? 'Activar sonido' : 'Silenciar', icon: 'volume', action: () => this.toggleMute() }
      ]);
    };
    $('#tray-network').oncontextmenu = e => {
      e.preventDefault();
      this.menuAt(e.clientX, e.clientY, [
        { label: 'Abrir Centro de redes', icon: 'network', action: () => this.openApp('control', { page: 'network' }) },
        { label: 'Diagnosticar problemas', icon: 'help', action: () => toast('Red', navigator.onLine ? 'La conexión parece funcionar correctamente.' : 'No se detecta una conexión a Internet.') }
      ]);
    };
  },

  toggleFlyout(kind) {
    const host = $('#tray-flyout');
    if (!host) return;
    if (this.flyKind === kind && !host.classList.contains('hidden')) { this.closeFlyout(); return; }
    this.flyKind = kind;
    host.dataset.kind = kind;
    host.classList.remove('hidden');
    if (kind === 'clock') this.renderClockFlyout();
    else if (kind === 'volume') this.renderVolumeFlyout();
    else this.renderNetworkFlyout();
  },

  closeFlyout() {
    const host = $('#tray-flyout');
    if (host) host.classList.add('hidden');
    this.flyKind = null;
  },

  renderClockFlyout() {
    const host = $('#tray-flyout');
    const now = new Date();
    const cal = this.calendarHTML(this.flyMonth);
    host.innerHTML = `
      <div class="fly-head">Fecha y hora</div>
      <div class="fly-body">
        <div class="fly-clock">${fmtTime(now, true)}</div>
        <div class="fly-date">${now.toLocaleDateString('es-ES', { weekday: 'long', day: 'numeric', month: 'long', year: 'numeric' })}</div>
        ${cal}
        <div class="fly-nearest"><span>Zona horaria: ${esc(Intl.DateTimeFormat().resolvedOptions().timeZone || 'Local')}</span></div>
        <div class="fly-actions"><button class="btn" id="fly-datetime">Cambiar fecha y hora…</button></div>
      </div>`;
    $$('[data-cal]', host).forEach(b => b.onclick = () => {
      const d = new Date(this.flyMonth);
      d.setMonth(d.getMonth() + (+b.dataset.cal));
      this.flyMonth = d;
      this.renderClockFlyout();
    });
    const dt = $('#fly-datetime', host);
    if (dt) dt.onclick = () => { this.closeFlyout(); this.openApp('control', { page: 'datetime' }); };
  },

  calendarHTML(date) {
    const y = date.getFullYear(), m = date.getMonth();
    const first = new Date(y, m, 1);
    const start = (first.getDay() + 6) % 7; // Monday first
    const days = new Date(y, m + 1, 0).getDate();
    const prevDays = new Date(y, m, 0).getDate();
    const today = new Date();
    const isThisMonth = today.getFullYear() === y && today.getMonth() === m;
    let cells = [];
    for (let i = 0; i < start; i++) cells.push({ d: prevDays - start + 1 + i, muted: true });
    for (let d = 1; d <= days; d++) cells.push({ d, muted: false, today: isThisMonth && d === today.getDate() });
    while (cells.length % 7) cells.push({ d: cells.length - start - days + 1, muted: true });
    const rows = [];
    for (let i = 0; i < cells.length; i += 7) rows.push(cells.slice(i, i + 7));
    return `<div class="cal-head"><button data-cal="-1" aria-label="Mes anterior">‹</button>
      <span>${date.toLocaleDateString('es-ES', { month: 'long', year: 'numeric' })}</span>
      <button data-cal="1" aria-label="Mes siguiente">›</button></div>
      <table class="calendar"><thead><tr>${['L', 'M', 'X', 'J', 'V', 'S', 'D'].map(d => `<th>${d}</th>`).join('')}</tr></thead>
      <tbody>${rows.map(r => `<tr>${r.map(c => `<td class="${c.muted ? 'muted' : ''} ${c.today ? 'today' : ''}">${c.d}</td>`).join('')}</tr>`).join('')}</tbody></table>`;
  },

  renderVolumeFlyout() {
    const host = $('#tray-flyout');
    const vol = Settings.data.volume == null ? 80 : Settings.data.volume;
    host.innerHTML = `
      <div class="fly-head">Volumen</div>
      <div class="fly-body">
        <div class="fly-row"><span class="fly-round">${iconSvg('volume')}</span>
          <b id="vol-label" style="font-size:15px;font-weight:300">${Settings.data.muted ? 'Silenciado' : vol + '%'}</b></div>
        <input type="range" class="vol-slider" id="vol-range" min="0" max="100" value="${vol}">
        <div class="fly-row"><label style="display:flex;align-items:center;gap:7px"><input type="checkbox" id="vol-mute" ${Settings.data.muted ? 'checked' : ''}> Silenciar</label>
          <button class="btn" id="vol-mixer">Mezclador…</button></div>
        <div class="fly-nearest">Los sonidos del sistema y el reproductor usan este volumen.</div>
      </div>`;
    const range = $('#vol-range', host);
    range.oninput = () => {
      Settings.save({ volume: +range.value, muted: false });
      Settings.data.muted = false;
      if (typeof MediaPlayer !== 'undefined') MediaPlayer.setVolume(+range.value / 100);
      $('#vol-label').textContent = range.value + '%';
      $('#vol-mute').checked = false;
    };
    $('#vol-mute', host).onchange = e => this.toggleMute(e.target.checked);
    $('#vol-mixer', host).onclick = () => { this.closeFlyout(); this.openApp('control', { page: 'sound' }); };
  },

  toggleMute(force) {
    const muted = force == null ? !Settings.data.muted : !!force;
    Settings.save({ muted });
    if (typeof MediaPlayer !== 'undefined') {
      const v = muted ? 0 : (Settings.data.volume == null ? 80 : Settings.data.volume) / 100;
      if (MediaPlayer.master) MediaPlayer.master.gain.value = v * 0.5;
      if (MediaPlayer.local) MediaPlayer.local.volume = v;
    }
    if (this.flyKind === 'volume') this.renderVolumeFlyout();
    toast('Sonido', muted ? 'Silenciado.' : 'Sonido activado.', 'volume', 2200);
  },

  renderNetworkFlyout() {
    const host = $('#tray-flyout');
    const conn = navigator.connection || {};
    host.innerHTML = `
      <div class="fly-head">Red</div>
      <div class="fly-body">
        <div class="fly-row"><span class="fly-round">${iconSvg('network')}</span>
          <div><b>${navigator.onLine ? 'Conectado' : 'Sin conexión'}</b><div style="font-size:10.5px;color:#6f8698">${navigator.onLine ? 'Acceso a Internet disponible' : 'Trabajando sin conexión'}</div></div></div>
        <div class="fly-row"><span>Tipo</span><b>${esc(conn.effectiveType || 'desconocido')}</b></div>
        <div class="fly-row"><span>Velocidad</span><b>${conn.downlink ? conn.downlink + ' Mbps' : 'no disponible'}</b></div>
        <div class="fly-row"><span>Puente de MosaicOS</span><b>${FS.native ? 'Activo' : 'Demostración'}</b></div>
        <div class="fly-actions"><button class="btn" id="fly-network-cp">Abrir Centro de redes…</button><button class="btn" id="fly-network-test">Probar</button></div>
      </div>`;
    $('#fly-network-cp', host).onclick = () => { this.closeFlyout(); this.openApp('control', { page: 'network' }); };
    $('#fly-network-test', host).onclick = () => {
      toast('Red', navigator.onLine ? 'La conexión funciona correctamente.' : 'No se detecta ninguna conexión.', navigator.onLine ? 'check' : 'warning');
    };
  },

  /* ---------------- clock ---------------- */
  updateClock() {
    const now = new Date();
    const t = fmtTime(now);
    const d = now.toLocaleDateString('es-ES', { day: '2-digit', month: '2-digit', year: 'numeric' });
    const timeEl = $('#clock-time'), dateEl = $('#clock-date');
    if (timeEl) timeEl.textContent = t;
    if (dateEl) dateEl.textContent = d;
    const gt = $('#gadget-time'), gd = $('#gadget-date');
    if (gt) gt.textContent = now.toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit' });
    if (gd) gd.textContent = now.toLocaleDateString('es-ES', { weekday: 'short', day: 'numeric', month: 'short' });
    const clock = $('#gadget-clock');
    if (clock) {
      const h = clock.querySelector('.hour'), m = clock.querySelector('.minute'), s = clock.querySelector('.second');
      const sec = now.getSeconds(), min = now.getMinutes(), hr = now.getHours() % 12;
      if (h) h.style.transform = `rotate(${(hr + min / 60) * 30}deg)`;
      if (m) m.style.transform = `rotate(${(min + sec / 60) * 6}deg)`;
      if (s) s.style.transform = `rotate(${sec * 6}deg)`;
    }
    const lc = $('#lock-clock');
    if (lc) lc.textContent = now.toLocaleString('es-ES', { weekday: 'long', day: 'numeric', month: 'long', hour: '2-digit', minute: '2-digit' });
    if (this.flyKind === 'clock' && !$('#tray-flyout').classList.contains('hidden')) {
      const fc = $('#tray-flyout .fly-clock');
      if (fc) fc.textContent = fmtTime(now, true);
    }
  },

  /* ---------------- settings / appearance ---------------- */
  applySettings() {
    const root = document.documentElement;
    const accent = Settings.data.accent || '#2f7fbc';
    root.style.setProperty('--accent', accent);
    root.style.setProperty('--accent-dark', this.shade(accent, -35));
    root.style.setProperty('--accent-light', this.shade(accent, 55));
    const wall = WALLPAPERS[Settings.data.wallpaper] || WALLPAPERS.harmony;
    const layer = $('#wallpaper-layer');
    if (layer) layer.style.background = wall.css;
    document.body.classList.toggle('no-glass', !Settings.data.taskbarGlass);
    const gadgets = $('#gadgets');
    if (gadgets) gadgets.style.display = Settings.data.showGadgets === false ? 'none' : '';
    const name = Settings.data.userName || 'Mosaic';
    const initial = name.trim().charAt(0).toUpperCase() || 'M';
    ['#login-user', '#lock-name', '#sm-username'].forEach(sel => {
      const el = $(sel); if (el) el.textContent = name;
    });
    $$('.sm-avatar, .login-card .user-pic, .lock-user .user-pic').forEach(el => {
      el.textContent = initial;
      if (Settings.data.avatarColor) el.style.background = `linear-gradient(160deg, #ffffff, ${Settings.data.avatarColor})`;
    });
    if (this._iconSize !== Settings.data.iconSize) {
      this._iconSize = Settings.data.iconSize;
      $$('.desk-icon').forEach(el => el.classList.toggle('small', Settings.data.iconSize === 'small'));
    }
    const vol = $('#tray-volume');
    if (vol) vol.title = Settings.data.muted ? 'Volumen: silenciado' : 'Volumen: ' + (Settings.data.volume == null ? 80 : Settings.data.volume) + '%';
  },

  shade(hex, amount) {
    const m = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex || '');
    if (!m) return hex;
    const c = [parseInt(m[1], 16), parseInt(m[2], 16), parseInt(m[3], 16)].map(v => clamp(v + amount, 0, 255));
    return '#' + c.map(v => v.toString(16).padStart(2, '0')).join('');
  },

  /* ---------------- show desktop / lock / power ---------------- */
  showDesktop() {
    const visible = Array.from(WM.wins.values()).filter(w => !w.element.classList.contains('minimized'));
    if (visible.length) {
      this._showDesktop = true;
      this._minimizedByShow = visible.map(w => w.id);
      visible.forEach(w => WM.minimize(w.id));
    } else {
      this._showDesktop = false;
      (this._minimizedByShow || []).forEach(id => { const w = WM.wins.get(id); if (w) { w.element.classList.remove('minimized'); } });
      const top = WM.topVisible();
      if (top) WM.focus(top.id);
      this.onWindowsChanged();
    }
  },

  lock() {
    this.closeStart(); this.closeFlyout(); this.hideContextMenu();
    const lock = $('#lock-screen');
    if (!lock) return;
    $('#lock-name').textContent = Settings.data.userName || 'Mosaic';
    this.updateClock();
    lock.classList.remove('hidden');
    setTimeout(() => { const i = $('#unlock-pass'); if (i) { i.value = ''; i.focus(); } }, 120);
  },

  bindLockScreen() {
    const form = $('#unlock-form');
    if (form) form.onsubmit = e => { e.preventDefault(); this.unlock(); };
    const power = $('#lock-power');
    if (power) power.onclick = () => this.shutdown();
    const lock = $('#lock-screen');
    if (lock) lock.onclick = e => { if (e.target === lock || e.target.classList.contains('lock-blur')) $('#unlock-pass').focus(); };
  },

  unlock() {
    const lock = $('#lock-screen');
    if (lock) lock.classList.add('hidden');
    const p = $('#unlock-pass');
    if (p) p.value = '';
    AudioKit.notify();
    toast('MosaicOS', 'Sesión desbloqueada.', 'check', 2600);
  },

  async shutdown() {
    this.closeStart(); this.closeFlyout(); this.hideContextMenu();
    if (typeof MediaPlayer !== 'undefined' && MediaPlayer.playing) mediaStop();
    AudioKit.shutdown();
    if (FS.native) {
      try { fetch('api/power', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ action: 'shutdown' }) }); } catch (e) {}
    }
    const overlay = $('#power-overlay');
    if (!overlay) return;
    overlay.classList.remove('hidden');
    overlay.innerHTML = `<div class="power-box"><span class="spinner big"></span><p id="power-text">Apagando…</p></div>`;
    setTimeout(() => {
      overlay.innerHTML = `<div class="power-box power-done">
        <div class="power-glyph">⏻</div>
        <h2>Es seguro apagar el equipo</h2>
        <p>MosaicOS se ha cerrado. Puede cerrar esta pestaña o volver a iniciar el escritorio.</p>
        <button class="btn primary" id="power-again">Volver a iniciar</button></div>`;
      const again = $('#power-again');
      if (again) again.onclick = () => location.reload();
    }, 1600);
  },

  restart() {
    this.closeStart(); this.closeFlyout(); this.hideContextMenu();
    if (FS.native) {
      try { fetch('api/power', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ action: 'reboot' }) }); } catch (e) {}
    }
    const overlay = $('#power-overlay');
    if (overlay) {
      overlay.classList.remove('hidden');
      overlay.innerHTML = `<div class="power-box"><span class="spinner big"></span><p>Reiniciando…</p></div>`;
    }
    AudioKit.shutdown();
    setTimeout(() => location.reload(), 1600);
  },

  logoff() {
    this.closeStart(); this.closeFlyout(); this.hideContextMenu();
    WM.closeAll();
    setTimeout(() => location.reload(), 250);
  },

  /* ---------------- Alt+Tab ---------------- */
  altTabOrder() {
    return Array.from(WM.wins.values())
      .filter(w => !w.element.classList.contains('minimized'))
      .sort((a, b) => (+b.element.style.zIndex || 0) - (+a.element.style.zIndex || 0));
  },

  openAltTab() {
    const order = this.altTabOrder();
    if (order.length < 1) return;
    this._alt = { open: true, index: order.length > 1 ? 1 : 0, order };
    const host = $('#alt-tab');
    host.classList.remove('hidden');
    host.innerHTML = `<div class="alt-panel">${order.map((w, i) =>
      `<div class="alt-item ${i === this._alt.index ? 'sel' : ''}" data-i="${i}">${iconSvg(w.icon)}<span>${esc(w.title)}</span></div>`).join('')}</div>
      <div class="alt-hint">Mantenga Alt y pulse Tab · Enter para activar · Esc para cancelar</div>`;
    $$('.alt-item', host).forEach(el => el.onclick = () => { this._alt.index = +el.dataset.i; this.commitAltTab(); });
  },

  cycleAltTab(step) {
    if (!this._alt.open) return;
    const n = this._alt.order.length;
    this._alt.index = (this._alt.index + step + n) % n;
    $$('.alt-item', $('#alt-tab')).forEach((el, i) => el.classList.toggle('sel', i === this._alt.index));
  },

  commitAltTab() {
    const a = this._alt;
    this._alt = { open: false, index: 0, order: [] };
    const host = $('#alt-tab');
    if (host) { host.classList.add('hidden'); host.innerHTML = ''; }
    if (a.order[a.index]) WM.focus(a.order[a.index].id);
  },

  cancelAltTab() {
    this._alt = { open: false, index: 0, order: [] };
    const host = $('#alt-tab');
    if (host) { host.classList.add('hidden'); host.innerHTML = ''; }
  },

  /* ---------------- keyboard ---------------- */
  bindShortcuts() {
    document.addEventListener('keydown', e => {
      const desktop = $('#desktop');
      if (!desktop || desktop.classList.contains('hidden')) return;
      const inField = /^(input|textarea|select)$/i.test(e.target.tagName) || e.target.isContentEditable;
      const key = e.key;

      if (key === 'Alt' && !e.ctrlKey) { /* Alt key alone is handled on keyup */ }
      if (key === 'Tab' && e.altKey) {
        e.preventDefault();
        if (!this._alt.open) this.openAltTab();
        else this.cycleAltTab(e.shiftKey ? -1 : 1);
        return;
      }
      if (this._alt.open) {
        if (key === 'Escape') { e.preventDefault(); this.cancelAltTab(); }
        if (key === 'Enter') { e.preventDefault(); this.commitAltTab(); }
        return;
      }
      if (key === 'Escape') {
        if (!this._alt.open) { this.closeStart(); this.closeFlyout(); this.hideContextMenu(); }
        return;
      }
      if (e.metaKey || key === 'OS') {
        e.preventDefault();
        this.toggleStart();
        return;
      }
      if (e.ctrlKey && key === 'Escape') { e.preventDefault(); this.toggleStart(); return; }
      if (e.ctrlKey && key.toLowerCase() === 'd' && !inField) { e.preventDefault(); this.showDesktop(); return; }
      if (e.altKey && key === 'F4') {
        e.preventDefault();
        if (WM.activeId) WM.close(WM.activeId);
        else this.shutdown();
        return;
      }
      if ((e.ctrlKey || e.metaKey) && key.toLowerCase() === 'e' && !inField) { e.preventDefault(); this.openApp('explorer', { path: '/' }); return; }
      if ((e.ctrlKey || e.metaKey) && key.toLowerCase() === 'r' && !inField) { e.preventDefault(); this.runDialog(); return; }
      if ((e.ctrlKey || e.metaKey) && key.toLowerCase() === 'l' && !inField) { e.preventDefault(); this.lock(); return; }
      if (e.metaKey && key === 'ArrowLeft') { e.preventDefault(); if (WM.activeId) WM.snap ? WM.snap(WM.activeId, 'left') : null; return; }
      if (e.metaKey && key === 'ArrowRight') { e.preventDefault(); if (WM.activeId) WM.snap ? WM.snap(WM.activeId, 'right') : null; return; }
      if (e.metaKey && key === 'ArrowUp') { e.preventDefault(); if (WM.activeId) WM.maximize(WM.activeId); return; }
      if (e.metaKey && key === 'ArrowDown') { e.preventDefault(); if (WM.activeId) { WM.restoreFromMax(WM.activeId); WM.minimize(WM.activeId); } return; }
    });

    document.addEventListener('keyup', e => {
      if (e.key === 'Alt' && this._alt.open) { e.preventDefault(); this.commitAltTab(); }
    });
  }
};

/* Keep the shell reachable from the console and integration tests. */
window.MosaicOS = { version: '0.50', WM, Shell, FS, Settings, Apps, Dialog };

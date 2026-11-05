/* ==========================================================================
   MosaicOS 0.50 "Aero" — productivity applications
   Explorer · Notepad · Paint · Calculator · Terminal · File dialog
   ========================================================================== */
'use strict';

const Apps = {};

/* ------------------------------------------------------------------ */
/* File picker dialog                                                 */
/* ------------------------------------------------------------------ */
const FileDialog = {
  async show({ mode = 'open', title = 'Abrir', initial = FS.home, filename = '', filter = null } = {}) {
    let currentPath = initial || FS.home;
    let selected = null;
    let name = filename;
    let items = [];
    const modeSave = mode === 'save';

    const body = `
      <div class="fd">
        <div class="fd-top">
          <div class="fd-crumbs" id="fd-crumbs"></div>
          <div class="fd-tools"><button class="btn" id="fd-up">↑ Subir</button></div>
        </div>
        <div class="fd-pane">
          <div class="fd-places" id="fd-places"></div>
          <div class="fd-list" id="fd-list"></div>
        </div>
        <div class="fd-name"><label>Nombre:</label><input class="text-input" id="fd-name" value="${esc(filename)}"><span class="fd-filter">${filter ? filter.map(f => '*.' + f).join('; ') : 'Todos los archivos'}</span></div>
      </div>`;

    const answer = await Dialog.show({
      title, body, width: 650,
      buttons: [
        { label: modeSave ? 'Guardar' : 'Abrir', value: '__ok', primary: true },
        { label: 'Cancelar', value: false }
      ],
      onOpen: async (wrap) => {
        const listEl = $('#fd-list', wrap), crumbs = $('#fd-crumbs', wrap), nameEl = $('#fd-name', wrap);
        const places = $('#fd-places', wrap);
        const placeList = [
          ['Escritorio', joinPath(FS.home, 'Desktop'), 'folder'],
          ['Documentos', joinPath(FS.home, 'Documents'), 'text'],
          ['Descargas', joinPath(FS.home, 'Downloads'), 'folder'],
          ['Imágenes', joinPath(FS.home, 'Pictures'), 'image'],
          ['Música', joinPath(FS.home, 'Music'), 'music'],
          ['Equipo', '/', 'computer']
        ];
        places.innerHTML = `<div class="side-title">Buscar en</div>` + placeList.map(([n, p, i]) =>
          `<div class="side-item" data-path="${esc(p)}">${iconSvg(i)}<span>${esc(n)}</span></div>`).join('');
        $$('.side-item', places).forEach(el => el.onclick = () => navigate(el.dataset.path));

        async function refresh() {
          crumbs.innerHTML = pathCrumbs(currentPath);
          $$('.crumb', crumbs).forEach(c => c.onclick = () => navigate(c.dataset.path));
          listEl.innerHTML = '<div class="fd-loading">Cargando…</div>';
          try {
            items = await FS.list(currentPath);
            items = items.filter(i => i.type === 'dir' || !filter || filter.includes((i.ext || '').toLowerCase()));
          } catch (e) {
            listEl.innerHTML = `<div class="fd-empty">${esc(e.message)}</div>`;
            return;
          }
          if (!items.length) { listEl.innerHTML = '<div class="fd-empty">Esta carpeta está vacía.</div>'; return; }
          listEl.innerHTML = items.map(it => `
            <div class="fd-item" data-name="${esc(it.name)}" data-type="${it.type}">
              ${iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name), 'ico')}
              <span>${esc(it.name)}</span>
            </div>`).join('');
          $$('.fd-item', listEl).forEach(el => {
            el.onclick = () => {
              $$('.fd-item', listEl).forEach(x => x.classList.remove('selected'));
              el.classList.add('selected');
              const item = items.find(i => i.name === el.dataset.name);
              if (item && item.type === 'file') { selected = joinPath(currentPath, item.name); if (modeSave) nameEl.value = item.name; }
              else { selected = null; if (modeSave) nameEl.value = item.name; }
            };
            el.ondblclick = () => {
              const item = items.find(i => i.name === el.dataset.name);
              if (item && item.type === 'dir') navigate(joinPath(currentPath, item.name));
              else if (item && item.type === 'file') {
                selected = joinPath(currentPath, item.name);
                nameEl.value = item.name;
                const okBtn = $('.dialog-foot .btn.primary', wrap);
                if (okBtn) okBtn.click();
              }
            };
          });
        }
        async function navigate(p) {
          currentPath = p; selected = null;
          if (modeSave) nameEl.value = name;
          await refresh();
        }
        $$('.side-item', places).forEach(el => el.onclick = () => navigate(el.dataset.path));
        $('#fd-up', wrap).onclick = () => navigate(dirName(currentPath));
        nameEl.addEventListener('input', () => { name = nameEl.value; });
        await refresh();
      }
    });
    if (answer !== '__ok') return null;
    if (modeSave) {
      const n = (name || '').trim();
      return n ? joinPath(currentPath, n) : null;
    }
    return selected || null;
  }
};

function pathCrumbs(path) {
  const parts = String(path).split('/').filter(Boolean);
  let acc = '';
  let html = `<span class="crumb" data-path="/">${iconSvg('computer', 'ico crumb-ico')} Mosaic (C:)</span>`;
  for (const p of parts) {
    acc += '/' + p;
    html += `<span class="crumb-sep">›</span><span class="crumb" data-path="${esc(acc)}">${esc(p)}</span>`;
  }
  return html;
}

/* ------------------------------------------------------------------ */
/* Explorer                                                           */
/* ------------------------------------------------------------------ */
Apps.explorer = {
  title: 'Equipo', icon: 'computer', width: 940, height: 590, minW: 620, minH: 400,
  mount(body, win, args = {}) {
    body.classList.add('explorer');
    body.innerHTML = `
      <div class="toolbar">
        <button class="tool-btn" id="ex-back" title="Atrás"><svg class="ico" style="color:#2f6f9f"><use href="#i-back"></use></svg></button>
        <button class="tool-btn" id="ex-fwd" title="Adelante"><svg class="ico" style="color:#2f6f9f"><use href="#i-forward"></use></svg></button>
        <button class="tool-btn" id="ex-up" title="Subir un nivel"><svg class="ico" style="color:#2f6f9f"><use href="#i-up"></use></svg></button>
        <div class="address-crumbs" id="ex-crumbs"></div>
        <div class="search-bar"><svg class="ico"><use href="#i-search"></use></svg><input id="ex-search" placeholder="Buscar"></div>
      </div>
      <div class="toolbar" style="padding-top:3px;padding-bottom:3px">
        <button class="tool-btn" id="ex-organize">Organizar <span class="tool-arrow">▾</span></button>
        <button class="tool-btn" id="ex-new">Nueva carpeta</button>
        <div class="tool-sep"></div>
        <button class="tool-btn" id="ex-view">Vistas <span class="tool-arrow">▾</span></button>
        <div class="tool-sep"></div>
        <button class="tool-btn" id="ex-refresh">Actualizar</button>
      </div>
      <div class="pane">
        <div class="sidebar" id="ex-side"></div>
        <div class="content" id="ex-content"></div>
      </div>
      <div class="statusbar"><span id="ex-status">0 elementos</span><span class="grow"></span><span id="ex-sel"></span></div>`;

    const state = { path: args.path || FS.home, view: 'icons', sort: 'name', history: [], hi: -1, selected: null, items: [], query: '' };
    const content = $('#ex-content', body), crumbs = $('#ex-crumbs', body), status = $('#ex-status', body), selStatus = $('#ex-sel', body);

    const favorites = [
      ['Favoritos', null, null],
      ['Escritorio', joinPath(FS.home, 'Desktop'), 'folder'],
      ['Descargas', joinPath(FS.home, 'Downloads'), 'folder'],
      ['Documentos', joinPath(FS.home, 'Documents'), 'text'],
      ['Imágenes', joinPath(FS.home, 'Pictures'), 'image'],
      ['Música', joinPath(FS.home, 'Music'), 'music'],
      ['Equipo', '/', 'computer'],
      ['Papelera de reciclaje', '/Recycle Bin', 'recycle'],
      ['Red', null, 'network']
    ];
    $('#ex-side', body).innerHTML = favorites.map(([n, p, i]) =>
      i ? `<div class="side-item" data-path="${esc(p || '')}" data-name="${esc(n)}">${iconSvg(i)}<span>${esc(n)}</span></div>`
        : `<div class="side-title">${esc(n)}</div>`).join('');
    $$('.side-item', body).forEach(el => el.onclick = () => {
      if (!el.dataset.path) { Dialog.alert('Red', 'No hay ubicaciones de red disponibles.\nLa red se administra desde el Panel de control.'); return; }
      navigate(el.dataset.path);
    });

    function titleFor(p) {
      if (p === '/') return 'Equipo';
      const n = baseName(p);
      return n || 'Equipo';
    }
    function updateTitle() { win.setTitle(titleFor(state.path)); }

    async function load(path, push = true) {
      state.query = '';
      $('#ex-search', body).value = '';
      try {
        state.items = await FS.list(path);
      } catch (e) {
        state.items = [];
        Dialog.error('Explorador de MosaicOS', e.message);
      }
      state.path = path;
      state.selected = null;
      if (push) {
        state.history = state.history.slice(0, state.hi + 1);
        state.history.push(path); state.hi = state.history.length - 1;
      }
      $('#ex-back', body).disabled = state.hi <= 0;
      $('#ex-fwd', body).disabled = state.hi >= state.history.length - 1;
      crumbs.innerHTML = pathCrumbs(path);
      $$('.crumb', crumbs).forEach(c => c.onclick = () => navigate(c.dataset.path));
      $$('.side-item', body).forEach(el => el.classList.toggle('active', el.dataset.path === path));
      updateTitle(); render();
    }
    function navigate(p, push = true) { return load(p, push); }
    function sorted(items) {
      return items.slice().sort((a, b) => {
        if (a.type !== b.type) return a.type === 'dir' ? -1 : 1;
        if (state.sort === 'type') return (extOf(a.name) || '').localeCompare(extOf(b.name) || '') || a.name.localeCompare(b.name);
        if (state.sort === 'date') return (b.modified || 0) - (a.modified || 0);
        if (state.sort === 'size') return (b.size || 0) - (a.size || 0);
        return a.name.localeCompare(b.name, 'es');
      });
    }
    function render() {
      const q = state.query.toLowerCase();
      const items = sorted(state.items.filter(i => !q || i.name.toLowerCase().includes(q)));
      content.className = 'content' + (state.view === 'details' ? ' details' : '');
      if (!items.length) {
        content.innerHTML = `<div class="empty">${iconSvg(q ? 'search' : 'folder')}<span>${q ? 'No se encontraron elementos.' : 'Esta carpeta está vacía.'}</span></div>`;
      } else if (state.view === 'details') {
        content.innerHTML = `<table class="file-table"><thead><tr><th>Nombre</th><th>Fecha de modificación</th><th>Tipo</th><th>Tamaño</th></tr></thead><tbody>${items.map(it => `
          <tr data-name="${esc(it.name)}" class="${state.selected === it.name ? 'selected' : ''}">
            <td>${iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name))}${esc(it.name)}</td>
            <td>${it.modified ? fmtDate(it.modified) : '—'}</td>
            <td>${it.type === 'dir' ? 'Carpeta de archivos' : (extOf(it.name) ? 'Archivo ' + extOf(it.name).toUpperCase() : 'Archivo')}</td>
            <td style="text-align:right">${it.type === 'dir' ? '—' : fmtSize(it.size)}</td>
          </tr>`).join('')}</tbody></table>`;
      } else {
        content.innerHTML = `<div class="file-grid">${items.map(it => `
          <div class="file-cell ${state.selected === it.name ? 'selected' : ''}" data-name="${esc(it.name)}" title="${esc(it.name)}">
            ${iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name))}
            <span>${esc(it.name)}</span>
            <small>${it.type === 'file' ? fmtSize(it.size) : 'Carpeta'}</small>
          </div>`).join('')}</div>`;
      }
      const sel = items.find(i => i.name === state.selected);
      if (sel) {
        status.textContent = q ? `${items.length} de ${state.items.length} elementos` : `${state.items.length} elementos`;
        selStatus.textContent = `${sel.type === 'file' ? fmtSize(sel.size) : 'Carpeta'} — ${sel.name}`;
      } else {
        status.textContent = q ? `${items.length} de ${state.items.length} elementos` : `${state.items.length} elementos`;
        selStatus.textContent = '';
      }
      $$('.file-cell, .file-table tbody tr', content).forEach(el => {
        const name = el.dataset.name;
        el.onclick = e => { e.stopPropagation(); state.selected = name; render(); };
        el.ondblclick = () => openItem(items.find(i => i.name === name));
        el.oncontextmenu = e => { e.preventDefault(); e.stopPropagation(); state.selected = name; render(); itemMenu(e.clientX, e.clientY, items.find(i => i.name === name)); };
      });
      updateSidebar();
    }
    function updateSidebar() {
      $$('.side-item', body).forEach(el => {
        if (el.dataset.path) el.classList.toggle('active', el.dataset.path === state.path);
      });
    }
    function full(name) { return joinPath(state.path, name); }

    async function openItem(it) {
      if (!it) return;
      const p = full(it.name);
      if (it.type === 'dir') return navigate(p);
      const e = extOf(it.name);
      if (['txt', 'log', 'md', 'ini', 'conf', 'csv', 'json', 'xml', 'html', 'css', 'js', 'sh', 'bat'].includes(e)) return Shell.openApp('notepad', { path: p });
      if (['png', 'jpg', 'jpeg', 'gif', 'bmp', 'svg', 'webp'].includes(e)) return Shell.openApp('imgview', { path: p });
      if (['mp3', 'wav', 'ogg', 'flac', 'm4a'].includes(e)) return Shell.openApp('media', { path: p });
      let chosen = 'notepad';
      Dialog.show({
        title: 'Abrir con', icon: 'question', width: 460,
        body: `<p>MosaicOS no tiene una aplicación predeterminada para <b>${esc(it.name)}</b>.</p><p>Elija un programa:</p>
          <select class="text-input" id="ow-sel"><option value="notepad">Notepad</option><option value="media">Reproductor multimedia</option><option value="imgview">Visor de imágenes</option></select>`,
        buttons: [{ label: 'Abrir', value: '__ok', primary: true }, { label: 'Cancelar', value: false }],
        onOpen: (wrap) => {
          const sel = $('#ow-sel', wrap);
          if (sel) sel.onchange = () => { chosen = sel.value; };
        }
      }).then(v => {
        if (v === '__ok') Shell.openApp(chosen, { path: p });
      });
    }
    async function newFolder() {
      const n = await Dialog.prompt('Nueva carpeta', 'Nombre de la carpeta:', 'Nueva carpeta');
      if (!n) return;
      try { await FS.mkdir(full(n)); await load(state.path, false); }
      catch (e) { Dialog.error('No se pudo crear la carpeta', e.message); }
    }
    async function newText() {
      let n = await Dialog.prompt('Nuevo documento de texto', 'Nombre del archivo:', 'Nuevo documento de texto.txt');
      if (!n) return;
      if (!extOf(n)) n += '.txt';
      try { await FS.write(full(n), ''); await load(state.path, false); }
      catch (e) { Dialog.error('No se pudo crear el archivo', e.message); }
    }
    async function renameItem(it) {
      const n = await Dialog.prompt('Cambiar nombre', 'Nuevo nombre:', it.name);
      if (!n || n === it.name) return;
      try { await FS.rename(full(it.name), n); state.selected = null; await load(state.path, false); }
      catch (e) { Dialog.error('No se pudo cambiar el nombre', e.message); }
    }
    async function deleteItem(it) {
      const ok = await Dialog.confirm('Eliminar', `¿Está seguro de que desea mover "${it.name}" a la Papelera de reciclaje?`, true);
      if (!ok) return;
      try { await FS.remove(full(it.name)); state.selected = null; await load(state.path, false); }
      catch (e) { Dialog.error('No se pudo eliminar', e.message); }
    }
    async function props(it) {
      let size = it.size;
      if (it.type === 'dir' && !FS.native) size = await FS.sizeOf(full(it.name));
      const modified = it.modified ? fmtDate(it.modified) : '—';
      Dialog.show({
        title: `${it.name} — Propiedades`, icon: it.type === 'dir' ? 'folder' : iconForName(it.name), width: 430,
        body: `<div class="props">
          <div class="props-head">${iconSvg(it.type === 'dir' ? 'folder' : iconForName(it.name), 'ico props-ico')}<b>${esc(it.name)}</b></div>
          <table class="props-table">
            <tr><td>Tipo:</td><td>${it.type === 'dir' ? 'Carpeta de archivos' : 'Archivo ' + (extOf(it.name) || '')}</td></tr>
            <tr><td>Ubicación:</td><td>${esc(state.path)}</td></tr>
            <tr><td>Tamaño:</td><td>${it.type === 'dir' ? fmtSize(size) : fmtSize(size)}</td></tr>
            <tr><td>Modificado:</td><td>${modified}</td></tr>
            ${it.original ? `<tr><td>Original:</td><td>${esc(it.original)}</td></tr>` : ''}
          </table></div>`,
        buttons: [{ label: 'Aceptar', value: true, primary: true }]
      });
    }
    function itemMenu(x, y, it) {
      if (!it) return;
      const items = [
        { label: 'Abrir', icon: it.type === 'dir' ? 'folder-open' : 'program', action: () => openItem(it) },
        { sep: true },
        { label: 'Renombrar', icon: 'text', key: 'F2', action: () => renameItem(it) },
        { label: 'Eliminar', icon: 'recycle', key: 'Supr', action: () => deleteItem(it) }
      ];
      if (it.original) items.splice(2, 0, { label: 'Restaurar', icon: 'recycle', action: async () => { await FS.restore(full(it.name)); await load(state.path, false); } });
      items.push({ sep: true }, { label: 'Propiedades', icon: 'info', action: () => props(it) });
      Shell.menuAt(x, y, items);
    }
    function emptyMenu(x, y) {
      Shell.menuAt(x, y, [
        { label: 'Ver', icon: 'image', action: () => setView(state.view === 'icons' ? 'details' : 'icons') },
        { label: 'Ordenar por nombre', icon: 'text', action: () => { state.sort = 'name'; render(); } },
        { label: 'Ordenar por fecha', icon: 'clock', action: () => { state.sort = 'date'; render(); } },
        { label: 'Ordenar por tamaño', icon: 'drive', action: () => { state.sort = 'size'; render(); } },
        { sep: true },
        { label: 'Nueva carpeta', icon: 'folder', action: newFolder },
        { label: 'Nuevo documento de texto', icon: 'text', action: newText },
        { sep: true },
        { label: 'Actualizar', icon: 'forward', action: () => load(state.path, false) },
        { label: 'Propiedades de la carpeta', icon: 'info', action: () => props({ name: baseName(state.path) || 'Mosaic (C:)', type: 'dir', modified: null, size: null }) }
      ]);
    }
    function setView(v) {
      state.view = v;
      render();
      $('#ex-view', body).innerHTML = `Vistas: ${v === 'icons' ? 'Iconos' : 'Detalles'} <span class="tool-arrow">▾</span>`;
    }

    $('#ex-back', body).onclick = () => { if (state.hi > 0) { state.hi--; load(state.history[state.hi], false); } };
    $('#ex-fwd', body).onclick = () => { if (state.hi < state.history.length - 1) { state.hi++; load(state.history[state.hi], false); } };
    $('#ex-up', body).onclick = () => navigate(dirName(state.path));
    $('#ex-new', body).onclick = newFolder;
    $('#ex-refresh', body).onclick = () => load(state.path, false);
    $('#ex-search', body).oninput = e => { state.query = e.target.value.trim(); render(); };
    $('#ex-organize', body).onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left, r.bottom + 2, [
        { label: 'Nueva carpeta', icon: 'folder', action: newFolder },
        { label: 'Nuevo documento de texto', icon: 'text', action: newText },
        { sep: true },
        { label: 'Seleccionar todo', action: () => {} },
        { label: 'Propiedades', icon: 'info', action: () => props({ name: baseName(state.path) || 'Mosaic (C:)', type: 'dir', modified: null, size: null }) }
      ]);
    };
    $('#ex-view', body).onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left, r.bottom + 2, [
        { label: 'Iconos grandes', icon: 'image', action: () => setView('icons') },
        { label: 'Detalles', icon: 'text', action: () => setView('details') },
        { sep: true },
        { label: 'Ordenar por nombre', action: () => { state.sort = 'name'; render(); } },
        { label: 'Ordenar por fecha', action: () => { state.sort = 'date'; render(); } },
        { label: 'Ordenar por tamaño', action: () => { state.sort = 'size'; render(); } }
      ]);
    };
    content.oncontextmenu = e => { e.preventDefault(); state.selected = null; emptyMenu(e.clientX, e.clientY); };
    content.onclick = e => { if (e.target === content) { state.selected = null; render(); } };
    win.element.addEventListener('keydown', e => {
      if (e.key === 'Backspace' && !e.target.matches('input')) { e.preventDefault(); navigate(dirName(state.path)); }
      else if (e.key === 'F2' && state.selected) { e.preventDefault(); const it = state.items.find(i => i.name === state.selected); if (it) renameItem(it); }
      else if (e.key === 'Delete' && state.selected) { e.preventDefault(); const it = state.items.find(i => i.name === state.selected); if (it) deleteItem(it); }
      else if (e.key === 'Enter' && state.selected) { e.preventDefault(); openItem(state.items.find(i => i.name === state.selected)); }
      else if (e.key === 'F5') { e.preventDefault(); load(state.path, false); }
    });

    setView(state.view);
    load(state.path);
  }
};

/* ------------------------------------------------------------------ */
/* Notepad                                                            */
/* ------------------------------------------------------------------ */
Apps.notepad = {
  title: 'Sin título — Notepad', icon: 'notepad', width: 680, height: 500, minW: 380, minH: 240,
  mount(body, win, args = {}) {
    body.classList.add('notepad');
    body.innerHTML = `
      <div class="menubar">
        <button data-m="archivo">Archivo</button><button data-m="editar">Edición</button>
        <button data-m="formato">Formato</button><button data-m="ver">Ver</button><button data-m="ayuda">Ayuda</button>
      </div>
      <textarea spellcheck="false" aria-label="Texto"></textarea>
      <div class="statusbar"><span id="np-pos">Lín 1, Col 1</span><span class="grow"></span><span id="np-chars">0 caracteres</span></div>`;
    const ta = $('textarea', body), pos = $('#np-pos', body), chars = $('#np-chars', body);
    let path = args.path || null, dirty = false, wrap = true;
    ta.style.whiteSpace = wrap ? 'pre-wrap' : 'pre';
    ta.style.overflowWrap = wrap ? 'break-word' : 'normal';

    function name() { return path ? baseName(path) : 'Sin título'; }
    function updateTitle() { win.setTitle(`${name()}${dirty ? ' *' : ''} — Notepad`); }
    function updateStatus() {
      const upto = ta.value.slice(0, ta.selectionStart);
      const line = upto.split('\n').length;
      const col = upto.length - upto.lastIndexOf('\n');
      pos.textContent = `Lín ${line}, Col ${col}`;
      chars.textContent = `${ta.value.length} caracteres`;
    }
    async function doOpen() {
      const p = await FileDialog.show({ mode: 'open', title: 'Abrir', initial: path ? dirName(path) : joinPath(FS.home, 'Documents'), filter: ['txt', 'md', 'log', 'ini', 'conf', 'csv', 'json', 'xml', 'html', 'css', 'js', 'sh', 'bat'] });
      if (!p) return;
      try {
        const text = await FS.read(p);
        path = p; dirty = false; ta.value = text; updateTitle(); updateStatus();
      } catch (e) { Dialog.error('No se pudo abrir el archivo', e.message); }
    }
    async function doSaveAs() {
      const p = await FileDialog.show({ mode: 'save', title: 'Guardar como', initial: path ? dirName(path) : joinPath(FS.home, 'Documents'), filename: path ? baseName(path) : 'Sin título.txt' });
      if (!p) return false;
      try {
        await FS.write(p, ta.value);
        path = p; dirty = false; updateTitle();
        toast('Notepad', 'Archivo guardado en ' + p);
        return true;
      } catch (e) { Dialog.error('No se pudo guardar', e.message); return false; }
    }
    async function doSave() {
      if (!path) return doSaveAs();
      try { await FS.write(path, ta.value); dirty = false; updateTitle(); return true; }
      catch (e) { Dialog.error('No se pudo guardar', e.message); return false; }
    }
    async function doNew() {
      if (dirty && !(await Dialog.confirm('Notepad', 'El texto no se ha guardado. ¿Desea descartar los cambios?'))) return;
      path = null; dirty = false; ta.value = ''; updateTitle(); updateStatus();
    }
    function insertText(t) {
      const s = ta.selectionStart, e = ta.selectionEnd;
      ta.setRangeText(t, s, e, 'end');
      dirty = true; updateTitle(); updateStatus();
    }
    function menu(name) {
      const base = [
        { label: 'Nuevo', icon: 'text', key: 'Ctrl+N', action: doNew },
        { label: 'Abrir…', icon: 'folder-open', key: 'Ctrl+O', action: doOpen },
        { label: 'Guardar', icon: 'drive', key: 'Ctrl+S', action: doSave },
        { label: 'Guardar como…', action: doSaveAs },
        { sep: true },
        { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ];
      if (name === 'archivo') return base;
      if (name === 'editar') return [
        { label: 'Deshacer', key: 'Ctrl+Z', action: () => document.execCommand('undo') },
        { sep: true },
        { label: 'Cortar', key: 'Ctrl+X', action: () => document.execCommand('cut') },
        { label: 'Copiar', key: 'Ctrl+C', action: () => document.execCommand('copy') },
        { label: 'Pegar', key: 'Ctrl+V', action: () => document.execCommand('paste') },
        { sep: true },
        { label: 'Seleccionar todo', key: 'Ctrl+E', action: () => ta.select() },
        { label: 'Hora y fecha', key: 'F5', action: () => insertText(new Date().toLocaleString('es-ES')) }
      ];
      if (name === 'formato') return [
        { label: (wrap ? '✓ ' : '') + 'Ajuste de línea', action: () => { wrap = !wrap; ta.style.whiteSpace = wrap ? 'pre-wrap' : 'pre'; ta.style.overflowWrap = wrap ? 'break-word' : 'normal'; } },
        { label: 'Fuente…', action: () => chooseFont() }
      ];
      if (name === 'ver') return [
        { label: 'Ampliar', action: () => { ta.style.fontSize = (parseFloat(getComputedStyle(ta).fontSize) + 1) + 'px'; } },
        { label: 'Reducir', action: () => { ta.style.fontSize = (parseFloat(getComputedStyle(ta).fontSize) - 1) + 'px'; } },
        { label: 'Restablecer zoom', action: () => { ta.style.fontSize = '13px'; } }
      ];
      return [
        { label: 'Acerca de Notepad', icon: 'notepad', action: () => Dialog.alert('Acerca de Notepad', 'Notepad de MosaicOS 0.50\nUn editor de texto sencillo con guardado en el sistema de archivos.\n\nAtajos: Ctrl+N, Ctrl+O, Ctrl+S.') },
        { label: 'Ayuda de MosaicOS', icon: 'help', action: () => Shell.openApp('browser', { page: 'help' }) }
      ];
    }
    function chooseFont() {
      Dialog.show({
        title: 'Fuente', icon: 'text', width: 380,
        body: `<label class="field-label">Tamaño</label><select class="text-input" id="font-size"><option>11</option><option>12</option><option selected>13</option><option>14</option><option>16</option><option>18</option><option>20</option></select>`,
        buttons: [{ label: 'Aceptar', value: '__ok', primary: true }, { label: 'Cancelar', value: false }]
      }).then(v => {
        if (v === '__ok') {
          const s = $('#dialog-layer select'); if (s) { ta.style.fontSize = s.value + 'px'; }
        }
      });
    }
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left, r.bottom, menu(b.dataset.m));
    });
    ta.addEventListener('input', () => { dirty = true; updateTitle(); updateStatus(); });
    ta.addEventListener('click', updateStatus);
    ta.addEventListener('keyup', updateStatus);
    ta.addEventListener('keydown', e => {
      if (e.ctrlKey && e.key.toLowerCase() === 's') { e.preventDefault(); doSave(); }
      if (e.ctrlKey && e.key.toLowerCase() === 'o') { e.preventDefault(); doOpen(); }
      if (e.ctrlKey && e.key.toLowerCase() === 'n') { e.preventDefault(); doNew(); }
      if (e.key === 'F5') { e.preventDefault(); insertText(new Date().toLocaleString('es-ES')); }
    });
    win.beforeClose = async () => {
      if (!dirty) return true;
      return !!(await Dialog.confirm('Notepad', '¿Desea guardar los cambios antes de cerrar?'));
    };
    if (path) {
      FS.read(path).then(t => { ta.value = t; dirty = false; updateTitle(); updateStatus(); })
        .catch(e => Dialog.error('No se pudo abrir', e.message));
    }
    updateTitle(); updateStatus();
    setTimeout(() => ta.focus(), 60);
  }
};

/* ------------------------------------------------------------------ */
/* Paint                                                              */
/* ------------------------------------------------------------------ */
Apps.paint = {
  title: 'Sin título — Paint', icon: 'paint', width: 900, height: 620, minW: 620, minH: 420,
  mount(body, win, args = {}) {
    body.classList.add('paint');
    body.innerHTML = `
      <div class="menubar">
        <button data-m="archivo">Archivo</button><button data-m="editar">Editar</button>
        <button data-m="ver">Ver</button><button data-m="ayuda">Ayuda</button>
      </div>
      <div class="toolbar">
        <button class="tool-btn" id="pt-undo" title="Deshacer">↶ Deshacer</button>
        <button class="tool-btn" id="pt-redo" title="Rehacer">↷ Rehacer</button>
        <div class="tool-sep"></div>
        <span style="font-size:11px;color:#557082">Herramienta:</span><span id="pt-tool-name" style="font-size:11px;font-weight:600;color:#24597c">Pincel</span>
        <div class="tool-sep"></div>
        <span style="font-size:11px;color:#557082">Tamaño:</span><input type="range" id="pt-size" min="1" max="40" value="4" style="width:90px">
        <div class="tool-sep"></div>
        <button class="tool-btn" id="pt-clear">Borrar todo</button>
      </div>
      <div class="paint-body">
        <div class="pt-tools" id="pt-tools"></div>
        <div class="pt-canvas-wrap"><canvas id="pt-canvas" width="820" height="480"></canvas></div>
      </div>
      <div class="statusbar"><span id="pt-status">820 × 480 px</span><span class="grow"></span><span id="pt-pos">0, 0</span></div>`;
    const canvas = $('#pt-canvas', body), ctx = canvas.getContext('2d', { willReadFrequently: true });
    const palette = ['#000000', '#7f7f7f', '#880015', '#ed1c24', '#ff7f27', '#fff200', '#22b14c', '#00a2e8', '#3f48cc', '#a349a4', '#ffffff', '#c3c3c3', '#b97a57', '#ffaec9', '#ffc90e', '#efe4b0', '#b5e61d', '#99d9ea', '#7092be', '#c8bfe7'];
    let tool = 'brush', color = '#000000', size = 4, drawing = false, start = null, snapshot = null;
    let undoStack = [], redoStack = [], dirty = false, currentPath = args.path || null;
    ctx.lineCap = 'round'; ctx.lineJoin = 'round';

    function fillWhite() { ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, canvas.width, canvas.height); }
    fillWhite();

    const tools = [
      ['pencil', '✎', 'Lápiz'], ['brush', '🖌', 'Pincel'], ['eraser', '▨', 'Borrador'],
      ['line', '╱', 'Línea'], ['rect', '□', 'Rectángulo'], ['ellipse', '○', 'Elipse'],
      ['fill', '▩', 'Relleno'], ['picker', '◉', 'Cuentagotas']
    ];
    $('#pt-tools', body).innerHTML = tools.map(([id, ch, name]) =>
      `<button class="pt-tool ${id === tool ? 'active' : ''}" data-tool="${id}" title="${name}">${ch}</button>`).join('') +
      `<div class="pt-colors">${palette.map(c => `<button class="pt-color ${c === color ? 'active' : ''}" data-color="${c}" style="background:${c}" title="${c}"></button>`).join('')}</div>`;
    $('.pt-colors', body).insertAdjacentHTML('beforebegin', '<div style="width:100%;height:1px;background:#c1d2df;margin:4px 0"></div>');
    $$('.pt-tool', body).forEach(b => b.onclick = () => {
      tool = b.dataset.tool;
      $$('.pt-tool', body).forEach(x => x.classList.toggle('active', x === b));
      $('#pt-tool-name', body).textContent = tools.find(t => t[0] === tool)[2];
    });
    $$('.pt-color', body).forEach(b => b.onclick = () => {
      color = b.dataset.color;
      $$('.pt-color', body).forEach(x => x.classList.toggle('active', x === b));
    });
    $('#pt-size', body).oninput = e => { size = +e.target.value; };
    $('#pt-clear', body).onclick = () => {
      pushUndo(); fillWhite(); dirty = true;
    };
    $('#pt-undo', body).onclick = undo;
    $('#pt-redo', body).onclick = redo;

    function pushUndo() {
      try { undoStack.push(ctx.getImageData(0, 0, canvas.width, canvas.height)); if (undoStack.length > 15) undoStack.shift(); redoStack = []; } catch (e) {}
    }
    function undo() {
      if (!undoStack.length) return;
      redoStack.push(ctx.getImageData(0, 0, canvas.width, canvas.height));
      ctx.putImageData(undoStack.pop(), 0, 0); dirty = true;
    }
    function redo() {
      if (!redoStack.length) return;
      undoStack.push(ctx.getImageData(0, 0, canvas.width, canvas.height));
      ctx.putImageData(redoStack.pop(), 0, 0); dirty = true;
    }
    function pos(e) {
      const r = canvas.getBoundingClientRect();
      return { x: Math.round((e.clientX - r.left) * canvas.width / r.width), y: Math.round((e.clientY - r.top) * canvas.height / r.height) };
    }
    function stroke(a, b) {
      ctx.strokeStyle = tool === 'eraser' ? '#ffffff' : color;
      ctx.lineWidth = size;
      ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
    }
    function floodFill(x, y) {
      const img = ctx.getImageData(0, 0, canvas.width, canvas.height);
      const d = img.data, w = canvas.width, h = canvas.height;
      const idx = (y * w + x) * 4;
      const tr = d[idx], tg = d[idx + 1], tb = d[idx + 2], ta = d[idx + 3];
      const hex = color.replace('#', '');
      const fr = parseInt(hex.slice(0, 2), 16), fg = parseInt(hex.slice(2, 4), 16), fb = parseInt(hex.slice(4, 6), 16);
      if (tr === fr && tg === fg && tb === fb && ta === 255) return;
      const stack = [[x, y]];
      const seen = new Uint8Array(w * h);
      while (stack.length) {
        const [cx, cy] = stack.pop();
        if (cx < 0 || cy < 0 || cx >= w || cy >= h) continue;
        const p = cy * w + cx;
        if (seen[p]) continue; seen[p] = 1;
        const i = p * 4;
        if (Math.abs(d[i] - tr) > 18 || Math.abs(d[i + 1] - tg) > 18 || Math.abs(d[i + 2] - tb) > 18 || Math.abs(d[i + 3] - ta) > 18) continue;
        d[i] = fr; d[i + 1] = fg; d[i + 2] = fb; d[i + 3] = 255;
        stack.push([cx + 1, cy], [cx - 1, cy], [cx, cy + 1], [cx, cy - 1]);
      }
      ctx.putImageData(img, 0, 0);
    }
    canvas.addEventListener('pointerdown', e => {
      e.preventDefault(); canvas.setPointerCapture(e.pointerId);
      const p = pos(e); start = p;
      if (tool === 'fill') { pushUndo(); floodFill(p.x, p.y); dirty = true; return; }
      if (tool === 'picker') {
        const d = ctx.getImageData(p.x, p.y, 1, 1).data;
        color = '#' + [d[0], d[1], d[2]].map(v => v.toString(16).padStart(2, '0')).join('');
        $$('.pt-color', body).forEach(x => x.classList.toggle('active', x.dataset.color === color));
        return;
      }
      pushUndo(); drawing = true;
      snapshot = ctx.getImageData(0, 0, canvas.width, canvas.height);
      if (tool === 'pencil' || tool === 'brush' || tool === 'eraser') { stroke(p, { x: p.x + .01, y: p.y + .01 }); }
    });
    canvas.addEventListener('pointermove', e => {
      const p = pos(e);
      $('#pt-pos', body).textContent = `${p.x}, ${p.y}`;
      if (!drawing) return;
      if (tool === 'pencil' || tool === 'brush' || tool === 'eraser') { stroke(start, p); start = p; }
      else {
        ctx.putImageData(snapshot, 0, 0);
        ctx.strokeStyle = color; ctx.lineWidth = size;
        if (tool === 'line') { ctx.beginPath(); ctx.moveTo(start.x, start.y); ctx.lineTo(p.x, p.y); ctx.stroke(); }
        if (tool === 'rect') { ctx.strokeRect(start.x, start.y, p.x - start.x, p.y - start.y); }
        if (tool === 'ellipse') { ctx.beginPath(); ctx.ellipse((start.x + p.x) / 2, (start.y + p.y) / 2, Math.abs(p.x - start.x) / 2, Math.abs(p.y - start.y) / 2, 0, 0, Math.PI * 2); ctx.stroke(); }
      }
    });
    canvas.addEventListener('pointerup', () => { if (drawing) { drawing = false; dirty = true; win.setTitle(`${currentPath ? baseName(currentPath) : 'Sin título'}${dirty ? ' *' : ''} — Paint`); } });
    canvas.addEventListener('pointercancel', () => { drawing = false; });

    async function saveAs() {
      const p = await FileDialog.show({ mode: 'save', title: 'Guardar como', initial: currentPath ? dirName(currentPath) : joinPath(FS.home, 'Pictures'), filename: currentPath ? baseName(currentPath) : 'dibujo.png', filter: ['png'] });
      if (!p) return;
      try {
        await FS.write(p, canvas.toDataURL('image/png'));
        currentPath = p; dirty = false; win.setTitle(`${baseName(p)} — Paint`);
        toast('Paint', 'Imagen guardada en ' + p);
      } catch (e) { Dialog.error('No se pudo guardar la imagen', e.message); }
    }
    async function save() {
      if (!currentPath) return saveAs();
      try { await FS.write(currentPath, canvas.toDataURL('image/png')); dirty = false; win.setTitle(`${baseName(currentPath)} — Paint`); }
      catch (e) { Dialog.error('No se pudo guardar', e.message); }
    }
    function menu(name) {
      if (name === 'archivo') return [
        { label: 'Nuevo', icon: 'image', action: async () => { if (dirty && !(await confirmDiscard())) return; pushUndo(); fillWhite(); currentPath = null; dirty = false; win.setTitle('Sin título — Paint'); } },
        { label: 'Abrir…', icon: 'folder-open', action: openImage },
        { label: 'Guardar', icon: 'drive', key: 'Ctrl+S', action: save },
        { label: 'Guardar como…', action: saveAs },
        { sep: true }, { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ];
      if (name === 'editar') return [
        { label: 'Deshacer', key: 'Ctrl+Z', action: undo }, { label: 'Rehacer', key: 'Ctrl+Y', action: redo },
        { sep: true }, { label: 'Borrar imagen', action: () => { pushUndo(); fillWhite(); dirty = true; } }
      ];
      if (name === 'ver') return [{ label: 'Ampliar', action: () => zoom(1) }, { label: 'Reducir', action: () => zoom(-1) }, { label: 'Tamaño original', action: () => { canvas.style.width = ''; canvas.style.height = ''; } }];
      return [{ label: 'Acerca de Paint', icon: 'paint', action: () => Dialog.alert('Acerca de Paint', 'Paint de MosaicOS 0.50\nLápiz, pincel, borrador, formas, relleno y cuentagotas.\nGuarda imágenes PNG en el sistema de archivos.') }];
    }
    function confirmDiscard() { return Dialog.confirm('Paint', 'Hay cambios sin guardar. ¿Desea descartarlos?'); }
    function zoom(dir) {
      const w = parseInt(canvas.style.width) || canvas.width;
      canvas.style.width = clamp(w + dir * 80, 200, 2400) + 'px';
      canvas.style.height = 'auto';
    }
    async function openImage() {
      const p = await FileDialog.show({ mode: 'open', title: 'Abrir imagen', initial: joinPath(FS.home, 'Pictures'), filter: ['png', 'jpg', 'jpeg', 'gif', 'bmp', 'webp'] });
      if (!p) return;
      try {
        const data = await FS.read(p);
        const img = new Image();
        img.onload = () => {
          pushUndo();
          const scale = Math.min(canvas.width / img.width, canvas.height / img.height, 1);
          ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, canvas.width, canvas.height);
          ctx.drawImage(img, 0, 0, img.width * scale, img.height * scale);
          currentPath = p; dirty = false; win.setTitle(`${baseName(p)} — Paint`);
        };
        img.src = data;
      } catch (e) { Dialog.error('No se pudo abrir la imagen', e.message); }
    }
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left, r.bottom, menu(b.dataset.m));
    });
    win.element.addEventListener('keydown', e => {
      if (e.ctrlKey && e.key.toLowerCase() === 's') { e.preventDefault(); save(); }
      if (e.ctrlKey && e.key.toLowerCase() === 'z') { e.preventDefault(); undo(); }
      if (e.ctrlKey && e.key.toLowerCase() === 'y') { e.preventDefault(); redo(); }
    });
    win.beforeClose = async () => !dirty || !!(await Dialog.confirm('Paint', 'Hay cambios sin guardar. ¿Desea cerrar de todos modos?'));
    win.setTitle('Sin título — Paint');
  }
};

/* ------------------------------------------------------------------ */
/* Calculator                                                         */
/* ------------------------------------------------------------------ */
Apps.calc = {
  title: 'Calculadora', icon: 'calc', width: 296, height: 408, minW: 280, minH: 390,
  mount(body, win) {
    body.classList.add('calc');
    body.innerHTML = `
      <div class="menubar"><button data-m="ver">Ver</button><button data-m="editar">Edición</button><button data-m="ayuda">Ayuda</button></div>
      <div class="calc-body">
        <div class="calc-display"><div class="calc-mem" id="c-mem"></div><div class="calc-out" id="c-out">0</div></div>
        <div class="calc-grid" id="c-grid"></div>
      </div>`;
    const out = $('#c-out', body), memEl = $('#c-mem', body);
    const keys = [
      ['MC', 'mem'], ['MR', 'mem'], ['MS', 'mem'], ['M+', 'mem'], ['M-', 'mem'],
      ['←', ''], ['CE', ''], ['C', ''], ['±', ''], ['√', ''],
      ['7', ''], ['8', ''], ['9', ''], ['/', 'op'], ['%', ''],
      ['4', ''], ['5', ''], ['6', ''], ['*', 'op'], ['1/x', ''],
      ['1', ''], ['2', ''], ['3', ''], ['-', 'op'], ['=', 'eq'],
      ['0', ''], ['.', ''], ['+', 'op']
    ];
    let display = '0', acc = null, op = null, fresh = true, mem = 0, lastKey = '';

    function fmt(n) {
      if (!isFinite(n)) return 'No se puede dividir por cero';
      const a = Math.abs(n);
      if (a !== 0 && (a >= 1e16 || a < 1e-10)) return n.toExponential(10).replace('e', 'e');
      let s = String(parseFloat(n.toPrecision(15)));
      if (s.length > 18) s = n.toExponential(9);
      return s;
    }
    function show() { out.textContent = display; memEl.textContent = mem ? 'M' : ''; }
    function num(d) {
      if (fresh) { display = d === '.' ? '0.' : d; fresh = false; }
      else if (d === '.' ? !display.includes('.') : display.replace('-', '').replace('.', '').length < 16) display += d;
    }
    function calc(a, b, o) {
      switch (o) { case '+': return a + b; case '-': return a - b; case '*': return a * b; case '/': return b === 0 ? NaN : a / b; default: return b; }
    }
    function opPress(o) {
      if (op && !fresh) { const r = calc(acc, parseFloat(display), op); display = fmt(r); acc = r; }
      else acc = parseFloat(display);
      op = o; fresh = true;
    }
    function equals() {
      if (op == null) return;
      const b = parseFloat(display);
      const r = calc(acc, b, op);
      display = fmt(r); acc = null; op = null; fresh = true;
    }
    function unary(k) {
      const v = parseFloat(display);
      if (k === 'sqrt') display = fmt(Math.sqrt(v));
      else if (k === 'inv') display = fmt(1 / v);
      else if (k === 'neg') display = fmt(-v);
      else if (k === 'pct') display = fmt(acc != null ? acc * v / 100 : v / 100);
      else if (k === 'back') display = display.length > 1 ? display.slice(0, -1) : '0';
      else if (k === 'ce') { display = '0'; fresh = true; }
      else if (k === 'c') { display = '0'; acc = null; op = null; fresh = true; }
      fresh = true;
    }
    function memKey(k) {
      const v = parseFloat(display);
      if (k === 'mc') mem = 0;
      if (k === 'mr') { display = fmt(mem); fresh = true; }
      if (k === 'ms') mem = v;
      if (k === 'm+') mem += v;
      if (k === 'm-') mem -= v;
    }
    function press(k) {
      if (/^[0-9.]$/.test(k)) num(k);
      else if ('+-*/'.includes(k) && k.length === 1) opPress(k);
      else if (k === '=') equals();
      else if (k === '←') { display = display.length > 1 ? display.slice(0, -1) : '0'; }
      else if (k === 'CE') { display = '0'; fresh = true; }
      else if (k === 'C') { display = '0'; acc = null; op = null; fresh = true; }
      else if (k === '±') display = fmt(-parseFloat(display));
      else if (k === '√') display = fmt(Math.sqrt(parseFloat(display)));
      else if (k === '1/x') display = fmt(1 / parseFloat(display));
      else if (k === '%') display = fmt(acc != null ? acc * parseFloat(display) / 100 : parseFloat(display) / 100);
      else if (['MC', 'MR', 'MS', 'M+', 'M-'].includes(k)) memKey(k.toLowerCase());
      show();
    }
    $('#c-grid', body).innerHTML = keys.map(([k, c]) => `<button class="${c}" data-k="${k}">${k}</button>`).join('');
    $$('#c-grid button', body).forEach(b => b.onclick = () => { AudioKit.click(); press(b.dataset.k); });
    win.element.addEventListener('keydown', e => {
      const k = e.key;
      if (/^[0-9.]$/.test(k) || '+-*/'.includes(k)) { e.preventDefault(); press(k); }
      else if (k === 'Enter' || k === '=') { e.preventDefault(); press('='); }
      else if (k === 'Backspace') { e.preventDefault(); press('←'); }
      else if (k === 'Escape') { e.preventDefault(); press('C'); }
    });
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left, r.bottom, [
        { label: 'Calculadora estándar', icon: 'calc', action: () => {} },
        { sep: true },
        { label: 'Acerca de la Calculadora', icon: 'info', action: () => Dialog.alert('Calculadora', 'Calculadora estándar de MosaicOS 0.50.\nUse el teclado numérico o haga clic en los botones.') }
      ]);
    });
    show();
  }
};

/* ------------------------------------------------------------------ */
/* Terminal                                                           */
/* ------------------------------------------------------------------ */
Apps.terminal = {
  title: 'Símbolo del sistema', icon: 'terminal', width: 720, height: 460, minW: 460, minH: 260,
  mount(body, win, args = {}) {
    body.classList.add('terminal');
    body.innerHTML = `<div class="term-body" id="term-body"></div>`;
    const term = $('#term-body', body);
    let cwd = args.path || FS.home;
    let history = Store.get('term.history', []), hIndex = history.length;
    const started = Date.now();

    function print(text = '', cls = '') {
      const d = document.createElement('div');
      d.className = 'term-line ' + cls;
      d.textContent = text;
      term.appendChild(d);
      term.scrollTop = term.scrollHeight;
      return d;
    }
    function printHtml(html) {
      const d = document.createElement('div');
      d.className = 'term-line';
      d.innerHTML = html;
      term.appendChild(d);
      term.scrollTop = term.scrollHeight;
    }
    function dosPath(p) {
      if (p === '/') return 'C:\\';
      return 'C:' + p.replace(/\//g, '\\');
    }
    function promptEl() {
      const line = document.createElement('div');
      line.className = 'term-line term-inline';
      const span = document.createElement('span');
      span.className = 'term-prompt';
      span.textContent = `Mosaic@MosaicOS ${dosPath(cwd)}>`;
      const input = document.createElement('input');
      input.setAttribute('aria-label', 'Comando');
      input.autocomplete = 'off'; input.spellcheck = false;
      line.appendChild(span); line.appendChild(input);
      term.appendChild(line); term.scrollTop = term.scrollHeight;
      input.focus();
      input.addEventListener('keydown', async e => {
        if (e.key === 'Enter') {
          const cmd = input.value;
          input.disabled = true;
          line.removeChild(input);
          const echo = document.createElement('span');
          echo.textContent = cmd; line.appendChild(echo);
          if (cmd.trim()) { history.push(cmd); history = history.slice(-100); Store.set('term.history', history); }
          hIndex = history.length;
          await run(cmd);
          promptEl();
        } else if (e.key === 'ArrowUp') {
          e.preventDefault(); if (hIndex > 0) { hIndex--; input.value = history[hIndex] || ''; }
        } else if (e.key === 'ArrowDown') {
          e.preventDefault(); if (hIndex < history.length - 1) { hIndex++; input.value = history[hIndex] || ''; } else { hIndex = history.length; input.value = ''; }
        } else if (e.key === 'Tab') {
          e.preventDefault();
          const parts = input.value.split(' ');
          const last = parts.pop();
          try {
            const items = await FS.list(cwd);
            const m = items.map(i => i.name).filter(n => n.toLowerCase().startsWith(last.toLowerCase()));
            if (m.length === 1) input.value = parts.concat(m[0]).join(' ');
          } catch (err) {}
        }
      });
      line.onclick = () => input.focus();
    }
    function resolve(arg) {
      if (!arg || arg === '.') return cwd;
      if (arg === '..') return dirName(cwd);
      if (arg === '~') return FS.home;
      if (/^[a-zA-Z]:[\\/]/.test(arg)) return arg.replace(/^[a-zA-Z]:[\\/]?/, '/').replace(/\\/g, '/');
      if (arg.startsWith('/')) return arg.replace(/\\/g, '/');
      return joinPath(cwd, arg);
    }
    function parse(line) {
      const out = [];
      let cur = '', q = null;
      for (const ch of line) {
        if (q) { if (ch === q) q = null; else cur += ch; }
        else if (ch === '"' || ch === "'") q = ch;
        else if (/\s/.test(ch)) { if (cur) { out.push(cur); cur = ''; } }
        else cur += ch;
      }
      if (cur) out.push(cur);
      return out;
    }
    async function run(line) {
      const raw = line.trim();
      if (!raw) return;
      let redirect = null;
      const rm = raw.match(/^(.*?)\s*>\s*(\S+)\s*$/);
      let text = raw;
      if (rm) { text = rm[1]; redirect = resolve(rm[2]); }
      const [cmdRaw, ...args] = parse(text);
      const cmd = (cmdRaw || '').toLowerCase();
      const arg = args.join(' ');
      try {
        switch (cmd) {
          case 'help': case '?':
            print('Comandos disponibles:', 'term-dim');
            print('  dir, ls            Lista archivos y carpetas');
            print('  cd <ruta>          Cambia de carpeta');
            print('  type, cat <arch>   Muestra el contenido de un archivo');
            print('  echo <texto>       Escribe texto (use > archivo)');
            print('  mkdir, md <nombre> Crea una carpeta');
            print('  rm, del <nombre>   Elimina un elemento');
            print('  ren <a> <b>        Renombra');
            print('  tree               Muestra el árbol de la carpeta actual');
            print('  cls, clear         Limpia la pantalla');
            print('  date, time, ver    Fecha, hora y versión');
            print('  systeminfo         Información del sistema');
            print('  ipconfig           Estado de red');
            print('  open <app> [ruta]  Abre una aplicación');
            print('  notepad, calc, mspaint, explorer, control, media, browser');
            print('  neofetch           Logo e información de MosaicOS');
            print('  history            Historial de comandos');
            print('  exit               Cierra la terminal');
            break;
          case 'cls': case 'clear': term.innerHTML = ''; break;
          case 'dir': case 'ls': {
            let items = await FS.list(cwd);
            if (arg) items = items.filter(i => i.name.toLowerCase().includes(arg.toLowerCase()));
            print(` Directorio de ${dosPath(cwd)}`, 'term-dim');
            print('');
            if (!items.length) print(' No hay archivos.');
            let files = 0, dirs = 0;
            for (const it of items) {
              if (it.type === 'dir') { dirs++; print(`${fmtDate(it.modified).padEnd(20)} <DIR>          ${it.name}`); }
              else { files++; print(`${fmtDate(it.modified).padEnd(20)} ${String(fmtSize(it.size)).padStart(14)}  ${it.name}`); }
            }
            print('');
            print(`               ${files} archivo(s), ${dirs} carpeta(s)`, 'term-dim');
            break;
          }
          case 'cd': case 'chdir': {
            const t = resolve(arg || '~');
            const items = await FS.list(t);
            if (!Array.isArray(items)) throw new Error('No es una carpeta');
            cwd = t;
            break;
          }
          case 'type': case 'cat': {
            if (!arg) { print('Falta el nombre del archivo.', 'term-err'); break; }
            const content = await FS.read(resolve(arg));
            if (redirect) { await FS.write(redirect, content); print(`Contenido copiado a ${redirect}`); }
            else print(content);
            break;
          }
          case 'echo': {
            let out = arg;
            if (out.startsWith('"') && out.endsWith('"')) out = out.slice(1, -1);
            if (redirect) { await FS.write(redirect, out); print(`Texto escrito en ${redirect}`); }
            else print(out);
            break;
          }
          case 'mkdir': case 'md': {
            if (!arg) { print('Falta el nombre de la carpeta.', 'term-err'); break; }
            await FS.mkdir(resolve(arg)); print(`Carpeta creada: ${arg}`, 'term-ok');
            break;
          }
          case 'rm': case 'del': case 'rd': case 'rmdir': {
            if (!arg) { print('Falta el nombre del elemento.', 'term-err'); break; }
            await FS.remove(resolve(arg)); print(`Eliminado: ${arg}`, 'term-ok');
            break;
          }
          case 'ren': case 'rename': {
            if (args.length < 2) { print('Uso: ren <actual> <nuevo>', 'term-err'); break; }
            await FS.rename(resolve(args[0]), args[1]); print('Renombrado.', 'term-ok');
            break;
          }
          case 'tree': {
            async function walk(p, prefix, depth) {
              if (depth > 4) return;
              const its = await FS.list(p);
              for (let i = 0; i < its.length; i++) {
                const it = its[i], last = i === its.length - 1;
                print(prefix + (last ? '└── ' : '├── ') + it.name);
                if (it.type === 'dir') await walk(joinPath(p, it.name), prefix + (last ? '    ' : '│   '), depth + 1);
              }
            }
            print(dosPath(cwd));
            await walk(cwd, '', 0);
            break;
          }
          case 'date': print(new Date().toLocaleDateString('es-ES', { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' })); break;
          case 'time': print(fmtTime(new Date(), true)); break;
          case 'ver': print('MosaicOS [Versión 0.50.2026] — Aero Shell'); break;
          case 'whoami': print(Settings.data.userName.toLowerCase() + '\\mosaic'); break;
          case 'systeminfo':
            print(`Nombre del equipo:        MOSAICOS`);
            print(`Sistema operativo:        ${FS.native ? 'MosaicOS Daily 0.50 (Debian live)' : 'MosaicOS 0.50 Aero (shell web)'}`);
            print(`Versión:                  0.50`);
            print(`Fabricante del SO:        MosaicOS Project`);
            print(`Usuario:                  ${Settings.data.userName}`);
            print(`Modo de archivos:         ${FS.native ? 'Sistema real (' + FS.home + ')' : 'Almacenamiento local del navegador'}`);
            print(`Pantalla:                 ${screen.width} x ${screen.height}`);
            print(`Memoria del dispositivo:  ${navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'no disponible'}`);
            print(`Núcleos lógicos:          ${navigator.hardwareConcurrency || 'no disponible'}`);
            print(`Tiempo activo:            ${Math.round((Date.now() - started) / 60000)} min`);
            print(`Conexión:                 ${navigator.onLine ? 'Conectada' : 'Sin conexión'}`);
            break;
          case 'ipconfig':
            print('Configuración IP de MosaicOS', 'term-dim');
            print('');
            print(`   Estado: ${navigator.onLine ? 'Conectado' : 'Desconectado'}`);
            print(`   Tipo:   ${navigator.connection ? navigator.connection.effectiveType : 'desconocido'}`);
            print(`   Host:   mosaicos.local`);
            print(`   Puente: ${FS.native ? 'activo' : 'no disponible (modo demo)'}`);
            break;
          case 'open': case 'start': {
            const appName = (args[0] || '').toLowerCase();
            const map = { notepad: 'notepad', calc: 'calc', mspaint: 'paint', paint: 'paint', explorer: 'explorer', control: 'control', media: 'media', wmplayer: 'media', browser: 'browser', ie: 'browser', game: 'games', taskmgr: 'taskmgr' };
            if (!map[appName]) { print(`No se encuentra la aplicación: ${appName}`, 'term-err'); break; }
            Shell.openApp(map[appName], args[1] ? { path: resolve(args[1]) } : {});
            print(`Iniciando ${appName}…`, 'term-ok');
            break;
          }
          case 'notepad': Shell.openApp('notepad', {}); break;
          case 'calc': Shell.openApp('calc', {}); break;
          case 'mspaint': Shell.openApp('paint', {}); break;
          case 'explorer': Shell.openApp('explorer', { path: cwd }); break;
          case 'control': Shell.openApp('control', {}); break;
          case 'media': Shell.openApp('media', {}); break;
          case 'browser': Shell.openApp('browser', {}); break;
          case 'neofetch': {
            const logo = [
              '      ██████╗ ██╗   ██╗',
              '     ██╔═══██╗██║   ██║',
              '     ██║   ██║██║   ██║',
              '     ██║   ██║╚██╗ ██╔╝',
              '     ╚██████╔╝ ╚████╔╝ ',
              '      ╚═════╝   ╚═══╝  '
            ];
            print(`${Settings.data.userName}@mosaicos`);
            print('-----------------');
            print('OS: MosaicOS 0.50 Aero');
            print('Kernel: web-shell');
            print(`Shell: Mosaic Aero`);
            print(`Resolution: ${screen.width}x${screen.height}`);
            print(`WM: MosaicWM`);
            print(`Theme: Aero 7`);
            print(`Icons: Mosaic Glass`);
            print(`Terminal: MosaicTerm`);
            print(`CPU: ${navigator.hardwareConcurrency || '?'} cores`);
            print(`Memory: ${navigator.deviceMemory || '?'} GB`);
            break;
          }
          case 'history': history.forEach((h, i) => print(`  ${String(i + 1).padStart(3)}  ${h}`)); break;
          case 'exit': win.close(); break;
          case 'shutdown': Shell.shutdown(); break;
          case 'restart': Shell.restart(); break;
          default:
            print(`"${cmdRaw}" no se reconoce como un comando interno o externo.`, 'term-err');
            print('Escriba HELP para ver los comandos disponibles.', 'term-dim');
        }
      } catch (e) {
        print(e.message || String(e), 'term-err');
      }
    }

    print('MosaicOS [Versión 0.50.2026]', 'term-dim');
    print('(c) 2026 MosaicOS Project. Todos los derechos reservados.', 'term-dim');
    print('');
    print('Escriba HELP para ver los comandos disponibles.', 'term-dim');
    print('');
    promptEl();
    win.element.addEventListener('click', () => { const i = $('.term-inline input', term); if (i) i.focus(); });
  }
};

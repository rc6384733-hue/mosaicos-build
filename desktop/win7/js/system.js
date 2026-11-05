/* ==========================================================================
   MosaicOS 0.50 "Aero" — system tools, browser, notes and viewer
   Control Panel · Task Manager · Mosaic Web · Sticky Notes · Image Viewer · Welcome
   ========================================================================== */
'use strict';

/* ------------------------------------------------------------------ */
/* Control Panel                                                      */
/* ------------------------------------------------------------------ */
Apps.control = {
  title: 'Panel de control', icon: 'control', width: 880, height: 600, minW: 620, minH: 420,
  mount(body, win, args = {}) {
    body.classList.add('cpanel');
    body.innerHTML = `
      <div class="toolbar">
        <button class="tool-btn" id="cp-home">← Inicio</button>
        <div class="tool-sep"></div>
        <span style="font-size:11px;color:#557082" id="cp-crumb">Panel de control</span>
        <div class="search-bar" style="margin-left:auto"><svg class="ico"><use href="#i-search"></use></svg><input id="cp-search" placeholder="Buscar en el Panel de control"></div>
      </div>
      <div class="cp-body">
        <div class="cp-nav" id="cp-nav">
          <div class="side-item" data-p="home">${iconSvg('control')}Inicio</div>
          <div class="side-title">Sistema y seguridad</div>
          <div class="side-item" data-p="system">${iconSvg('computer')}Sistema</div>
          <div class="side-item" data-p="network">${iconSvg('network')}Red</div>
          <div class="side-item" data-p="recovery">${iconSvg('shield')}Recuperación</div>
          <div class="side-title">Apariencia y personalización</div>
          <div class="side-item" data-p="personalization">${iconSvg('wallpaper')}Personalización</div>
          <div class="side-item" data-p="datetime">${iconSvg('clock')}Fecha y hora</div>
          <div class="side-item" data-p="sound">${iconSvg('volume')}Sonido</div>
          <div class="side-title">Cuentas y programas</div>
          <div class="side-item" data-p="users">${iconSvg('user')}Cuentas de usuario</div>
          <div class="side-item" data-p="programs">${iconSvg('program')}Programas</div>
          <div class="side-title">Información</div>
          <div class="side-item" data-p="about">${iconSvg('info')}Acerca de</div>
        </div>
        <div class="cp-content" id="cp-content"></div>
      </div>`;
    const content = $('#cp-content', body), crumb = $('#cp-crumb', body);
    let page = args.page || 'home';

    const people = ['Mosaic', 'Aero', 'Usuario', 'Invitado'];
    const cap = s => esc(s);

    function row(label, value) { return `<div class="cp-row"><label>${label}</label><span class="value">${value}</span></div>`; }
    function group(title, inner) { return `<div class="cp-group"><h4>${title}</h4>${inner}</div>`; }
    function go(p) { page = p; render(); }

    function home() {
      return `
        <h2 class="cp-title">Ajustar la configuración del equipo</h2>
        <p class="cp-sub">Cambie la apariencia, el sonido, las cuentas y la información del sistema.</p>
        <div class="cp-home-grid">
          ${[
            ['system', 'Sistema', 'Ver información del equipo y del sistema operativo', 'computer'],
            ['personalization', 'Personalización', 'Fondo, colores, transparencia y gadgets', 'wallpaper'],
            ['datetime', 'Fecha y hora', 'Reloj del sistema y zona horaria', 'clock'],
            ['sound', 'Sonido', 'Volumen y sonidos del sistema', 'volume'],
            ['users', 'Cuentas de usuario', 'Nombre e imagen de la cuenta', 'user'],
            ['programs', 'Programas', 'Aplicaciones instaladas en MosaicOS', 'program'],
            ['network', 'Red', 'Estado de la conexión y del puente del sistema', 'network'],
            ['recovery', 'Recuperación', 'Restablecer la configuración y los archivos', 'shield']
          ].map(([p, t, d, i]) => `<div class="cp-home-card" data-p="${p}">${iconSvg(i, 'ico')}<b>${t}</b><span>${d}</span></div>`).join('')}
        </div>
        <div class="cp-tip">Los cambios de apariencia se guardan automáticamente y se aplican al escritorio.</div>`;
    }

    function system() {
      const mem = performance && performance.memory ? (performance.memory.usedJSHeapSize / 1048576).toFixed(1) + ' MB' : 'no disponible';
      return `
        <h2 class="cp-title">Sistema</h2>
        <p class="cp-sub">Información básica del equipo y de MosaicOS.</p>
        ${group('Edición de MosaicOS', row('Edición', 'MosaicOS 0.50 «Aero»') + row('Tipo de sistema', FS.native ? 'Sistema live Debian (64 bits)' : 'Shell web interactivo') + row('Versión', '0.50.2026') + row('Modo de archivos', FS.native ? 'Sistema real: ' + esc(FS.home) : 'Almacenamiento del navegador'))}
        ${group('Equipo', row('Procesador', (navigator.hardwareConcurrency || '?') + ' núcleos lógicos') + row('Memoria del dispositivo', navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'no disponible') + row('Memoria de la página', mem) + row('Resolución', screen.width + ' × ' + screen.height))}
        ${group('Sistema operativo', row('Plataforma', esc(navigator.platform || 'x86')) + row('Idioma', esc(navigator.language)) + row('Navegador', esc((navigator.userAgentData && navigator.userAgentData.brands || []).map(b => b.brand).join(', ') || navigator.userAgent.split(' ').slice(-2).join(' '))) + row('Conexión', navigator.onLine ? 'Conectada' : 'Sin conexión'))}
        <div class="cp-actions"><button class="btn" data-open="taskmgr">Administrador de tareas</button><button class="btn" data-open="explorer">Explorador de archivos</button><button class="btn" data-open="welcome">Bienvenida</button></div>`;
    }

    function personalization() {
      const wall = Settings.data.wallpaper;
      return `
        <h2 class="cp-title">Personalización</h2>
        <p class="cp-sub">Haga que MosaicOS se vea como usted quiera.</p>
        ${group('Fondo de escritorio', `<div class="cp-row" style="display:block"><div class="swatches">${Object.entries(WALLPAPERS).map(([id, w]) =>
          `<div class="swatch ${wall === id ? 'active' : ''}" data-wall="${id}" title="${esc(w.name)}" style="background:${w.css.startsWith('url') ? 'url(assets/wallpaper.svg) center / cover' : w.css}"></div>`).join('')}</div>
          <div style="margin-top:8px;font-size:11px;color:#6a8294">Fondo actual: <b>${esc((WALLPAPERS[wall] || WALLPAPERS.harmony).name)}</b></div></div>`)}
        ${group('Color de Aero', `<div class="cp-row" style="display:block"><div class="swatches">${ACCENTS.map(a => `<div class="swatch ${Settings.data.accent === a.color ? 'active' : ''}" data-accent="${a.color}" title="${a.name}" style="background:linear-gradient(160deg, #fff, ${a.color});width:38px;height:28px"></div>`).join('')}</div></div>`)}
        ${group('Efectos del escritorio',
          `<div class="cp-row"><label>Transparencia de la barra de tareas</label><div class="toggle ${Settings.data.taskbarGlass ? 'on' : ''}" data-toggle="taskbarGlass"></div></div>` +
          `<div class="cp-row"><label>Mostrar gadgets (reloj y tiempo)</label><div class="toggle ${Settings.data.showGadgets ? 'on' : ''}" data-toggle="showGadgets"></div></div>` +
          `<div class="cp-row"><label>Sonidos del sistema</label><div class="toggle ${Settings.data.sounds ? 'on' : ''}" data-toggle="sounds"></div></div>` +
          `<div class="cp-row"><label>Tamaño de los iconos del escritorio</label><select class="text-input" id="cp-iconsize" style="width:120px"><option value="medium" ${Settings.data.iconSize === 'medium' ? 'selected' : ''}>Mediano</option><option value="small" ${Settings.data.iconSize === 'small' ? 'selected' : ''}>Pequeño</option></select></div>`)}
        <div class="cp-actions"><button class="btn" data-open="explorer">Abrir carpeta de fondos</button><button class="btn" id="cp-reset-theme">Restablecer apariencia</button></div>`;
    }

    function datetime() {
      const d = new Date();
      return `
        <h2 class="cp-title">Fecha y hora</h2>
        <p class="cp-sub">Reloj del sistema. La hora se toma del equipo anfitrión.</p>
        ${group('Fecha y hora actuales', `<div class="cp-clock">${fmtTime(d, true)}</div><div class="cp-date">${d.toLocaleDateString('es-ES', { weekday: 'long', day: 'numeric', month: 'long', year: 'numeric' })}</div>`)}
        ${group('Zona horaria', row('Zona', esc(Intl.DateTimeFormat().resolvedOptions().timeZone || 'Local')) + row('Desfase UTC', 'UTC' + (-d.getTimezoneOffset() / 60 >= 0 ? '+' : '') + (-d.getTimezoneOffset() / 60)) + row('Formato', '24 horas'))}
        ${group('Sincronización', row('Servidor de hora', 'Sistema anfitrión') + row('Estado', '<span style="color:#2e7d32">Sincronizado</span>'))}`;
    }

    function sound() {
      return `
        <h2 class="cp-title">Sonido</h2>
        <p class="cp-sub">Volumen general y sonidos de la interfaz.</p>
        ${group('Volumen', `<div class="cp-row"><label>Volumen principal</label><input type="range" id="cp-vol" min="0" max="100" value="${Settings.data.volume == null ? 80 : Settings.data.volume}" style="width:220px"></div><div class="cp-row"><label>Silenciar</label><div class="toggle ${Settings.data.muted ? 'on' : ''}" data-toggle="muted"></div></div>`)}
        ${group('Sonidos del sistema', `<div class="cp-row"><label>Reproducir sonidos de Aero</label><div class="toggle ${Settings.data.sounds ? 'on' : ''}" data-toggle="sounds"></div></div><div class="cp-row"><label>Probar sonido</label><button class="btn" id="cp-test-sound">Reproducir</button></div>`)}
        <div class="cp-tip">El volumen afecta al reproductor multimedia y a los sonidos del sistema.</div>`;
    }

    function users() {
      return `
        <h2 class="cp-title">Cuentas de usuario</h2>
        <p class="cp-sub">Personalice la cuenta con la que ha iniciado sesión.</p>
        ${group('Cuenta actual', `<div class="cp-user"><div class="user-pic" style="width:64px;height:64px;font-size:30px">${esc((Settings.data.userName || 'M')[0].toUpperCase())}</div><div><b>${esc(Settings.data.userName)}</b><div style="color:#6a8294;font-size:11px">Administrador · Cuenta local</div></div></div>`)}
        ${group('Cambiar nombre', `<div class="cp-row"><label>Nombre para mostrar</label><input class="text-input" id="cp-username" value="${esc(Settings.data.userName)}" style="width:200px"><button class="btn" id="cp-apply-user">Aplicar</button></div>`)}
        ${group('Foto de la cuenta', `<div class="cp-row" style="display:block"><div class="avatar-choices">${['#2f7fbc', '#2e8b57', '#b03a3a', '#6d4ca8', '#c27c12', '#3b4a55'].map(c => `<button class="avatar-choice ${Settings.data.avatarColor === c ? 'active' : ''}" data-avatar="${c}" style="background:linear-gradient(160deg,#fff,${c})"></button>`).join('')}</div></div>`)}
        ${group('Otras cuentas', row('Invitado', 'Cuenta deshabilitada'))}`;
    }

    function programs() {
      const list = Object.entries(Apps).filter(([, a]) => a.title).map(([id, a]) => ({ id, ...a }));
      return `
        <h2 class="cp-title">Programas</h2>
        <p class="cp-sub">Aplicaciones incluidas en MosaicOS 0.50.</p>
        <div class="cp-programs">${list.map(a => `<div class="cp-program">${iconSvg(a.icon, 'ico')}<div><b>${esc(a.title)}</b><small>${esc(a.id)}</small></div><button class="btn" data-open="${a.id}">Abrir</button></div>`).join('')}</div>
        ${group('Mosaic Store', row('Catálogo de aplicaciones', 'Disponible en la edición Debian') + row('Instalación', 'Con confirmación y sin recomendaciones'))}`;
    }

    function network() {
      const conn = navigator.connection || {};
      return `
        <h2 class="cp-title">Red</h2>
        <p class="cp-sub">Estado de la conexión del sistema.</p>
        ${group('Estado de la red', row('Conexión', navigator.onLine ? '<span style="color:#2e7d32">Conectada</span>' : '<span style="color:#b03a3a">Sin conexión</span>') + row('Tipo estimado', esc(conn.effectiveType || 'desconocido')) + row('Velocidad estimada', conn.downlink ? conn.downlink + ' Mbps' : 'no disponible') + row('Puente nativo', FS.native ? '<span style="color:#2e7d32">Activo</span>' : 'Modo demostración'))}
        ${group('Puente de MosaicOS', row('Servicio', 'mosaic-shell (Python)') + row('Carpeta personal', esc(FS.home)) + row('Acciones nativas', FS.native ? 'Lanzar aplicaciones y gestionar archivos reales' : 'No disponible en el navegador'))}
        <div class="cp-actions"><button class="btn" id="cp-test-net">Comprobar conexión</button><button class="btn" data-open="browser">Abrir Mosaic Web</button></div>`;
    }

    function recovery() {
      return `
        <h2 class="cp-title">Recuperación</h2>
        <p class="cp-sub">Restaure la configuración de fábrica de MosaicOS.</p>
        ${group('Apariencia', row('Fondo, colores y efectos', 'Se pueden restablecer sin borrar archivos') + `<div class="cp-row"><label></label><button class="btn" id="cp-reset-theme">Restablecer apariencia</button></div>`)}
        ${group('Archivos', row('Sistema de archivos', FS.native ? 'Los archivos reales no se modifican desde aquí' : 'Los archivos de demostración se guardan en el navegador') + `<div class="cp-row"><label></label><button class="btn" id="cp-reset-files" ${FS.native ? 'disabled' : ''}>Restablecer archivos de demostración</button></div>`)}
        ${group('Todo el sistema', row('Configuración y archivos', 'Vuelve al estado inicial de MosaicOS') + `<div class="cp-row"><label></label><button class="btn danger" id="cp-reset-all">Restablecer todo</button></div>`)}
        <div class="cp-tip">En la edición live, sus archivos personales reales nunca se borran desde el Panel de control.</div>`;
    }

    function about() {
      return `
        <h2 class="cp-title">Acerca de MosaicOS</h2>
        <p class="cp-sub">MosaicOS 0.50 «Aero» — 2026</p>
        ${group('MosaicOS', row('Versión', '0.50') + row('Nombre clave', 'Aero') + row('Interfaz', 'Escritorio clásico inspirado en Windows 7') + row('Licencia', 'Software libre — consulte LICENSE'))}
        ${group('Créditos', '<div style="padding:10px 11px;line-height:1.6;color:#4e6b7e">Interfaz, aplicaciones y sistema de ventanas desarrollados para el proyecto MosaicOS. Iconos vectoriales propios. Sons sintetizados con WebAudio. Sin dependencias externas.</div>')}
        <div class="cp-actions"><button class="btn" data-open="welcome">Abrir Bienvenida</button><button class="btn" data-open="browser">Ayuda y soporte</button></div>`;
    }

    const pages = { home, system, personalization, datetime, sound, users, programs, network, recovery, about };
    function render() {
      content.innerHTML = pages[page] ? pages[page]() : pages.home();
      crumb.textContent = 'Panel de control' + (page === 'home' ? '' : ' › ' + ({ system: 'Sistema', personalization: 'Personalización', datetime: 'Fecha y hora', sound: 'Sonido', users: 'Cuentas de usuario', programs: 'Programas', network: 'Red', recovery: 'Recuperación', about: 'Acerca de' }[page] || ''));
      win.setTitle('Panel de control — ' + (page === 'home' ? 'Inicio' : ({ system: 'Sistema', personalization: 'Personalización', datetime: 'Fecha y hora', sound: 'Sonido', users: 'Cuentas de usuario', programs: 'Programas', network: 'Red', recovery: 'Recuperación', about: 'Acerca de' }[page] || '')));
      $$('.cp-nav .side-item', body).forEach(el => el.classList.toggle('active', el.dataset.p === page));
      $$('[data-open]', content).forEach(b => b.onclick = () => Shell.openApp(b.dataset.open, {}));
      $$('.cp-home-card', content).forEach(c => c.onclick = () => go(c.dataset.p));
      $$('.swatch[data-wall]', content).forEach(s => s.onclick = () => { Settings.save({ wallpaper: s.dataset.wall }); Shell.applySettings(); render(); });
      $$('.swatch[data-accent]', content).forEach(s => s.onclick = () => { Settings.save({ accent: s.dataset.accent }); Shell.applySettings(); render(); });
      $$('.toggle[data-toggle]', content).forEach(t => t.onclick = () => {
        const k = t.dataset.toggle;
        const v = !Settings.data[k];
        Settings.save({ [k]: v }); Shell.applySettings(); render();
      });
      const iconSel = $('#cp-iconsize', content);
      if (iconSel) iconSel.onchange = () => { Settings.save({ iconSize: iconSel.value }); Shell.applySettings(); };
      const vol = $('#cp-vol', content);
      if (vol) vol.oninput = () => { Settings.save({ volume: +vol.value, muted: false }); if (window.MediaPlayer) window.MediaPlayer.setVolume(+vol.value / 100); };
      const testSound = $('#cp-test-sound', content);
      if (testSound) testSound.onclick = () => AudioKit.startup();
      const userBtn = $('#cp-apply-user', content);
      if (userBtn) userBtn.onclick = () => {
        const v = $('#cp-username', content).value.trim() || 'Mosaic';
        Settings.save({ userName: v }); Shell.applySettings(); toast('Cuentas de usuario', 'Nombre actualizado a ' + v); render();
      };
      $$('[data-avatar]', content).forEach(b => b.onclick = () => { Settings.save({ avatarColor: b.dataset.avatar }); Shell.applySettings(); render(); });
      const t1 = $('#cp-reset-theme', content);
      if (t1) t1.onclick = async () => {
        if (!(await Dialog.confirm('Restablecer apariencia', '¿Restaurar el fondo, los colores y los efectos originales?'))) return;
        Settings.save({ wallpaper: 'harmony', accent: '#2f7fbc', taskbarGlass: true, showGadgets: true, sounds: true, iconSize: 'medium' });
        Shell.applySettings(); render(); toast('Personalización', 'Apariencia restablecida.');
      };
      const t2 = $('#cp-reset-files', content);
      if (t2) t2.onclick = async () => {
        if (!(await Dialog.confirm('Restablecer archivos', '¿Restaurar los archivos de demostración? Se perderán los cambios en las carpetas de ejemplo.', true))) return;
        try { localStorage.removeItem('mosaic.fs.v1'); FS._loadVirtual(); toast('Recuperación', 'Archivos de demostración restablecidos.'); } catch (e) { Dialog.error('Error', e.message); }
        Shell.refreshDesktop(); render();
      };
      const t3 = $('#cp-reset-all', content);
      if (t3) t3.onclick = async () => {
        if (!(await Dialog.confirm('Restablecer todo', 'Se restablecerá toda la configuración y los archivos de demostración. ¿Continuar?', true))) return;
        try { localStorage.removeItem('mosaic.fs.v1'); localStorage.removeItem('mosaic.settings.v1'); localStorage.removeItem('mosaic.notes'); } catch (e) {}
        location.reload();
      };
      const net = $('#cp-test-net', content);
      if (net) net.onclick = async () => {
        if (FS.native) {
          try { const r = await fetch('api/system'); const j = await r.json(); toast('Red', 'Puente activo: ' + (j.host || 'mosaic-shell')); }
          catch (e) { toast('Red', 'No se pudo contactar con el puente.'); }
        } else toast('Red', navigator.onLine ? 'Conexión a Internet disponible.' : 'Sin conexión a Internet.');
      };
    }
    $$('.cp-nav .side-item', body).forEach(el => el.onclick = () => go(el.dataset.p));
    $('#cp-home', body).onclick = () => go('home');
    $('#cp-search', body).oninput = e => {
      const q = e.target.value.toLowerCase().trim();
      if (!q) return;
      const map = { sistema: 'system', fondo: 'personalization', color: 'personalization', hora: 'datetime', sonido: 'sound', usuario: 'users', cuenta: 'users', programa: 'programs', red: 'network', recuperar: 'recovery', acerca: 'about' };
      const hit = Object.keys(map).find(k => k.includes(q) || q.includes(k));
      if (hit) go(map[hit]);
    };
    render();
  }
};

/* ------------------------------------------------------------------ */
/* Task Manager                                                       */
/* ------------------------------------------------------------------ */
Apps.taskmgr = {
  title: 'Administrador de tareas de MosaicOS', icon: 'taskmgr', width: 680, height: 480, minW: 520, minH: 340,
  mount(body, win) {
    body.classList.add('taskmgr');
    body.innerHTML = `
      <div class="menubar"><button data-m="archivo">Archivo</button><button data-m="opciones">Opciones</button><button data-m="ayuda">Ayuda</button></div>
      <div class="tm-tabs"><button class="tm-tab active" data-t="processes">Procesos</button><button class="tm-tab" data-t="performance">Rendimiento</button><button class="tm-tab" data-t="network">Red</button></div>
      <div class="tm-body"><div class="tm-content" id="tm-content"></div><div class="tm-actions"><button class="btn" id="tm-end">Finalizar tarea</button></div></div>`;
    const content = $('#tm-content', body);
    let tab = 'processes';
    let selId = null;
    const started = Date.now();
    let cpu = 7, mem = 38;

    function processes() {
      const wins = Array.from(WM.wins.values());
      const rows = [{ id: 'shell', title: 'Mosaic Shell (explorer.exe)', icon: 'computer', appId: 'shell', mem: 42 }, ...wins.map(w => ({ id: w.id, title: w.title, icon: w.icon, appId: w.appId, mem: 18 + (w.body ? w.body.innerHTML.length / 1024 : 0) }))];
      return `<table class="tm-table"><thead><tr><th style="width:38%">Nombre de la imagen</th><th>Estado</th><th>Usuario</th><th style="text-align:right">Memoria</th></tr></thead><tbody>
        ${rows.map(r => `<tr data-id="${r.id}" class="${selId === r.id ? 'selected' : ''}"><td>${iconSvg(r.icon || 'program')} ${esc(r.title)}</td><td>${r.id === 'shell' ? 'En ejecución' : 'En ejecución'}</td><td>${esc(Settings.data.userName)}</td><td style="text-align:right">${Math.round(r.mem)} MB</td></tr>`).join('')}
      </tbody></table>`;
    }
    function performance() {
      cpu = clamp(cpu + (Math.random() * 8 - 4), 2, 92);
      mem = clamp(mem + (Math.random() * 2 - 1), 25, 85);
      const used = performance && performance.memory ? (performance.memory.usedJSHeapSize / 1048576) : mem * 0.4;
      return `
        <div class="tm-graph">
          <div class="tm-meter"><b>Uso de CPU</b><div class="tm-bar"><span style="width:${cpu.toFixed(0)}%"></span></div><div style="font-size:22px;font-weight:300;color:#2b5470;margin-top:5px">${cpu.toFixed(0)}%</div></div>
          <div class="tm-meter"><b>Memoria física</b><div class="tm-bar mem"><span style="width:${mem.toFixed(0)}%"></span></div><div style="font-size:22px;font-weight:300;color:#2b5470;margin-top:5px">${used.toFixed(1)} MB</div></div>
        </div>
        ${`<table class="tm-table"><tbody>
          <tr><th>Tiempo activo</th><td>${Math.floor((Date.now() - started) / 60000)} min ${Math.floor(((Date.now() - started) % 60000) / 1000)} s</td></tr>
          <tr><th>Procesos</th><td>${WM.wins.size + 1}</td></tr>
          <tr><th>Ventanas abiertas</th><td>${WM.wins.size}</td></tr>
          <tr><th>Núcleos lógicos</th><td>${navigator.hardwareConcurrency || 'no disponible'}</td></tr>
          <tr><th>Memoria del dispositivo</th><td>${navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'no disponible'}</td></tr>
        </tbody></table>`}`;
    }
    function network() {
      const conn = navigator.connection || {};
      return `<table class="tm-table"><tbody>
        <tr><th>Estado</th><td>${navigator.onLine ? 'Conectado' : 'Desconectado'}</td></tr>
        <tr><th>Tipo de conexión</th><td>${esc(conn.effectiveType || 'desconocido')}</td></tr>
        <tr><th>Velocidad de bajada</th><td>${conn.downlink ? conn.downlink + ' Mbps' : 'no disponible'}</td></tr>
        <tr><th>Latencia estimada</th><td>${conn.rtt ? conn.rtt + ' ms' : 'no disponible'}</td></tr>
        <tr><th>Puente nativo</th><td>${FS.native ? 'Activo — ' + esc(FS.home) : 'Modo demostración'}</td></tr>
        <tr><th>Agente</th><td style="word-break:break-all">${esc(navigator.userAgent)}</td></tr>
      </tbody></table>`;
    }
    function render() {
      if (tab === 'processes') content.innerHTML = processes();
      else if (tab === 'performance') content.innerHTML = performance();
      else content.innerHTML = network();
      $$('.tm-tab', body).forEach(b => b.classList.toggle('active', b.dataset.t === tab));
      $$('#tm-content tr[data-id]', body).forEach(tr => tr.onclick = () => { selId = tr.dataset.id; render(); });
    }
    $$('.tm-tab', body).forEach(b => b.onclick = () => { tab = b.dataset.t; selId = null; render(); });
    $('#tm-end', body).onclick = () => {
      if (!selId || selId === 'shell') { Dialog.alert('Administrador de tareas', selId === 'shell' ? 'No se puede finalizar el proceso del shell.' : 'Seleccione un proceso para finalizar.'); return; }
      const w = WM.wins.get(selId);
      if (w) { w.close(); selId = null; render(); }
    };
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      const label = b.dataset.m;
      Shell.menuAt(r.left, r.bottom, label === 'archivo' ? [
        { label: 'Nueva tarea…', icon: 'run', action: () => Shell.runDialog() },
        { sep: true }, { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ] : [
        { label: 'Actualizar ahora', action: () => render() },
        { label: 'Siempre visible', action: () => {} },
        { sep: true }, { label: 'Acerca del Administrador de tareas', icon: 'info', action: () => Dialog.alert('Administrador de tareas', 'MosaicOS 0.50\nSupervisa las ventanas abiertas y el uso estimado de recursos.') }
      ]);
    });
    render();
    const timer = setInterval(() => { if (tab === 'performance') render(); }, 1400);
    win.onClosed = () => clearInterval(timer);
    win.setTitle('Administrador de tareas de MosaicOS');
  }
};

/* ------------------------------------------------------------------ */
/* Mosaic Web (light browser)                                         */
/* ------------------------------------------------------------------ */
Apps.browser = {
  title: 'Mosaic Web', icon: 'browser', width: 880, height: 600, minW: 560, minH: 380,
  mount(body, win, args = {}) {
    body.classList.add('browser');
    body.innerHTML = `
      <div class="toolbar">
        <button class="tool-btn" id="bw-back" title="Atrás"><svg class="ico" style="color:#2f6f9f"><use href="#i-back"></use></svg></button>
        <button class="tool-btn" id="bw-fwd" title="Adelante"><svg class="ico" style="color:#2f6f9f"><use href="#i-forward"></use></svg></button>
        <button class="tool-btn" id="bw-home" title="Página principal"><svg class="ico" style="color:#2f6f9f"><use href="#i-up"></use></svg></button>
        <div class="address-bar"><svg class="ico"><use href="#i-browser"></use></svg><input id="bw-url" value="mosaic://home" aria-label="Dirección"></div>
        <button class="tool-btn" id="bw-go">Ir</button>
        <button class="tool-btn" id="bw-menu">≡</button>
      </div>
      <div class="bw-page" id="bw-page"></div>
      <div class="statusbar"><span id="bw-status">Listo</span><span class="grow"></span><span>Mosaic Web · modo seguro</span></div>`;
    const pageEl = $('#bw-page', body), input = $('#bw-url', body), status = $('#bw-status', body);
    let stack = [], index = -1;

    const bookmarks = [
      ['Inicio', 'mosaic://home', 'Página de inicio de Mosaic Web'],
      ['Ayuda', 'mosaic://help', 'Guía de MosaicOS 0.50'],
      ['Aplicaciones', 'mosaic://apps', 'Catálogo de aplicaciones'],
      ['Notas de versión', 'mosaic://release', 'Novedades de MosaicOS 0.50'],
      ['Sistema', 'mosaic://system', 'Información del sistema en vivo'],
      ['Consejos', 'mosaic://tips', 'Atajos y trucos del escritorio']
    ];

    function home() {
      return `<div class="bw-hero"><h2>Mosaic Web</h2><p>Su ventana al sistema. Busque aplicaciones, lea la ayuda o consulte el estado del equipo.</p>
        <div class="bw-search"><input id="bw-search-input" placeholder="Buscar en MosaicOS o escribir una dirección"><button class="btn primary" id="bw-search-go">Buscar</button></div></div>
        <div class="bw-content"><h3 style="font-weight:400;color:#24597c;margin:0 0 4px">Marcadores</h3>
        <div class="bw-cards">${bookmarks.map(([t, u, d]) => `<div class="bw-card" data-url="${u}"><b>${t}</b><span>${d}</span></div>`).join('')}</div>
        <h3 style="font-weight:400;color:#24597c;margin:20px 0 4px">Accesos rápidos</h3>
        <div class="bw-cards">
          <div class="bw-card" data-app="explorer"><b>Archivos</b><span>Examinar las carpetas del sistema</span></div>
          <div class="bw-card" data-app="control"><b>Configuración</b><span>Apariencia, sonido y cuentas</span></div>
          <div class="bw-card" data-app="terminal"><b>Terminal</b><span>Comandos y herramientas</span></div>
          <div class="bw-card" data-app="games"><b>Juegos</b><span>Minesweeper, Solitaire y Snake</span></div>
        </div></div>`;
    }
    function help() {
      return `<div class="bw-content bw-doc">
        <h2>Ayuda de MosaicOS 0.50</h2>
        <p>MosaicOS «Aero» es un escritorio interactivo inspirado en la interfaz clásica de Windows 7. Todo funciona sin conexión y sin dependencias externas.</p>
        <h3>Primeros pasos</h3>
        <ul>
          <li>Haga <b>doble clic</b> en un icono del escritorio para abrir un programa.</li>
          <li>Use la <b>tecla Windows</b> o el botón <b>Inicio</b> para abrir el menú.</li>
          <li>Haga clic con el <b>botón derecho</b> en el escritorio para cambiar el fondo o crear archivos.</li>
          <li>Arrastre una ventana a los <b>bordes</b> para organizarla (Aero Snap).</li>
          <li>Use <b>Alt+Tab</b> para cambiar entre ventanas y <b>Win+D</b> para mostrar el escritorio.</li>
        </ul>
        <h3>Aplicaciones</h3>
        <p>Explorador, Notepad, Paint, Calculadora, Terminal, Panel de control, Reproductor multimedia, Mosaic Web, Minesweeper, Solitaire, Snake, Notas rápidas, Visor de imágenes y Administrador de tareas.</p>
        <h3>Archivos</h3>
        <p>En el navegador, los archivos se guardan en el almacenamiento local. En la edición live de Debian, el puente <code>mosaic-shell</code> permite leer y escribir en su carpeta personal real.</p>
        <h3>Atajos de teclado</h3>
        <ul>
          <li><code>Win</code> / <code>Ctrl+Esc</code> — Menú Inicio</li>
          <li><code>Win+E</code> — Equipo · <code>Win+R</code> — Ejecutar · <code>Win+L</code> — Bloquear</li>
          <li><code>Win+D</code> — Mostrar escritorio · <code>Alt+Tab</code> — Cambiar de ventana</li>
          <li><code>Alt+F4</code> — Cerrar la ventana activa</li>
        </ul>
        <h3>Más información</h3>
        <p>Consulte <a href="#" data-url="mosaic://release">las notas de versión</a> o <a href="#" data-url="mosaic://system">el estado del sistema</a>.</p>
      </div>`;
    }
    function release() {
      return `<div class="bw-content bw-doc">
        <h2>MosaicOS 0.50 «Aero»</h2>
        <p><b>Nuevo en esta versión</b></p>
        <ul>
          <li>Escritorio web completo con ventanas, barra de tareas, menú Inicio y Aero Snap.</li>
          <li>Interfaz inspirada en Windows 7: Aero Glass, orbe de inicio, gadgets y pantalla de bloqueo.</li>
          <li>16 aplicaciones funcionales, incluidos juegos y herramientas del sistema.</li>
          <li>Sistema de archivos virtual persistente y puente opcional con el sistema real.</li>
          <li>Personalización de fondos, colores, sonidos, gadgets y cuentas.</li>
          <li>Integración con la edición live Debian: el shell se inicia automáticamente.</li>
        </ul>
        <h3>Edición Daily</h3>
        <p>La edición Debian conserva el escritorio LXQt/Openbox como respaldo y añade el shell Mosaic como experiencia principal. Las aplicaciones nativas se pueden lanzar desde el menú Inicio.</p>
        <h3>Estado</h3>
        <p>Las pruebas de la ISO y del hardware real deben realizarse en un equipo compatible. Este shell se ha validado en navegadores Chromium/Firefox modernos.</p>
      </div>`;
    }
    function system() {
      return `<div class="bw-content bw-doc">
        <h2>Información del sistema</h2>
        <table class="tm-table" style="max-width:620px">
          <tr><th>Edición</th><td>MosaicOS 0.50 «Aero»</td></tr>
          <tr><th>Interfaz</th><td>Mosaic Aero Shell</td></tr>
          <tr><th>Modo de archivos</th><td>${FS.native ? 'Sistema real: ' + esc(FS.home) : 'Almacenamiento del navegador'}</td></tr>
          <tr><th>Resolución</th><td>${screen.width} × ${screen.height}</td></tr>
          <tr><th>Núcleos lógicos</th><td>${navigator.hardwareConcurrency || 'no disponible'}</td></tr>
          <tr><th>Memoria</th><td>${navigator.deviceMemory ? navigator.deviceMemory + ' GB' : 'no disponible'}</td></tr>
          <tr><th>Conexión</th><td>${navigator.onLine ? 'Conectada' : 'Sin conexión'}</td></tr>
          <tr><th>Ventanas abiertas</th><td>${WM.wins.size}</td></tr>
        </table>
        <p style="margin-top:16px"><button class="btn" data-open="taskmgr">Abrir Administrador de tareas</button> <button class="btn" data-open="control">Abrir Panel de control</button></p>
      </div>`;
    }
    function tips() {
      return `<div class="bw-content bw-doc">
        <h2>Consejos y atajos</h2>
        <h3>Ventanas</h3>
        <ul><li>Arrastre el título para mover. Arrastre los bordes para redimensionar.</li>
        <li>Doble clic en el título para maximizar o restaurar.</li>
        <li>Arrastre al borde superior para maximizar; a los lados para dividir la pantalla.</li>
        <li>Clic central en un botón de la barra de tareas para cerrar la ventana.</li></ul>
        <h3>Escritorio</h3>
        <ul><li>Botón derecho → Personalizar para cambiar el fondo.</li>
        <li>Botón derecho → Nueva carpeta para crear carpetas en el escritorio.</li>
        <li>El reloj de la barra de tareas abre el calendario; el altavoz abre el volumen.</li></ul>
        <h3>Productividad</h3>
        <ul><li>En Notepad: Ctrl+S guardar, Ctrl+O abrir, F5 insertar fecha y hora.</li>
        <li>En Paint: Ctrl+Z deshacer, Ctrl+S guardar como PNG.</li>
        <li>En Terminal: escriba <code>help</code> para ver todos los comandos.</li></ul>
      </div>`;
    }
    function appsPage() {
      const list = Object.entries(Apps).filter(([, a]) => a.title).map(([id, a]) => ({ id, ...a }));
      return `<div class="bw-content"><h2 style="font-weight:300;color:#1b5277">Aplicaciones de MosaicOS</h2>
        <p style="color:#6a8294;font-size:12px">Haga clic en una aplicación para abrirla.</p>
        <div class="bw-cards">${list.map(a => `<div class="bw-card" data-app="${a.id}">${iconSvg(a.icon, 'ico')}<b>${esc(a.title)}</b><span>${esc(a.id)}</span></div>`).join('')}</div></div>`;
    }
    function external(url) {
      return `<div class="bw-content bw-doc"><h2>Enlace externo</h2><p>Mosaic Web es un navegador ligero y seguro integrado en el escritorio. Los enlaces externos se abren en el navegador del sistema para proteger la sesión.</p>
        <p style="word-break:break-all;color:#24597c"><b>${esc(url)}</b></p>
        <p><button class="btn primary" id="bw-external">Abrir en el navegador del sistema</button></p>
        <p style="font-size:11px;color:#7b8f9e">En el modo demostración se abrirá una pestaña nueva del navegador actual.</p></div>`;
    }
    function searchPage(q) {
      const term = String(q == null ? '' : q).trim().toLowerCase();
      const apps = Object.entries(Apps).filter(([id, a]) => {
        if (!a || !a.title) return false;
        if (!term) return true;
        return String(a.title).toLowerCase().includes(term) || String(a.id || id).toLowerCase().includes(term);
      });
      const label = term || 'todas';
      return `<div class="bw-content bw-doc"><h2>Resultados para «${esc(label)}»</h2>
        ${apps.length ? `<p>Se encontraron ${apps.length} aplicaciones:</p><div class="bw-cards">${apps.map(([id, a]) => `<div class="bw-card" data-app="${id}">${iconSvg(a.icon, 'ico')}<b>${esc(a.title)}</b><span>${esc(id)}</span></div>`).join('')}</div>` : '<p>No se encontraron aplicaciones. Pruebe con «ayuda», «sistema» o «juegos».</p>'}
        <p style="margin-top:16px"><button class="btn" data-url="mosaic://help">Abrir la ayuda</button> <button class="btn" data-url="mosaic://apps">Ver todas las aplicaciones</button></p></div>`;
    }

    function renderPage(url, push = true) {
      const u = String(url || '').trim();
      let html, label = u;
      if (u === 'mosaic://home' || u === 'home') { html = home(); label = 'mosaic://home'; }
      else if (u === 'mosaic://help') { html = help(); }
      else if (u === 'mosaic://release') { html = release(); }
      else if (u === 'mosaic://system') { html = system(); }
      else if (u === 'mosaic://tips') { html = tips(); }
      else if (u === 'mosaic://apps') { html = appsPage(); }
      else if (u.startsWith('mosaic://search?q=')) { const q = decodeURIComponent(u.split('=').slice(1).join('=')); html = searchPage(q); label = u; }
      else if (/^https?:\/\//i.test(u)) { html = external(u); }
      else if (u && !u.includes(' ')) { html = external('https://' + u.replace(/^\/\//, '')); label = 'https://' + u.replace(/^\/\//, ''); }
      else { const q = u || 'mosaic'; html = searchPage(q.toLowerCase()); label = 'mosaic://search?q=' + encodeURIComponent(q); }
      pageEl.innerHTML = html;
      input.value = label;
      win.setTitle(`${label.replace('mosaic://', '')} — Mosaic Web`);
      status.textContent = 'Listo';
      if (push) { stack = stack.slice(0, index + 1); stack.push(label); index = stack.length - 1; }
      $('#bw-back', body).disabled = index <= 0;
      $('#bw-fwd', body).disabled = index >= stack.length - 1;
      bindPage();
    }
    function bindPage() {
      $$('[data-url]', pageEl).forEach(el => el.onclick = e => { e.preventDefault(); navigate(el.dataset.url); });
      $$('[data-app]', pageEl).forEach(el => el.onclick = () => Shell.openApp(el.dataset.app, {}));
      const s = $('#bw-search-input', pageEl);
      if (s) {
        const go = () => { const q = s.value.trim(); if (q) navigate(/^https?:|^mosaic:/.test(q) ? q : 'mosaic://search?q=' + encodeURIComponent(q)); };
        s.addEventListener('keydown', e => { if (e.key === 'Enter') go(); });
        $('#bw-search-go', pageEl).onclick = go;
      }
      const ext = $('#bw-external', pageEl);
      if (ext) ext.onclick = () => { const u = input.value; Shell.openExternal(u); toast('Mosaic Web', 'Abriendo ' + u); };
    }
    function navigate(url, push = true) {
      renderPage(url, push);
      if (push) AudioKit.click();
    }
    function goAddress() {
      let v = input.value.trim();
      if (!v) return;
      if (v.startsWith('mosaic://')) navigate(v);
      else if (/^https?:\/\//i.test(v)) navigate(v);
      else if (/\./.test(v) && !v.includes(' ')) navigate('https://' + v);
      else navigate('mosaic://search?q=' + encodeURIComponent(v));
    }
    $('#bw-go', body).onclick = goAddress;
    input.addEventListener('keydown', e => { if (e.key === 'Enter') goAddress(); });
    $('#bw-home', body).onclick = () => navigate('mosaic://home');
    $('#bw-back', body).onclick = () => { if (index > 0) { index--; renderPage(stack[index], false); } };
    $('#bw-fwd', body).onclick = () => { if (index < stack.length - 1) { index++; renderPage(stack[index], false); } };
    $('#bw-menu', body).onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      Shell.menuAt(r.left - 150, r.bottom, [
        { label: 'Página principal', icon: 'browser', action: () => navigate('mosaic://home') },
        { label: 'Ayuda', icon: 'help', action: () => navigate('mosaic://help') },
        { label: 'Aplicaciones', icon: 'program', action: () => navigate('mosaic://apps') },
        { sep: true },
        { label: 'Abrir Panel de control', icon: 'control', action: () => Shell.openApp('control', {}) },
        { label: 'Abrir Terminal', icon: 'terminal', action: () => Shell.openApp('terminal', {}) },
        { sep: true },
        { label: 'Acerca de Mosaic Web', icon: 'info', action: () => Dialog.alert('Mosaic Web', 'Navegador ligero de MosaicOS 0.50.\nPáginas internas: mosaic://home, mosaic://help, mosaic://apps, mosaic://release, mosaic://system, mosaic://tips.') }
      ]);
    };
    const initial = args.page ? 'mosaic://' + args.page : (args.url || 'mosaic://home');
    renderPage(initial);
  }
};

/* ------------------------------------------------------------------ */
/* Sticky Notes                                                       */
/* ------------------------------------------------------------------ */
Apps.sticky = {
  title: 'Nota rápida', icon: 'sticky', width: 300, height: 300, minW: 210, minH: 180,
  mount(body, win, args = {}) {
    body.classList.add('sticky');
    let notes = Store.get('notes', {});
    let id = args.noteId;
    if (!id || !notes[id]) {
      id = uid('note');
      notes[id] = { text: '', created: Date.now() };
      Store.set('notes', notes);
    }
    body.innerHTML = `<div class="sticky-body"><textarea placeholder="Escriba una nota…" spellcheck="false">${esc(notes[id].text || '')}</textarea></div>`;
    const ta = $('textarea', body);
    let timer = null;
    function save() {
      notes[id].text = ta.value;
      Store.set('notes', notes);
      const first = ta.value.split('\n').find(l => l.trim()) || 'Nota rápida';
      win.setTitle(first.slice(0, 32) + (first.length > 32 ? '…' : '') + ' — Notas rápidas');
    }
    ta.addEventListener('input', () => { clearTimeout(timer); timer = setTimeout(save, 350); });
    win.beforeClose = () => { clearTimeout(timer); notes[id].text = ta.value; Store.set('notes', notes); return true; };
    save();
    setTimeout(() => ta.focus(), 80);
  }
};

/* ------------------------------------------------------------------ */
/* Image Viewer                                                       */
/* ------------------------------------------------------------------ */
Apps.imgview = {
  title: 'Visor de imágenes', icon: 'image', width: 720, height: 520, minW: 380, minH: 280,
  mount(body, win, args = {}) {
    body.classList.add('imgview');
    body.innerHTML = `
      <div class="toolbar">
        <button class="tool-btn" id="iv-open">Abrir…</button>
        <div class="tool-sep"></div>
        <button class="tool-btn" id="iv-out">−</button><span id="iv-zoom" style="font-size:11px;min-width:42px;text-align:center">100%</span><button class="tool-btn" id="iv-in">+</button>
        <button class="tool-btn" id="iv-fit">Ajustar</button>
        <div class="tool-sep"></div>
        <button class="tool-btn" id="iv-save">Guardar copia…</button>
      </div>
      <div class="iv-body" id="iv-body"><span style="color:#cfe3ef;font-size:12px">Sin imagen</span></div>
      <div class="statusbar"><span id="iv-status">Listo</span></div>`;
    const host = $('#iv-body', body), status = $('#iv-status', body);
    let path = args.path || null, zoom = 1, dataUrl = null;
    function show(data, name) {
      dataUrl = data;
      host.innerHTML = `<img alt="${esc(name || 'Imagen')}" src="${data}">`;
      const img = $('img', host);
      img.style.transformOrigin = 'center';
      setZoom(1);
      status.textContent = name || 'Imagen';
      win.setTitle((name || 'Imagen') + ' — Visor de imágenes');
    }
    function setZoom(z) {
      zoom = clamp(z, 0.1, 6);
      const img = $('img', host);
      if (img) img.style.width = (img.naturalWidth * zoom) + 'px';
      $('#iv-zoom', body).textContent = Math.round(zoom * 100) + '%';
    }
    async function load(p) {
      try {
        let data = await FS.read(p);
        if (!/^data:/.test(data)) {
          if (/\.svg$/i.test(p)) data = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(data);
          else data = 'data:image/png;base64,' + data;
        }
        path = p;
        show(data, baseName(p));
      } catch (e) { Dialog.error('No se pudo abrir la imagen', e.message); }
    }
    $('#iv-open', body).onclick = async () => {
      const p = await FileDialog.show({ mode: 'open', title: 'Abrir imagen', initial: path ? dirName(path) : joinPath(FS.home, 'Pictures'), filter: ['png', 'jpg', 'jpeg', 'gif', 'bmp', 'svg', 'webp'] });
      if (p) load(p);
    };
    $('#iv-in', body).onclick = () => setZoom(zoom + .2);
    $('#iv-out', body).onclick = () => setZoom(zoom - .2);
    $('#iv-fit', body).onclick = () => {
      const img = $('img', host);
      if (!img) return;
      const z = Math.min(host.clientWidth / img.naturalWidth, host.clientHeight / img.naturalHeight, 1);
      setZoom(z);
    };
    $('#iv-save', body).onclick = async () => {
      if (!dataUrl) return;
      const p = await FileDialog.show({ mode: 'save', title: 'Guardar copia', initial: joinPath(FS.home, 'Pictures'), filename: 'copia.png', filter: ['png'] });
      if (!p) return;
      try { await FS.write(p, dataUrl); toast('Visor de imágenes', 'Copia guardada en ' + p); } catch (e) { Dialog.error('No se pudo guardar', e.message); }
    };
    win.element.addEventListener('keydown', e => {
      if (e.key === '+' || e.key === '=') setZoom(zoom + .2);
      if (e.key === '-') setZoom(zoom - .2);
    });
    if (path) load(path);
  }
};

/* ------------------------------------------------------------------ */
/* Welcome / Getting Started                                          */
/* ------------------------------------------------------------------ */
Apps.welcome = {
  title: 'Bienvenido a MosaicOS', icon: 'help', width: 820, height: 560, minW: 600, minH: 400,
  mount(body, win) {
    body.classList.add('welcome');
    body.innerHTML = `
      <div class="wc-body">
        <div class="wc-side">
          <h3>MosaicOS 0.50</h3>
          <p>Su equipo, con una interfaz familiar.</p>
          <div class="side-item active" data-p="start">${iconSvg('help')}Primeros pasos</div>
          <div class="side-item" data-p="apps">${iconSvg('program')}Aplicaciones</div>
          <div class="side-item" data-p="tips">${iconSvg('star')}Consejos</div>
          <div class="side-item" data-p="system">${iconSvg('computer')}Sistema</div>
          <div class="side-item" data-p="about">${iconSvg('info')}Acerca de</div>
          <div style="margin-top:18px;font-size:10px;color:#5e7f93;line-height:1.5">© 2026 MosaicOS Project<br>Edición «Aero»</div>
        </div>
        <div class="wc-main" id="wc-main"></div>
      </div>`;
    const main = $('#wc-main', body);
    let page = 'start';
    function start() {
      return `<h2>Bienvenido, ${esc(Settings.data.userName)}</h2><p class="lead">MosaicOS 0.50 «Aero» está listo. Explore el escritorio, abra aplicaciones y personalícelo a su gusto.</p>
        <div class="wc-cards">
          <div class="wc-card" data-open="explorer">${iconSvg('computer')}<b>Examinar archivos</b><span>Abra el Explorador y descubra las carpetas.</span></div>
          <div class="wc-card" data-open="browser">${iconSvg('browser')}<b>Mosaic Web</b><span>Lea la ayuda y las notas de versión.</span></div>
          <div class="wc-card" data-open="control">${iconSvg('control')}<b>Personalizar</b><span>Cambie el fondo, los colores y el sonido.</span></div>
          <div class="wc-card" data-open="games">${iconSvg('games')}<b>Jugar</b><span>Minesweeper, Solitaire y Snake.</span></div>
        </div>
        <div class="wc-tip">Consejo: pulse la tecla <kbd>Windows</kbd> para abrir el menú Inicio y <kbd>Alt</kbd>+<kbd>Tab</kbd> para cambiar de ventana.</div>`;
    }
    function apps() {
      const list = Object.entries(Apps).filter(([, a]) => a.title).map(([id, a]) => ({ id, ...a }));
      return `<h2>Aplicaciones incluidas</h2><p class="lead">Dieciséis aplicaciones listas para usar, sin conexión.</p>
        <div class="wc-cards">${list.map(a => `<div class="wc-card" data-open="${a.id}">${iconSvg(a.icon)}<b>${esc(a.title)}</b><span>${esc(a.id)}</span></div>`).join('')}</div>`;
    }
    function tips() {
      return `<h2>Consejos y atajos</h2><p class="lead">Aproveche al máximo el escritorio.</p>
        <div class="wc-cards">
          <div class="wc-card" data-open="browser" data-page="tips">${iconSvg('star')}<b>Todos los atajos</b><span>Consulte la guía completa de teclado y ventanas.</span></div>
          <div class="wc-card" data-open="terminal">${iconSvg('terminal')}<b>Terminal</b><span>Escriba <kbd>help</kbd> para ver los comandos.</span></div>
          <div class="wc-card" data-open="notepad">${iconSvg('notepad')}<b>Notepad</b><span>Guarde notas en Documentos con Ctrl+S.</span></div>
          <div class="wc-card" data-open="paint">${iconSvg('paint')}<b>Paint</b><span>Dibuje y guarde imágenes PNG.</span></div>
        </div>
        <div class="wc-tip">Arrastre una ventana al borde superior, izquierdo o derecho para organizarla con Aero Snap.</div>`;
    }
    function system() {
      return `<h2>Estado del sistema</h2><p class="lead">Información en tiempo real de esta sesión.</p>
        <div class="wc-cards">
          <div class="wc-card">${iconSvg('computer')}<b>${FS.native ? 'Edición live' : 'Modo demostración'}</b><span>${esc(FS.home)}</span></div>
          <div class="wc-card">${iconSvg('network')}<b>${navigator.onLine ? 'Conectado' : 'Sin conexión'}</b><span>Red del equipo</span></div>
          <div class="wc-card">${iconSvg('wallpaper')}<b>${screen.width} × ${screen.height}</b><span>Resolución de pantalla</span></div>
          <div class="wc-card">${iconSvg('taskmgr')}<b>${WM.wins.size} ventanas</b><span>Aplicaciones abiertas</span></div>
        </div>
        <div class="wc-tip">Las ventanas, archivos y preferencias se conservan al recargar. Use el Panel de control para restablecerlos.</div>`;
    }
    function about() {
      return `<h2>Acerca de MosaicOS</h2><p class="lead">MosaicOS 0.50 «Aero» · 2026</p>
        <p style="font-size:12px;line-height:1.6;color:#3c5668;max-width:620px">MosaicOS es un proyecto de sistema operativo que combina una base Debian con una experiencia de escritorio propia. Esta edición «Aero» recupera la claridad visual de Windows 7 con un shell moderno, ligero y sin dependencias externas.</p>
        <div class="wc-cards" style="margin-top:14px">
          <div class="wc-card" data-open="browser" data-page="release">${iconSvg('info')}<b>Notas de versión</b><span>Qué hay nuevo en 0.50</span></div>
          <div class="wc-card" data-open="browser" data-page="help">${iconSvg('help')}<b>Ayuda</b><span>Guía de uso completa</span></div>
        </div>
        <div class="wc-tip">Software libre. Consulte el archivo LICENSE incluido con el proyecto.</div>`;
    }
    function render() {
      main.innerHTML = ({ start, apps, tips, system, about })[page]();
      $$('.wc-side .side-item', body).forEach(el => el.classList.toggle('active', el.dataset.p === page));
      $$('[data-open]', main).forEach(el => el.onclick = () => Shell.openApp(el.dataset.open, el.dataset.page ? { page: el.dataset.page } : {}));
      win.setTitle('Bienvenido a MosaicOS — ' + ({ start: 'Primeros pasos', apps: 'Aplicaciones', tips: 'Consejos', system: 'Sistema', about: 'Acerca de' })[page]);
    }
    $$('.wc-side .side-item', body).forEach(el => el.onclick = () => { page = el.dataset.p; render(); });
    render();
  }
};

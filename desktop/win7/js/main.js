/* ==========================================================================
   MosaicOS 0.50 "Aero" — boot, login and session start
   ========================================================================== */
'use strict';

(async function startMosaicOS() {
  const params = new URLSearchParams(location.search);
  const fast = params.has('fast') || params.has('noboot');
  const autoLogin = params.has('autologin') || params.get('login') === '0';

  const boot = $('#boot-screen');
  const login = $('#login-screen');

  function syncIdentity() {
    const name = Settings.data.userName || 'Mosaic';
    const initial = name.trim().charAt(0).toUpperCase() || 'M';
    const lu = $('#login-user');
    if (lu) lu.textContent = name;
    $$('.login-card .user-pic, .lock-user .user-pic, .sm-avatar').forEach(el => {
      el.textContent = initial;
      if (Settings.data.avatarColor) el.style.background = `linear-gradient(160deg, #ffffff, ${Settings.data.avatarColor})`;
    });
  }

  function showLogin() {
    if (boot) { boot.classList.add('fade-out'); }
    setTimeout(() => {
      if (boot) { boot.classList.add('hidden'); boot.classList.remove('fade-out'); }
      if (login) { login.classList.remove('hidden'); login.classList.add('fade-out'); requestAnimationFrame(() => login.classList.remove('fade-out')); }
      syncIdentity();
      const pass = $('#login-pass');
      if (pass) setTimeout(() => pass.focus(), 80);
    }, fast ? 0 : 520);
  }

  function showDesktop() {
    const loading = $('#login-loading');
    const card = $('.login-card');
    if (card) card.style.visibility = 'hidden';
    if (loading) loading.classList.remove('hidden');
    setTimeout(() => {
      if (login) login.classList.add('hidden');
      const desk = $('#desktop');
      if (desk) desk.classList.remove('hidden');
      try {
        Shell.init();
      } catch (e) {
        console.error(e);
        toast('MosaicOS', 'Se produjo un error al iniciar el escritorio: ' + e.message, 'error');
      }
      AudioKit.startup();
      if (Settings.data.firstRun) {
        Settings.save({ firstRun: false });
        setTimeout(() => Shell.openApp('welcome', {}), 650);
        setTimeout(() => toast('Bienvenido a MosaicOS', 'Su escritorio está listo. Haga doble clic en un icono o pulse la tecla Windows.', 'star', 7000), 1200);
      } else {
        setTimeout(() => toast('MosaicOS 0.50 «Aero»', 'Sesión iniciada como ' + (Settings.data.userName || 'Mosaic') + '.', 'user', 3800), 500);
      }
    }, 780);
  }

  function doLogin() {
    if (!login || login.classList.contains('hidden')) return;
    AudioKit.notify();
    showDesktop();
  }

  // Prepare the file system before the session starts (native bridge or demo tree).
  try {
    await FS.init();
  } catch (e) {
    console.warn('No se pudo inicializar el sistema de archivos nativo:', e);
    try { FS._loadVirtual(); } catch (err) {}
  }
  syncIdentity();

  if (fast) {
    if (boot) boot.classList.add('hidden');
    if (login) login.classList.remove('hidden');
    syncIdentity();
    if (autoLogin) setTimeout(doLogin, 150);
    else setTimeout(() => { const p = $('#login-pass'); if (p) p.focus(); }, 120);
  } else {
    setTimeout(showLogin, 2100);
  }

  // Login form
  const form = $('#login-form');
  if (form) form.addEventListener('submit', e => { e.preventDefault(); doLogin(); });
  if (autoLogin && !fast) setTimeout(doLogin, 2600);

  // Accessibility entry point
  const access = $('#login-access');
  if (access) access.onclick = () => {
    Dialog.show({
      title: 'Accesibilidad', icon: 'user', width: 440,
      body: `<p>Opciones rápidas de accesibilidad para esta sesión.</p>
        <div class="cp-row"><label>Aumentar el tamaño del texto y los iconos</label><button class="btn" id="acc-large">Activar</button></div>
        <div class="cp-row"><label>Volver al tamaño normal</label><button class="btn" id="acc-normal">Restablecer</button></div>`,
      buttons: [{ label: 'Cerrar', value: true, primary: true }],
      onOpen: (wrap, close) => {
        $('#acc-large', wrap).onclick = () => { document.body.classList.add('access-large'); close(true); };
        $('#acc-normal', wrap).onclick = () => { document.body.classList.remove('access-large'); close(true); };
      }
    });
  };

  // Power button on the login screen
  const loginPower = $('#login-power');
  if (loginPower) loginPower.onclick = async () => {
    const ok = await Dialog.confirm('Apagar el equipo', '¿Desea apagar MosaicOS?');
    if (!ok) return;
    AudioKit.shutdown();
    if (FS.native) {
      try { fetch('api/power', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ action: 'shutdown' }) }); } catch (e) {}
    }
    if (login) login.classList.add('fade-out');
    setTimeout(() => {
      if (login) login.classList.add('hidden');
      if (boot) {
        boot.classList.remove('hidden', 'fade-out');
        boot.innerHTML = `<div class="power-box power-done">
          <div class="power-glyph">⏻</div>
          <h2>Es seguro apagar el equipo</h2>
          <p>MosaicOS se ha cerrado. Puede cerrar esta pestaña o volver a iniciar el sistema.</p>
          <button class="btn primary" id="boot-again">Volver a iniciar</button></div>`;
        const again = $('#boot-again');
        if (again) again.onclick = () => location.reload();
      }
    }, 800);
  };

  // Keep the page stable if a user drops a file on it accidentally.
  window.addEventListener('dragover', e => e.preventDefault());
  window.addEventListener('drop', e => e.preventDefault());
})();

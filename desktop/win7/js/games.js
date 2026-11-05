/* ==========================================================================
   MosaicOS 0.50 "Aero" — games
   Minesweeper · Solitaire (Klondike) · Snake · Games hub
   ========================================================================== */
'use strict';

/* ------------------------------------------------------------------ */
/* Games hub                                                          */
/* ------------------------------------------------------------------ */
Apps.games = {
  title: 'Juegos', icon: 'games', width: 620, height: 440, minW: 480, minH: 340,
  mount(body, win) {
    body.classList.add('welcome');
    body.innerHTML = `
      <div class="wc-body">
        <div class="wc-side" style="width:180px">
          <h3>Juegos</h3>
          <p>Una colección clásica para MosaicOS.</p>
          <div class="side-item active">${iconSvg('games')}Todos los juegos</div>
          <div class="side-item" data-open="minesweeper">${iconSvg('mine')}Minesweeper</div>
          <div class="side-item" data-open="solitaire">${iconSvg('cards')}Solitaire</div>
          <div class="side-item" data-open="snake">${iconSvg('snake')}Snake</div>
          <div style="margin-top:18px;font-size:10px;color:#5e7f93;line-height:1.5">Puntuaciones guardadas<br>en este equipo.</div>
        </div>
        <div class="wc-main">
          <h2>Todos los juegos</h2>
          <p class="lead">Elija un juego y empiece a jugar.</p>
          <div class="wc-cards">
            <div class="wc-card" data-open="minesweeper">${iconSvg('mine')}<b>Minesweeper</b><span>Despeje el campo sin pisar una mina. Tres niveles.</span></div>
            <div class="wc-card" data-open="solitaire">${iconSvg('cards')}<b>Solitaire</b><span>Klondike clásico. Mueva todas las cartas a las fundaciones.</span></div>
            <div class="wc-card" data-open="snake">${iconSvg('snake')}<b>Snake</b><span>Coma manzanas y crezca sin chocar con los bordes.</span></div>
          </div>
          <div class="wc-tip">Los juegos se pueden abrir varias veces. Las puntuaciones se conservan en este equipo.</div>
        </div>
      </div>`;
    $$('[data-open]', body).forEach(el => el.onclick = () => Shell.openApp(el.dataset.open, {}));
  }
};

/* ------------------------------------------------------------------ */
/* Minesweeper                                                        */
/* ------------------------------------------------------------------ */
Apps.minesweeper = {
  title: 'Minesweeper', icon: 'mine', width: 460, height: 530, minW: 330, minH: 360,
  mount(body, win) {
    body.classList.add('minesweeper');
    body.innerHTML = `
      <div class="menubar"><button data-m="juego">Juego</button><button data-m="ayuda">Ayuda</button></div>
      <div class="ms-body">
        <div class="ms-hud"><span class="ms-count" id="ms-mines">010</span><button class="ms-face" id="ms-face">🙂</button><span class="ms-count" id="ms-time">000</span></div>
        <div class="ms-grid" id="ms-grid"></div>
        <div style="font-size:10.5px;color:#557082;text-align:center">Clic izquierdo: descubrir · Clic derecho: marcar mina</div>
      </div>`;
    const gridEl = $('#ms-grid', body), minesEl = $('#ms-mines', body), timeEl = $('#ms-time', body), faceEl = $('#ms-face', body);
    let level = 'beginner';
    const levels = { beginner: [9, 9, 10], intermediate: [16, 16, 40], expert: [30, 16, 99] };
    let cols, rows, mines, cells, started, dead, won, flagged, revealed, timer, seconds, first;

    function init(lv = level) {
      level = lv;
      [cols, rows, mines] = levels[level];
      cells = Array.from({ length: rows }, () => Array.from({ length: cols }, () => ({ mine: false, open: false, flag: false, n: 0 })));
      started = false; dead = false; won = false; flagged = 0; revealed = 0; seconds = 0; first = true;
      clearInterval(timer); timer = null;
      minesEl.textContent = String(mines).padStart(3, '0');
      timeEl.textContent = '000';
      faceEl.textContent = '🙂';
      gridEl.style.gridTemplateColumns = `repeat(${cols}, 26px)`;
      gridEl.innerHTML = '';
      for (let y = 0; y < rows; y++) for (let x = 0; x < cols; x++) {
        const b = document.createElement('button');
        b.className = 'ms-cell'; b.dataset.x = x; b.dataset.y = y;
        b.addEventListener('click', () => reveal(x, y));
        b.addEventListener('contextmenu', e => { e.preventDefault(); toggleFlag(x, y); });
        b.addEventListener('dblclick', () => chord(x, y));
        gridEl.appendChild(b);
      }
      win.setTitle(`Minesweeper — ${level === 'beginner' ? 'Principiante' : level === 'intermediate' ? 'Intermedio' : 'Experto'}`);
    }
    function placeMines(sx, sy) {
      const safe = new Set();
      for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) safe.add(`${sx + dx},${sy + dy}`);
      let placed = 0;
      while (placed < mines) {
        const x = Math.floor(Math.random() * cols), y = Math.floor(Math.random() * rows);
        if (cells[y][x].mine || safe.has(`${x},${y}`)) continue;
        cells[y][x].mine = true; placed++;
      }
      for (let y = 0; y < rows; y++) for (let x = 0; x < cols; x++) {
        let n = 0;
        for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
          const nx = x + dx, ny = y + dy;
          if (nx >= 0 && ny >= 0 && nx < cols && ny < rows && cells[ny][nx].mine) n++;
        }
        cells[y][x].n = n;
      }
    }
    function startTimer() {
      if (timer) return;
      timer = setInterval(() => {
        if (dead || won) return;
        seconds++;
        timeEl.textContent = String(Math.min(seconds, 999)).padStart(3, '0');
      }, 1000);
    }
    function cellEl(x, y) { return gridEl.children[y * cols + x]; }
    function reveal(x, y) {
      if (dead || won) return;
      const c = cells[y][x];
      if (c.flag || c.open) return;
      if (first) { first = false; placeMines(x, y); started = true; startTimer(); }
      c.open = true; revealed++;
      const el = cellEl(x, y);
      el.classList.add('open');
      if (c.mine) { el.classList.add('mine'); el.textContent = '💣'; lose(); return; }
      if (c.n) { el.textContent = c.n; el.classList.add('n' + c.n); }
      else {
        for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
          const nx = x + dx, ny = y + dy;
          if (nx >= 0 && ny >= 0 && nx < cols && ny < rows && !cells[ny][nx].open && !cells[ny][nx].flag) reveal(nx, ny);
        }
      }
      checkWin();
    }
    function toggleFlag(x, y) {
      if (dead || won) return;
      const c = cells[y][x];
      if (c.open) return;
      c.flag = !c.flag;
      flagged += c.flag ? 1 : -1;
      const el = cellEl(x, y);
      el.classList.toggle('flag', c.flag);
      minesEl.textContent = String(Math.max(0, mines - flagged)).padStart(3, '0');
    }
    function chord(x, y) {
      const c = cells[y][x];
      if (!c.open || !c.n) return;
      let f = 0;
      for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
        const nx = x + dx, ny = y + dy;
        if (nx >= 0 && ny >= 0 && nx < cols && ny < rows && cells[ny][nx].flag) f++;
      }
      if (f === c.n) for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++) {
        const nx = x + dx, ny = y + dy;
        if (nx >= 0 && ny >= 0 && nx < cols && ny < rows && !cells[ny][nx].flag && !cells[ny][nx].open) reveal(nx, ny);
      }
    }
    function lose() {
      dead = true; clearInterval(timer); timer = null;
      faceEl.textContent = '😵';
      for (let y = 0; y < rows; y++) for (let x = 0; x < cols; x++) {
        const c = cells[y][x], el = cellEl(x, y);
        if (c.mine && !c.flag) { el.classList.add('open', 'mine'); el.textContent = '💣'; }
      }
      AudioKit.error();
      setTimeout(() => Dialog.alert('Minesweeper', '¡Ha pisado una mina! Pulse la cara para volver a jugar.'), 250);
    }
    function checkWin() {
      if (revealed === cols * rows - mines) {
        won = true; clearInterval(timer); timer = null;
        faceEl.textContent = '😎';
        for (let y = 0; y < rows; y++) for (let x = 0; x < cols; x++) {
          if (cells[y][x].mine) { cells[y][x].flag = true; cellEl(x, y).classList.add('flag'); }
        }
        minesEl.textContent = '000';
        AudioKit.startup();
        setTimeout(() => Dialog.alert('Minesweeper', `¡Felicidades! Ha despejado el campo en ${seconds} segundos.`), 250);
      }
    }
    function restart() { init(level); }
    faceEl.onclick = restart;
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      if (b.dataset.m === 'juego') Shell.menuAt(r.left, r.bottom, [
        { label: 'Nuevo juego', icon: 'mine', key: 'F2', action: restart },
        { sep: true },
        { label: (level === 'beginner' ? '✓ ' : '') + 'Principiante (9×9, 10 minas)', action: () => { init('beginner'); } },
        { label: (level === 'intermediate' ? '✓ ' : '') + 'Intermedio (16×16, 40 minas)', action: () => { init('intermediate'); } },
        { label: (level === 'expert' ? '✓ ' : '') + 'Experto (30×16, 99 minas)', action: () => { init('expert'); } },
        { sep: true },
        { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ]);
      else Shell.menuAt(r.left, r.bottom, [
        { label: 'Cómo jugar', icon: 'help', action: () => Dialog.alert('Minesweeper', 'Descubra todas las casillas sin minas.\n\nClic izquierdo: descubrir una casilla.\nClic derecho: colocar o quitar una bandera.\nDoble clic sobre un número: descubrir las casillas vecinas si el número de banderas coincide.\n\nEl primer clic siempre es seguro.') },
        { label: 'Acerca de Minesweeper', icon: 'info', action: () => Dialog.alert('Minesweeper', 'Minesweeper de MosaicOS 0.50\nTres niveles de dificultad con primer clic seguro.') }
      ]);
    });
    win.element.addEventListener('keydown', e => { if (e.key === 'F2') { e.preventDefault(); restart(); } });
    win.onClosed = () => clearInterval(timer);
    init();
  }
};

/* ------------------------------------------------------------------ */
/* Solitaire (Klondike)                                               */
/* ------------------------------------------------------------------ */
Apps.solitaire = {
  title: 'Solitaire', icon: 'cards', width: 860, height: 620, minW: 700, minH: 480,
  mount(body, win) {
    body.classList.add('solitaire');
    body.innerHTML = `
      <div class="menubar"><button data-m="juego">Juego</button><button data-m="ayuda">Ayuda</button></div>
      <div class="sol-body">
        <div class="sol-top" id="sol-top"></div>
        <div class="sol-tableau" id="sol-tableau"></div>
        <div class="sol-hud"><span id="sol-score">Puntuación: 0</span><span id="sol-time">Tiempo: 0:00</span><span style="flex:1"></span><button id="sol-new">Nuevo juego</button><button id="sol-undo">Deshacer</button></div>
      </div>`;
    const topEl = $('#sol-top', body), tabEl = $('#sol-tableau', body);
    const SUITS = [{ s: '♠', c: 'black' }, { s: '♥', c: 'red' }, { s: '♦', c: 'red' }, { s: '♣', c: 'black' }];
    const RANKS = ['A', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K'];
    let stock = [], waste = [], foundations = [[], [], [], []], tableau = [], selected = null, undoStack = [], score = 0, moves = 0, seconds = 0, timer = null, won = false;

    function snapshot() {
      return JSON.stringify({ stock, waste, foundations, tableau, score, moves, selected });
    }
    function pushUndo() { undoStack.push(snapshot()); if (undoStack.length > 40) undoStack.shift(); }
    function restore(s) {
      const o = JSON.parse(s);
      stock = o.stock; waste = o.waste; foundations = o.foundations; tableau = o.tableau;
      score = o.score; moves = o.moves; selected = o.selected;
      render();
    }
    function newGame() {
      const deck = [];
      for (let si = 0; si < 4; si++) for (let r = 1; r <= 13; r++) deck.push({ suit: si, rank: r });
      for (let i = deck.length - 1; i > 0; i--) { const j = Math.floor(Math.random() * (i + 1)); [deck[i], deck[j]] = [deck[j], deck[i]]; }
      stock = []; waste = []; foundations = [[], [], [], []]; tableau = []; selected = null; undoStack = []; score = 0; moves = 0; seconds = 0; won = false;
      for (let i = 0; i < 7; i++) {
        tableau.push([]);
        for (let j = 0; j <= i; j++) tableau[i].push({ ...deck.pop(), faceUp: j === i });
      }
      stock = deck;
      clearInterval(timer); timer = setInterval(() => { if (!won) { seconds++; updateHud(); } }, 1000);
      render(); updateHud();
    }
    function updateHud() {
      $('#sol-score', body).textContent = `Puntuación: ${score} · Movimientos: ${moves}`;
      $('#sol-time', body).textContent = `Tiempo: ${Math.floor(seconds / 60)}:${pad2(seconds % 60)}`;
    }
    function canStack(card, target) {
      if (!target) return card.rank === 13;
      const cs = SUITS[card.suit].c, ts = SUITS[target.suit].c;
      return cs !== ts && card.rank === target.rank - 1;
    }
    function cardHtml(c, i, extra = '') {
      const s = SUITS[c.suit], rank = RANKS[c.rank - 1];
      const faceUp = c.faceUp !== false;
      const red = s.c === 'red' ? ' red' : '';
      const sel = extra.includes('sel') ? ' sel' : '';
      if (!faceUp) return `<div class="card face-down" data-idx="${i}"></div>`;
      const pip = c.rank === 1 ? s.s : c.rank > 10 ? RANKS[c.rank - 1] : s.s;
      return `<div class="card${red}${sel}" data-idx="${i}">
        <div class="corner">${rank}<br>${s.s}</div>
        <div class="pip">${pip}</div>
        <div class="corner bot">${rank}<br>${s.s}</div>
      </div>`;
    }
    function render() {
      // top row
      topEl.innerHTML = `
        <div class="sol-stock" id="sol-stock" title="Robar carta">${stock.length ? '<div class="card face-down"></div>' : '<span style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:#cfe8d8;font-size:24px">↻</span>'}</div>
        <div class="sol-waste" id="sol-waste">${waste.length ? cardHtml(waste[waste.length - 1], waste.length - 1, selected && selected.pile === 'waste' ? 'sel' : '') : ''}</div>
        <div class="sol-spacer"></div>
        ${foundations.map((f, i) => `<div class="sol-found" data-found="${i}">${f.length ? cardHtml(f[f.length - 1], f.length - 1, selected && selected.pile === 'found' && selected.index === i ? 'sel' : '') : `<span style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;color:rgba(255,255,255,.35);font-size:26px">${SUITS[i].s}</span>`}</div>`).join('')}`;
      // tableau
      tabEl.innerHTML = tableau.map((col, ci) => {
        let html = `<div class="sol-col" data-col="${ci}"><div class="slot"></div>`;
        let top = 0;
        col.forEach((c, i) => {
          const selectedRun = selected && selected.pile === 'tab' && selected.col === ci && i >= selected.index;
          html += `<div style="position:absolute;left:0;right:0;top:${top}px">${cardHtml(c, i, selectedRun ? 'sel' : '')}</div>`;
          top += c.faceUp ? 20 : 7;
        });
        return html + '</div>';
      }).join('');
      bindBoard();
    }
    function bindBoard() {
      const stockEl = $('#sol-stock', body);
      if (stockEl) stockEl.onclick = drawStock;
      const wasteEl = $('#sol-waste', body);
      if (wasteEl) {
        wasteEl.onclick = e => { e.stopPropagation(); if (!waste.length) return; selectWaste(); };
        wasteEl.ondblclick = e => { e.stopPropagation(); autoFoundation({ pile: 'waste' }); };
      }
      $$('.sol-found', body).forEach(el => el.onclick = e => {
        e.stopPropagation();
        if (selected) moveToFoundation(+el.dataset.found);
      });
      $$('.sol-col', body).forEach(el => {
        const ci = +el.dataset.col;
        el.onclick = e => {
          if (e.target.classList.contains('card') || e.target.closest('.card')) return;
          if (selected) moveToTableau(ci);
        };
        $$('.card', el).forEach(cardEl => {
          const i = +cardEl.dataset.idx;
          cardEl.onclick = e => {
            e.stopPropagation();
            if (selected && selected.pile === 'tab' && selected.col === ci && i >= selected.index) { selected = null; render(); return; }
            clickTableau(ci, i);
          };
          cardEl.ondblclick = e => { e.stopPropagation(); autoFoundation({ pile: 'tab', col: ci, index: i }); };
        });
      });
      $$('.sol-found .card', body).forEach(c => c.onclick = e => e.stopPropagation());
    }
    function drawStock() {
      if (won) return;
      pushUndo();
      if (stock.length) {
        const c = stock.pop();
        c.faceUp = true; waste.push(c);
        score = Math.max(0, score - 2);
      } else if (waste.length) {
        stock = waste.reverse().map(c => ({ ...c, faceUp: false }));
        waste = [];
        score = Math.max(0, score - 20);
      }
      selected = null; moves++; render(); updateHud();
    }
    function selectWaste() {
      selected = selected && selected.pile === 'waste' ? null : { pile: 'waste' };
      render();
    }
    function clickTableau(ci, i) {
      const col = tableau[ci], c = col[i];
      if (!c.faceUp) return;
      if (selected) {
        if (selected.pile === 'waste') { moveToTableau(ci); return; }
        if (selected.pile === 'tab') {
          if (selected.col === ci) { selected = selected.index === i ? null : { pile: 'tab', col: ci, index: i }; render(); return; }
          moveToTableau(ci); return;
        }
        if (selected.pile === 'found') { moveToTableau(ci); return; }
      }
      // select run only if valid sequence
      if (validRun(col, i)) selected = { pile: 'tab', col: ci, index: i };
      else selected = { pile: 'tab', col: ci, index: i }; // allow single card selection
      render();
    }
    function validRun(col, i) {
      for (let k = i; k < col.length - 1; k++) {
        if (!col[k].faceUp || !col[k + 1].faceUp) return false;
        if (col[k].rank !== col[k + 1].rank + 1) return false;
        if (SUITS[col[k].suit].c === SUITS[col[k + 1].suit].c) return false;
      }
      return true;
    }
    function moveToTableau(ci) {
      const col = tableau[ci];
      if (!selected) return;
      let moving = [], from = null;
      if (selected.pile === 'waste') {
        if (!waste.length) return;
        moving = [waste[waste.length - 1]]; from = 'waste';
      } else if (selected.pile === 'tab') {
        if (selected.col === ci) { selected = null; render(); return; }
        const src = tableau[selected.col];
        if (!validRun(src, selected.index)) { selected = null; render(); return; }
        moving = src.slice(selected.index); from = 'tab';
      } else if (selected.pile === 'found') {
        const f = foundations[selected.index];
        if (!f.length) return;
        moving = [f[f.length - 1]]; from = 'found';
      }
      const target = col[col.length - 1];
      if (!canStack(moving[0], target)) { selected = null; render(); return; }
      pushUndo();
      if (from === 'waste') waste.pop();
      else if (from === 'found') foundations[selected.index].pop();
      else {
        const src = tableau[selected.col];
        src.splice(selected.index);
        if (src.length && !src[src.length - 1].faceUp) { src[src.length - 1].faceUp = true; score += 5; }
      }
      col.push(...moving);
      moves++; selected = null; render(); updateHud(); checkWin();
    }
    function moveToFoundation(fi) {
      if (!selected) return;
      const f = foundations[fi];
      let card = null, from = null;
      if (selected.pile === 'waste') { card = waste[waste.length - 1]; from = 'waste'; }
      else if (selected.pile === 'tab') {
        const src = tableau[selected.col];
        if (selected.index !== src.length - 1) { selected = null; render(); return; }
        card = src[src.length - 1]; from = 'tab';
      } else if (selected.pile === 'found') {
        if (selected.index === fi) { selected = null; render(); return; }
        card = foundations[selected.index][foundations[selected.index].length - 1]; from = 'found';
      }
      if (!card) return;
      const top = f[f.length - 1];
      if (f.length ? (card.suit !== top.suit || card.rank !== top.rank + 1) : card.rank !== 1) { selected = null; render(); return; }
      pushUndo();
      if (from === 'waste') waste.pop();
      else if (from === 'tab') {
        const src = tableau[selected.col]; src.pop();
        if (src.length && !src[src.length - 1].faceUp) { src[src.length - 1].faceUp = true; score += 5; }
      } else foundations[selected.index].pop();
      f.push(card); score += 10; moves++;
      selected = null; render(); updateHud(); checkWin();
    }
    function autoFoundation(sel) {
      const old = selected;
      selected = sel.pile === 'waste' ? { pile: 'waste' } : { pile: 'tab', col: sel.col, index: sel.index };
      // try each foundation
      const card = sel.pile === 'waste' ? waste[waste.length - 1] : tableau[sel.col][sel.index];
      if (!card) { selected = old; return; }
      for (let i = 0; i < 4; i++) {
        const f = foundations[i];
        const top = f[f.length - 1];
        if (f.length ? (card.suit === top.suit && card.rank === top.rank + 1) : card.rank === 1) {
          moveToFoundation(i); return;
        }
      }
      selected = old; render();
    }
    function fallback() {}
    function checkWin() {
      if (foundations.every(f => f.length === 13)) {
        won = true; clearInterval(timer); timer = null;
        score += 100;
        setTimeout(() => { Dialog.alert('Solitaire', `¡Enhorabuena! Ha ganado la partida en ${Math.floor(seconds / 60)}:${pad2(seconds % 60)} con ${score} puntos.`); }, 300);
      }
    }
    function undo() {
      if (!undoStack.length) return;
      restore(undoStack.pop());
    }
    $('#sol-new', body).onclick = () => { if (seconds > 0 && !won) { Dialog.confirm('Solitaire', '¿Empezar una partida nueva?').then(ok => ok && newGame()); } else newGame(); };
    $('#sol-undo', body).onclick = undo;
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      if (b.dataset.m === 'juego') Shell.menuAt(r.left, r.bottom, [
        { label: 'Nuevo juego', icon: 'cards', key: 'F2', action: newGame },
        { label: 'Deshacer', key: 'Ctrl+Z', action: undo },
        { sep: true },
        { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ]);
      else Shell.menuAt(r.left, r.bottom, [
        { label: 'Cómo jugar', icon: 'help', action: () => Dialog.alert('Solitaire', 'Mueva todas las cartas a las cuatro fundaciones.\n\n· Haga clic en la baraja para robar una carta.\n· Haga clic en una carta y luego en otra columna para moverla.\n· Doble clic en una carta para enviarla a una fundación.\n· Las columnas vacías solo aceptan un Rey.\n· Ordene las columnas de forma descendente y alternando colores.') },
        { label: 'Acerca de Solitaire', icon: 'info', action: () => Dialog.alert('Solitaire', 'Klondike para MosaicOS 0.50\nBaraja de una carta, deshacer y puntuación.') }
      ]);
    });
    win.element.addEventListener('keydown', e => {
      if (e.key === 'F2') { e.preventDefault(); newGame(); }
      if (e.ctrlKey && e.key.toLowerCase() === 'z') { e.preventDefault(); undo(); }
      if (e.key === 'Escape') { selected = null; render(); }
    });
    win.onClosed = () => clearInterval(timer);
    newGame();
  }
};

/* ------------------------------------------------------------------ */
/* Snake                                                              */
/* ------------------------------------------------------------------ */
Apps.snake = {
  title: 'Snake', icon: 'snake', width: 520, height: 570, minW: 420, minH: 470,
  mount(body, win) {
    body.classList.add('snake');
    body.innerHTML = `
      <div class="menubar"><button data-m="juego">Juego</button><button data-m="ayuda">Ayuda</button></div>
      <div class="sn-body">
        <div class="sn-hud"><span>Puntuación: <b id="sn-score">0</b></span><span>Récord: <b id="sn-best">0</b></span></div>
        <canvas id="sn-canvas" width="420" height="420"></canvas>
        <div class="sn-help">Flechas o WASD para moverse · Espacio para pausa · Enter para reiniciar</div>
      </div>`;
    const canvas = $('#sn-canvas', body), ctx = canvas.getContext('2d');
    const CELL = 20, COLS = canvas.width / CELL, ROWS = canvas.height / CELL;
    let snake, dir, nextDir, food, score, best = Store.get('snake.best', 0), alive, paused, acc, last, raf, timer;
    $('#sn-best', body).textContent = best;

    function reset() {
      snake = [{ x: 10, y: 10 }, { x: 9, y: 10 }, { x: 8, y: 10 }];
      dir = { x: 1, y: 0 }; nextDir = { x: 1, y: 0 };
      food = randFood(); score = 0; alive = true; paused = false; acc = 0; last = 0;
      $('#sn-score', body).textContent = '0';
      draw();
    }
    function randFood() {
      let p;
      do { p = { x: Math.floor(Math.random() * COLS), y: Math.floor(Math.random() * ROWS) }; }
      while (snake && snake.some(s => s.x === p.x && s.y === p.y));
      return p;
    }
    function draw() {
      ctx.fillStyle = '#1d2a1e'; ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.strokeStyle = 'rgba(120,180,130,.08)';
      for (let i = 1; i < COLS; i++) { ctx.beginPath(); ctx.moveTo(i * CELL, 0); ctx.lineTo(i * CELL, canvas.height); ctx.stroke(); }
      for (let i = 1; i < ROWS; i++) { ctx.beginPath(); ctx.moveTo(0, i * CELL); ctx.lineTo(canvas.width, i * CELL); ctx.stroke(); }
      // food
      ctx.fillStyle = '#e05a4f';
      ctx.beginPath(); ctx.arc(food.x * CELL + CELL / 2, food.y * CELL + CELL / 2, CELL / 2 - 2, 0, Math.PI * 2); ctx.fill();
      ctx.fillStyle = '#8ce46a';
      ctx.fillRect(food.x * CELL + CELL / 2, food.y * CELL + 2, 2, 5);
      // snake
      snake.forEach((s, i) => {
        const g = 210 - i * 3;
        ctx.fillStyle = i === 0 ? '#9fe493' : `rgb(${50 + i * 2}, ${120 + Math.max(0, 70 - i * 5)}, 60)`;
        ctx.beginPath();
        const r = 5;
        const x = s.x * CELL + 1, y = s.y * CELL + 1, w = CELL - 2, h = CELL - 2;
        ctx.roundRect(x, y, w, h, r); ctx.fill();
        if (i === 0) {
          ctx.fillStyle = '#14200f';
          const ex = dir.x > 0 ? 4 : dir.x < 0 ? -4 : 0, ey = dir.y > 0 ? 4 : dir.y < 0 ? -4 : 0;
          ctx.beginPath(); ctx.arc(s.x * CELL + CELL / 2 - 4 + ex, s.y * CELL + CELL / 2 - 3 + ey, 2, 0, 7); ctx.fill();
          ctx.beginPath(); ctx.arc(s.x * CELL + CELL / 2 + 4 + ex, s.y * CELL + CELL / 2 - 3 + ey, 2, 0, 7); ctx.fill();
        }
      });
      if (!alive) overlay('GAME OVER', 'Pulse Enter o haga clic para reiniciar');
      else if (paused) overlay('PAUSA', 'Pulse Espacio para continuar');
    }
    function overlay(t, sub) {
      ctx.fillStyle = 'rgba(5,12,6,.68)'; ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = '#d7f5d0'; ctx.textAlign = 'center';
      ctx.font = 'bold 30px "Segoe UI", sans-serif'; ctx.fillText(t, canvas.width / 2, canvas.height / 2 - 6);
      ctx.font = '13px "Segoe UI", sans-serif'; ctx.fillStyle = '#a9c9a5'; ctx.fillText(sub, canvas.width / 2, canvas.height / 2 + 24);
    }
    function step() {
      dir = nextDir;
      const head = { x: snake[0].x + dir.x, y: snake[0].y + dir.y };
      if (head.x < 0 || head.y < 0 || head.x >= COLS || head.y >= ROWS || snake.some(s => s.x === head.x && s.y === head.y)) return die();
      snake.unshift(head);
      if (head.x === food.x && head.y === food.y) {
        score += 10;
        $('#sn-score', body).textContent = score;
        if (score > best) { best = score; Store.set('snake.best', best); $('#sn-best', body).textContent = best; }
        food = randFood();
        AudioKit.tone(880, .08, 'square', .04);
      } else snake.pop();
    }
    function die() {
      alive = false;
      AudioKit.error();
      draw();
    }
    function loop(t) {
      if (!alive) { raf = requestAnimationFrame(loop); return; }
      if (!paused) {
        if (!last) last = t;
        acc += t - last; last = t;
        const speed = Math.max(70, 150 - Math.floor(score / 20) * 8);
        while (acc > speed) { step(); acc -= speed; if (!alive) break; }
      } else last = t;
      draw();
      raf = requestAnimationFrame(loop);
    }
    function turn(nx, ny) {
      if (nx === -dir.x && ny === -dir.y) return;
      nextDir = { x: nx, y: ny };
    }
    win.element.addEventListener('keydown', e => {
      const k = e.key.toLowerCase();
      if (e.key === 'ArrowUp' || k === 'w') { e.preventDefault(); turn(0, -1); }
      else if (e.key === 'ArrowDown' || k === 's') { e.preventDefault(); turn(0, 1); }
      else if (e.key === 'ArrowLeft' || k === 'a') { e.preventDefault(); turn(-1, 0); }
      else if (e.key === 'ArrowRight' || k === 'd') { e.preventDefault(); turn(1, 0); }
      else if (e.key === ' ') { e.preventDefault(); if (!alive) reset(); else paused = !paused; }
      else if (e.key === 'Enter') { e.preventDefault(); reset(); }
    });
    canvas.addEventListener('click', () => { if (!alive) reset(); });
    $$('.menubar button', body).forEach(b => b.onclick = e => {
      const r = e.currentTarget.getBoundingClientRect();
      if (b.dataset.m === 'juego') Shell.menuAt(r.left, r.bottom, [
        { label: 'Nuevo juego', icon: 'snake', key: 'Enter', action: reset },
        { label: 'Pausa', key: 'Espacio', action: () => { if (alive) paused = !paused; } },
        { sep: true }, { label: 'Salir', icon: 'shutdown', action: () => win.close() }
      ]);
      else Shell.menuAt(r.left, r.bottom, [
        { label: 'Cómo jugar', icon: 'help', action: () => Dialog.alert('Snake', 'Use las flechas o WASD para mover la serpiente.\nComa las manzanas rojas para crecer y sumar puntos.\nNo choque con los bordes ni con su propio cuerpo.\n\nEspacio: pausa · Enter: reiniciar.') },
        { label: 'Acerca de Snake', icon: 'info', action: () => Dialog.alert('Snake', 'Snake de MosaicOS 0.50\nPuntuación máxima guardada en este equipo.') }
      ]);
    });
    win.onClosed = () => cancelAnimationFrame(raf);
    reset();
    raf = requestAnimationFrame(loop);
  }
};

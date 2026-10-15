(() => {
  "use strict";

  /* =====================================================================
     CONFIG — point these at the C++ backend.
     Expected: GET {API_BASE}{SUGGEST_PATH}?q=<prefix>&k=<K>
     Response: ["python tutorial", ...]  or  { "suggestions": [...] }
               items may be strings or objects with { query | text | suggestion }
     ===================================================================== */
  const CONFIG = {
    API_BASE: "http://localhost:8080",
    SUGGEST_PATH: "/suggest",
    K: 5,
    DEBOUNCE_MS: 120,
    TIMEOUT_MS: 1500,
    // Where a chosen query goes. Set to "" to only emit the `trievo:search` event.
    RESULTS_URL: "https://duckduckgo.com/?q=",
  };

  const $ = (id) => document.getElementById(id);
  const input = $("q"), form = $("search-form"), dropdown = $("dropdown");
  const listbox = $("listbox"), statusEl = $("status"), clearBtn = $("clear"), live = $("live");

  const state = { items: [], active: -1, ctrl: null, timer: null, open: false };

  /* ---------- data ---------- */
  function normalize(payload) {
    const arr = Array.isArray(payload) ? payload : (payload && payload.suggestions) || [];
    return arr
      .map((x) => (typeof x === "string" ? x : x && (x.query || x.text || x.suggestion)))
      .filter((s) => typeof s === "string" && s.length)
      .slice(0, CONFIG.K);
  }

  async function fetchSuggestions(q) {
    const ctrl = new AbortController();
    state.ctrl && state.ctrl.abort();
    state.ctrl = ctrl;
    const timeout = setTimeout(() => ctrl.abort(), CONFIG.TIMEOUT_MS);
    try {
      const url = new URL(CONFIG.SUGGEST_PATH, CONFIG.API_BASE);
      url.searchParams.set("q", q);
      url.searchParams.set("k", String(CONFIG.K));
      const res = await fetch(url, { signal: ctrl.signal, headers: { Accept: "application/json" } });
      if (!res.ok) throw new Error("HTTP " + res.status);
      return { items: normalize(await res.json()), demo: false };
    } catch (err) {
      if (ctrl !== state.ctrl) return null; // superseded by a newer keystroke
      return { items: [], demo: false, offline: true };
    } finally {
      clearTimeout(timeout);
    }
  }

  /* ---------- rendering ---------- */
  const SEARCH_ICON =
    '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true"><circle cx="10.5" cy="10.5" r="6.5"/><path d="M20 20l-4.8-4.8"/></svg>';

  function render(items, typed, meta) {
    listbox.textContent = "";
    items.forEach((text, i) => {
      const li = document.createElement("li");
      li.className = "option";
      li.id = "opt-" + i;
      li.setAttribute("role", "option");
      li.setAttribute("aria-selected", "false");
      li.innerHTML = SEARCH_ICON;

      const label = document.createElement("span");
      label.className = "label";
      const n = text.toLowerCase().startsWith(typed.toLowerCase()) ? typed.length : 0;
      if (n) {
        const b = document.createElement("b");
        b.textContent = text.slice(0, n);
        label.append(b, document.createTextNode(text.slice(n)));
      } else {
        label.textContent = text;
      }

      const hint = document.createElement("span");
      hint.className = "hint";
      hint.textContent = "Enter";
      hint.setAttribute("aria-hidden", "true");

      li.append(label, hint);
      li.addEventListener("mousedown", (e) => e.preventDefault());
      li.addEventListener("mousemove", () => setActive(i));
      li.addEventListener("click", () => choose(text));
      listbox.append(li);
    });

    if (meta && meta.offline) {
      statusEl.textContent = "Backend not reachable";
      statusEl.hidden = false;
    } else {
      statusEl.hidden = true;
    }

    state.items = items;
    state.active = -1;
    const show = items.length > 0 || (meta && (meta.offline));
    show ? openList() : closeList();
    live.textContent = items.length
      ? items.length + (items.length === 1 ? " suggestion" : " suggestions") + " available"
      : "";
  }

  function openList() {
    state.open = true;
    dropdown.classList.add("open");
    input.setAttribute("aria-expanded", "true");
  }

  function closeList() {
    state.open = false;
    dropdown.classList.remove("open");
    input.setAttribute("aria-expanded", "false");
    input.removeAttribute("aria-activedescendant");
    state.active = -1;
  }

  function setActive(i) {
    const opts = listbox.children;
    if (state.active >= 0 && opts[state.active]) opts[state.active].setAttribute("aria-selected", "false");
    state.active = i;
    if (i >= 0 && opts[i]) {
      opts[i].setAttribute("aria-selected", "true");
      input.setAttribute("aria-activedescendant", opts[i].id);
    } else {
      input.removeAttribute("aria-activedescendant");
    }
  }

  /* ---------- actions ---------- */
  function runSearch(query) {
    const q = query.trim();
    if (!q) return;
    document.dispatchEvent(new CustomEvent("trievo:search", { detail: { query: q } }));
    if (CONFIG.RESULTS_URL) window.location.href = CONFIG.RESULTS_URL + encodeURIComponent(q);
  }

  function choose(text) {
    input.value = text;
    syncClear();
    closeList();
    runSearch(text);
  }

  function syncClear() {
    clearBtn.classList.toggle("visible", input.value.length > 0);
  }

  async function update() {
    const q = input.value.trim();
    syncClear();
    if (!q) {
      state.ctrl && state.ctrl.abort();
      closeList();
      return;
    }
    const result = await fetchSuggestions(q);
    if (!result || input.value.trim() !== q) return;
    render(result.items, q, result);
  }

  /* ---------- events ---------- */
  input.addEventListener("input", () => {
    clearTimeout(state.timer);
    state.timer = setTimeout(update, CONFIG.DEBOUNCE_MS);
    syncClear();
  });

  input.addEventListener("keydown", (e) => {
    const n = state.items.length;
    switch (e.key) {
      case "ArrowDown":
        if (!state.open && n) { openList(); }
        if (n) { e.preventDefault(); setActive((state.active + 1) % n); }
        break;
      case "ArrowUp":
        if (n) { e.preventDefault(); setActive(state.active <= 0 ? n - 1 : state.active - 1); }
        break;
      case "Enter":
        e.preventDefault();
        if (state.open && state.active >= 0) choose(state.items[state.active]);
        else { closeList(); runSearch(input.value); }
        break;
      case "Escape":
        if (state.open) { e.preventDefault(); closeList(); }
        break;
    }
  });

  input.addEventListener("focus", () => {
    if (state.items.length && input.value.trim()) openList();
  });

  input.addEventListener("blur", () => closeList());

  form.addEventListener("submit", (e) => e.preventDefault());

  clearBtn.addEventListener("mousedown", (e) => e.preventDefault());
  clearBtn.addEventListener("click", () => {
    input.value = "";
    state.items = [];
    syncClear();
    closeList();
    input.focus();
  });

  document.querySelectorAll(".chip").forEach((chip) =>
    chip.addEventListener("click", () => {
      input.value = chip.dataset.q;
      input.focus();
      update();
    })
  );

  // "/" focuses the search bar
  document.addEventListener("keydown", (e) => {
    if (e.key === "/" && document.activeElement !== input && !e.metaKey && !e.ctrlKey && !e.altKey) {
      e.preventDefault();
      input.focus();
    }
  });

  input.focus({ preventScroll: true });
})();

/* ---------- Logo animation helpers ---------- */
(() => {
  "use strict";
  const stage = document.getElementById("logo-stage");
  const flip = document.getElementById("flip");
  if (!stage || !flip) return;

  // Extruded edge: a few dark copies of the ring + handle stacked behind the
  // front face, so the logo has visible thickness while it turns.
  const NS = "http://www.w3.org/2000/svg";
  const LAYERS = 6, STEP = 1.1;
  const shades = ["#7a4fe0", "#6b42cc", "#5d37b8", "#502ea4", "#442690", "#381f7c"];
  for (let i = 1; i <= LAYERS; i++) {
    const svg = document.createElementNS(NS, "svg");
    svg.setAttribute("viewBox", "0 0 120 120");
    svg.setAttribute("aria-hidden", "true");
    svg.setAttribute("class", "depth");
    svg.style.transform = "translateZ(" + (-i * STEP) + "px)";
    svg.innerHTML =
      '<circle cx="54" cy="54" r="38" fill="none" stroke="' + shades[i - 1] + '" stroke-width="9"/>' +
      '<line x1="81" y1="81" x2="106" y2="106" stroke="' + shades[i - 1] + '" stroke-width="12" stroke-linecap="round"/>';
    flip.insertBefore(svg, flip.firstChild);
  }

  // Click the logo to replay the intro.
  stage.addEventListener("click", () => {
    stage.getAnimations({ subtree: true }).forEach((a) => { a.cancel(); a.play(); });
  });
})();

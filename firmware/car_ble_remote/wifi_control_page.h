#pragma once

const char WIFI_CONTROL_PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
    <head>
        <meta charset="utf-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no" />
        <title>ESP32-S3 WiFi 小车驾驶舱</title>
        <style>
            * {
                box-sizing: border-box;
                -webkit-tap-highlight-color: transparent;
                -webkit-touch-callout: none;
                -webkit-user-select: none;
                user-select: none;
            }
            html {
                overscroll-behavior: none;
            }
            body {
                margin: 0;
                background: #0b1117;
                color: #edf3f8;
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
                letter-spacing: 0;
                overflow-x: hidden;
                overscroll-behavior: none;
            }
            header {
                position: sticky;
                top: 0;
                z-index: 3;
                background: #101820;
                border-bottom: 1px solid #26323e;
                padding: 10px 12px;
            }
            h1 {
                margin: 0;
                font-size: 17px;
                line-height: 1.2;
            }
            .topline {
                display: flex;
                align-items: center;
                justify-content: space-between;
                gap: 10px;
            }
            .link {
                display: flex;
                gap: 6px;
                align-items: center;
                font-size: 12px;
                color: #9fb0c0;
                white-space: nowrap;
            }
            .dot {
                width: 8px;
                height: 8px;
                border-radius: 50%;
                background: #667085;
            }
            .dot.ok {
                background: #32d583;
                box-shadow: 0 0 10px #32d583;
            }
            .dot.bad {
                background: #ff4d4f;
                box-shadow: 0 0 10px #ff4d4f;
            }
            main {
                display: grid;
                gap: 10px;
                padding: 10px 10px 86px;
                min-width: 0;
                max-width: 100%;
            }
            section {
                background: #121b24;
                border: 1px solid #26323e;
                border-radius: 8px;
                padding: 10px;
                min-width: 0;
                max-width: 100%;
            }
            .grid4 {
                display: grid;
                grid-template-columns: repeat(4, 1fr);
                gap: 7px;
                margin-top: 10px;
            }
            .tile {
                min-width: 0;
                background: #0e151c;
                border: 1px solid #26323e;
                border-radius: 8px;
                padding: 7px;
            }
            .tile span {
                display: block;
                color: #8ea0b2;
                font-size: 11px;
                line-height: 1.2;
            }
            .tile strong {
                display: block;
                margin-top: 3px;
                overflow: hidden;
                text-overflow: ellipsis;
                white-space: nowrap;
                font-size: 15px;
            }
            .cockpit {
                display: grid;
                grid-template-columns: 1fr 116px;
                gap: 10px;
                align-items: stretch;
            }
            .drive {
                min-height: 152px;
                display: grid;
                place-items: center;
                border-radius: 8px;
                background: radial-gradient(circle at 50% 38%, #203040 0, #111a23 58%, #0c1218 100%);
                border: 1px solid #2a3846;
                position: relative;
                overflow: hidden;
            }
            .drive::before {
                content: "";
                position: absolute;
                inset: 18px;
                border: 1px solid #24313e;
                border-radius: 50%;
            }
            .arrow {
                position: relative;
                font-size: 62px;
                line-height: 1;
                color: #f5fbff;
                text-shadow: 0 0 18px #2f6df6;
            }
            .caption {
                position: absolute;
                left: 10px;
                right: 10px;
                bottom: 9px;
                display: flex;
                justify-content: space-between;
                color: #9fb0c0;
                font-size: 12px;
            }
            .statuscol {
                display: grid;
                gap: 7px;
            }
            .statuscol .tile {
                display: grid;
                align-content: center;
                min-height: 46px;
            }
            .controls {
                display: grid;
                grid-template-columns: 1fr 116px;
                gap: 10px;
            }
            button {
                border: 0;
                border-radius: 8px;
                min-height: 48px;
                background: #24313e;
                color: #eef6ff;
                font-size: 16px;
                font-weight: 750;
                touch-action: manipulation;
            }
            button:active,
            button.down {
                transform: scale(0.98);
                background: #2f6df6;
            }
            button.primary {
                background: #244a8f;
            }
            button.warn {
                background: #c92a2a;
            }
            button.ghost {
                background: #17232e;
                border: 1px solid #2d3a47;
            }
            .joystick {
                width: min(100%, 226px);
                aspect-ratio: 1;
                margin: 0 auto;
                border-radius: 50%;
                border: 1px solid #334455;
                background: radial-gradient(circle at 50% 50%, #17232e 0 26%, #0e151c 27% 100%);
                position: relative;
                touch-action: none;
                overflow: hidden;
                -webkit-user-drag: none;
            }
            .joystick::before,
            .joystick::after {
                content: "";
                position: absolute;
                background: #2a3846;
                opacity: 0.9;
            }
            .joystick::before {
                left: 50%;
                top: 10px;
                bottom: 10px;
                width: 1px;
            }
            .joystick::after {
                top: 50%;
                left: 10px;
                right: 10px;
                height: 1px;
            }
            .joymark {
                position: absolute;
                color: #8ea0b2;
                font-size: 16px;
                font-weight: 800;
                line-height: 1;
            }
            .joymark.up {
                top: 13px;
                left: 50%;
                transform: translateX(-50%);
            }
            .joymark.down {
                bottom: 13px;
                left: 50%;
                transform: translateX(-50%);
            }
            .joymark.left {
                left: 15px;
                top: 50%;
                transform: translateY(-50%);
            }
            .joymark.right {
                right: 15px;
                top: 50%;
                transform: translateY(-50%);
            }
            .stick {
                position: absolute;
                left: 50%;
                top: 50%;
                width: 72px;
                height: 72px;
                border-radius: 50%;
                transform: translate(-50%, -50%);
                background: #2f6df6;
                box-shadow: 0 0 20px rgba(47, 109, 246, 0.5);
                display: grid;
                place-items: center;
                color: #fff;
                font-size: 24px;
                font-weight: 900;
                z-index: 1;
            }
            .joystick.active .stick {
                background: #32d583;
                box-shadow: 0 0 22px rgba(50, 213, 131, 0.55);
            }
            .modebar {
                display: grid;
                grid-template-columns: repeat(2, 1fr);
                gap: 8px;
            }
            .speed {
                display: grid;
                gap: 8px;
                margin-top: 9px;
            }
            input[type="range"] {
                width: 100%;
                accent-color: #2f6df6;
            }
            .chips {
                display: grid;
                grid-template-columns: repeat(3, 1fr);
                gap: 7px;
            }
            .chips button {
                min-height: 36px;
                font-size: 13px;
            }
            .scanner {
                display: grid;
                gap: 9px;
            }
            .rail {
                height: 54px;
                border: 1px solid #324252;
                border-radius: 8px;
                background: #0e151c;
                display: grid;
                grid-template-columns: repeat(5, 1fr);
                gap: 6px;
                padding: 7px;
                position: relative;
            }
            .beam {
                border-radius: 6px;
                background: #17232e;
                border: 1px solid #2a3846;
            }
            .beam.on {
                background: #f4f7fb;
                box-shadow: 0 0 14px #f4f7fb;
            }
            .cursor {
                position: absolute;
                bottom: -7px;
                left: 50%;
                width: 18px;
                height: 18px;
                border-radius: 50%;
                transform: translateX(-50%);
                background: #32d583;
                border: 3px solid #0e151c;
            }
            .scanner.lost .cursor {
                background: #ff4d4f;
            }
            .scanner.wide .cursor {
                background: #fdb022;
            }
            .scanmeta {
                display: flex;
                justify-content: space-between;
                gap: 8px;
                color: #9fb0c0;
                font-size: 12px;
            }
            .route-strip {
                display: flex;
                flex-wrap: wrap;
                gap: 6px;
                width: 100%;
                min-width: 0;
                max-width: 100%;
                max-height: 150px;
                overflow-x: hidden;
                overflow-y: auto;
                padding-bottom: 4px;
                align-content: flex-start;
            }
            .route-chip {
                min-width: 40px;
                height: 42px;
                border: 1px solid #334455;
                border-radius: 8px;
                display: grid;
                place-items: center;
                background: #0e151c;
                font-size: 19px;
                position: relative;
            }
            .route-chip small {
                position: absolute;
                right: 4px;
                bottom: 2px;
                color: #8ea0b2;
                font-size: 9px;
            }
            .route-chip.active {
                border-color: #2f6df6;
                box-shadow: inset 0 0 0 1px #2f6df6;
            }
            .route-chip.bad {
                border-color: #ff4d4f;
                background: #32191b;
            }
            details {
                background: #121b24;
                border: 1px solid #26323e;
                border-radius: 8px;
                padding: 10px;
                min-width: 0;
                max-width: 100%;
            }
            summary {
                cursor: pointer;
                font-weight: 750;
            }
            .diag {
                display: grid;
                grid-template-columns: repeat(2, 1fr);
                gap: 7px;
                margin-top: 10px;
            }
            .log {
                display: grid;
                gap: 4px;
                max-height: 130px;
                overflow: auto;
                margin-top: 9px;
                color: #9fb0c0;
                font-size: 12px;
            }
            .stopbar {
                position: fixed;
                left: 0;
                right: 0;
                bottom: 0;
                z-index: 4;
                padding: 10px;
                background: linear-gradient(180deg, rgba(11, 17, 23, 0), #0b1117 28%);
            }
            .stopbar button {
                width: 100%;
                min-height: 58px;
                font-size: 20px;
                background: #d92d20;
            }
            @media (max-width: 380px) {
                .cockpit,
                .controls {
                    grid-template-columns: 1fr;
                }
                .statuscol {
                    grid-template-columns: repeat(3, 1fr);
                }
                .grid4 {
                    grid-template-columns: repeat(2, 1fr);
                }
            }
        </style>
    </head>
    <body>
        <header>
            <div class="topline">
                <h1>WiFi 小车驾驶舱</h1>
                <div class="link"><i class="dot" id="linkDot"></i><span id="linkText">连接中</span></div>
            </div>
            <div class="grid4">
                <div class="tile"><span>模式</span><strong id="mode">--</strong></div>
                <div class="tile"><span>状态</span><strong id="state">--</strong></div>
                <div class="tile"><span>速度</span><strong id="speedLive">--</strong></div>
                <div class="tile"><span>延迟</span><strong id="latency">--</strong></div>
            </div>
        </header>
        <main>
            <section class="cockpit">
                <div class="drive">
                    <div class="arrow" id="driveArrow">●</div>
                    <div class="caption"><span id="cmdText">CMD --</span><span id="qualityText">LINE --</span></div>
                </div>
                <div class="statuscol">
                    <div class="tile"><span>WiFi</span><strong id="clients">--</strong></div>
                    <div class="tile"><span>运行</span><strong id="uptime">--</strong></div>
                    <div class="tile"><span>IP</span><strong id="ip">--</strong></div>
                </div>
            </section>
            <section class="controls">
                <div>
                    <div class="joystick" id="joystick" aria-label="方向摇杆">
                        <span class="joymark up">↑</span>
                        <span class="joymark down">↓</span>
                        <span class="joymark left">←</span>
                        <span class="joymark right">→</span>
                        <div class="stick" id="stick">●</div>
                    </div>
                    <div class="speed">
                        <div class="scanmeta"><strong>速度 <b id="speedText">180</b></strong><span id="ackText">待命</span></div>
                        <input id="speed" type="range" min="50" max="255" value="180" />
                        <div class="chips">
                            <button class="ghost" data-speed="100">慢</button>
                            <button class="ghost" data-speed="160">稳</button>
                            <button class="ghost" data-speed="220">快</button>
                        </div>
                    </div>
                </div>
                <div class="modebar">
                    <button class="primary" data-cmd="T">寻迹</button>
                    <button class="primary" data-cmd="E">学习</button>
                    <button class="primary" data-cmd="P">记忆跑</button>
                    <button data-cmd="C">清路线</button>
                </div>
            </section>
            <section class="scanner" id="scanner">
                <div class="scanmeta"><strong>五路雷达</strong><span id="sensorText">mask --</span></div>
                <div class="rail" id="sensors"><i class="cursor" id="cursor"></i></div>
                <div class="scanmeta"><span id="lineText">等待数据</span><span id="searchText">SEARCH --</span></div>
            </section>
            <section>
                <div class="scanmeta"><strong>路线段</strong><span id="routeMeta">还没有路线</span></div>
                <div class="route-strip" id="routeStrip"></div>
            </section>
            <details>
                <summary>诊断</summary>
                <div class="diag">
                    <div class="tile"><span>原始</span><strong id="rawMask">--</strong></div>
                    <div class="tile"><span>误差</span><strong id="lineError">--</strong></div>
                    <div class="tile"><span>预期</span><strong id="expected">--</strong></div>
                    <div class="tile"><span>实际</span><strong id="observed">--</strong></div>
                    <div class="tile"><span>路线</span><strong id="routeIndex">--</strong></div>
                    <div class="tile"><span>匹配</span><strong id="matchState">--</strong></div>
                </div>
                <div class="log" id="eventLog"></div>
            </details>
        </main>
        <div class="stopbar"><button data-cmd="S">急停</button></div>
        <script>
            const $ = (id) => document.getElementById(id);
            const arrows = { F: "↑", B: "↓", L: "←", R: "→", l: "↖", r: "↗", S: "●", T: "◎", E: "●", P: "▶", C: "×" };
            const routeSymbols = { S: "↑", L: "↰", R: "↱", W: "▣", X: "!" };
            const routeLabels = { S: "直", L: "左", R: "右", W: "宽", X: "丢" };
            const qualityLabels = { LOST: "丢线", LEFT: "偏左", CENTER: "居中", RIGHT: "偏右", WIDE: "宽线" };
            const state = { speed: 180, lastRouteKey: "", lastEventKey: "", log: [], hold: 0, activeBtn: null, joyCmd: "", joyActive: false };
            const sensorEl = $("sensors");
            for (let i = 0; i < 5; i++) {
                const d = document.createElement("div");
                d.className = "beam";
                sensorEl.insertBefore(d, $("cursor"));
            }
            function setSpeed(v) {
                state.speed = Math.max(50, Math.min(255, Number(v) || 180));
                $("speed").value = state.speed;
                $("speedText").textContent = state.speed;
            }
            $("speed").addEventListener("input", (e) => setSpeed(e.target.value));
            document.querySelectorAll("[data-speed]").forEach((btn) => btn.addEventListener("click", () => setSpeed(btn.dataset.speed)));
            function withSpeed(cmd) {
                return ["F", "B", "L", "R", "l", "r", "T", "E", "P"].includes(cmd) ? cmd + state.speed : cmd;
            }
            async function send(cmd) {
                const started = performance.now();
                try {
                    await fetch("/api/cmd?c=" + encodeURIComponent(withSpeed(cmd)), { method: "POST", cache: "no-store" });
                    const ms = Math.round(performance.now() - started);
                    $("ackText").textContent = "ACK " + ms + "ms";
                    setLink(true, ms);
                } catch (e) {
                    $("ackText").textContent = "发送失败";
                    setLink(false);
                }
            }
            function bindButton(btn) {
                const cmd = btn.dataset.cmd;
                const hold = btn.dataset.hold === "1";
                if (!hold) {
                    btn.addEventListener("click", (e) => {
                        e.preventDefault();
                        send(cmd);
                    });
                    return;
                }
                const start = (e) => {
                    e.preventDefault();
                    btn.classList.add("down");
                    state.activeBtn = btn;
                    send(cmd);
                    if (hold) {
                        clearInterval(state.hold);
                        state.hold = setInterval(() => send(cmd), 220);
                    }
                };
                const stop = (e) => {
                    if (e) e.preventDefault();
                    btn.classList.remove("down");
                    if (state.activeBtn === btn) state.activeBtn = null;
                    clearInterval(state.hold);
                    state.hold = 0;
                    if (hold) send("S");
                };
                btn.addEventListener("pointerdown", start);
                btn.addEventListener("pointerup", stop);
                btn.addEventListener("pointercancel", stop);
                btn.addEventListener("pointerleave", () => {
                    if (state.activeBtn === btn) stop();
                });
            }
            document.querySelectorAll("button[data-cmd]").forEach(bindButton);
            function bindJoystick() {
                const pad = $("joystick");
                const stick = $("stick");
                if (!pad || !stick) return;
                const max = 56;
                const dead = 18;
                const diagonalRatio = 0.34;
                let pointerId = null;
                const cmdFor = (dx, dy) => {
                    if (Math.hypot(dx, dy) < dead) return "";
                    if (dy < -dead * 0.75 && Math.abs(dx) > Math.abs(dy) * diagonalRatio) {
                        return dx < 0 ? "l" : "r";
                    }
                    return Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? "L" : "R") : (dy < 0 ? "F" : "B");
                };
                const moveStick = (dx, dy) => {
                    const dist = Math.hypot(dx, dy);
                    const scale = dist > max ? max / dist : 1;
                    stick.style.transform = `translate(calc(-50% + ${Math.round(dx * scale)}px), calc(-50% + ${Math.round(dy * scale)}px))`;
                };
                const update = (e) => {
                    const rect = pad.getBoundingClientRect();
                    const dx = e.clientX - rect.left - rect.width / 2;
                    const dy = e.clientY - rect.top - rect.height / 2;
                    moveStick(dx, dy);
                    const cmd = cmdFor(dx, dy);
                    stick.textContent = arrows[cmd] || "●";
                    if (cmd && cmd !== state.joyCmd) {
                        state.joyCmd = cmd;
                        send(cmd);
                        clearInterval(state.hold);
                        state.hold = setInterval(() => {
                            if (state.joyCmd) send(state.joyCmd);
                        }, 220);
                    }
                    if (!cmd && state.joyCmd) {
                        state.joyCmd = "";
                        clearInterval(state.hold);
                        state.hold = 0;
                        send("S");
                    }
                };
                const stop = () => {
                    if (pointerId !== null) {
                        try { pad.releasePointerCapture(pointerId); } catch (e) {}
                    }
                    pointerId = null;
                    state.joyActive = false;
                    pad.classList.remove("active");
                    stick.style.transform = "translate(-50%, -50%)";
                    stick.textContent = "●";
                    clearInterval(state.hold);
                    state.hold = 0;
                    if (state.joyCmd) {
                        state.joyCmd = "";
                        send("S");
                    }
                };
                pad.addEventListener("pointerdown", (e) => {
                    e.preventDefault();
                    pointerId = e.pointerId;
                    state.joyActive = true;
                    pad.setPointerCapture(pointerId);
                    pad.classList.add("active");
                    update(e);
                });
                pad.addEventListener("pointermove", (e) => {
                    if (e.pointerId === pointerId) {
                        e.preventDefault();
                        update(e);
                    }
                });
                pad.addEventListener("pointerup", stop);
                pad.addEventListener("pointercancel", stop);
                pad.addEventListener("contextmenu", (e) => e.preventDefault());
                pad.addEventListener("touchstart", (e) => e.preventDefault(), { passive: false });
                pad.addEventListener("touchmove", (e) => e.preventDefault(), { passive: false });
                pad.addEventListener("touchend", (e) => e.preventDefault(), { passive: false });
                document.addEventListener("touchmove", (e) => {
                    if (state.joyActive) e.preventDefault();
                }, { passive: false });
            }
            bindJoystick();
            function setLink(ok, ms) {
                $("linkDot").className = "dot " + (ok ? "ok" : "bad");
                $("linkText").textContent = ok ? "在线" : "离线";
                if (ms !== undefined) $("latency").textContent = ms + "ms";
            }
            function fmtTime(sec) {
                sec = Number(sec) || 0;
                const m = Math.floor(sec / 60);
                const s = sec % 60;
                return m + ":" + String(s).padStart(2, "0");
            }
            function lineOffset(mask) {
                let sum = 0, count = 0;
                for (let slot = 0; slot < 5; slot++) {
                    const sensorIndex = 4 - slot;
                    if (mask & (1 << sensorIndex)) {
                        sum += slot;
                        count++;
                    }
                }
                return count ? (sum / count) * 25 : 50;
            }
            function updateSensors(s) {
                const mask = Number(s.sensorMask) || 0;
                const cells = [...sensorEl.querySelectorAll(".beam")];
                cells.forEach((cell, slot) => {
                    const sensorIndex = 4 - slot;
                    cell.classList.toggle("on", (mask & (1 << sensorIndex)) !== 0);
                });
                $("cursor").style.left = Math.max(0, Math.min(100, lineOffset(mask))) + "%";
                $("sensorText").textContent = "mask " + mask.toString(2).padStart(5, "0");
                $("rawMask").textContent = mask.toString(2).padStart(5, "0");
                $("scanner").className = "scanner " + String(s.lineQuality || "").toLowerCase();
                $("lineText").textContent = (qualityLabels[s.lineQuality] || "--") + " / " + (s.activeSensors || 0) + "路";
                $("qualityText").textContent = "LINE " + (qualityLabels[s.lineQuality] || "--");
                $("lineError").textContent = s.lineError ?? "--";
                $("searchText").textContent = s.lineSearch ? ("SEARCH " + (s.searchDirection < 0 ? "←" : "→") + " #" + (s.searchStep || 0)) : "SEARCH --";
            }
            function drawRoute(route, current, bad) {
                const strip = $("routeStrip");
                strip.innerHTML = "";
                if (!route.length) {
                    strip.textContent = "暂无路线";
                    return;
                }
                route.forEach((seg, i) => {
                    const chip = document.createElement("div");
                    chip.className = "route-chip";
                    if (i === current) chip.classList.add(bad ? "bad" : "active");
                    chip.title = (routeLabels[seg[0]] || seg[0]) + " " + seg[1] + "格";
                    chip.textContent = routeSymbols[seg[0]] || "?";
                    const n = document.createElement("small");
                    n.textContent = seg[1];
                    chip.appendChild(n);
                    strip.appendChild(chip);
                });
                const active = strip.querySelector(".active,.bad");
                if (active) active.scrollIntoView({ block: "nearest", inline: "nearest" });
            }
            function addLog(text) {
                const now = new Date();
                state.log.unshift(now.toLocaleTimeString() + "  " + text);
                state.log = state.log.slice(0, 20);
                $("eventLog").innerHTML = state.log.map((x) => "<div>" + x + "</div>").join("");
            }
            function trackEvents(s) {
                const key = [s.mode, s.status, s.lineQuality, s.lineSearch, s.mismatch, s.routeIndex].join("|");
                if (key === state.lastEventKey) return;
                state.lastEventKey = key;
                addLog((s.mode || "--") + " / " + (s.status || "--") + " / " + (qualityLabels[s.lineQuality] || s.lineQuality || "--"));
            }
            async function refresh() {
                const started = performance.now();
                try {
                    const s = await fetch("/api/state", { cache: "no-store" }).then((r) => r.json());
                    const ms = Math.round(performance.now() - started);
                    setLink(true, ms);
                    $("mode").textContent = s.mode || "--";
                    $("state").textContent = s.status || "--";
                    $("speedLive").textContent = s.speed ?? "--";
                    $("clients").textContent = (s.wifiClients ?? "--") + " 台";
                    $("uptime").textContent = fmtTime(s.uptime);
                    $("ip").textContent = s.ip || "--";
                    $("cmdText").textContent = "CMD " + (s.cmd || "--");
                    $("driveArrow").textContent = arrows[s.cmd] || "●";
                    updateSensors(s);
                    const route = s.route || [];
                    const current = s.routeIndex ?? -1;
                    const bad = !!s.mismatch;
                    $("routeMeta").textContent = route.length + "段 / 当前 " + (current >= 0 ? current + 1 : "--") + " / " + (s.routeTicks || 0) + "格";
                    $("expected").textContent = routeLabels[s.expected] || s.expected || "--";
                    $("observed").textContent = routeLabels[s.observed] || s.observed || "--";
                    $("routeIndex").textContent = current >= 0 ? current + 1 : "--";
                    $("matchState").textContent = bad ? "不匹配" : "正常";
                    const routeKey = JSON.stringify(route) + "|" + current + "|" + bad;
                    if (routeKey !== state.lastRouteKey) {
                        drawRoute(route, current, bad);
                        state.lastRouteKey = routeKey;
                    }
                    trackEvents(s);
                } catch (e) {
                    setLink(false);
                }
            }
            setSpeed(180);
            setInterval(refresh, 300);
            refresh();
        </script>
    </body>
</html>

)HTML";

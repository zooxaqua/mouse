// =========================================================
// WebSocket Heartbeat
// =========================================================

const HEARTBEAT_INTERVAL_MS = 200;
const HEARTBEAT_MAX_MISSES = 5;

let heartbeatStarted = false;
let heartbeatMissCount = 0;
let heartbeatWaitingForAck = false;

// =========================================================
// Joy Stick
// =========================================================
const JOYSTICK_SEND_INTERVAL_MS = 100;
let leftStickX = 0;
let leftStickY = 0;

let rightStickX = 0;
let rightStickY = 0;

let lastSentLeftStickX = 0;
let lastSentLeftStickY = 0;

let lastSentRightStickX = 0;
let lastSentRightStickY = 0;
// =========================================================
// WebSocket
// =========================================================

let webSocket = null;

// =========================================================
// Info Log
// =========================================================

const MAX_INFO_LOGS = 50;
let infoLogs = [];

// ---------------------------------------------------------
// WebSocket接続
// ---------------------------------------------------------

function connectWebSocket()
{
    const host = window.location.hostname;

    webSocket = new WebSocket(
        "ws://" + host + ":81/"
    );


    webSocket.onopen = function()
    {
        console.log("WebSocket connected");
        infoLogs.push("[WebSocket] Connected");

        heartbeatStarted = false;
        heartbeatMissCount = 0;
        heartbeatWaitingForAck = false;

        if (infoLogs.length > MAX_INFO_LOGS) {
            infoLogs.shift();
        }

        updateDebugConsole();

        lastSentLeftStickX = 0;
        lastSentLeftStickY = 0;
        lastSentRightStickX = 0;
        lastSentRightStickY = 0;

        const webSocketStatus = document.getElementById("webSocketStatus");
        if (webSocketStatus !== null) {
            webSocketStatus.value = "CONNECTED";
        }
    };


    webSocket.onmessage = function(event)
    {
        const messages = event.data.split("\n");

        for (const message of messages) {

            if (message.length === 0) {
                continue;
            }

            if (message === "heartbeat_ack") {
                heartbeatStarted = true;
                heartbeatWaitingForAck = false;
                heartbeatMissCount = 0;
                
                continue;
            }

            infoLogs.push(message);

            if (infoLogs.length > MAX_INFO_LOGS) {
                infoLogs.shift();
            }

            console.log("[INFO]", message);
        }

        updateDebugConsole();
    };


    webSocket.onclose = function()
    {
        console.log("WebSocket disconnected");

        infoLogs.push("[WebSocket] Browser Side Disconnected");

        if (infoLogs.length > MAX_INFO_LOGS) {
            infoLogs.shift();
        }

        updateDebugConsole();

        const webSocketStatus = document.getElementById("webSocketStatus");

        if (webSocketStatus !== null) {
            webSocketStatus.value = "OFF";
        }

        webSocket = null;

        // 全入力をニュートラルへ戻す
        resetAllInputs();
    };


    webSocket.onerror = function(error)
    {
        console.log("WebSocket error:", error);
    };
}

function updateDebugConsole()
{
    const consoleElement =
        document.getElementById("debugConsole");

    if (!consoleElement) {
        return;
    }

    consoleElement.innerHTML = "";

    for (const message of infoLogs) {

        const line = document.createElement("div");

        line.textContent = message;

        consoleElement.appendChild(line);
    }

    // 常に最新ログが見えるようにする
    consoleElement.scrollTop =
        consoleElement.scrollHeight;
}

function resetAllInputs()
{
    // -----------------------------------------------------
    // Left Stick
    // -----------------------------------------------------

    const leftStick =
        document.getElementById("leftStick");

    const leftStickValue =
        document.getElementById("leftStickValue");

    if (leftStick !== null &&
        leftStickValue !== null) {

        resetJoystick(
            leftStick,
            leftStickValue,
            leftStick.querySelector(".joystick-knob")
        );
    }


    // -----------------------------------------------------
    // Right Stick
    // -----------------------------------------------------

    const rightStick =
        document.getElementById("rightStick");

    const rightStickValue =
        document.getElementById("rightStickValue");

    if (rightStick !== null &&
        rightStickValue !== null) {

        resetJoystick(
            rightStick,
            rightStickValue,
            rightStick.querySelector(".joystick-knob")
        );
    }
}

// =========================================================
// WebSocket送信
// =========================================================

function sendWebSocket(message)
{
    if (webSocket !== null &&
        webSocket.readyState === WebSocket.OPEN) {

        webSocket.send(message);
    }
}


// ---------------------------------------------------------
// Heartbeat送信
// ---------------------------------------------------------
setInterval(
    function()
    {
        try {

            if (webSocket !== null &&
                webSocket.readyState === WebSocket.OPEN) {

                // ACK監視開始後、前回のACKがなければミスカウント
                if (heartbeatStarted) {

                    if (heartbeatWaitingForAck) {

                        heartbeatMissCount++;
                        //タイムアウト処理
                        if (heartbeatMissCount >= HEARTBEAT_MAX_MISSES) {

                            infoLogs.push("[WebSocket] Disconnected");
                            updateDebugConsole();
                            webSocket.close();
                            return;
                        }
                    }
                }

                heartbeatWaitingForAck = true;

                webSocket.send("heartbeat");
            }

        }
        catch (error) {

            infoLogs.push(
                "[Heartbeat] ERROR: " + error
            );

            updateDebugConsole();
        }
    },
    HEARTBEAT_INTERVAL_MS
);
// ---------------------------------------------------------
// Joystick送信
// ---------------------------------------------------------

setInterval(
    function()
    {
        if (webSocket !== null &&
            webSocket.readyState === WebSocket.OPEN) {

            // -------------------------------------------------
            // Left Stick
            // -------------------------------------------------

            if (leftStickX !== lastSentLeftStickX ||
                leftStickY !== lastSentLeftStickY) {

                sendWebSocket(
                    "stick:left:" +
                    leftStickX +
                    ":" +
                    leftStickY
                );

                lastSentLeftStickX = leftStickX;
                lastSentLeftStickY = leftStickY;
            }


            // -------------------------------------------------
            // Right Stick
            // -------------------------------------------------

            if (rightStickX !== lastSentRightStickX ||
                rightStickY !== lastSentRightStickY) {

                sendWebSocket(
                    "stick:right:" +
                    rightStickX +
                    ":" +
                    rightStickY
                );

                lastSentRightStickX = rightStickX;
                lastSentRightStickY = rightStickY;
            }
        }
    },
    JOYSTICK_SEND_INTERVAL_MS
);

// =========================================================
// Button
// =========================================================
function setupButton(buttonId, buttonName)
{
    const button =
        document.getElementById(buttonId);


    // -----------------------------------------------------
    // ボタン押下
    // -----------------------------------------------------

    button.addEventListener(
        "pointerdown",
        function(event)
        {
            event.preventDefault();

            // このポインターの操作をボタンが捕捉する
            button.setPointerCapture(
                event.pointerId
            );

            sendWebSocket(
                "button:" + buttonName + ":pressed"
            );
        }
    );


    // -----------------------------------------------------
    // ボタンを離した
    // -----------------------------------------------------

    button.addEventListener(
        "pointerup",
        function(event)
        {
            event.preventDefault();

            sendWebSocket(
                "button:" + buttonName + ":released"
            );

            // Pointer Captureを解放
            if (button.hasPointerCapture(
                event.pointerId
            )) {
                button.releasePointerCapture(
                    event.pointerId
                );
            }
        }
    );


    // -----------------------------------------------------
    // 操作キャンセル
    // -----------------------------------------------------

    button.addEventListener(
        "pointercancel",
        function(event)
        {
            event.preventDefault();

            sendWebSocket(
                "button:" + buttonName + ":released"
            );

            // Pointer Captureを解放
            if (button.hasPointerCapture(
                event.pointerId
            )) {
                button.releasePointerCapture(
                    event.pointerId
                );
            }
        }
    );
}

// =========================================================
// Joystick
// =========================================================

function setupJoystick(joystickId, valueId)
{
    const joystick = document.getElementById(joystickId);
    const value = document.getElementById(valueId);
    const knob = joystick.querySelector(".joystick-knob");

    // -----------------------------------------------------
    // Pointer Down
    // -----------------------------------------------------

    joystick.addEventListener(
        "pointerdown",
        function(event)
        {
            event.preventDefault();

            joystick.setPointerCapture(
                event.pointerId
            );

            updateJoystick(
                joystick,
                value,
                knob,
                event
            );
        }
    );


    // -----------------------------------------------------
    // Pointer Move
    // -----------------------------------------------------

    joystick.addEventListener(
        "pointermove",
        function(event)
        {
            if (!joystick.hasPointerCapture(
                event.pointerId
            )) {
                return;
            }

            event.preventDefault();

            updateJoystick(
                joystick,
                value,
                knob,
                event
            );
        }
    );


    // -----------------------------------------------------
    // Pointer Up
    // -----------------------------------------------------

    joystick.addEventListener(
        "pointerup",
        function(event)
        {
            event.preventDefault();

            resetJoystick(
                joystick,
                value,
                knob
            );


            if (joystick.hasPointerCapture(
                event.pointerId
            )) {
                joystick.releasePointerCapture(
                    event.pointerId
                );
            }
        }
    );


    // -----------------------------------------------------
    // Pointer Cancel
    // -----------------------------------------------------

    joystick.addEventListener(
        "pointercancel",
        function(event)
        {
            event.preventDefault();

            resetJoystick(
                joystick,
                value,
                knob
            );

            if (joystick.hasPointerCapture(
                event.pointerId
            )) {
                joystick.releasePointerCapture(
                    event.pointerId
                );
            }
        }
    );

    // -----------------------------------------------------
    // Pointer Capture Lost
    // -----------------------------------------------------

    joystick.addEventListener(
        "lostpointercapture",
        function(event)
        {
            resetJoystick(
                joystick,
                value,
                knob
            );
        }
    );
}


// =========================================================
// Joystick Position
// =========================================================

function updateJoystick(
    joystick,
    value,
    knob,
    event
)
{
    const rect =
        joystick.getBoundingClientRect();


    const centerX =
        rect.left + rect.width / 2;

    const centerY =
        rect.top + rect.height / 2;


    let x =
        event.clientX - centerX;

    let y =
        event.clientY - centerY;


    const radius =
        Math.min(
            rect.width,
            rect.height
        ) / 2;

    // -----------------------------------------------------
    // 円の外側には出さない
    // -----------------------------------------------------

    const distance =
        Math.sqrt(
            x * x +
            y * y
        );


    if (distance > radius) {

        x =
            x / distance * radius;

        y =
            y / distance * radius;
    }


    // -----------------------------------------------------
    // -100 ～ +100へ変換
    // -----------------------------------------------------

    const normalizedX =
        Math.round(
            x / radius * 100
        );

    const normalizedY =
        Math.round(
            y / radius * 100
        );


    // -----------------------------------------------------
    // 最新値を保存
    // -----------------------------------------------------

    if (joystick.id === "leftStick") {

        leftStickX = normalizedX;
        leftStickY = normalizedY;

    }
    else {

        rightStickX = normalizedX;
        rightStickY = normalizedY;
    }


    value.value =
        "X: " +
        normalizedX +
        " / Y: " +
        normalizedY;
    // -----------------------------------------------------
    // ノブ移動
    // -----------------------------------------------------

    knob.style.left =
        (x + radius - knob.offsetWidth / 2) + "px";

    knob.style.top =
        (y + radius - knob.offsetHeight / 2) + "px";
}


// =========================================================
// Joystick Reset
// =========================================================

function resetJoystick(joystick, value, knob)
{
    if (joystick.id === "leftStick") {
        leftStickX = 0;
        leftStickY = 0;
    }
    else {
        rightStickX = 0;
        rightStickY = 0;
    }

    value.value = "X: 0 / Y: 0";

    // -----------------------------------------------------
    // ノブを中央へ戻す
    // -----------------------------------------------------

    knob.style.left =
        (joystick.offsetWidth - knob.offsetWidth) / 2 + "px";

    knob.style.top =
        (joystick.offsetHeight - knob.offsetHeight) / 2 + "px";
}

// =========================================================
// Page Load
// =========================================================

window.addEventListener("load", function()
{
    const isSmartphone =
        /iPhone|iPad|iPod|Android/i.test(
            navigator.userAgent
        );

    if (isSmartphone) {
        document.body.classList.add("smartphone");
    }

    // WebSocket自動接続
    connectWebSocket();

    // -----------------------------------------------------
    // ボタンイベント登録
    // -----------------------------------------------------
    setupButton("buttonA", "A");          // Aボタン
    setupButton("buttonB", "B");          // Bボタン
    setupButton("buttonX", "X");          // Xボタン
    setupButton("buttonY", "Y");          // Yボタン
    setupButton("dpadUp", "UP");          // UPボタン
    setupButton("dpadDown", "DOWN");      // DOWNボタン
    setupButton("dpadLeft", "LEFT");      // LEFTボタン
    setupButton("dpadRight", "RIGHT");    // RIGHTボタン


    // -----------------------------------------------------
    // Joystickイベント登録
    // -----------------------------------------------------

    setupJoystick("leftStick", "leftStickValue");
    setupJoystick("rightStick", "rightStickValue");
});


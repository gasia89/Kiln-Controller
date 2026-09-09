#include "KilnWebServer.h"

KilnWebServer::KilnWebServer(KilnController& controller) : controller_(controller) {}

void KilnWebServer::begin() {
    server_.on("/", HTTP_GET, [this]() {
        server_.send(200, "text/html", R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Kiln Manager</title><style>
body{font-family:system-ui,sans-serif;max-width:720px;margin:2rem auto;padding:0 1rem;background:#f4f1ea;color:#24231f}
main{background:white;padding:1.5rem;border:1px solid #d7d0c2;border-radius:8px}h1{margin-top:0}
label{display:block;margin-top:1rem;font-weight:600}input,select,button{font:inherit;padding:.55rem;margin-top:.35rem}
input,select{width:100%;box-sizing:border-box}button{cursor:pointer;margin-right:.4rem}.stop{background:#a92323;color:white;border:0}
#readings{margin:1rem 0;padding:.8rem;background:#eee9df;min-height:2rem}
</style></head><body><main><h1>Kiln Manager</h1>
<div id="readings">Waiting for readings...</div>
<label>Mode<select id="mode"><option value="manual">Manual</option><option value="automatic">Automatic</option></select></label>
<label>Target Celsius<input id="target" type="number" min="0" step="1" value="0"></label>
<button onclick="setMode()">Set mode</button><button onclick="setTarget()">Set target</button>
<h2>Manual coils</h2><label>Coil 1 power %<input id="p1" type="range" min="0" max="100" value="0" oninput="v1.textContent=this.value"></label><span id="v1">0</span>%
<button onclick="setCoil(1,true)">Coil 1 on</button><button onclick="setCoil(1,false)">Coil 1 off</button>
<label>Coil 2 power %<input id="p2" type="range" min="0" max="100" value="0" oninput="v2.textContent=this.value"></label><span id="v2">0</span>%
<button onclick="setCoil(2,true)">Coil 2 on</button><button onclick="setCoil(2,false)">Coil 2 off</button>
<p><button class="stop" onclick="fetch('/api/stop',{method:'POST'})">EMERGENCY STOP</button></p>
</main><script>
const post=(u)=>fetch(u,{method:'POST'}); function setMode(){post('/api/mode?mode='+mode.value)}
function setTarget(){post('/api/target?celsius='+target.value)} function setCoil(c,on){post('/api/coils?coil='+c+'&enabled='+(on?1:0)+'&power='+document.getElementById('p'+c).value)}
async function refresh(){const s=await (await fetch('/api/status')).json();readings.innerHTML=s.sensors.map((x,i)=>'Sensor '+(i+1)+': '+(x.valid?x.celsius+' C / '+x.fahrenheit+' F':'invalid')).join('<br>')||'No sensors';target.value=s.targetCelsius;mode.value=s.mode} setInterval(refresh,2000);refresh();
</script></body></html>)HTML");
    });
    server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
    server_.on("/api/mode", HTTP_POST, [this]() { handleMode(); });
    server_.on("/api/target", HTTP_POST, [this]() { handleTarget(); });
    server_.on("/api/coils", HTTP_POST, [this]() { handleCoils(); });
    server_.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
    server_.begin();
}

void KilnWebServer::handleClient() { server_.handleClient(); }

void KilnWebServer::sendStatus() {
    String json = "{\"mode\":\"" + String(controller_.mode() == KilnController::Mode::Automatic ? "automatic" : "manual") + "\",\"targetCelsius\":" + String(controller_.targetCelsius(), 1) + ",\"sensors\":[";
    for (size_t index = 0; index < controller_.sensorCount(); ++index) {
        if (index > 0) json += ",";
        const auto& reading = controller_.sample(index);
        json += "{\"valid\":" + String(reading.valid ? "true" : "false") + ",\"celsius\":" + String(reading.celsius, 1) + ",\"fahrenheit\":" + String(reading.fahrenheit(), 1) + "}";
    }
    json += "]}";
    server_.send(200, "application/json", json);
}

void KilnWebServer::handleMode() {
    const String mode = server_.arg("mode");
    controller_.setMode(mode == "automatic" ? KilnController::Mode::Automatic : KilnController::Mode::Manual);
    server_.send(200, "application/json", "{\"ok\":true}");
}

void KilnWebServer::handleTarget() {
    controller_.setTargetCelsius(server_.arg("celsius").toFloat());
    server_.send(200, "application/json", "{\"ok\":true}");
}

void KilnWebServer::handleCoils() {
    const size_t coil = server_.arg("coil").toInt() - 1;
    controller_.setManualCoil(coil, server_.arg("enabled").toInt() != 0, server_.arg("power").toFloat());
    server_.send(200, "application/json", "{\"ok\":true}");
}

void KilnWebServer::handleStop() {
    controller_.stop();
    server_.send(200, "application/json", "{\"ok\":true}");
}

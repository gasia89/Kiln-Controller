#include "KilnWebServer.h"

KilnWebServer::KilnWebServer(KilnController& controller, WiFiProvisioningManager& wifi)
    : controller_(controller), wifi_(wifi) {}

void KilnWebServer::begin() {
    server_.on("/", HTTP_GET, [this]() {
        if (wifi_.isSetupMode()) {
            server_.send(200, "text/html", R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Kiln Manager Setup</title><style>
body{font-family:system-ui,sans-serif;max-width:720px;margin:2rem auto;padding:0 1rem;background:#f4f1ea;color:#24231f}
main{background:white;padding:1.5rem;border:1px solid #d7d0c2;border-radius:8px}h1{margin-top:0}
label{display:block;margin-top:1rem;font-weight:600}input,select,button{font:inherit;padding:.55rem;margin-top:.35rem}
input,select{width:100%;box-sizing:border-box}button{cursor:pointer;margin-right:.4rem}
#networks{margin:1rem 0;padding:.8rem;background:#eee9df;min-height:2rem}
.network{display:block;width:100%;text-align:left;border:1px solid #d7d0c2;background:white}
</style></head><body><main><h1>Kiln Manager Setup</h1>
<p>Connect this device to a local Wi-Fi network. Setup access point: <strong id="setupSsid">loading</strong></p>
<button onclick="scan()">Scan for networks</button><div id="networks">No scan performed.</div>
<label>Selected SSID<input id="ssid" maxlength="32" autocomplete="off"></label>
<label>Wi-Fi password<input id="password" type="password" maxlength="63" autocomplete="off"></label>
<button onclick="connectWifi()">Connect</button><p id="message"></p>
</main><script>
async function status(){const s=await (await fetch('/api/wifi/status')).json();setupSsid.textContent=s.setupSsid+' ('+s.ip+')';message.textContent=s.state}
async function scan(){message.textContent='Scanning...';await fetch('/api/wifi/scan',{method:'POST'});let timer=setInterval(async()=>{const r=await (await fetch('/api/wifi/scan')).json();if(!r.scanning){clearInterval(timer);networks.innerHTML=r.networks.map(n=>'<button class="network" onclick="ssid.value=decodeURIComponent(\''+encodeURIComponent(n.ssid)+ '\')">'+(n.ssid||'(hidden)')+' ('+n.rssi+' dBm)'+(n.open?'':' [secured]')+'</button>').join('')||'No networks found.';message.textContent='Scan complete'}},1000)}
async function connectWifi(){message.textContent='Connecting...';const body=new URLSearchParams({ssid:ssid.value,password:password.value});const response=await fetch('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(!response.ok){message.textContent='Invalid SSID or password format';return}let timer=setInterval(async()=>{try{const s=await (await fetch('/api/wifi/status')).json();message.textContent=s.state==='connected'?'Connected at '+s.ip:s.state==='recovery_ap'?'Connection failed; setup AP remains available at '+s.ip:s.state;if(s.state==='connected'||s.state==='recovery_ap'){clearInterval(timer)} }catch(error){message.textContent='Waiting for connection result...'}},1000)} status();
</script></body></html>)HTML");
            return;
        }
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
    server_.on("/api/wifi/status", HTTP_GET, [this]() { sendWiFiStatus(); });
    server_.on("/api/wifi/scan", HTTP_GET, [this]() { sendWiFiScan(); });
    server_.on("/api/wifi/scan", HTTP_POST, [this]() { handleWiFiScan(); });
    server_.on("/api/wifi/connect", HTTP_POST, [this]() { handleWiFiConnect(); });
    server_.on("/api/wifi/reset", HTTP_POST, [this]() { handleWiFiReset(); });
    server_.begin();
}

void KilnWebServer::handleClient() { server_.handleClient(); }

void KilnWebServer::sendStatus() {
    String json = "{\"mode\":\"" + String(controller_.mode() == KilnController::Mode::Automatic ? "automatic" : "manual") + "\",\"targetCelsius\":" + String(controller_.targetCelsius(), 1) + ",\"wifiState\":\"" + wifi_.stateName() + "\",\"ip\":\"" + wifi_.ipAddress() + "\",\"sensors\":[";
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

void KilnWebServer::sendWiFiStatus() {
    String json = "{\"state\":\"" + String(wifi_.stateName()) + "\",\"ip\":\"" + wifi_.ipAddress() + "\",\"setupSsid\":\"" + wifi_.setupSsid() + "\",\"setupMode\":" + String(wifi_.isSetupMode() ? "true" : "false") + "}";
    server_.send(200, "application/json", json);
}

void KilnWebServer::sendWiFiScan() {
    server_.send(200, "application/json", "{\"scanning\":" + String(wifi_.scanInProgress() ? "true" : "false") + ",\"networks\":" + wifi_.scanResultsJson() + "}");
}

void KilnWebServer::handleWiFiScan() {
    const bool accepted = wifi_.startScan();
    server_.send(accepted ? 202 : 409, "application/json", accepted ? "{\"ok\":true}" : "{\"ok\":false}");
}

void KilnWebServer::handleWiFiConnect() {
    const bool accepted = wifi_.connect(server_.arg("ssid"), server_.arg("password"));
    server_.send(accepted ? 202 : 400, "application/json", accepted ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"invalid credentials\"}");
}

void KilnWebServer::handleWiFiReset() {
    wifi_.clearCredentials();
    server_.send(200, "application/json", "{\"ok\":true}");
}

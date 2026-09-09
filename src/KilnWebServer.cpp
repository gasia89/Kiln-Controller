#include "KilnWebServer.h"
#include <ArduinoJson.h>
#include "PreferencesHeatingProfileRepository.h"

namespace {
void copyArgument(WebServer& server, const char* name, char* destination, size_t capacity) {
    const String value = server.arg(name);
    strncpy(destination, value.c_str(), capacity - 1);
    destination[capacity - 1] = '\0';
}

void appendJsonString(String& json, const char* value) {
    String escaped(value == nullptr ? "" : value);
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    json += escaped;
}

const char* segmentTypeName(HeatingSegmentType type) {
    switch (type) {
    case HeatingSegmentType::Ramp: return "ramp";
    case HeatingSegmentType::Soak: return "soak";
    case HeatingSegmentType::Cooling: return "cooling";
    }
    return "unknown";
}

const char* profileValidationErrorName(HeatingProfileValidationError error) {
    switch (error) {
    case HeatingProfileValidationError::None: return "none";
    case HeatingProfileValidationError::EmptyName: return "profile name is required";
    case HeatingProfileValidationError::EmptySegmentName: return "segment name is required";
    case HeatingProfileValidationError::EmptyProfile: return "at least one segment is required";
    case HeatingProfileValidationError::TooManySegments: return "too many segments";
    case HeatingProfileValidationError::InvalidSegmentType: return "invalid segment type";
    case HeatingProfileValidationError::InvalidTarget: return "invalid target temperature";
    case HeatingProfileValidationError::InvalidRampRate: return "invalid ramp rate";
    case HeatingProfileValidationError::InvalidSoakDuration: return "invalid soak duration";
    case HeatingProfileValidationError::SoakMustFollowSegment: return "soak must follow another segment";
    case HeatingProfileValidationError::InvalidSegmentOrder: return "invalid segment duration";
    case HeatingProfileValidationError::ProfileTooLong: return "profile is too long";
    }
    return "invalid profile";
}

HeatingSegmentType segmentTypeFromName(const String& value) {
    if (value == "soak") return HeatingSegmentType::Soak;
    if (value == "cooling") return HeatingSegmentType::Cooling;
    return HeatingSegmentType::Ramp;
}
}

KilnWebServer::KilnWebServer(KilnController& controller, WiFiProvisioningManager& wifi, IHeatingProfileRepository& profiles)
    : controller_(controller), wifi_(wifi), profiles_(profiles) {}

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
.profile{border-top:1px solid #d7d0c2;margin-top:1.5rem;padding-top:1rem}.segment{border:1px solid #d7d0c2;margin:.6rem 0;background:#faf9f6}.segment-header{display:flex;justify-content:space-between;align-items:center;padding:.6rem;cursor:grab}.segment-body{padding:0 .6rem .6rem}.segment.collapsed .segment-body{display:none}.segment-toggle{font-weight:700;font-size:1.1rem}
</style></head><body><main><h1>Kiln Manager</h1>
<div id="readings">Waiting for readings...</div>
<label>Mode<select id="mode"><option value="manual">Manual</option><option value="automatic">Automatic</option></select></label>
<label>Target Celsius<input id="target" type="number" min="0" step="1" value="0"></label>
<button onclick="setMode()">Set mode</button><button onclick="setTarget()">Set target</button>
<section class="profile"><h2>Scheduled profiles</h2>
<label>Saved profile<select id="profileSelect" onchange="loadSelectedProfile()"><option value="">Create a new profile</option></select></label>
<label>Name<input id="profileName" maxlength="47" value="New profile"></label>
<label>Application<select id="profileApplication"><option>General</option><option>Pottery</option><option>Investment casting burnout</option><option>Metal heat treatment</option></select></label>
<label>Description<input id="profileDescription" maxlength="127"></label>
<div id="segments"></div>
<button onclick="addSegment('ramp')">Add ramp</button><button onclick="addSegment('soak')">Add soak</button><button onclick="addSegment('cooling')">Add cooling</button>
<p><button onclick="saveProfile()">Save profile</button><button onclick="startSelectedProfile()">Start selected profile</button><button onclick="deleteSelectedProfile()">Delete selected</button><button onclick="exportProfile()">Export selected</button></p><label>Import profile JSON<input id="profileFile" type="file" accept="application/json" onchange="importProfile()"></label><p id="profileMessage"></p></section>
<h2>Manual coils</h2><label>Coil 1 power %<input id="p1" type="range" min="0" max="100" value="0" oninput="v1.textContent=this.value"></label><span id="v1">0</span>%
<button onclick="setCoil(1,true)">Coil 1 on</button><button onclick="setCoil(1,false)">Coil 1 off</button>
<label>Coil 2 power %<input id="p2" type="range" min="0" max="100" value="0" oninput="v2.textContent=this.value"></label><span id="v2">0</span>%
<button onclick="setCoil(2,true)">Coil 2 on</button><button onclick="setCoil(2,false)">Coil 2 off</button>
<p><button class="stop" onclick="fetch('/api/stop',{method:'POST'})">EMERGENCY STOP</button></p>
</main><script>
const post=(u)=>fetch(u,{method:'POST'}); function setMode(){post('/api/mode?mode='+mode.value)}
function setTarget(){post('/api/target?celsius='+target.value)} function setCoil(c,on){post('/api/coils?coil='+c+'&enabled='+(on?1:0)+'&power='+document.getElementById('p'+c).value)}
let profileId=0;let draft=[];let draggedIndex=-1;function normalizeSoakTargets(){draft.forEach((s,i)=>{if(s.type==='soak'&&i>0)s.targetCelsius=draft[i-1].targetCelsius})}
function addSegment(type,collapsed=true){draft.push({name:type.charAt(0).toUpperCase()+type.slice(1)+' '+(draft.length+1),type:type,targetCelsius:type==='cooling'?100:1000,rateCelsiusPerHour:100,durationMinutes:type==='soak'?30:0,collapsed:collapsed});normalizeSoakTargets();renderSegments()}
function segmentDurationLabel(s,i){if(s.type==='soak')return 'Hold: '+s.durationMinutes+' minutes';const start=i>0?draft[i-1].targetCelsius:0;const hours=s.rateCelsiusPerHour>0?Math.abs(s.targetCelsius-start)/s.rateCelsiusPerHour:0;return (s.type==='cooling'?'Estimated cooling: ':'Estimated duration: ')+hours.toFixed(1)+' hours'}
function toggleSegment(i){draft[i].collapsed=!draft[i].collapsed;renderSegments()}
function beginDrag(i){draggedIndex=i}
function dropSegment(i){if(draggedIndex<0||draggedIndex===i)return;const moved=draft.splice(draggedIndex,1)[0];draft.splice(i,0,moved);draggedIndex=-1;normalizeSoakTargets();renderSegments()}
function renderSegments(){normalizeSoakTargets();segments.innerHTML=draft.map((s,i)=>{const soak=s.type==='soak';const target=i>0&&soak?draft[i-1].targetCelsius:s.targetCelsius;return '<div class="segment '+(s.collapsed?'collapsed':'')+'" ondragover="event.preventDefault()" ondrop="dropSegment('+i+')"><div class="segment-header" draggable="true" ondragstart="beginDrag('+i+')" onclick="toggleSegment('+i+')"><strong>'+s.name+'</strong><span class="segment-toggle">'+(s.collapsed?'+':'-')+'</span></div><div class="segment-body"><label>Name<input data-i="'+i+'" data-field="name" maxlength="39" value="'+s.name+'"></label><label>Type<select data-i="'+i+'" data-field="type"><option '+(s.type==='ramp'?'selected':'')+'>ramp</option><option '+(s.type==='soak'?'selected':'')+'>soak</option><option '+(s.type==='cooling'?'selected':'')+'>cooling</option></select></label><label>Target C<input data-i="'+i+'" data-field="targetCelsius" type="number" min="0" max="1372" step="1" value="'+target+'" '+(soak?'disabled':'')+'></label>'+(soak?'':'<label>Rate C/hour<input data-i="'+i+'" data-field="rateCelsiusPerHour" type="number" min="1" max="1000" step="1" value="'+s.rateCelsiusPerHour+'"></label>')+'<label>Soak minutes<input data-i="'+i+'" data-field="durationMinutes" type="number" min="0" max="10080" step="1" value="'+s.durationMinutes+'"></label><div>'+segmentDurationLabel(s,i)+'</div><button onclick="addSegmentAfter('+i+')">Add</button><button onclick="removeSegment('+i+')">Remove</button></div></div>'}).join('');segments.querySelectorAll('[data-field]').forEach((input)=>input.onchange=()=>{draft[input.dataset.i][input.dataset.field]=input.type==='number'?Number(input.value):input.value;renderSegments()})}
function addSegmentAfter(i){draft[i].collapsed=true;draft.splice(i+1,0,{name:'Ramp '+(i+2),type:'ramp',targetCelsius:1000,rateCelsiusPerHour:100,durationMinutes:0,collapsed:false});renderSegments()}
function removeSegment(i){draft.splice(i,1);renderSegments()}
async function loadProfiles(){const response=await fetch('/api/profiles');const payload=await response.json();if(!response.ok||!Array.isArray(payload))throw new Error(payload.error||'Unable to load profiles');profileSelect.innerHTML='<option value="">Create a new profile</option>'+payload.map((p)=>'<option value="'+p.id+'">'+p.name+' ('+Math.round(p.durationSeconds/60)+' min)</option>').join('');return payload}
async function loadSelectedProfile(){profileId=Number(profileSelect.value)||0;if(!profileId){profileName.value='New profile';profileDescription.value='';draft=[];renderSegments();return}const p=await (await fetch('/api/profile?id='+profileId)).json();profileName.value=p.name;profileApplication.value=p.application;profileDescription.value=p.description;draft=p.segments.map((s)=>({...s,collapsed:false}));renderSegments()}
async function saveProfile(){normalizeSoakTargets();const body=new URLSearchParams({id:profileId,name:profileName.value.trim(),application:profileApplication.value,description:profileDescription.value,segmentCount:draft.length});draft.forEach((s,i)=>{body.set('segment'+i+'_name',s.name.trim());body.set('segment'+i+'_type',s.type);body.set('segment'+i+'_targetCelsius',s.targetCelsius);body.set('segment'+i+'_rateCelsiusPerHour',s.rateCelsiusPerHour);body.set('segment'+i+'_durationMinutes',s.durationMinutes)});const response=await fetch('/api/profiles',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const result=await response.json();if(!result.ok){profileMessage.textContent='Profile rejected: '+(result.error||'unknown error');return}profileId=result.id;try{const items=await loadProfiles();if(!items.some((p)=>Number(p.id)===profileId)){throw new Error('saved profile was not returned by the device')}profileSelect.value=profileId;profileMessage.textContent='Profile saved'}catch(error){profileMessage.textContent='Profile saved, but list refresh failed: '+error.message}}
async function startSelectedProfile(){if(!profileId){profileMessage.textContent='Select a saved profile first';return}const response=await fetch('/api/profile-run/start?id='+profileId,{method:'POST'});profileMessage.textContent=response.ok?'Profile started':'Profile cannot start'}
async function deleteSelectedProfile(){if(!profileId)return;await fetch('/api/profiles?id='+profileId,{method:'DELETE'});profileId=0;await loadProfiles();loadSelectedProfile()}
function exportProfile(){if(profileId)window.location='/api/profile/export?id='+profileId}
async function importProfile(){const file=profileFile.files[0];if(!file)return;const response=await fetch('/api/profiles/import',{method:'POST',headers:{'Content-Type':'application/json'},body:await file.text()});const result=await response.json();profileMessage.textContent=result.ok?'Profile imported':'Import rejected';if(result.ok){profileId=result.id;await loadProfiles();profileSelect.value=profileId;await loadSelectedProfile()}}
async function refresh(){const s=await (await fetch('/api/status')).json();readings.innerHTML=s.sensors.map((x,i)=>'Sensor '+(i+1)+': '+(x.valid?x.celsius+' C / '+x.fahrenheit+' F':'invalid')).join('<br>')||'No sensors';target.value=s.targetCelsius;mode.value=s.mode} addSegment('ramp',false);loadProfiles();setInterval(refresh,2000);refresh();
</script></body></html>)HTML");
    });
    server_.on("/api/status", HTTP_GET, [this]() { sendStatus(); });
    server_.on("/api/profiles", HTTP_GET, [this]() { sendProfiles(); });
    server_.on("/api/profiles", HTTP_POST, [this]() { handleProfileSave(); });
    server_.on("/api/profiles/import", HTTP_POST, [this]() { handleProfileImport(); });
    server_.on("/api/profiles", HTTP_DELETE, [this]() { handleProfileDelete(); });
    server_.on("/api/profile", HTTP_GET, [this]() { sendProfile(); });
    server_.on("/api/profile/export", HTTP_GET, [this]() { sendProfileExport(); });
    server_.on("/api/profile-run/start", HTTP_POST, [this]() { handleProfileStart(); });
    server_.on("/api/profile-run/pause", HTTP_POST, [this]() { handleProfilePause(); });
    server_.on("/api/profile-run/resume", HTTP_POST, [this]() { handleProfileResume(); });
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
    const char* mode = controller_.mode() == KilnController::Mode::Automatic ? "automatic" : controller_.mode() == KilnController::Mode::Profile ? "profile" : "manual";
    String json = "{\"mode\":\"" + String(mode) + "\",\"targetCelsius\":" + String(controller_.targetCelsius(), 1) + ",\"profileState\":" + String(static_cast<int>(controller_.profileState())) + ",\"wifiState\":\"" + wifi_.stateName() + "\",\"ip\":\"" + wifi_.ipAddress() + "\",\"sensors\":[";
    for (size_t index = 0; index < controller_.sensorCount(); ++index) {
        if (index > 0) json += ",";
        const auto& reading = controller_.sample(index);
        json += "{\"valid\":" + String(reading.valid ? "true" : "false") + ",\"celsius\":" + String(reading.celsius, 1) + ",\"fahrenheit\":" + String(reading.fahrenheit(), 1) + "}";
    }
    json += "]}";
    server_.send(200, "application/json", json);
}

void KilnWebServer::sendProfiles() {
    static HeatingProfile profiles[PreferencesHeatingProfileRepository::kMaximumProfiles];
    size_t count = 0;
    const HeatingProfileRepositoryError result = profiles_.list(profiles, PreferencesHeatingProfileRepository::kMaximumProfiles, count);
    if (result != HeatingProfileRepositoryError::None) {
        server_.send(500, "application/json", "{\"ok\":false,\"error\":\"profile storage\"}");
        return;
    }
    String json = "[";
    for (size_t index = 0; index < count; ++index) {
        if (index > 0) json += ",";
        json += "{\"id\":" + String(profiles[index].id) + ",\"name\":\"";
        appendJsonString(json, profiles[index].name);
        json += "\",\"application\":\"";
        appendJsonString(json, profiles[index].application);
        json += "\",\"durationSeconds\":" + String(profiles[index].totalDurationSeconds(), 0) + "}";
    }
    json += "]";
    server_.send(200, "application/json", json);
}

void KilnWebServer::sendProfile() {
    HeatingProfile profile;
    const HeatingProfileRepositoryError result = profiles_.load(server_.arg("id").toInt(), profile);
    if (result != HeatingProfileRepositoryError::None) {
        server_.send(404, "application/json", "{\"ok\":false,\"error\":\"profile not found\"}");
        return;
    }
    String json = "{\"id\":" + String(profile.id) + ",\"name\":\"";
    appendJsonString(json, profile.name);
    json += "\",\"application\":\"";
    appendJsonString(json, profile.application);
    json += "\",\"description\":\"";
    appendJsonString(json, profile.description);
    json += "\",\"segments\":[";
    for (size_t index = 0; index < profile.segmentCount; ++index) {
        if (index > 0) json += ",";
        const HeatingSegment& segment = profile.segments[index];
        json += "{\"name\":\"";
        appendJsonString(json, segment.name);
        json += "\",\"type\":\"" + String(segmentTypeName(segment.type)) + "\",\"targetCelsius\":" + String(segment.targetCelsius, 1) + ",\"rateCelsiusPerHour\":" + String(segment.rateCelsiusPerHour, 1) + ",\"durationMinutes\":" + String(segment.durationMinutes) + "}";
    }
    json += "]}";
    server_.send(200, "application/json", json);
}

void KilnWebServer::sendProfileExport() {
    HeatingProfile profile;
    if (profiles_.load(server_.arg("id").toInt(), profile) != HeatingProfileRepositoryError::None) {
        server_.send(404, "application/json", "{\"ok\":false,\"error\":\"profile not found\"}");
        return;
    }
    JsonDocument document;
    document["format"] = "kilnmanager-profile";
    document["version"] = 1;
    JsonObject exported = document["profile"].to<JsonObject>();
    exported["name"] = profile.name;
    exported["application"] = profile.application;
    exported["description"] = profile.description;
    JsonArray segments = exported["segments"].to<JsonArray>();
    for (size_t index = 0; index < profile.segmentCount; ++index) {
        const HeatingSegment& segment = profile.segments[index];
        JsonObject value = segments.add<JsonObject>();
        value["name"] = segment.name;
        value["type"] = segmentTypeName(segment.type);
        value["targetCelsius"] = segment.targetCelsius;
        value["rateCelsiusPerHour"] = segment.rateCelsiusPerHour;
        value["durationMinutes"] = segment.durationMinutes;
    }
    String json;
    serializeJson(document, json);
    server_.sendHeader("Content-Disposition", "attachment; filename=kilnmanager-profile.json");
    server_.send(200, "application/json", json);
}

void KilnWebServer::handleProfileSave() {
    HeatingProfile profile;
    profile.id = server_.arg("id").toInt();
    copyArgument(server_, "name", profile.name, sizeof(profile.name));
    copyArgument(server_, "application", profile.application, sizeof(profile.application));
    copyArgument(server_, "description", profile.description, sizeof(profile.description));
    profile.segmentCount = min(static_cast<size_t>(server_.arg("segmentCount").toInt()), HeatingProfileLimits::kMaxSegments);
    for (size_t index = 0; index < profile.segmentCount; ++index) {
        const String prefix = "segment" + String(index) + "_";
        copyArgument(server_, (prefix + "name").c_str(), profile.segments[index].name, sizeof(profile.segments[index].name));
        profile.segments[index].type = segmentTypeFromName(server_.arg(prefix + "type"));
        profile.segments[index].targetCelsius = server_.arg(prefix + "targetCelsius").toFloat();
        profile.segments[index].rateCelsiusPerHour = server_.arg(prefix + "rateCelsiusPerHour").toFloat();
        profile.segments[index].durationMinutes = server_.arg(prefix + "durationMinutes").toInt();
    }
    const HeatingProfileRepositoryError result = profiles_.save(profile, profile.id != 0);
    if (result != HeatingProfileRepositoryError::None) {
        if (result == HeatingProfileRepositoryError::InvalidProfile) {
            const HeatingProfileValidationResult validation = profile.validate();
            server_.send(400, "application/json", "{\"ok\":false,\"error\":\"" + String(profileValidationErrorName(validation.error)) + "\"}");
        } else {
            server_.send(400, "application/json", "{\"ok\":false,\"error\":\"profile storage\"}");
        }
        return;
    }
    server_.send(200, "application/json", "{\"ok\":true,\"id\":" + String(profile.id) + "}");
}

void KilnWebServer::handleProfileImport() {
    JsonDocument document;
    const DeserializationError parseError = deserializeJson(document, server_.arg("plain"));
    const char* format = document["format"] | "";
    const int version = document["version"] | 0;
    if (parseError || strcmp(format, "kilnmanager-profile") != 0 || version != 1) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid profile document\"}");
        return;
    }

    JsonObject source = document["profile"].as<JsonObject>();
    JsonArray sourceSegments = source["segments"].as<JsonArray>();
    if (source.isNull() || sourceSegments.isNull() || sourceSegments.size() > HeatingProfileLimits::kMaxSegments) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid profile document\"}");
        return;
    }

    HeatingProfile profile;
    strncpy(profile.name, source["name"] | "", sizeof(profile.name) - 1);
    strncpy(profile.application, source["application"] | "General", sizeof(profile.application) - 1);
    strncpy(profile.description, source["description"] | "", sizeof(profile.description) - 1);
    profile.name[sizeof(profile.name) - 1] = '\0';
    profile.application[sizeof(profile.application) - 1] = '\0';
    profile.description[sizeof(profile.description) - 1] = '\0';
    profile.segmentCount = sourceSegments.size();
    for (size_t index = 0; index < profile.segmentCount; ++index) {
        JsonObject sourceSegment = sourceSegments[index].as<JsonObject>();
        profile.segments[index].type = segmentTypeFromName(String(sourceSegment["type"] | "ramp"));
        String defaultName = String(segmentTypeName(profile.segments[index].type)) + " " + String(index + 1);
        strncpy(profile.segments[index].name, sourceSegment["name"] | defaultName.c_str(), sizeof(profile.segments[index].name) - 1);
        profile.segments[index].name[sizeof(profile.segments[index].name) - 1] = '\0';
        profile.segments[index].targetCelsius = sourceSegment["targetCelsius"] | 0.0F;
        profile.segments[index].rateCelsiusPerHour = sourceSegment["rateCelsiusPerHour"] | 0.0F;
        profile.segments[index].durationMinutes = sourceSegment["durationMinutes"] | 0U;
    }
    const HeatingProfileRepositoryError result = profiles_.save(profile, false);
    if (result != HeatingProfileRepositoryError::None) {
        server_.send(400, "application/json", "{\"ok\":false,\"error\":\"profile rejected\"}");
        return;
    }
    server_.send(200, "application/json", "{\"ok\":true,\"id\":" + String(profile.id) + "}");
}

void KilnWebServer::handleProfileDelete() {
    const HeatingProfileRepositoryError result = profiles_.remove(server_.arg("id").toInt());
    server_.send(result == HeatingProfileRepositoryError::None ? 200 : 404, "application/json", result == HeatingProfileRepositoryError::None ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"profile not found\"}");
}

void KilnWebServer::handleProfileStart() {
    HeatingProfile profile;
    const bool loaded = profiles_.load(server_.arg("id").toInt(), profile) == HeatingProfileRepositoryError::None;
    const bool started = loaded && controller_.startProfile(profile, millis());
    server_.send(started ? 202 : 400, "application/json", started ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"profile cannot start\"}");
}

void KilnWebServer::handleProfilePause() {
    controller_.pauseProfile(millis());
    server_.send(200, "application/json", "{\"ok\":true}");
}

void KilnWebServer::handleProfileResume() {
    controller_.resumeProfile(millis());
    server_.send(200, "application/json", "{\"ok\":true}");
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

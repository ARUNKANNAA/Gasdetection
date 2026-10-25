 #include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

// Wi-Fi access point
const char* AP_SSID     = "GasMonitor";
const char* AP_PASSWORD = "12345678";

// Pin configuration
#define MQ2_PIN    A0
#define DHT_PIN    D2
#define DHT_TYPE   DHT11
#define BUZZER_PIN D5

// Settings
int  gasThreshold   = 400;      // raw ADC (0-1023), adjustable from web UI
const unsigned long WARMUP_MS = 20000;   // MQ2 warm-up (ideal: several minutes)
const unsigned long SENSOR_INTERVAL = 2000;

DHT dht(DHT_PIN, DHT_TYPE);
ESP8266WebServer server(80);

int   gasValue = 0;
float temperature = NAN;
float humidity = NAN;
bool  alarmActive = false;
bool  muted = false;
unsigned long lastRead = 0;
unsigned long lastBeep = 0;
bool  beepState = false;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Gas Monitor</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:18px;min-height:100vh;font-family:'Segoe UI',Roboto,sans-serif;color:#fff;text-align:center;
 background:linear-gradient(135deg,#0f2027,#203a43,#2c5364);background-attachment:fixed;transition:background .6s}
body.alarm{background:linear-gradient(135deg,#3a0d0d,#7a1212,#b71c1c);animation:flash 1s infinite alternate}
body.warm{background:linear-gradient(135deg,#2b1d0e,#5c3a0a,#8a5a00)}
@keyframes flash{from{filter:brightness(.85)}to{filter:brightness(1.2)}}
h2{margin:6px 0 2px;font-size:1.8em;letter-spacing:1px;
 background:linear-gradient(90deg,#00e5ff,#76ff03,#ffea00);-webkit-background-clip:text;background-clip:text;color:transparent}
.sub{opacity:.6;font-size:.85em;margin-bottom:14px}
.wrap{max-width:400px;margin:0 auto}
.card{background:rgba(255,255,255,.09);border:1px solid rgba(255,255,255,.15);
 border-radius:20px;padding:18px;margin:14px 0;box-shadow:0 8px 32px rgba(0,0,0,.35);backdrop-filter:blur(8px)}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:14px}
.grid .card{margin:0}
.lbl{font-size:.85em;opacity:.75;text-transform:uppercase;letter-spacing:1.5px}
.big{font-size:2.2em;font-weight:700;margin-top:6px}
.ico{font-size:1.8em}
.temp{background:linear-gradient(135deg,rgba(255,112,67,.55),rgba(255,64,129,.45))}
.hum{background:linear-gradient(135deg,rgba(0,229,255,.45),rgba(41,121,255,.5))}
.gauge{position:relative;width:200px;height:200px;margin:8px auto}
.gauge svg{transform:rotate(-90deg)}
.gauge .c{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center}
.gauge .n{font-size:2.8em;font-weight:800}
#arc{transition:stroke-dashoffset .6s,stroke .6s}
.pill{display:inline-block;padding:7px 18px;border-radius:30px;font-weight:700;margin-top:6px;transition:.4s}
.ok{background:#00c853;box-shadow:0 0 16px #00c853aa}
.bad{background:#ff1744;box-shadow:0 0 20px #ff1744;animation:pulse .6s infinite alternate}
.wm{background:#ff9100;box-shadow:0 0 16px #ff9100aa}
@keyframes pulse{to{transform:scale(1.1)}}
.bar{position:relative;height:12px;background:rgba(255,255,255,.15);border-radius:10px;margin:14px 0 6px;overflow:visible}
#fill{height:100%;width:0;border-radius:10px;background:linear-gradient(90deg,#00e676,#ffea00,#ff1744);transition:width .6s}
#mark{position:absolute;top:-5px;width:3px;height:22px;background:#fff;border-radius:2px;box-shadow:0 0 6px #fff}
.row{display:flex;gap:8px;justify-content:center;align-items:center;flex-wrap:wrap;margin-top:10px}
input{width:100px;padding:11px;border-radius:12px;border:2px solid rgba(255,255,255,.25);
 background:rgba(0,0,0,.3);color:#fff;font-size:1em;text-align:center;outline:none}
input:focus{border-color:#00e5ff}
button{padding:11px 22px;border:none;border-radius:12px;font-size:1em;font-weight:700;color:#fff;cursor:pointer;transition:.2s}
button:active{transform:scale(.94)}
.b1{background:linear-gradient(90deg,#00b0ff,#651fff)}
.b2{background:linear-gradient(90deg,#ff9100,#ff1744)}
</style></head><body id="body">
<div class="wrap">
<h2>&#128293; GAS MONITOR</h2>
<div class="sub">Wemos D1 mini &bull; MQ2 + DHT11</div>

<div class="card">
 <div class="lbl">Gas Level</div>
 <div class="gauge">
  <svg width="200" height="200" viewBox="0 0 120 120">
   <circle cx="60" cy="60" r="52" fill="none" stroke="rgba(255,255,255,.12)" stroke-width="10"/>
   <circle id="arc" cx="60" cy="60" r="52" fill="none" stroke="#00e676" stroke-width="10"
    stroke-linecap="round" stroke-dasharray="326.7" stroke-dashoffset="326.7"/>
  </svg>
  <div class="c"><div class="n" id="gas">--</div><div class="lbl">raw</div></div>
 </div>
 <div id="status" class="pill ok">--</div>
 <div class="bar"><div id="fill"></div><div id="mark"></div></div>
 <div class="lbl">Threshold line</div>
</div>

<div class="grid">
 <div class="card temp"><div class="ico">&#127777;&#65039;</div><div class="lbl">Temp</div><div class="big" id="t">--</div></div>
 <div class="card hum"><div class="ico">&#128167;</div><div class="lbl">Humidity</div><div class="big" id="h">--</div></div>
</div>

<div class="card">
 <div class="lbl">Alarm Threshold: <b id="th" style="color:#ffea00">--</b></div>
 <div class="row">
  <input type="number" id="nth" min="0" max="1023" placeholder="0-1023">
  <button class="b1" onclick="setTh()">Set</button>
 </div>
 <div class="row"><button class="b2" id="mb" onclick="mute()">&#128263; Mute</button></div>
</div>
</div>

<script>
const $=id=>document.getElementById(id);
function upd(){
 fetch('/data').then(r=>r.json()).then(d=>{
  $('gas').innerText=d.gas;
  $('t').innerText=d.temp===null?'--':d.temp.toFixed(1)+'\u00B0C';
  $('h').innerText=d.hum===null?'--':d.hum.toFixed(0)+'%';
  $('th').innerText=d.th;
  $('mb').innerHTML=d.muted?'&#128266; Unmute':'&#128263; Mute';
  var p=Math.min(d.gas/1023,1);
  $('arc').style.strokeDashoffset=326.7*(1-p);
  $('fill').style.width=(p*100)+'%';
  $('mark').style.left='calc('+(d.th/1023*100)+'% - 1px)';
  var col=d.warmup?'#ff9100':(d.alarm?'#ff1744':(d.gas>d.th*0.7?'#ffea00':'#00e676'));
  $('arc').style.stroke=col;
  var s=$('status');
  s.className='pill '+(d.warmup?'wm':(d.alarm?'bad':'ok'));
  s.innerText=d.warmup?'\u23F3 Warming up...':(d.alarm?'\u26A0\uFE0F GAS DETECTED!':'\u2705 Air normal');
  $('body').className=d.warmup?'warm':(d.alarm?'alarm':'');
 }).catch(()=>{});
}
function setTh(){fetch('/set?th='+$('nth').value).then(upd)}
function mute(){fetch('/mute').then(upd)}
setInterval(upd,1000);upd();
</script></body></html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleData() {
  String json = "{";
  json += "\"gas\":" + String(gasValue) + ",";
  json += "\"temp\":" + (isnan(temperature) ? String("null") : String(temperature, 1)) + ",";
  json += "\"hum\":"  + (isnan(humidity)    ? String("null") : String(humidity, 0)) + ",";
  json += "\"th\":" + String(gasThreshold) + ",";
  json += "\"alarm\":" + String(alarmActive ? "true" : "false") + ",";
  json += "\"muted\":" + String(muted ? "true" : "false") + ",";
  json += "\"warmup\":" + String(millis() < WARMUP_MS ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleSet() {
  if (server.hasArg("th")) {
    int v = server.arg("th").toInt();
    gasThreshold = constrain(v, 0, 1023);
  }
  server.send(200, "text/plain", "OK");
}

void handleMute() {
  muted = !muted;
  server.send(200, "text/plain", "OK");
}

void readSensors() {
  // average 5 ADC samples
  long sum = 0;
  for (int i = 0; i < 5; i++) { sum += analogRead(MQ2_PIN); delay(2); }
  gasValue = sum / 5;

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  bool warming = millis() < WARMUP_MS;
  alarmActive = !warming && (gasValue >= gasThreshold);
  if (!alarmActive) muted = false;   // re-arm when air is clear
}

void updateBuzzer() {
  if (alarmActive && !muted) {
    if (millis() - lastBeep >= 250) {
      lastBeep = millis();
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState);
    }
  } else {
    beepState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  dht.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());   // default 192.168.4.1

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);
  server.on("/mute", handleMute);
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastRead >= SENSOR_INTERVAL) {
    lastRead = millis();
    readSensors();
    Serial.printf("Gas:%d Temp:%.1f Hum:%.0f Alarm:%d\n",
                  gasValue, temperature, humidity, alarmActive);
  }

  updateBuzzer();
}
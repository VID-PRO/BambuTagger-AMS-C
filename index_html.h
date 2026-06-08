#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>BambuTagger AMS</title>
<link rel="icon" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgAgMAAAAOFJJnAAABhWlDQ1BJQ0MgcHJvZmlsZQAAKJF9kb9Lw0AcxV9bS6VUHawg4pChOrWLijiWKhbBQmkrtOpgcukvaNKQpLg4Cq4FB38sVh1cnHV1cBUEwR8g/gHipOgiJX4vKbSI8eC4D+/uPe7eAd5WjSlGXxxQVFPPJBNCvrAqBF7hRxAjGERUZIaWyi7m4Dq+7uHh612MZ7mf+3MMyEWDAR6BOM403STeIJ7dNDXO+8RhVhFl4nPiqE4XJH7kuuTwG+eyzV6eGdZzmXniMLFQ7mGph1lFV4hniCOyolK+N++wzHmLs1JrsM49+QtDRXUly3Wa40hiCSmkIUBCA1XUYCJGq0qKgQztJ1z8Y7Y/TS6JXFUwciygDgWi7Qf/g9/dGqXpKScplAD8L5b1MQEEdoF207K+jy2rfQL4noErteuvt4C5T9KbXS1yBAxtAxfXXU3aAy53gNEnTdRFW/LR9JZKwPsZfVMBGL4FgmtOb519nD4AOepq+QY4OAQmy5S97vLu/t7e/j3T6e8HrYRyvp7c8c0AAAAJUExURXIA83m/boC9efRkY8YAAAABdFJOUwBA5thmAAAAAWJLR0QAiAUdSAAAAL1JREFUGNNNkLEKg0AMhv8GHO52H0FR36SbCJHD6XASn+Lazb1XHG8R1Kds7kqLgZAvGZL/D3CJbXCp1sxTAs/cx5HmvWErUJhvYgsA9QJv6AAvzUqeXe5Au5qPNrMyL4GXslCuAppbK4B6AhkUDr6PUDpYBQiUgZw2Bs02KJsn4KVjgWLh+8idQbawVTwaqGeERwttfZv1coKMXrMgR2lFVcX9f2GIm5PUwpBL4omPOdlJBpNT9bOM87y+5AM/WTesHvLO9wAAAABJRU5ErkJggg==" type="image/svg+xml" />
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
background:#0d1117;color:#c9d1d9;min-height:100vh}
header{background:#161b22;border-bottom:1px solid #30363d;padding:12px 24px}
header .logo{display:flex;align-items:center;gap:10px}
header .logo svg{width:28px;height:28px;flex-shrink:0}
header h1{font-size:18px;color:#58a6ff}
nav{background:#161b22;border-bottom:1px solid #30363d;display:flex;gap:0}
nav a{padding:10px 20px;color:#8b949e;text-decoration:none;font-size:14px;
border-bottom:2px solid transparent;cursor:pointer}
nav a:hover{color:#c9d1d9}
nav a.active{color:#58a6ff;border-bottom-color:#58a6ff}
.container{max-width:800px;margin:0 auto;padding:20px}
.page{display:none}
.page.active{display:block}
.card{background:#161b22;border:1px solid #30363d;border-radius:6px;padding:20px;margin-bottom:16px}
.card h2{font-size:16px;margin-bottom:16px;color:#f0f6fc;border-bottom:1px solid #30363d;padding-bottom:8px}
.row{display:flex;flex-wrap:wrap;gap:16px}
.col{flex:1;min-width:200px}
label{display:block;font-size:12px;color:#8b949e;margin-bottom:4px}
input,select{width:100%;padding:8px 12px;background:#0d1117;border:1px solid #30363d;
border-radius:4px;color:#c9d1d9;font-size:14px}
input:focus,select:focus{outline:none;border-color:#58a6ff}
.btn{padding:8px 16px;border:none;border-radius:4px;font-size:14px;cursor:pointer}
.btn-primary{background:#238636;color:#fff}
.btn-primary:hover{background:#2ea043}
.btn-danger{background:#da3633;color:#fff}
.btn-secondary{background:#30363d;color:#c9d1d9}
.toggle{position:relative;display:inline-block;width:44px;height:24px}
.toggle input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
background:#30363d;border-radius:24px;transition:.3s}
.slider:before{position:absolute;content:"";height:18px;width:18px;left:3px;bottom:3px;
background:#c9d1d9;border-radius:50%;transition:.3s}
input:checked+.slider{background:#238636}
input:checked+.slider:before{transform:translateX(20px)}
.slot-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:600px){.slot-grid{grid-template-columns:1fr}}
.slot{border:1px solid #30363d;border-radius:6px;padding:12px}
.slot-title{font-weight:bold;margin-bottom:8px;font-size:14px}
.slot-title .led{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px}
.led-green{background:#3fb950}
.led-yellow{background:#d29922}
.led-red{background:#da3633}
.led-off{background:#30363d}
.led-blue{background:#58a6ff}
.slot-detail{font-size:12px;color:#8b949e;line-height:1.6}
.status-bar{display:flex;gap:16px;align-items:center;margin-top:12px;flex-wrap:wrap}
.status-dot{width:8px;height:8px;border-radius:50%;display:inline-block;margin-right:4px}
.actions{display:flex;gap:8px;margin-top:12px}
.toast{position:fixed;bottom:20px;right:20px;padding:12px 20px;border-radius:6px;
color:#fff;font-size:14px;opacity:0;transition:opacity .3s;z-index:1000}
.toast.show{opacity:1}
.toast-success{background:#238636}
.toast-error{background:#da3633}
footer{position:fixed;bottom:0;left:0;right:0;text-align:center;padding:6px;
font-size:10px;color:#484f58;background:#0d1117;border-top:1px solid #30363d}
body{padding-bottom:28px}
#otaOverlay{display:none;position:fixed;top:0;left:0;right:0;bottom:0;
background:rgba(0,0,0,0.85);z-index:9999;flex-direction:column;
justify-content:center;align-items:center;gap:16px}
#otaOverlay.active{display:flex}
#otaOverlay .spinner{width:48px;height:48px;border:4px solid #30363d;
border-top-color:#58a6ff;border-radius:50%;animation:spin .8s linear infinite}
#otaOverlay .msg{color:#c9d1d9;font-size:16px}
#otaOverlay .sub{color:#8b949e;font-size:13px}
#otaOverlay .bar{width:240px;height:8px;background:#30363d;border-radius:4px;overflow:hidden}
#otaOverlay .barFill{height:100%;background:#238636;transition:width .3s}
@keyframes spin{to{transform:rotate(360deg)}}
</style>
</head>
<body>
<header><div class="logo"><img style="vertical-align:middle" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgAgMAAAAOFJJnAAABhWlDQ1BJQ0MgcHJvZmlsZQAAKJF9kb9Lw0AcxV9bS6VUHawg4pChOrWLijiWKhbBQmkrtOpgcukvaNKQpLg4Cq4FB38sVh1cnHV1cBUEwR8g/gHipOgiJX4vKbSI8eC4D+/uPe7eAd5WjSlGXxxQVFPPJBNCvrAqBF7hRxAjGERUZIaWyi7m4Dq+7uHh612MZ7mf+3MMyEWDAR6BOM403STeIJ7dNDXO+8RhVhFl4nPiqE4XJH7kuuTwG+eyzV6eGdZzmXniMLFQ7mGph1lFV4hniCOyolK+N++wzHmLs1JrsM49+QtDRXUly3Wa40hiCSmkIUBCA1XUYCJGq0qKgQztJ1z8Y7Y/TS6JXFUwciygDgWi7Qf/g9/dGqXpKScplAD8L5b1MQEEdoF207K+jy2rfQL4noErteuvt4C5T9KbXS1yBAxtAxfXXU3aAy53gNEnTdRFW/LR9JZKwPsZfVMBGL4FgmtOb519nD4AOepq+QY4OAQmy5S97vLu/t7e/j3T6e8HrYRyvp7c8c0AAAAJUExURXIA83m/boC9efRkY8YAAAABdFJOUwBA5thmAAAAAWJLR0QAiAUdSAAAAL1JREFUGNNNkLEKg0AMhv8GHO52H0FR36SbCJHD6XASn+Lazb1XHG8R1Kds7kqLgZAvGZL/D3CJbXCp1sxTAs/cx5HmvWErUJhvYgsA9QJv6AAvzUqeXe5Au5qPNrMyL4GXslCuAppbK4B6AhkUDr6PUDpYBQiUgZw2Bs02KJsn4KVjgWLh+8idQbawVTwaqGeERwttfZv1coKMXrMgR2lFVcX9f2GIm5PUwpBL4omPOdlJBpNT9bOM87y+5AM/WTesHvLO9wAAAABJRU5ErkJggg=="><h1>BambuTagger-AMS <span style="font-size:10px;color:#8b949e" id="fwVersion"></span></h1></div></header>
<nav>
<a class="active" onclick="switchPage('status')">Status</a>
<a onclick="switchPage('printer')">Printer Config</a>
<a onclick="switchPage('wifi')">WiFi Config</a>
</nav>

<div class="container">

<div id="page-status" class="page active">
<div class="card">
<h2>Slot Status</h2>
<div class="slot-grid" id="slotGrid"></div>
<div id="bmeBox" style="text-align:center;padding:8px;font-size:18px;color:#c9d1d9;display:none">
<span style="font-size:24px;font-weight:bold" id="bmeTemp"></span>
<span style="color:#8b949e;font-size:12px" id="bmeHum"></span>
</div>
<div class="status-bar">
<div><span class="status-dot" id="wifiDot"></span><span id="wifiStatus">WiFi: --</span></div>
<div><span class="status-dot" id="mqttDot"></span><span id="mqttStatus">MQTT: --</span></div>
<div><span class="status-dot" id="printerDot"></span><span id="printerStatus">Printer: --</span><span id="bmeInline" style="font-size:12px;color:#8b949e;margin-left:8px"></span></div>
<div style="margin-left:auto"><span id="bmeData" style="font-size:12px;color:#8b949e"></span></div>
</div>
  <div class="actions">
  <button class="btn btn-primary" onclick="scanAll()">Scan All Slots</button>
  <button class="btn btn-secondary" onclick="sendAll()">Send to Printer</button>
  <button class="btn btn-secondary" onclick="syncPrinter()">Sync From Printer</button>
  <button class="btn btn-danger" id="otaBtn" onclick="updateFirmware()">Update Firmware</button>
  </div>
</div>

<div class="card">
<h2>Printer AMS Slots</h2>
<div id="printerAmsCards"><span style="color:#8b949e">Not synced — click Sync From Printer</span></div>
</div>
</div>

<div id="page-printer" class="page">
<div class="card">
<h2>Printer Connection</h2>
<div class="row">
<div class="col"><label>Printer IP</label><input type="text" id="printerIP" placeholder="192.168.1.100"></div>
<div class="col"><label>Port</label><input type="number" id="printerPort" value="8883"></div>
</div>
<div class="row" style="margin-top:12px">
<div class="col"><label>Access Code</label><input type="password" id="printerAccessCode" placeholder="Printer access code"></div>
<div class="col"><label>Serial Number</label><input type="text" id="printerSerial" placeholder="Printer serial"></div>
</div>
<div class="row" style="margin-top:12px">
<div class="col"><label>AMS Unit</label><select id="amsUnit"><option value="0">A</option><option value="1">B</option><option value="2">C</option><option value="3">D</option></select></div>
<div class="col"><label>Printer AMS Status</label><span id="amsDetected" style="font-size:12px">Not connected</span></div>
</div>
<div class="row" style="margin-top:12px">
<div class="col"><label>MQTT Enabled</label><label class="toggle"><input type="checkbox" id="mqttEnabled"><span class="slider"></span></label></div>
<div class="col"><label>MQTT TLS</label><label class="toggle"><input type="checkbox" id="mqttUseTLS"><span class="slider"></span></label></div>
<div class="col"><label>MQTT Update Interval (ms)</label><input type="number" id="mqttInterval" value="3000"></div>
</div>
<div class="actions"><button class="btn btn-primary" onclick="savePrinter()">Save Printer Settings</button></div>
</div>
</div>

<div id="page-wifi" class="page">
<div class="card">
<h2>WiFi Configuration</h2>
<div class="row">
<div class="col"><label>SSID</label><input type="text" id="wifiSSID" placeholder="WiFi network name"></div>
<div class="col"><label>Password</label><input type="password" id="wifiPassword" placeholder="WiFi password"></div>
</div>
<div class="row" style="margin-top:12px">
<div class="col"><label>Device Name</label><input type="text" id="deviceName" placeholder="BambuTagger-AMS"></div>
</div>
<div class="actions"><button class="btn btn-primary" onclick="saveWiFi()">Save WiFi Settings</button></div>
</div>
</div>

</div>

<div class="toast" id="toast"></div>

<script>
function switchPage(name){
var pages=document.getElementsByClassName('page');
for(var i=0;i<pages.length;i++)pages[i].classList.remove('active');
var links=document.querySelectorAll('nav a');
for(var j=0;j<links.length;j++)links[j].classList.remove('active');
document.getElementById('page-'+name).classList.add('active');
event.target.classList.add('active')}

function showToast(msg,ok){var t=document.getElementById('toast');t.textContent=msg;
t.className='toast '+(ok?'toast-success':'toast-error')+' show';
setTimeout(function(){t.className='toast'},3000)}

function getLedClass(info){if(!info.present)return'led-off';
if(info.tagReadSuccess)return'led-green';
return'led-yellow'}

function getPrinterLedClass(ps){
if(!ps.amsConnected)return'led-off';
if(ps.hasSpool)return'led-green';
return'led-yellow'
}

function updateSlots(data){
var g=document.getElementById('slotGrid'),h='';
var ps=data.printerSlots||[];
// index scanned tags by slot
var tagBySlot={};
for(var i=0;i<data.slots.length;i++){ var s=data.slots[i]; if(s.present) tagBySlot[s.slot]=s; }
for(var i=0;i<ps.length;i++){
var p=ps[i];
var t=tagBySlot[i];
var hasSpool=p.hasSpool;
var showColor=t&&t.colorHex?t.colorHex.substring(0,6):(p.hasSpool&&p.color?p.color.substring(0,6):null);
h+='<div class="slot">';
h+='<div class="slot-title"><span class="led '+(p.hasSpool?'led-green':'led-yellow')+'"></span>Slot '+(i+1)+' <button class="btn btn-secondary" style="font-size:10px;padding:2px 6px;margin-left:8px" onclick="readAmsSlot('+i+')">Read</button></div>';
h+='<div style="display:flex;align-items:center;gap:8px">';
h+='<div class="slot-detail" style="flex:1">';
if(p.hasSpool){
h+='Type: '+p.trayType+'<br>';
var sub = (t&&t.detailedType&&t.detailedType.length>0)?t.detailedType:p.trayType;
h+='Sub: '+sub+'<br>';
h+='Material: '+p.material+'<br>';
h+='Color: '+p.color;
}else{h+='AMS: empty'}
if(t){
h+='<br><span style="color:#58a6ff">Tag: ';
if(t.detailedType&&t.detailedType.length>0)h+=t.detailedType+' - ';
h+=t.materialType+'</span>';
h+=' &middot; '+t.colorHex;
if(t.totalGrams>0)h+=' &middot; '+t.remainingGrams+'g/'+t.totalGrams+'g';
}
h+='</div>';
if(showColor)h+='<div style="width:36px;height:36px;border-radius:4px;background:#'+showColor+';flex-shrink:0;border:1px solid #484f58"></div>';
h+='</div></div>'}
g.innerHTML=h;
document.getElementById('wifiStatus').textContent='WiFi: '+(data.wifiConnected?'Connected':'Disconnected');
document.getElementById('wifiDot').className='status-dot '+(data.wifiConnected?'led-green':'led-red');
document.getElementById('mqttStatus').textContent='MQTT: '+(data.mqttConnected?'Connected':'Disconnected');
document.getElementById('mqttDot').className='status-dot '+(data.mqttConnected?'led-green':'led-red');
document.getElementById('printerStatus').textContent='Printer: '+(data.printerOnline?'Online':'Offline');
document.getElementById('printerDot').className='status-dot '+(data.printerOnline?'led-green':'led-red');
if(data.bmeOk){
document.getElementById('bmeInline').innerHTML='&#9728; '+data.temperature.toFixed(1)+'C &#128167; '+data.humidity.toFixed(0)+'%';
}else{document.getElementById('bmeInline').textContent=''}
if(data.detectedAms){
var ams=data.detectedAms,labels=['A','B','C','D'];

// printer AMS cards - one per AMS unit
var printerHtml='';
for(var a=0;a<ams.units.length;a++){
var u=ams.units[a];
if(!u.connected)continue;
printerHtml+='<div style="border:1px solid #30363d;border-radius:6px;padding:12px;margin-bottom:8px">';
printerHtml+='<div style="font-weight:bold;font-size:13px;color:#58a6ff;margin-bottom:4px">'+labels[a];
if(u.temperature>0)printerHtml+=' <span style="font-size:10px;color:#8b949e">&#9728;'+u.temperature.toFixed(1)+'C &#128167;'+u.humidity.toFixed(0)+'%</span>';
printerHtml+='</div>';
printerHtml+='<div style="font-size:11px;color:#8b949e;margin-bottom:8px">';
if(!u.productName||!u.productName[0]){printerHtml+='AMS '+(u.productName||'')}
else printerHtml+=u.productName;
if(u.fwVer&&u.fwVer[0]){printerHtml+=' &middot; FW '+u.fwVer}
if(u.serial&&u.serial[0]){printerHtml+=' &middot; SN '+u.serial}
printerHtml+='</div>';
printerHtml+='<div style="display:grid;grid-template-columns:1fr 1fr;gap:8px">';
for(var t=0;t<u.trays.length;t++){
var tr=u.trays[t];
var hasSpool=tr.trayType&&tr.trayType.length>0;
printerHtml+='<div style="border:1px solid #30363d;border-radius:4px;padding:8px;font-size:12px;display:flex;align-items:center;gap:6px">';
printerHtml+='<span style="flex:1">SLOT'+(t+1)+': '+(hasSpool?('<span style="color:#3fb950">'+tr.trayType+'</span>'):'<span style="color:#8b949e">empty</span>');
if(hasSpool&&tr.material&&tr.material.length>0)printerHtml+=' <span style="color:#8b949e;font-size:10px">'+tr.material+'</span>';
var st=tagBySlot[t];
if(st&&st.detailedType&&st.detailedType.length>0)printerHtml+=' <span style="color:#58a6ff;font-size:10px">('+st.detailedType+')</span>';
printerHtml+='</span>';
if(hasSpool&&tr.color){printerHtml+='<span style="width:24px;height:24px;border-radius:4px;background:#'+tr.color.substring(0,6)+';flex-shrink:0;border:1px solid #484f58"></span>'}
printerHtml+='</div>'}
printerHtml+='</div></div>'}
document.getElementById('printerAmsCards').innerHTML=printerHtml||'<span style="color:#8b949e">No AMS data — click Sync From Printer</span>';

// AMS status dropdown
var status=[];
for(var a=0;a<ams.units.length;a++){
var u=ams.units[a];
status.push('AMS '+labels[a]+':'+(u.connected?'<span class="led-green" style="width:8px;height:8px;display:inline-block;border-radius:50%"></span>':'<span class="led-red" style="width:8px;height:8px;display:inline-block;border-radius:50%"></span>'));
if(u.connected){
var sel=document.getElementById('amsUnit');
for(var o=0;o<sel.options.length;o++)if(sel.options[o].value==a){sel.options[o].text=labels[a]+' (detected)'}
if(ams.configuredUnit==a&&!u.connected)sel.style.background='#3a1c1c'}
}
document.getElementById('amsDetected').innerHTML=status.join(' &nbsp; ');
if(ams.count==1&&ams.configuredUnit!=ams.units.find(function(u){return u.connected}).id){
document.getElementById('amsUnit').value=ams.units.find(function(u){return u.connected}).id}
}}

function fetchStatus(){fetch('/api/status').then(function(r){return r.json()}).then(updateSlots)
.catch(function(){console.log('Status fetch failed')})}

function scanAll(){fetch('/api/scan',{method:'POST'}).then(function(r){return r.json()})
.then(function(d){showToast('Scan triggered',true);fetchStatus()})
.catch(function(){showToast('Scan failed',false)})}

function sendAll(){fetch('/api/send',{method:'POST'}).then(function(r){return r.json()})
.then(function(d){showToast('Data sent to printer',true)})
.catch(function(){showToast('Send failed',false)})}

function syncPrinter(){fetch('/api/sync',{method:'POST'}).then(function(r){return r.json()})
.then(function(d){showToast(d.ok?'Syncing from printer...':'Sync failed: '+d.error,d.ok)})
.catch(function(){showToast('Sync failed',false)})}

function readAmsSlot(tray){fetch('/api/ams-get-rfid?tray='+tray,{method:'POST'}).then(function(r){return r.json()})
.then(function(d){showToast(d.ok?'Reading AMS slot...':'Failed: '+d.error,d.ok)})
.catch(function(){showToast('Read failed',false)})}

function updateFirmware(){
var b=document.getElementById('otaBtn');
if(b.textContent.indexOf('up to date')>=0)return;
if(!confirm('Update firmware from GitHub? Device will reboot.'))return;
b.disabled=true;b.textContent='Updating...';b.className='btn btn-secondary';
var ov=document.getElementById('otaOverlay');
ov.classList.add('active');
document.getElementById('otaOverlaySub').textContent='Downloading...';
var bar=document.getElementById('otaBar');
bar.style.width='10%';
fetch('/api/ota',{method:'POST'}).then(function(r){return r.json()})
.then(function(d){
  document.getElementById('otaOverlaySub').textContent='Waiting for reboot...';
  bar.style.width='30%';
  var pct=30,attempts=0;
  var sim=setInterval(function(){pct+=1;if(pct<95)bar.style.width=pct+'%'},800);
  var check=setInterval(function(){
    attempts++;
    fetch('/api/version').then(function(r){return r.json()}).then(function(v){
      clearInterval(sim);clearInterval(check);
      bar.style.width='100%';
      document.getElementById('otaOverlaySub').textContent='Device is back! Reloading...';
      setTimeout(function(){location.reload()},1500);
    }).catch(function(){});
    if(attempts>40){clearInterval(sim);clearInterval(check);
      ov.classList.remove('active');b.disabled=false;
      showToast('Device did not come back — try manual reboot',false)}
  },3000);
})
.catch(function(){ov.classList.remove('active');b.disabled=false;showToast('OTA failed',false)})}

function checkOta(){
fetch('/api/ota-check').then(function(r){return r.json()}).then(function(v){
var b=document.getElementById('otaBtn');
if(v.error){b.textContent='OTA: '+v.error;b.disabled=true;return}
if(v.newer){b.textContent='Update to '+v.latest;b.className='btn btn-primary'}
else{b.textContent='Firmware up to date ('+v.current+')';b.className='btn btn-secondary'}
})}

function saveWiFi(){var d={wifiSSID:document.getElementById('wifiSSID').value,
wifiPassword:document.getElementById('wifiPassword').value,
deviceName:document.getElementById('deviceName').value};
fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
body:JSON.stringify(d)}).then(function(r){return r.json()})
.then(function(j){showToast(j.ok?'Settings saved. Rebooting...':'Save failed',j.ok)})
.catch(function(){showToast('Save failed',false)})}

function savePrinter(){var d={printerIP:document.getElementById('printerIP').value,
printerPort:parseInt(document.getElementById('printerPort').value)||8883,
printerAccessCode:document.getElementById('printerAccessCode').value,
printerSerial:document.getElementById('printerSerial').value,
amsUnit:parseInt(document.getElementById('amsUnit').value)||0,
mqttEnabled:document.getElementById('mqttEnabled').checked,
mqttUseTLS:document.getElementById('mqttUseTLS').checked,
mqttInterval:parseInt(document.getElementById('mqttInterval').value)||3000};
fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},
body:JSON.stringify(d)}).then(function(r){return r.json()})
.then(function(j){showToast(j.ok?'Settings saved. Rebooting...':'Save failed',j.ok)})
.catch(function(){showToast('Save failed',false)})}

function loadConfig(){fetch('/api/config').then(function(r){return r.json()}).then(function(c){
document.getElementById('wifiSSID').value=c.wifiSSID||'';
document.getElementById('wifiPassword').value=c.wifiPassword||'';
document.getElementById('deviceName').value=c.deviceName||'';
document.getElementById('printerIP').value=c.printerIP||'';
document.getElementById('printerPort').value=c.printerPort||8883;
document.getElementById('printerAccessCode').value=c.printerAccessCode||'';
document.getElementById('printerSerial').value=c.printerSerial||'';
document.getElementById('mqttEnabled').checked=c.mqttEnabled||false;
document.getElementById('mqttUseTLS').checked=c.mqttUseTLS||false;
document.getElementById('mqttInterval').value=c.mqttUpdateIntervalMs||3000;
document.getElementById('amsUnit').value=c.amsUnit||0})}

loadConfig();
fetchStatus();
setInterval(fetchStatus,3000);
fetch('/api/version').then(function(r){return r.json()}).then(function(v){
document.getElementById('fwVersion').textContent='v'+v.version})
setTimeout(function(){checkOta()},5000)
</script>
<div id="otaOverlay"><div class="spinner"></div><div class="msg">Installing update...</div><div class="sub" id="otaOverlaySub"></div><div class="bar"><div class="barFill" id="otaBar" style="width:0%"></div></div></div>
<footer>&copy; 2026 by <a href="https://www.bambutagger.de" target="_blank" style="color:#484f58;text-decoration:none" onmouseover="this.style.color='#c9d1d9'" onmouseout="this.style.color='#484f58'">BambuTagger</a></footer>
</body>
</html>
)rawliteral";

#endif

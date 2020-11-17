var serverAddr;
var refreshQuery;
var webSocketUrl;
var state;
var webSocket;
var textSizeBugLoopId;

function connect() {
   webSocket = new WebSocket(webSocketUrl);
   webSocket.onopen = function() {
      console.debug('WebSocket connected.');
   };

   webSocket.onmessage = handleWebSocketMessage;

   webSocket.onclose = function(e) {
   if(!document.hidden){
      console.debug('WebSocket is closed. Reconnect will be attempted in 1 second.', e.reason);
      setTimeout(function() {
         refreshState();
         connect();
      }, 1000);
   }
  };

   webSocket.onerror = function(err) {
      console.error('Closing socket due to error.');
      disconnect();
   };
};

function renderSkeleton(){
  var webRoot = 'https://jackhiggins.ie/traffic-light-controller';

  var basicSkeleton = `
  <div class="grid-container">
      <div id="red">
            <div class="sensor infoText" id="red-sensor"></div>
         </div>
      <div id="green">
            <div class="sensor infoText" id="green-sensor"></div>
            <div class="infoText" id="playing-song"></div>
         </div>
    </div>
    <div id="circle-container">
        <div id="circle"></div>
    </div>
  `

  document.body.innerHTML = basicSkeleton;

  var appleIcon = document.createElement('link');
  appleIcon.rel = 'apple-touch-icon';
  appleIcon.sizes = '180x180';
  appleIcon.href = webRoot + '/favicon_io/apple-touch-icon.png?version=3b46ba220d050242c7f21fb268de7558a0c7d943';
  document.head.appendChild(appleIcon);

  var icon = document.createElement('link');
  icon.rel = 'icon';
  icon.type = 'image/png';
  icon.href = webRoot + '/favicon_io/favicon-32x32.png?version=3b46ba220d050242c7f21fb268de7558a0c7d943';
  document.head.appendChild(icon);

  var stylesheet = document.createElement('link');
  stylesheet.rel = 'stylesheet';
  stylesheet.type = 'text/css';
  stylesheet.href = webRoot + '/style.css?version=3b46ba220d050242c7f21fb268de7558a0c7d943';
  document.head.appendChild(stylesheet);

  document.title = "Traffic Light";
}

function app(){
    serverAddr = '/api';
    state = {
        'bpm' : 100,
        'party' : false,
        'red' : false,
        'green' : false,
        'redTemperature' : 0.0,
        'greenTemperature' : 0.0,
        'title': '',
        'album': '',
        'artist': ''
    };

    renderSkeleton();

    var supportsTouch = 'ontouchstart' in window || navigator.msMaxTouchPoints;
    var eventName = supportsTouch ? 'touchend' : 'click';

    $('circle').addEventListener(eventName, party);
    $('green').addEventListener(eventName, green);
    $('red').addEventListener(eventName, red);

    webSocketUrl = 'ws://' + window.location.host + ':81';
    refreshQuery = new XMLHttpRequest();
    
    textSizeBugLoopId = setInterval(textSizeBugFix, 100);

    refreshState();
    connect();
    document.addEventListener('visibilitychange', visibilityHandler);
    window.onunload = window.onbeforeunload = disconnect();
};

function textSizeBugFix(){
  var circleFontSize = window.getComputedStyle($('circle'), null).getPropertyValue('font-size');
  var expectedFontSize = "90px";

  if(circleFontSize === expectedFontSize){
    console.debug("Circle font size ok!");
    clearInterval(textSizeBugLoopId);
  }else{
    $('circle').style.fontSize = expectedFontSize;
    $('circle').style.display = 'none';
    $('circle').style.display = 'block';
  }

}

function disconnect(){
   if(webSocket.readyState === WebSocket.OPEN){
      webSocket.close();
   }
};

function visibilityHandler(){
   if(document.hidden){
      disconnect();
   }else{
      refreshState();
      connect();
   }
};

function handleWebSocketMessage(event){
   var messageContents = JSON.parse(event.data);
   
   for(var key of Object.keys(messageContents)){
      state[key] = messageContents[key];
   }

   updateScreen();
};

function updateScreen(){
  var circleDiv = $('circle');
  var greenDiv = $('green');
  var redDiv = $('red');
  var redSensorDiv = $('red-sensor');
  var greenSensorDiv = $('green-sensor');
  var playingSongDiv = $('playing-song');

	if(state['green']){
		greenDiv.className = 'bright-green';
	} else {
		greenDiv.className = 'dark-green';
	}
	if(state['red']){
		redDiv.className = 'bright-red';
	} else {
		redDiv.className = 'dark-red';
	}
  if(state['party']){
    circleDiv.className = 'party';
    circleDiv.innerHTML = '' + state['bpm'] + ' BPM';
  }else{
    circleDiv.className = 'noParty';
    circleDiv.innerHTML = 'Party Mode';
  }

  greenSensorDiv.innerHTML = "" + Math.round(state['greenTemperature']) + "°C";
  redSensorDiv.innerHTML = "" + Math.round(state['redTemperature']) + "°C";

  var artistLine = state.artist.length === 0 ? "" : "" + state["artist"] + "<br/>";
  var titleLine = state.title.length === 0 ? "" : "<i>" + state['title'] + "</i>" + "<br/>";
  var albumLine = state.album.length === 0 ? "" : "" + state["album"];
  playingSongDiv.innerHTML = artistLine + titleLine + albumLine;
};

function refreshState(){
  refreshQuery = new XMLHttpRequest();
  refreshQuery.open('GET', serverAddr + '/status', true);
  refreshQuery.onreadystatechange = function(){
    if (refreshQuery.readyState == XMLHttpRequest.DONE) {
      if (refreshQuery.status == 200) {
        state =  JSON.parse(refreshQuery.responseText);
        updateScreen();
      }
    }
  };
  refreshQuery.send();
};

function green(){
  refreshQuery.abort();
  var xhr = new XMLHttpRequest();
  if(state['green']){
    xhr.open('DELETE', serverAddr + '/green', true);
  }else{
    xhr.open('PUT', serverAddr + '/green', true);
  }
	
  xhr.send();
  state['green'] = !state['green'];
  updateScreen();
};

function red(){
	refreshQuery.abort();
	var xhr = new XMLHttpRequest();
	if(state['red']){
		xhr.open('DELETE', serverAddr + '/red', true);
	}else{
		xhr.open('PUT', serverAddr + '/red', true);
	}
	
  xhr.send();
  state['red'] = !state['red'];
  updateScreen();
};

function party(){
  refreshQuery.abort();
  if(state['party']){
    var xhr = new XMLHttpRequest();
    xhr.open('DELETE', serverAddr + '/party', true);
    xhr.send();
	} else {
    var xhr = new XMLHttpRequest();
    xhr.open('PUT', serverAddr + '/party', true);
    xhr.send();
	}
	state['party'] = !state['party'];
  updateScreen();
};

function $(elementId){
  return document.getElementById(elementId);
}

var serverAddr = '/api';
var refreshQuery = new XMLHttpRequest();
var webSocketUrl = 'ws://' + window.location.host + ':81';
var state = {
   'bpm' : 100,
   'party' : false,
   'red' : false,
   'green' : false,
   'redTemperature' : 0.0,
   'greenTemperature' : 0.0
};
var webSocket;

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

function app(){
   refreshState();
   connect();
   document.addEventListener('visibilitychange', visibilityHandler);
   window.onunload = window.onbeforeunload = disconnect();
};

function disconnect(){
   if(webSocket.readyState === WebSocket.OPEN){
      webSocket.close();
   }
};

function visibilityHandler(visibilityChange){
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
   var circleDiv = document.getElementById('circle');
   var greenDiv = document.getElementById('green');
   var redDiv = document.getElementById('red');
   var redSensorDiv = document.getElementById('red-sensor');
   var greenSensorDiv = document.getElementById('green-sensor');

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

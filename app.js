var serverAddr = '/api';
var greenOn = true;
var redOn = true;
var refreshQuery = new XMLHttpRequest();
var isParty = false;
var bpm = 100;

function setupLoop(){
	setInterval(refreshState, 500);
};

function updateColourBlocks(){
   var circleDiv = document.getElementById('circle');
   var greenDiv = document.getElementById('green');
   var redDiv = document.getElementById('red');

	if(greenOn){
		greenDiv.className = 'green';
	} else {
		greenDiv.className = 'green dark';
	}
	if(redOn){
		redDiv.className = 'red';
	} else {
		redDiv.className = 'red dark';
	}
   if(isParty){
      circleDiv.className = 'party';
      circleDiv.innerHTML = '' + bpm + ' BPM';
   }else{
      circleDiv.className = 'noParty';
      circleDiv.innerHTML = 'Party Mode';
   }
};

function refreshState(){
	refreshQuery = new XMLHttpRequest();
	refreshQuery.open('GET', serverAddr + '/status', true);
	refreshQuery.onreadystatechange = function(){
		if (refreshQuery.readyState == XMLHttpRequest.DONE) {
			if (refreshQuery.status == 200) {
				var result = JSON.parse(refreshQuery.responseText);
				greenOn = result.green;
				redOn = result.red;
            isParty = result.party;
            bpm = result.bpm;
				updateColourBlocks();
			}
		}
	};
	refreshQuery.send();
};

function green(){
	refreshQuery.abort();
	var xhr = new XMLHttpRequest();
	if(greenOn){
		xhr.open('DELETE', serverAddr + '/green', true);
	}else{
		xhr.open('PUT', serverAddr + '/green', true);
	}
	
		xhr.send();
		greenOn = !greenOn;
		updateColourBlocks();
};

function red(){
	refreshQuery.abort();
	var xhr = new XMLHttpRequest();
	if(redOn){
		xhr.open('DELETE', serverAddr + '/red', true);
	}else{
		xhr.open('PUT', serverAddr + '/red', true);
	}
	
		xhr.send();
		redOn = !redOn;
		updateColourBlocks();
};

function party(){
   refreshQuery.abort();
	if(isParty){
		var xhr = new XMLHttpRequest();
		xhr.open('DELETE', serverAddr + '/party', true);
	      xhr.send();
	} else {
		var xhr = new XMLHttpRequest();
		xhr.open('PUT', serverAddr + '/party', true);
		   xhr.send();
	}
	isParty = !isParty;
   updateColourBlocks();
};
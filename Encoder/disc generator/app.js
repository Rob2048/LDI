'use strict'

function init() {
	let websocket = new WebSocket('ws://192.168.3.15:5000/');
	// let canvas = document.getElementById('mainCanvas');
	// let ctx = canvas.getContext('2d');
	// let width = canvas.width;
	// let height = canvas.height;	
	// ctx.fillStyle = 'rgb(200, 200, 200)';
	// ctx.fillRect(0, 0, width, height);
	// var img = document.getElementById('imgpic');	
	// var canvas = document.getElementById('imgCanvas');
	// var c = canvas.getContext("2d");	
	// c.drawImage(img, 0, 0);

	let mainImg = document.getElementById('mainImage');

	websocket.onopen = (evt) => {
		console.log('WS open');
	};
	
	websocket.onclose = (evt) => {
		console.log('WS close');
		setTimeout(init, 1000);
	};
	
	websocket.onmessage = (evt) => {
		//console.log('WS message', evt.data);

		if (mainImg.src) {
			URL.revokeObjectURL(mainImg.src);
		}

		mainImg.src = URL.createObjectURL(evt.data);
		
		// let r = new FileReader(event);
		// r.onload = () => {
		// 	console.log('Read data: ' + event.target.result);
		// }

		// r.readAsArrayBuffer(evt.data);
	};
	
	websocket.onerror = (evt) => {
		console.log('WS error: ', evt);
	};
}

window.addEventListener("load", init, false);
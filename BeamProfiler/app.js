'use strict'

let lastMs = 0;

let ctx;
let imgUrlObject;

let imgOb = new Image();

function animate() {
	// let currentMs = new Date().getMilliseconds();
	// let deltaMs = currentMs - lastMs;
	// lastMs = currentMs;

	// console.log(deltaMs + " ms");

	ctx.fillStyle = 'rgb(200, 0, 0)';
	ctx.fillRect(10, 10, 50, 50);

	if (imgUrlObject) {
		ctx.drawImage(imgOb, 0, 0);
	}

	requestAnimationFrame(animate);
}

function init() {
	let websocket = new WebSocket('ws://192.168.3.16:5000/');
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

		let currentMs = Date.now();
		let deltaMs = currentMs - lastMs;
		lastMs = currentMs;
		console.log(deltaMs + " ms");

		if (imgUrlObject) {
			URL.revokeObjectURL(imgUrlObject);
		}

		imgUrlObject = URL.createObjectURL(evt.data);
		imgOb.src = imgUrlObject;
	};
	
	websocket.onerror = (evt) => {
		console.log('WS error: ', evt);
	};

	ctx = mainImg.getContext('2d');

	animate();
}

window.addEventListener("load", init, false);

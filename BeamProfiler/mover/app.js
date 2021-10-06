'use strict'

const net = require('net');

// Rest work space to 0: G10 L20 P1 X0 Y0 Z0

// Connect to the GRBL controller.

let grbl = new net.Socket();
grbl.connect(9800, '192.168.3.19', ()=> {
	console.log('Connected');
});

grbl.on('data', (data)=> {
	console.log('Received: ' + data);
	// client.destroy(); // kill client after server's response
});

client.on('close', function() {
	console.log('Connection closed');
});

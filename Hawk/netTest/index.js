'use strict'

const net = require('net');

const server = net.createServer((socket) => {
	  socket.write('goodbye\n');
});

server.on('error', (err) => {
	  throw err;
});

server.listen(5000, () => {
	console.log('server bound');
});
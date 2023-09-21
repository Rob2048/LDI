'use strict'

const { SerialPort } = require('serialport')

// Create a port
const port = new SerialPort({
  path: 'COM12',
  baudRate: 57600,
  autoOpen: false
});

port.open(function (err) {
  if (err) {
    return console.log('Error opening port: ', err.message)
  }
})

let recvByteCount = 0;

// Switches the port into "flowing mode"
port.on('data', function (data) {
	recvByteCount += data.length;
	console.log('Recv:', data.length, recvByteCount)
//   console.log('Data:', data)
})
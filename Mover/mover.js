'use strict'

const net = require('net');

//------------------------------------------------------------------------------------------------------------------------
// GRBL controller.
//------------------------------------------------------------------------------------------------------------------------

// Rest work space to 0: G10 L20 P1 X0 Y0 Z0

const wposRegex = /WPos:(-?\d+.\d+),(-?\d+.\d+),(-?\d+.\d+)/
const wcoRegex = /WCO:(-?\d+.\d+),(-?\d+.\d+),(-?\d+.\d+)/

let grblWpos = {
	x: 0,
	y: 0,
	z: 0
};

let grblWco = {
	x: 0,
	y: 0,
	z: 0
};

let grblState = 'idle';

let grblCallback = null;

let grbl = new net.Socket();

let grblLine = '';

grbl.connect(9800, '192.168.3.19', () => {
	console.log('GRBL connected');

	// grbl.write('G1 X100 Y0 Z0 F1000\n');

	setInterval(() => {
		grbl.write('?\n');
	}, 100);
});

grbl.on('data', (data) => {
	// console.log('Received: ' + data);

	for (let i = 0; i < data.length; ++i) {
		if (data[i] == 10) {
			// console.log('Line: ' + grblLine);
			processGrblLine(grblLine);
			grblLine = '';
		} else if (data[i] == 13) {

		} else {
			grblLine += String.fromCharCode(data[i]);
		}
	}
});

grbl.on('close', () => {
	console.log('GRBL connection closed');
});

function processGrblLine(line) {
	if (line.startsWith('<') && line.endsWith('>')) {
		// console.log('Status line');

		let trimmed = line.substr(1, line.length - 2);
		let entries = trimmed.split('|');

		for (let i = 0; i < entries.length; ++i) {
			let entry = entries[i];

			if (entry == 'Run') {
				if (grblState != 'run') {
					grblState = 'run';
					// console.log('Run');
				}

				continue;
			}

			if (entry == 'Idle') {
				if (grblState != 'idle') {
					grblState = 'idle';
					// console.log('Idle');
				}

				continue;
			}

			let match = entry.match(wposRegex);

			if (match != null) {
				let x = parseFloat(match[1]);
				let y = parseFloat(match[2]);
				let z = parseFloat(match[3]);

				if (grblWpos.x != x || grblWpos.y != y || grblWpos.z != z) {
					grblWpos.x = x;
					grblWpos.y = y;
					grblWpos.z = z;
					// console.log(grblWpos);
				}
				
				continue;
			}

			match = entry.match(wcoRegex);

			if (match != null) {
				let x = parseFloat(match[1]);
				let y = parseFloat(match[2]);
				let z = parseFloat(match[3]);

				if (grblWco.x != x || grblWco.y != y || grblWco.z != z) {
					grblWco.x = x;
					grblWco.y = y;
					grblWco.z = z;
					// console.log(grblWco);
				}

				continue;
			}
		}

		// Resolve any commands waiting.
		if (grblCallback != null) {
			if (grblCallback()) {
				grblCallback = null;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------
// Laser PWM controller.
//------------------------------------------------------------------------------------------------------------------------

let laser = new net.Socket();
let laserCallback = null;

laser.connect(9800, '192.168.3.16', () => {
	console.log('Laser connected');

	laser.write('b\n');
});

laser.on('data', (data)=> {
	// console.log('Laser received: ' + data);

	if (laserCallback != null) {
		let temp = laserCallback;
		laserCallback = null;
		temp();
	}
});

laser.on('close', ()=> {
	console.log('Laser connection closed');
});

//------------------------------------------------------------------------------------------------------------------------
// Overall controller.
//------------------------------------------------------------------------------------------------------------------------

function comparePosition(A, B) {
	return A >= B - 0.0001 && A <= B + 0.0001;
}

async function delay(timeMs) {
	// console.log('Delay');
	return new Promise((resolve, reject) => {
		setTimeout(resolve, timeMs);
	});
}

async function moveTo(x, y, z) {
	console.log('Move: ' + x + ', ' + y + ', ' + z);
	return new Promise((resolve, reject) => {
		grbl.write(`G1 X${x} Y${y} Z${z} F400\n`);
		
		grblCallback = () => {
			// console.log('Compare: ' + grblWpos.x + ', ' + grblWpos.y + ', ' + grblWpos.z);
			// console.log('To: ' + x + ', ' + y + ', ' + z);
			if (comparePosition(grblWpos.x, x) && comparePosition(grblWpos.y, y) && comparePosition(grblWpos.z, z)) {
				resolve();
				return true;
			};

			// console.log('Does not compare');

			return false;
		};
	});
}

async function laserPulseUs(timeUs) {
	// console.log('Pulse us');
	return new Promise((resolve, reject) => {
		laser.write(`c${timeUs}\n`);
		laserCallback = () => {
			resolve();
		};
	});
}

async function laserPulseMs(timeMs) {
	// console.log('Pulse ms');
	return new Promise((resolve, reject) => {
		laser.write(`d${timeMs}\n`);
		laserCallback = () => {
			resolve();
		};
	});
}

async function laserPulse(cycles, pulseUs, delayUs) {
	return new Promise((resolve, reject) => {
		laser.write(`e${cycles};${pulseUs};${delayUs}\n`);
		laserCallback = () => {
			resolve();
		};
	});
}

async function runTestProg() {
	console.log('Run test program.');

	// for (let iZ = 7; iZ < 8; ++iZ) {
	// 	for (let iY= 0; iY < 4; ++iY) {
	// 		for (let iX = 0; iX < 30; ++iX) {
	// 			await moveTo(iX * 0.2 + 32, iY * 0.2 + iZ, 5 - iZ);
	// 			await delay(100);
	// 			await laserPulseMs(25);
	// 		}
	// 	}
	// }

	for (let iY = 0; iY < 10; ++iY) {
		for (let iX = 0; iX < 10; ++iX) {
			await moveTo(iX * 0.2 + 9, iY * 0.5 + 1.0, 2.0);
			await delay(100);
			// await laserPulse(3000, 10, 5);
			await laserPulseUs(100 + iY * 50);
		}
	}


	// Best time: 25ms
	// Best focus: 109mm (Laser body)

	console.log('Test program complete.');
}

setTimeout(runTestProg, 1000);
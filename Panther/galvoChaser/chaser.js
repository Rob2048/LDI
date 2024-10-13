'use strict'

const timePerPixelUs = 1000;
const pixelCount = 800;
const dacCounts = 4096;
const updateTimeUs = 10;
const pixelSizeUm = 50;

const totalTime = pixelCount * timePerPixelUs;
const totalUpdates = totalTime / updateTimeUs;
const countsPerUpdate = dacCounts / totalUpdates;
const countsPerPixel = dacCounts / pixelCount;
const mmPerSecond = pixelSizeUm / timePerPixelUs * 1000;
const pixelsPerUpdate = pixelCount / totalUpdates;
const pixelsPerSecond = 1000000 / timePerPixelUs;
const pixelFrequency = 1000000 / timePerPixelUs;

console.log('Speed: ' + mmPerSecond + ' mm/s');
console.log('Counts per pixel: ' + countsPerPixel);
console.log('Total time: ' + totalTime + ' us');
console.log('Total updates: ' + totalUpdates);
console.log('Counts per update: ' + countsPerUpdate);
console.log('Pixels per update: ' + pixelsPerUpdate);
console.log('Pixels per second: ' + pixelsPerSecond);
console.log('Pixel frequency: ' + pixelFrequency);
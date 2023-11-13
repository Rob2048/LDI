import os
import sys
import time
import math
import board
import neopixel
import select

def pulseWave(offset, period, pulseTime):
	t = (time.time() + offset) % period
	pulse = 0

	if t < pulseTime:
		pulse = math.sin((math.pi / pulseTime) * t)

	return pulse

def circularRun():
	intensity = 10
	totalTime = 2.0
	div = totalTime / 8

	for i in range(8):
		pixels[i] = (0, 0, pulseWave(i * div, totalTime, 1.0) * intensity)


	pixels[5] = (0, pulseWave(0, 1, 0.5) * intensity, pixels[5][2])
	
	pixels.show()

def circularMulti():
	intensity = 5
	totalTime = 0.5
	div = totalTime / 8

	for i in range(8):
		pulse = pulseWave(i * div, totalTime, 0.3)

		# Get color for this light based on a rainbow
		r = ((i + 0) % 8) * 255 / 8
		g = ((i + 3) % 8) * 255 / 8
		b = ((i + 6) % 8) * 255 / 8

		r = r / (255 / intensity)
		g = g / (255 / intensity)
		b = b / (255 / intensity)
		
		pixels[i] = (r * pulse, g * pulse, b * pulse)

	pixels.show()

def blinkWhite(delayTime, onTime):
	intensity = 255
	totalTime = 1
	div = totalTime / 8

	pixels.fill((0, 0, 0))
	pixels.show()

	time.sleep(delayTime)
	
	for i in range(8):
		pixels[i] = (intensity, intensity, intensity)
		
	pixels.show()

	time.sleep(onTime)

	pixels.fill((0, 0, 0))
	pixels.show()

#--------------------------------------------------------------------------------------------
# App start.
#--------------------------------------------------------------------------------------------
pixels = neopixel.NeoPixel(board.D18, 8, auto_write=False)
pixels.fill((0, 0, 0))
pixels.show()

time.sleep(0.1)

fn = sys.stdin.fileno()
stdinFile = os.fdopen(fn)

programState = 0

update_fps = 30
update_time = 1 / update_fps
next_update_time = time.time() + update_time

while True:
	wait_time = next_update_time - time.time()

	if wait_time < 0:
		wait_time = 0
	
	readable, writable, exceptional = select.select([sys.stdin], [], [], wait_time)

	if not (readable or writable or exceptional):
		# Timeout happened.
		pass
	else:		
		# print("Select triggered")

		if sys.stdin in readable:
			# print("Got stdin input")
			_line = sys.stdin.readline()
			if _line:
				line = _line.strip().lower()
				# print("got line [{}]".format(line))

				args = line.split(" ")

				if len(args) == 1:
					if args[0] == "0":
						pixels.fill((0, 0, 0))
						pixels.show()
						programState = 0
					elif args[0] == "1":
						programState = 1
					elif args[0] == "2":
						programState = 2
					elif args[0] == "3":
						programState = 0
						blinkWhite(0.95, 0.05)
					elif args[0] == "4":
						programState = 0
						blinkWhite(0, 0.3)
					elif args[0] == "5":
						programState = 0
						blinkWhite(0.240, 0.04)
					elif args[0] == "6":
						programState = 0
						for i in range(8):
							pixels[i] = (255, 255, 255)
							pixels.show()
					elif args[0] == "9":
						print("Sync test: " + str(time.monotonic()))
						programState = 0
			# else:
			# 	print("No line")
			# 	readStr = sys.stdin.read(1)
			# 	print("No line: " + readStr)


	# Check if time to update.
	if time.time() >= next_update_time:
		next_update_time = time.time() + update_time

		if programState == 0:
			pass
		elif programState == 1:
			circularRun()
		elif programState == 2:
			circularMulti()

		# print("Time: " + str(time.time()) + " " + str(time.monotonic()))


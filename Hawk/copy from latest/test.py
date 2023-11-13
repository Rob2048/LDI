import time
import board
import neopixel
pixels = neopixel.NeoPixel(board.D18, 8)

def setLights(valR, valG, valB):
	pixels[0] = (valR, valG, valB)
	pixels[1] = (valR, valG, valB)
	pixels[2] = (valR, valG, valB)
	pixels[3] = (valR, valG, valB)
	pixels[4] = (valR, valG, valB)
	pixels[5] = (valR, valG, valB)
	pixels[6] = (valR, valG, valB)
	pixels[7] = (valR, valG, valB)

while True:
	pixels[0] = (1, 0, 0)
	pixels[1] = (5, 0, 0)
	pixels[2] = (1, 0, 0)
	# setLights(0, 0, 0)
	# time.sleep(1)
	# setLights(255, 255, 255)
	# time.sleep(1)


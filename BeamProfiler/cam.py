import time
import picamera
import socket
from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket

# List of frames captured by camera. Stores time and image data for each frame.
frameBuffer = []

#----------------------------------------------------------------------------------------------------------------
# Object used by picamera to process individual frames.
#----------------------------------------------------------------------------------------------------------------
class FrameCaptureOutput(object):
	def __init__(self):
		self.lastFrameTime = 0
		
	def write(self, frameData):
		global camera
		global frameBuffer
		
		try:			
			frameTime = camera.frame.timestamp
			
			# Detect if we have a valid frame.
			if frameTime == None:
				frameTime = 0
				return

			# Check how many frames in the buffer and remove some if we exceed 2 seconds of frames at 90 fps.
			while len(frameBuffer) >= 180:
				frameBuffer.pop(0)

			# Add the frame data and time as an entry to the frameBuffer array.
			#frameBuffer.append({ 'time': frameTime, 'data': frameData })

			data = list(bytearray(frameData))

			for client in clients:
				client.sendMessage(data)

			# Print some stats for the frame.
			diff = frameTime - self.lastFrameTime
			self.lastFrameTime = frameTime
			print('Frame {} {} {} {}'.format(camera.frame.index, diff, frameTime, len(frameData)))

		except Exception,e:
			print 'Camera error: ' + str(e)

#----------------------------------------------------------------------------------------------------------------
# Save frame data to a file using the frame's time as its name.
#----------------------------------------------------------------------------------------------------------------
def saveImage(frameBufferIndex):
	# Make sure the requested frame index exists in the frame buffer.
	if frameBufferIndex >= 0 and frameBufferIndex < len(frameBuffer):
		frameEntry = frameBuffer[frameBufferIndex]
		imgFile = open('frame_' + str(frameEntry['time']) + '.jpg', 'wb')
		imgFile.write(frameEntry['data'])
		imgFile.close()

#----------------------------------------------------------------------------------------------------------------
# Get 3 previous frames closest to the current time (adjusted by offset in milliseconds).
#----------------------------------------------------------------------------------------------------------------
def getFrames(timeOffset):
	captureTime = camera.timestamp + timeOffset	
	targetFrameIndex = -1

	while True:
		# Search the frame buffer to find frames taken at the requested time.
		for i in range(len(frameBuffer)):
			if frameBuffer[i]['time'] >= (captureTime):
				targetFrameIndex = i
				break

		if targetFrameIndex == -1:
			# Requested frame time has not been found, so wait for a bit (40ms).
			time.sleep(0.04)
		else:
			# Save the frame data to a file.
			saveImage(targetFrameIndex - 0)
			saveImage(targetFrameIndex - 1)
			saveImage(targetFrameIndex - 2)
			break

#----------------------------------------------------------------------------------------------------------------
# Setup the camera and begin recording.
#----------------------------------------------------------------------------------------------------------------
camera = picamera.PiCamera(sensor_mode = 1, clock_mode = 'raw')
camera.sensor_mode = 1
camera.resolution = (1080, 1920)
camera.framerate = 10
camera.rotation = 90
camera.shutter_speed = 50000
camera.iso = 800

camera.awb_mode = 'off'
camera.awb_gains = (1.1, 1.1)
#camera.exposure_mode = 'sports'

camera.start_recording(FrameCaptureOutput(), format='mjpeg', quality=0)

#----------------------------------------------------------------------------------------------------------------
# Websocket server.
#----------------------------------------------------------------------------------------------------------------
clients = []
class SimpleChat(WebSocket):

	def handleMessage(self):
		for client in clients:
			if client != self:
				client.sendMessage(self.address[0] + u' - ' + self.data)

	def handleConnected(self):
		print(self.address, 'connected')
		clients.append(self)

	def handleClose(self):
		clients.remove(self)
		print(self.address, 'closed')

server = SimpleWebSocketServer('', 5000, SimpleChat)
server.serveforever()

#----------------------------------------------------------------------------------------------------------------
# Loop just to keep program running.
#----------------------------------------------------------------------------------------------------------------
#while True:			
	#time.sleep(1.0)
	#saveImage(len(frameBuffer) - 1)
	
	
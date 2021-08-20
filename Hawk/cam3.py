import io
import socket
import time
import picamera
import socket
from socket import *
from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket
import Queue
import threading
import re
import sys
import struct
import traceback

# List of frames captured by camera. Stores time and image data for each frame.
frameBuffer = []

#----------------------------------------------------------------------------------------------------------------
# Net.
#----------------------------------------------------------------------------------------------------------------
netSend = Queue.Queue()
serverFile = None

#------------------------------------------------------------------------------
# Helper functions.
#------------------------------------------------------------------------------
def GetSerial():
	cpuserial = "00000000"

	try:
		f = open('/proc/cpuinfo','r')

		for line in f:
			if line[0:6]=='Serial':
				cpuserial = line[18:26]
		
		f.close()

	except:
		pass

	return int(cpuserial, 16)

#------------------------------------------------------------------------------
# Network Command Thread.
#------------------------------------------------------------------------------
class NetworkReaderThread(threading.Thread):
	def __init__(self):
		super(NetworkReaderThread, self).__init__()
		self.daemon = True
		self.broadcastSocket = None
		self.serverIp = None
		self.serverSocket = None
		self.start()

	def FindServer(self):
		# NOTE: We need to drain the socket as old broadcast msgs remain.
		self.broadcastSocket.setblocking(0)
		
		while True:
			try:
				recvData = self.broadcastSocket.recv(1024)
			except:
				break

		self.broadcastSocket.settimeout(0.5)

		while True:
			try:
				print('Waiting for Server IP')
				msg = self.broadcastSocket.recvfrom(1024)

				r = re.search('Server:(\d+\.\d+\.\d+\.\d+)', msg[0])
				
				if r != None:
					print 'Found Server IP: {} from {}'.format(r.group(1), msg[1])
					return r.group(1)
			except:
				continue

	def run(self):
		global setRunCamera
		global netSend
		global serverFile
		global camera
		global setCamFps
		global setCamMode
		global deviceSerial
		
		print('Starting Network Thread')

		self.broadcastSocket = socket(AF_INET, SOCK_DGRAM)
		self.broadcastSocket.bind(('', 45454))

		while True:
			self.serverSocket = socket()
			self.serverIP = self.FindServer()
			#blobdetect.masterconnectionmade(self.serverIP)
			#self.serverIP = '192.168.1.106'

			try:
				self.serverSocket.connect((self.serverIP, 8000))
				self.serverSocket.setsockopt(IPPROTO_TCP, TCP_NODELAY, 1)
			except:
				print('Cant Connect')
				continue

			serverFile = self.serverSocket.makefile('wb')
			# TODO: Need to clear this
			#netSend = Queue.Queue()
			print('Connected to Server')

			while True:
				# TODO: Catch exceptions here.
				try:
					data = serverFile.readline()
					#print('Socket Read ' + data)
					# Check all the things
					if not data:
						#blobdetect.masterconnectionlost()
						print('Socket Dead')
						setRunCamera = False
						serverFile = None
						break
					else:
						data = data.rstrip()
						args = data.split(',')
						
						if args[0] == 'sc':
							print('Net start cam')
							setRunCamera = True
						elif args[0] == 'ec':
							print('Net stop cam')
							setRunCamera = False
						elif args[0] == 'pe':
							print('Set exposure ' + args[1])
							camera.shutter_speed = int(args[1])
						elif args[0] == 'pi':
							print('Set ISO ' + args[1])
							camera.iso = int(args[1])
						elif args[0] == 'pf':
							print('Set FPS ' + args[1])
							setCamFps = int(args[1])
						elif args[0] == 'gi':
							configStr = json.dumps(config)
							data = struct.pack('IIII', 4, version, deviceSerial, len(configStr))
							netSend.put(data)
							netSend.put(configStr)
						elif args[0] == 'cm':
							print('Set camera mode ' + args[1])
							setCamMode = int(args[1])
						elif args[0] == 'sm':
							print('Set mask')
							config['mask'] = args[1]
							#blobdetect.setmask(args[1])
							SaveConfig()
						elif args[0] == 'sp':
							print('Set properties')
							config.update(json.loads(data[3:]))
							#blobdetect.setmask(config['mask'])
							SaveConfig()

				except Exception,e:
					#blobdetect.masterconnectionlost()
					print('Socket Dead: ' + str(e))
					setRunCamera = False
					serverFile = None
					break

class NetworkWriterThread(threading.Thread):
	def __init__(self):
		super(NetworkWriterThread, self).__init__()
		self.daemon = True
		self.start()

	def run(self):
		global setRunCamera
		#global netSend
		global serverFile

		print('Starting Network Writer Thread')

		while True:
			data = netSend.get()

			try:
				if serverFile != None:
					# print('Write..:' + str(netSend.qsize()))
					serverFile.write(data)
					# print('Written')
					# serverFile.flush()
			except Exception as e:
				print("Net Write Ex:" + str(e))
				#continue


NetworkReaderThread()
NetworkWriterThread()

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

			diff = frameTime - self.lastFrameTime
			self.lastFrameTime = frameTime

			print('{:>6} {:>6} {:>6}'.format(frameTime, diff, len(frameData)))

			# The first 1920 * 1088 bytes are the lum values.

			# Check how many frames in the buffer and remove some if we exceed 2 seconds of frames at 90 fps.
			# while len(frameBuffer) >= 180:
				# frameBuffer.pop(0)

			# Add the frame data and time as an entry to the frameBuffer array.
			#frameBuffer.append({ 'time': frameTime, 'data': frameData })

			# data = list(bytearray(frameData))

			# for client in clients:
			# 	client.sendMessage(data)

			# # Print some stats for the frame.
			# diff = frameTime - self.lastFrameTime
			# self.lastFrameTime = frameTime
			# print('Frame {} {} {} {}'.format(camera.frame.index, diff, frameTime, len(frameData)))

			packetSize = 1920 * 1080 + 1

			if netSend.qsize() == 0:
				packetHeader = struct.pack('IB', packetSize, 1)
				netSend.put(packetHeader)
				netSend.put(frameData[0:(1920 * 1080)])
			else:
				print("skip sending")

		except Exception,e:
			print 'Camera error: ' + str(e)
			traceback.print_exc()

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
camera.resolution = (1920, 1080)
camera.framerate = 20
camera.rotation = 180
camera.shutter_speed = 10000
camera.iso = 100
camera.hflip = True

camera.awb_mode = 'off'
camera.awb_gains = (1.0, 1.0)
#camera.exposure_mode = 'sports'

camera.start_recording(FrameCaptureOutput(), format='yuv')

#----------------------------------------------------------------------------------------------------------------
# Websocket server.
#----------------------------------------------------------------------------------------------------------------
# clients = []
# class SimpleChat(WebSocket):

# 	def handleMessage(self):
# 		for client in clients:
# 			if client != self:
# 				client.sendMessage(self.address[0] + u' - ' + self.data)

# 	def handleConnected(self):
# 		print(self.address, 'connected')
# 		clients.append(self)

# 	def handleClose(self):
# 		clients.remove(self)
# 		print(self.address, 'closed')

# server = SimpleWebSocketServer('', 5000, SimpleChat)
# server.serveforever()

#----------------------------------------------------------------------------------------------------------------
# Loop just to keep program running.
#----------------------------------------------------------------------------------------------------------------
while True:
	time.sleep(1.0)
	#saveImage(len(frameBuffer) - 1)
	
	
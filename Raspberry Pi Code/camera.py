import RPi.GPIO as GPIO
from time import sleep
import picamera
from os import listdir

#Get the next file index that is available for saving
def get_current_index():
	files = [f for f in listdir("/home/pi/selfdrivingcar/images")]
	print "Starting selfdrivingcar camera capture"
	ret = 0
	for f in files:
		last = f.split(".jpg")
		if (last[0].isdigit()):
			if (int(last[0]) > ret):
				ret = int(last[0])
	return ret + 1

index = get_current_index()
#Uses open-source picamera library
camera = picamera.PiCamera()

# use P1 header pin numbering convention
GPIO.setmode(GPIO.BOARD)

# Set up the GPIO channels - one input and one output
GPIO.setup(11, GPIO.IN)

#Takes a picture
def take_picture():
	global index
	print "Saving as " + str(index) + ".jpg"
	camera.capture("/home/pi/selfdrivingcar/images/" + str(index) + '.jpg')
	index += 1

#Infinite loop checks for positive edge, takes picture if it is detected, then resets
while (True):
	# Input from pin 11
	input_value = GPIO.input(11)
	if (input_value > 0):
		while (GPIO.input(11) > 0):
			sleep(0.05) 
		take_picture()
	sleep(0.05) #Minimum time between piectures is 100 ms
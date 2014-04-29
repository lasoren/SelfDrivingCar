import RPi.GPIO as GPIO
from time import sleep
import picamera
from os import listdir

index = get_current_index()
camera = picamera.PiCamera()

# use P1 header pin numbering convention
GPIO.setmode(GPIO.BOARD)

# Set up the GPIO channels - one input and one output
GPIO.setup(11, GPIO.IN)

def take_picture():
	camera.capture(str(index) + '.jpg')

def get_current_index():
	files = [ f for f in listdir("/home/pi/selfdrivingcar/images")]
	ret = 0
	for f in files:
		last = f[:-1]
		if (last.isdigit()):
			if (int(last) > ret):
				ret = int(last)
	return ret

while (true):
	# Input from pin 11
	input_value = GPIO.input(11)
	if (input_value > 0):
		while (GPIO.input(11) > 0):
			sleep(0.05)
		take_picture()
	sleep(0.05)
import os
import pygame
import platform
import serial

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

AXIS_STEETING_WHEEL = 0
AXIS_PEDAL_CLUTCH = 1
AXIS_PEDAL_BRAKE = 2
AXIS_PEDAL_ACCELERATE = 3

JOYSTICK_ID = 0

OPERATING_SYSTEM = platform.system()
if OPERATING_SYSTEM == "Linux":
    os.system("clear")
elif OPERATING_SYSTEM == "Windows":
    os.system("cls")
elif OPERATING_SYSTEM == "Darwin":
    print("The fuck run it on a mac ?")
    raise Exception("fuck mac")
else:
    raise Exception("Unknown os: " + str(OPERATING_SYSTEM))

print("Joystick-Bridge v1")
print("Running on: " + str(OPERATING_SYSTEM))

JOYSTICK = pygame.joystick.Joystick(JOYSTICK_ID)
JOYSTICK.init()


class Utils:

    @staticmethod
    def input_number(string):
        while True:
            try:
                return int(input(string))
            except:
                pass


class Communicator:

    def __init__(self, com, baud):
        self.com = com

        if baud == 0:
            baud = 9600

        self.baud = baud

        serial_path = "/dev/tty.usbserial"
        if OPERATING_SYSTEM == "Windows":
            serial_path = "\\.\COM" + str(com)

        self.serial = serial.Serial(serial_path, baud)

    def send(self, data):
        self.serial.write(data)


class Bridge:

    def __init__(self, communicator):
        self.communicator = communicator

    def task(self):
        pass


class AxedBridge(Bridge):

    def __init__(self, communicator, axis):
        Bridge.__init__(self, communicator)
        self.axis = axis
        self.old_value = -1

    def get_axis_value(self):
        return joystick.get_axis(self.axis)

    def send_rotation(self, rotation):
        request = self.format_request(rotation)

        if request != None and (self.old_value == -1 or self.old_value != rotation):
            self.old_value = rotation

            self.communicator.send(request)

    def format_request(self, value):
        return str(value)


class Pedal(AxedBridge):

    def __init__(self, communicator, axis):
        AxedBridge.__init__(self, communicator, axis)

    def task(self):
        value = self.get_axis_value()

        self.send_rotation(value)

    def get_axis_value(self):
        if self.axis not in [ AXIS_PEDAL_CLUTCH, AXIS_PEDAL_BRAKE, AXIS_PEDAL_ACCELERATE ]:
            return 0

        return super().get_axis_value()

    def format_request(self, value):
        if self.axis == AXIS_PEDAL_CLUTCH:
            if value > 0.5:
                return "down"
        elif self.axis == AXIS_PEDAL_BRAKE:
            if value > 0.1:
                return "center"
        elif self.axis == AXIS_PEDAL_ACCELERATE:
            if value > 0.5:
                return "up"

        return None


class SteeringWheel(AxedBridge):

    def __init__(self, communicator, axis):
        AxedBridge.__init__(self, communicator, axis)

    def task(self):
        rotation = self.get_axis_value()
        rotation += 1
        rotation *= 90
        rotation = int(round(rotation, 0) / 10)

    def format_request(self, value):
        return "rotation" + str(value)


communicator = Communicator(Utils.input_number("com? "), Utils.input_number("baud (0= 9600)? "))

steeringWheel = SteeringWheel(communicator, AXIS_STEETING_WHEEL)

clutchPedal = Pedal(communicator, AXIS_PEDAL_CLUTCH)
brakePedal = Pedal(communicator, AXIS_PEDAL_BRAKE)
acceleratePedal = Pedal(communicator, AXIS_PEDAL_ACCELERATE)

bridges = [ steeringWheel, clutchPedal, brakePedal, acceleratePedal ]


def loop():
    for bridge in bridges:
        bridge.task()

"""

Bellow are debug graphical code

"""


class TextPrint:

    def __init__(self):
        self.reset()
        self.font = pygame.font.Font(None, 20)

    def print(self, screen, textString):
        textBitmap = self.font.render(textString, True, BLACK)
        screen.blit(textBitmap, [self.x, self.y])
        self.y += self.line_height

    def reset(self):
        self.x = 10
        self.y = 10
        self.line_height = 15

    def indent(self):
        self.x += 10

    def unindent(self):
        self.x -= 10


pygame.init()

# Set the width and height of the screen [width,height]
size = [500, 700]
screen = pygame.display.set_mode(size)

pygame.display.set_caption("My Game")

# Loop until the user clicks the close button.
done = False

# Used to manage how fast the screen updates
clock = pygame.time.Clock()

# Initialize the joysticks
pygame.joystick.init()

# Get ready to print
textPrint = TextPrint()

while done == False:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            done = True

        # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
        if event.type == pygame.JOYBUTTONDOWN:
            print("Joystick button pressed.")
        if event.type == pygame.JOYBUTTONUP:
            print("Joystick button released.")

    screen.fill(WHITE)
    textPrint.reset()

    joystick_count = pygame.joystick.get_count()

    textPrint.print(screen, "Number of joysticks: {}".format(joystick_count))
    textPrint.indent()

    for i in range(joystick_count):
        joystick = pygame.joystick.Joystick(i)
        joystick.init()

        textPrint.print(screen, "Joystick {}".format(i))
        textPrint.indent()

        name = joystick.get_name()
        textPrint.print(screen, "Joystick name: {}".format(name))

        axes = joystick.get_numaxes()
        textPrint.print(screen, "Number of axes: {}".format(axes))
        textPrint.indent()

        for i in range(axes):
            axis = joystick.get_axis(i)
            textPrint.print(screen, "Axis {} value: {:>6.3f}".format(i, axis))
        textPrint.unindent()

        buttons = joystick.get_numbuttons()
        textPrint.print(screen, "Number of buttons: {}".format(buttons))
        textPrint.indent()

        for i in range(buttons):
            button = joystick.get_button(i)
            textPrint.print(screen, "Button {:>2} value: {}".format(i, button))
        textPrint.unindent()

        hats = joystick.get_numhats()
        textPrint.print(screen, "Number of hats: {}".format(hats))
        textPrint.indent()

        for i in range(hats):
            hat = joystick.get_hat(i)
            textPrint.print(screen, "Hat {} value: {}".format(i, str(hat)))
        textPrint.unindent()

        textPrint.unindent()

    loop()

    pygame.display.flip()
    clock.tick(20)

pygame.quit()

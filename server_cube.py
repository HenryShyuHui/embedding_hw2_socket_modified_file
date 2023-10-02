#!/usr/bin/env python

import signal
import threading

from OpenGL.GL import *
from OpenGL.GLU import *

import pygame
from pygame.locals import *

import socket
import json
import numpy as np
import matplotlib.pyplot as plot
HOST = '192.168.1.104' # IP address
PORT = 30000 # Port to listen on (use ports > 1023)

exit_event = threading.Event()

ax = ay = az = 0.0
yaw_mode = True

GL_EXIT = False

def init():
    glShadeModel(GL_SMOOTH)
    glClearColor(0.0, 0.0, 0.0, 0.0)
    glClearDepth(1.0)
    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LEQUAL)
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)

def resize(width, height):
    if height==0:
        height=1
    glViewport(0, 0, width, height)
    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(45, 1.0*width/height, 0.1, 100.0)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()

def drawText(position, textString):     
    font = pygame.font.SysFont ("Courier", 18, True)
    textSurface = font.render(textString, True, (255,255,255,255), (0,0,0,255))     
    textData = pygame.image.tostring(textSurface, "RGBA", True)     
    glRasterPos3d(*position)     
    glDrawPixels(textSurface.get_width(), textSurface.get_height(), GL_RGBA, GL_UNSIGNED_BYTE, textData)

def draw():
    global rquad
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
    
    glLoadIdentity()
    glTranslatef(0,0.0,-7.0)

    osd_text = "pitch: " + str("{0:.2f}".format(ay)) + ", roll: " + str("{0:.2f}".format(-ax))

    if yaw_mode:
        osd_line = osd_text + ", yaw: " + str("{0:.2f}".format(az))
    else:
        osd_line = osd_text

    drawText((-2,-2, 2), osd_line)

    # The way I'm holding the IMU board, X and Y axis are switched 
    # with respect to the OpenGL coordinate system
    if yaw_mode:                            # experimental
        glRotatef(az, 0.0, 1.0, 0.0)        # Yaw,   rotate around y-axis
    else:
        glRotatef(0.0, 0.0, 1.0, 0.0)

    glRotatef(-1*ax, 1.0, 0.0, 0.0)        # Roll,  rotate around x-axis
    glRotatef(ay, 0.0, 0.0, 1.0)           # Pitch, rotate around z-axis

    glBegin(GL_QUADS)	
    glColor3f(0.0,1.0,0.0)
    glVertex3f( 1.0, 0.2,-1.0)
    glVertex3f(-1.0, 0.2,-1.0)		
    glVertex3f(-1.0, 0.2, 1.0)		
    glVertex3f( 1.0, 0.2, 1.0)		

    glColor3f(1.0,0.5,0.0)	
    glVertex3f( 1.0,-0.2, 1.0)
    glVertex3f(-1.0,-0.2, 1.0)		
    glVertex3f(-1.0,-0.2,-1.0)		
    glVertex3f( 1.0,-0.2,-1.0)		

    glColor3f(1.0,0.0,0.0)		
    glVertex3f( 1.0, 0.2, 1.0)
    glVertex3f(-1.0, 0.2, 1.0)		
    glVertex3f(-1.0,-0.2, 1.0)		
    glVertex3f( 1.0,-0.2, 1.0)		

    glColor3f(1.0,1.0,0.0)	
    glVertex3f( 1.0,-0.2,-1.0)
    glVertex3f(-1.0,-0.2,-1.0)
    glVertex3f(-1.0, 0.2,-1.0)		
    glVertex3f( 1.0, 0.2,-1.0)		

    glColor3f(0.0,0.0,1.0)	
    glVertex3f(-1.0, 0.2, 1.0)
    glVertex3f(-1.0, 0.2,-1.0)		
    glVertex3f(-1.0,-0.2,-1.0)		
    glVertex3f(-1.0,-0.2, 1.0)		

    glColor3f(1.0,0.0,1.0)	
    glVertex3f( 1.0, 0.2,-1.0)
    glVertex3f( 1.0, 0.2, 1.0)
    glVertex3f( 1.0,-0.2, 1.0)		
    glVertex3f( 1.0,-0.2,-1.0)		
    glEnd()	

def worker_serial():
    global ax, ay, az
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print("Starting server at: ", (HOST, PORT))
        conn, addr = s.accept()
        with conn:
            print("Connected at", addr)
            # Complementary filter data
            roll = pitch = yaw = 0.0
            aver_roll = aver_pitch = aver_yaw = 0.0
            bias = 1000

            while True:
                try:
                    data = conn.recv(1024).decode('utf-8')
                    print("Received from socket server:", data)
                    if (data.count('{') != 1):
                        # Incomplete data are received.
                        choose = 0
                        buffer_data = data.split('}')
                        while buffer_data[choose][0] != '{':
                            choose += 1
                        data = buffer_data[choose] + '}'
                    obj = json.loads(data)

                    # Raw data

                    # ax = float(data[0])
                    # ay = float(data[1])
                    # az = float(data[2])

                    # x = float(data[3])
                    # y = float(data[4])
                    # z = float(data[5])

                    # Complementary filter data
                    roll = obj['x']
                    pitch = obj['y']
                    yaw = obj['z']

                    # if (obj['s'] < bias):
                    #     aver_roll = aver_roll + roll
                    #     aver_pitch = aver_pitch + pitch
                    #     aver_yaw = aver_yaw + yaw

                    #     ax = roll
                    #     ay = pitch
                    #     az = yaw
                    #     print ("fixed bias, %d", obj['s'])
                    
                    # else:
                    #     print ("finish bias, %d", obj['s'])
                    #     ax = roll - aver_roll/bias
                    #     ay = pitch - aver_pitch/bias
                    #     az = yaw - aver_yaw/bias

                    ax = roll
                    ay = pitch
                    az = yaw

                    # print("Acc: %f %f %f Gyr: %f %f %f Pos %f %f %f" % (ax, ay, az, x, y, z, roll, pitch, yaw))

                except Exception as e:
                    print(e)
                    pass

                if exit_event.is_set() or GL_EXIT:
                    data.close()
                    return

def signal_handler(signum, frame):
    exit_event.set()

if __name__ == '__main__':

    t1 = threading.Thread(target=worker_serial, daemon=True)
    t1.start()
   
    signal.signal(signal.SIGINT, signal_handler)

    video_flags = OPENGL|DOUBLEBUF

    pygame.init()

    screen = pygame.display.set_mode((1920,1080), video_flags)
    pygame.display.set_caption("Press Esc to quit, z toggles yaw mode")

    resize(1920, 1080)
    init()

    try:
        while True:
            event = pygame.event.poll()

            if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
                # Quit pygame properly
                pygame.quit()  

                GL_EXIT = True

                break      

            draw()
            pygame.display.flip()

    except KeyboardInterrupt:
        t1.join()

    print('Exiting main thread.')

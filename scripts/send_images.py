import socket
import time
from argparse import ArgumentParser
from threading import Event, Thread

import cv2 as cv

import numpy as np

import zmq


class Capture:

    def __init__(self, cap):
        self.cap = cap
        self.frame = None
        self.has_frame = Event()

    def run(self):
        while True:
            ret, self.frame = cap.read()
            self.has_frame.set()

    def get_frame(self):
        self.has_frame.clear()
        self.has_frame.wait()
        return self.frame


parser = ArgumentParser('Publish images over 0mq')
parser.add_argument('--no-capture', action='store_true',
                    help='If specified, avoid capturing images')
parser.add_argument('--width', type=int, default=800,
                    help='The image width (default: 800)')
parser.add_argument('--height', type=int, default=600,
                    help='The image height (default: 600)')
parser.add_argument('--port', type=int, default=10001,
                    help='Publisher port')
parser.add_argument('--device', type=int, default=0,
                    help='The video device to use (default: 0)')
parser.add_argument('--quality', type=int, default=95,
                    help='JPEG quality to use (default: 95)')

args = parser.parse_args()

if not args.no_capture:
    cap = cv.VideoCapture(args.device)
    cap.set(cv.CAP_PROP_FRAME_WIDTH, args.width)
    cap.set(cv.CAP_PROP_FRAME_HEIGHT, args.height)

capture = Capture(cap)
Thread(target=capture.run).start()

context = zmq.Context()
sock = context.socket(zmq.REP)
sock.bind(f'tcp://*:{args.port}')
print(f'Ready to serve images')

count = 0
while True:
    msg = sock.recv_json()
    quality = msg['quality']
    if args.no_capture:
        frame = np.zeros((args.height, args.width, 3), np.uint8)
    else:
        frame = capture.get_frame()
    now = time.time()
    ret, compressed = cv.imencode('.jpg', frame,
                                  [cv.IMWRITE_JPEG_QUALITY, quality])

    msg = {
        'time': now,
        'shape': frame.shape,
        'quality': quality,
        'data': compressed.tobytes()
    }
    sock.send_pyobj(msg)
    count += 1
    print(f'Sent {count} images')

import sys
import time
from argparse import ArgumentParser
from threading import Event, Thread

import cv2 as cv

import numpy as np

import zmq


class ImageCapturer:

    def __init__(self, sock, prefix, index):
        self.sock = sock
        self.prefix = prefix
        self.index = index
        self.frame = None
        self.frame_ready = Event()

        Thread(target=self.recv_images).start()

    def recv_images(self):
        while True:
            msg = sock.recv_pyobj()
            t = msg['time']
            shape = msg['shape']
            buf = msg['data']
            raw = np.frombuffer(buf, dtype=np.uint8)
            self.frame = cv.imdecode(raw, cv.IMREAD_UNCHANGED)
            self.frame_ready.set()

    def run(self):
        while True:
            self.frame_ready.clear()
            self.frame_ready.wait()
            self.show_image(self.frame)

    def show_image(self, frame):
        h, w = frame.shape[:2]
        copy = frame.copy()
        cv.line(copy, (w//2, 0), (w//2, h), (0,0,255), 1)
        cv.line(copy, (0, h//2), (w, h//2), (0,0,255), 1)
        cv.imshow('image', copy)
        key = cv.waitKey(0)
        if key == 27:
            sys.exit(1)
        if key == 's'.encode()[0]:
            path = f'{self.prefix}{self.index:03d}.png'
            cv.imwrite(path, frame)
            self.index += 1


parser = ArgumentParser('Capture images from 0mq publisher')
parser.add_argument('host', default='localhost',
                    help='Publisher hostname (default: localhost)')
parser.add_argument('--port', type=int, default=10002,
                    help='Publisher port (default: 10002)')
parser.add_argument('--save', action='store_true',
                    help='If specified, save images to files')
parser.add_argument('--quality', type=int, default=95,
                    help='JPEG quality to use for frames')
parser.add_argument('--start-index', type=int, default=1,
                    help='Starting index for images')
parser.add_argument('prefix',
                    help='Prefix for image files (default: images/img)')

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.SUB)
sock.set_string(zmq.SUBSCRIBE, '')
sock.connect(f'tcp://{args.host}:{args.port}')

ImageCapturer(sock, args.prefix, args.start_index).run()


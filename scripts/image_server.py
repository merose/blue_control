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
parser.add_argument('--frame-rate', type=int, default=5,
                    help='The frame rate in frames/sec (default: 5)')
parser.add_argument('--width', type=int, default=1280,
                    help='The image width (default: 1280)')
parser.add_argument('--height', type=int, default=720,
                    help='The image height (default: 720)')
parser.add_argument('--port', type=int, default=10002,
                    help='Publisher port (default: 10002)')
parser.add_argument('--device', type=int, default=0,
                    help='The video device to use (default: 0)')
parser.add_argument('--quality', type=int, default=95,
                    help='JPEG quality to use (default: 95)')
parser.add_argument('--crop-left', type=int, default=0,
                    help='Amount to crop on the left edge (default: 0)')
parser.add_argument('--crop-right', type=int, default=0,
                    help='Amount to crop on the right edge (default: 0)')
parser.add_argument('--scale', type=float, help='Scale factor for result image')

args = parser.parse_args()

if args.no_capture:
    cap = None
else:
    cap = cv.VideoCapture(args.device)
    cap.set(cv.CAP_PROP_FRAME_HEIGHT, args.height)
    cap.set(cv.CAP_PROP_FRAME_WIDTH, args.width)
    cap.set(cv.CAP_PROP_FPS, args.frame_rate)
    capture = Capture(cap)
    Thread(target=capture.run).start()

context = zmq.Context()
sock = context.socket(zmq.REP)
sock.bind(f'tcp://*:{args.port}')
print(f'Ready to serve images')

count = 0
while True:
    msg = sock.recv_json()
    print(f'Received image request - quality={msg["quality"]}')
    quality = msg['quality'] if 'quality' in msg else args.quality
    if args.no_capture:
        frame = np.zeros((args.height, args.width, 3), np.uint8)
        x, y = 100, args.height-100
        cv.putText(frame, f'Image {count+1}', (x, y), cv.FONT_HERSHEY_SIMPLEX,
                   5, (255, 255, 255), 3)
    else:
        frame = capture.get_frame()
    now = time.time()
    if args.crop_right > 0:
        frame = frame[:, args.crop_left:-args.crop_right, :]
    else:
        frame = frame[:, args.crop_left:, :]
    if args.scale:
        frame = cv.resize(frame, None, fx=args.scale, fy=args.scale)

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

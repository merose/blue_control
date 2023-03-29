import pickle
import time
from argparse import ArgumentParser

import cv2 as cv

import numpy as np

import zmq


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
sock = context.socket(zmq.REQ)
sock.connect(f'tcp://{args.host}:{args.port}')

index = args.start_index
while True:
    sock.send_json({'quality': args.quality})
    msg = sock.recv_pyobj()
    t = msg['time']
    shape = msg['shape']
    buf = msg['data']
    raw = np.frombuffer(buf, dtype=np.uint8)
    frame = cv.imdecode(raw, cv.IMREAD_UNCHANGED)

    cv.imshow('image', frame)
    key = cv.waitKey(0)
    if key == 27:
        break;
    if key == 's'.encode()[0]:
        path = f'{args.prefix}{index:03d}.png'
        cv.imwrite(path, frame)
        index += 1

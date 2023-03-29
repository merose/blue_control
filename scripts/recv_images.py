import pickle
import time
from argparse import ArgumentParser

import zmq


parser = ArgumentParser('Receive images from 0mq publisher')
parser.add_argument('host', default='localhost',
                    help='Publisher hostname (default: localhost)')
parser.add_argument('--port', type=int, default=10002,
                    help='Publisher port (default: 10002)')
parser.add_argument('--count', type=int, default=10,
                    help='Number of frames to receive')
parser.add_argument('--save', action='store_true',
                    help='If specified, save images to files')
parser.add_argument('--quality', type=int, default=95,
                    help='JPEG quality to use for frames')

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.REQ)
sock.connect(f'tcp://{args.host}:{args.port}')

start = time.time()
for i in range(args.count):
    sock.send_json({'quality': args.quality})
    msg = sock.recv_pyobj()
    t = msg['time']
    shape = msg['shape']
    buf = msg['data']
    print(f'time={t} length={len(buf)}')
    if args.save:
        with open(f'save_img_{i:05d}.raw', 'wb') as outfile:
            outfile.write(buf)

stop = time.time()
print(f'Elapsed={stop-start:.1f} FPS={args.count/(stop-start):.1f}')

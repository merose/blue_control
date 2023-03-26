"""Send a mobility request periodically."""

import struct
import time
from argparse import ArgumentParser

import zmq


parser = ArgumentParser('Show mobility monitoring messages')
parser.add_argument('--url', default="tcp://localhost:10000",
                    help="0MQ server URL (default: tcp://localhost:10000)");
parser.add_argument('--duration', type=float, default=5.0,
                    help='Duration of the request in seconds (default: 5)')
parser.add_argument('--interval', type=float, default=1.0,
                    help='Interval between commands in seconds (default: 1)')
parser.add_argument('left', type=int, help='Left motor target')
parser.add_argument('right', type=int, help='Right motor target')

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.PUB)
sock.connect(args.url)

start = time.time()
count = 0
while time.time() - start <= args.duration:
    msg = struct.pack('<ii', args.left, args.right)
    sock.send(msg)
    count += 1
    print(f'Sent {count} requests')
    time.sleep(args.interval)

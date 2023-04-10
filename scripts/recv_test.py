import pickle
import time
from argparse import ArgumentParser

import zmq


parser = ArgumentParser('Receive messages from 0mq publisher')
parser.add_argument('host', default='localhost',
                    help='Publisher hostname (default: localhost)')
parser.add_argument('--port', type=int, default=10000,
                    help='Publisher port (default: 10000)')

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.SUB)
sock.set_string(zmq.SUBSCRIBE, '')
sock.connect(f'tcp://{args.host}:{args.port}')

count = 0
start = None
while True:
    msg = sock.recv_multipart()
    now = time.time()
    if start is None:
        start = now
    stop = now
    count += 1
    #print(f'Message {count} chunk lengths: {[len(b) for b in msg]}')
    buf = msg[0]
    if buf[0] != 0:
        break

# Ignore the first message
count -= 1

print(f'Received {count} msgs in {stop-start:.1f} second, '
      + f'msg/sec={count/(stop-start):.1f}')

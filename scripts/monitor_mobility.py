"""Send a mobility request periodically."""

import struct
import time
from argparse import ArgumentParser

import zmq


parser = ArgumentParser('Show mobility monitoring messages')
parser.add_argument('--url', default="tcp://localhost:10001",
                    help="0MQ server URL (default: tcp://localhost:10001)");

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.SUB)
sock.set_string(zmq.SUBSCRIBE, '')
sock.connect(args.url)

start = time.time()
count = 0
while True:
    msg = sock.recv_json();
    now = time.time()
    count += 1
    rate = count / (now - start)
    print(f'left={msg["rate_left"]} right={msg["rate_right"]} '
          + f'battery={msg["battery_voltage"]} '
          + f'jack={msg["jack_voltage"]} msg rate={rate:.1f}/sec')

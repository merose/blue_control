"""Send a message and get a reply."""

from argparse import ArgumentParser

import zmq


parser = ArgumentParser('Send a request and show the reply')
parser.add_argument('--url', default="tcp://localhost:10000",
                    help="0MQ server URL (default: tcp://localhost:10000)");
parser.add_argument('message', help="The message to send");

args = parser.parse_args()

context = zmq.Context()
sock = context.socket(zmq.REQ)
sock.connect(args.url)

sock.send_string(args.message)
msg = sock.recv_string()

print(f'Received: {msg}')

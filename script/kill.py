#!/usr/bin/env python

import os, sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--machines', dest='machine_file', default='', 
  help='List of machine IPs. If not supplied, run locally.')

args = parser.parse_args()
assert args.machine_file != '', '--machines is required.'

prog_name = "hotbox_client_main"

# Get host IPs
with open(args.machine_file, "r") as f:
  hostlines = f.read().splitlines()
host_ips = [line.strip() for line in hostlines]

ssh_cmd = (
    "ssh "
    "-o StrictHostKeyChecking=no "
    "-o UserKnownHostsFile=/dev/null "
    )

for ip in host_ips:
  cmd = ssh_cmd + ip + " killall -q " + prog_name
  os.system(cmd)
print "Done killing"

#!/usr/bin/env python

from datetime import datetime

import ConfigParser
import argparse
import os
import subprocess
#import zmq

"""
def get_view(if_print):

    status = {}
    init_list = [0, 0, 0, 0]

    socket.connect(wrap_tcp(coordinator_endpoint))
    socket.send("0000")
    message = socket.recv()
    nodes = message.split(";")

    nodes = filter(lambda x: len(x) > 0, nodes)

    max_worker_id = 0
    max_server_id = 0

    for node in nodes:
        node_id, ip = node.split(",")
        ip = ip.split(":")[1][2:]

        index = int(node_id) % 10
        if status.has_key(ip):
            status.get(ip)[0] += 1
            status.get(ip)[index] += 1
        else:
            sub = init_list
            sub[0] += 1
            sub[index] += 1
            status[ip] = sub

        if is_worker(node_id) and node_to_id(node_id) > max_worker_id:
            max_worker_id = node_to_id(node_id)

        if is_server(node_id) and node_to_id(node_id) > max_server_id:
            max_server_id = node_to_id(node_id)

    if if_print:
        print "ip| driver server worker"
        for key in status.keys():
            sub = status.get(key)
            print key+"| "+str(sub[1])+" "+str(sub[2])+" "+str(sub[3]),

    return [max_server_id, max_worker_id, status]
"""

def do_add(args, config):
  with open(args.hostfile, 'r') as f:
    for line in f:
      line = line.split('#',1)[0].strip()
      if line:
        tokens = line.split()
        ip = tokens[0]
        num_sync_threads = tokens[1]
        num_table_threads = tokens[2]
        num_work_threads = tokens[3]
        command = get_ssh_command(ip)
        command += get_env(args, config)
        command.append(config.get('System', 'application_path'))
        command += get_common_args(args, config)
        command.append('--num_sync_threads=' + str(num_sync_threads))
        command.append('--num_table_threads=' + str(num_table_threads))
        command.append('--num_work_threads=' + str(num_work_threads))
        subprocess.Popen(command)
        print ' '.join(command)

def do_launch(args, config):
  master_ip = config.get('System', 'master_endpoint').split(':')[0]
  command = get_ssh_command(master_ip)
  command += get_env(args, config)
  command.append(config.get('System', 'application_path'))
  command += get_common_args(args, config)
  command.append('--master')
  command.append('--num_sync_threads=0')
  command.append('--num_table_threads=0')
  command.append('--num_work_threads=0')
  print ' '.join(command)
  subprocess.Popen(command)
  if args.hostfile:
    do_add(args, config)

def do_list(args, config):
  pass

def do_remove(args, config):
  pass

def get_common_args(args, config):
  ret = []
  if config.has_option('System', 'checkpoint_interval'):
    checkpoint_interval = config.get('System', 'checkpoint_interval')
    ret.append('--checkpoint_interval=' + checkpoint_interval)
  if config.has_option('System', 'checkpoint_path'):
    checkpoint_path = config.get('System', 'checkpoint_path')
    ret.append('--checkpoint_path=' + checkpoint_path)
  ret.append('--master_endpoint=tcp://' +
             config.get('System', 'master_endpoint'))
  ret.append('--min_table_threads=' +
             config.get('System', 'min_table_threads'))
  ret.append('--min_work_threads=' +
             config.get('System', 'min_work_threads'))
  ret.append('--num_table_partitions=' +
             config.get('System', 'num_table_partitions'))
  ret.append('--num_work_partitions=' +
             config.get('System', 'num_work_partitions'))
  if config.has_option('System', 'thread_report_interval'):
    ret.append('--thread_report_interval=' +
               config.get('System', 'thread_report_interval'))
  for key, val in config.items('Application'):
    ret.append('--' + key + '=' + val)
  return ret

def get_env(args, config):
  ret = []
  if config.has_option('System', 'log_dir'):
    log_dir = config.get('System', 'log_dir')
    ret.append('GLOG_log_dir=%s' % log_dir)
    if config.has_option('System', 'profiling'):
      if config.getboolean('System', 'profiling'):
        application_path = config.get('System', 'application_path')
        path = os.path.join(log_dir, os.path.basename(application_path))
        path += '.`hostname`'
        path += '.`whoami`'
        path += '.pprof.'
        path += datetime.now().strftime('%Y%m%d-%H%M%S.%f')
        ret.append('CPUPROFILE=%s' % path)
  else:
    ret.append('GLOG_logtostderr=1')
  return ret

def get_ssh_command(ip):
  return ['ssh', '-o', 'UserKnownHostsFile=/dev/null',
          '-o', 'StrictHostKeyChecking=no', ip]

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers(title='subcommands', dest='subparser')
  add_parser = subparsers.add_parser(
    'add', help='Add a process to a running application.')
  add_parser.add_argument(
    'configfile', type=str, help='Path to configuration file.')
  add_parser.add_argument(
      'hostfile', type=str, default='', help='Path to host file.')
  launch_parser = subparsers.add_parser(
    'launch', help='Launch master process for an application.')
  launch_parser.add_argument(
    'configfile', type=str, help='Path to configuration file.')
  launch_parser.add_argument(
    '--hostfile', type=str, default='', help='Path to host file.')
  list_parser = subparsers.add_parser(
    'list', help='List processes and threads for a running application.')
  list_parser.add_argument(
    'configfile', type=str, help='Path to configuration file.')
  remove_parser = subparsers.add_parser(
    'remove', help='Remove a process from a running application.')
  remove_parser.add_argument(
    'configfile', type=str, help='Path to configuration file.')
  remove_parser.add_argument(
    'proc_id', type=int, help="Process ID as listed by the 'list' command.")

  args = parser.parse_args()
  config = ConfigParser.SafeConfigParser()
  config.read(args.configfile)
  
  if args.subparser == 'add':
    do_add(args, config)
  elif args.subparser == 'launch':
    do_launch(args, config)
  elif args.subparser == 'list':
    do_list(args, config)
  elif args.subparser == 'remove':
    do_remove(args, config)

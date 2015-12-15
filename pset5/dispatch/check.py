#!/usr/bin/env python3

import re,os,sys,subprocess

class bcolors:
  HEADER = '\033[95m'
  OKBLUE = '\033[94m'
  OKGREEN = '\033[92m'
  WARNING = '\033[93m'
  FAIL = '\033[91m'
  ENDC = '\033[0m'
  BOLD = '\033[1m'
  UNDERLINE = '\033[4m'

def prepare(testfile, pattern):
  plan = dict()
  f = open(testfile, 'r')
  for line in f.readlines():
    m = pattern.match(line)
    plan[int(m.group(2))] = int(m.group(1))
  f.close()
  return plan

def validate(output, plan, pattern):
  lines = re.findall(pattern, output)
  if len(lines) != len(plan):
    return False
  for line in lines:
    cid = int(line[1])
    time = int(line[2])
    if (not (time in plan)) or (plan[time] != cid):
      return False
  return True

def write_debug_out(test, out):
  debug_file = 'debug/' + test + '.out'
  wf = open(debug_file, 'wb')
  wf.write(out)
  wf.close()
  return debug_file

def run_test(prog, test_dir, test, patterns, rep):
  testfile = test_dir + '/' + test
  plan = prepare(testfile, patterns[0])
  for i in range(0, rep):
    f = open(testfile, 'r')
    try:
      out = subprocess.check_output(prog, stdin=f, timeout=10)
    except subprocess.TimeoutExpired:
      f.close()
      df = write_debug_out(test, out)
      return bcolors.FAIL + 'timed out' + bcolors.ENDC
    f.close()
    if not validate(out, plan, patterns[1]):
      df = write_debug_out(test, out)
      return (bcolors.FAIL + 'failed' + bcolors.ENDC + ' Output saved in ' + df)
  return bcolors.OKGREEN + 'passed' + bcolors.ENDC

if __name__ == "__main__":
  if len(sys.argv) < 3:
    exit()
  prog = sys.argv[1]
  test_dir = sys.argv[2]
  rep = 10
  in_pattern = re.compile('(\d+) (\d+)')
  out_pattern = re.compile(b'Driver (\d+): Responding to customer (\d+) at time (\d+)')
  tests = sorted(os.listdir(test_dir))
  for t in tests:
    sys.stdout.write(t + '\t')
    print (run_test(prog, test_dir, t, [in_pattern, out_pattern], rep))

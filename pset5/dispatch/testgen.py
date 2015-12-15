#!/usr/bin/env python

import sys,random

def get_random_path():
  orig_x = random.uniform(-90.0, 90.0)
  orig_y = random.uniform(-180.0, 180.0)
  dst_x = random.uniform(-90.0, 90.0)
  dst_y = random.uniform(-180.0, 180.0)
  return "%.2f %.2f %.2f %.2f" % (orig_x, orig_y, dst_x, dst_y)

if __name__ == "__main__":
  test_len = 50
  if len(sys.argv) >= 2:
    test_len = int(sys.argv[1])

  for i in range(0, test_len):
    cid = random.randint(0,99)
    line = "%d %d " % (cid, i)
    line += get_random_path()
    print line

  exit()

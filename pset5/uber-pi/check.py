#!/usr/bin/env python3

import subprocess
import sys

print("Running tests....")
print("The more concurrency you are able to achieve, the larger the fraction\n of time drivers are driving, which makes everyone happy!\n")

print("Vanilla passenger version:")
try:
    subprocess.call('./uber-pi 0', shell=True, timeout=30)
except subprocess.TimeoutExpired:
    print("Process timed out after 30 seconds")
print("\nBetter initialization: ")
try:
    subprocess.call('./uber-pi 1', shell=True, timeout=30)
except subprocess.TimeoutExpired:
    print("Process timed out after 30 seconds")
print("\nBetter initialization and trylocking: ")
try:
    subprocess.call('./uber-pi 2', shell=True, timeout=30)
except subprocess.TimeoutExpired:
    print("Process timed out after 30 seconds")

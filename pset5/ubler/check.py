#!/usr/bin/env python3

import subprocess
import sys

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def test(cmd, desc):
    return (cmd, desc)

def run_tests(numMin=0, numMax=0, extra=False):
    tests = {
        1: test("./ubler_test 10 1 5", "basic functionality"),
        2: test("./ubler_test 50 5 5", "multiple drivers"),
        3: test("./ubler_test 100 20 5", "more requests and drivers"),
        4: test("./ubler_test 4000 100 5", "stress test"),
        5: test("./ubler_test 1 100 1", "many drivers, one request"),
        6: test("./ubler_test 1 1 1", "one request, one driver"),
        7: test("./ubler_test 10 1 5 --mix-up-meals", "mix up meals"),
        8: test("./ubler_test 10 5 5 --mix-up-meals", "mix up meals with more drivers"),
        9: test("./ubler_test 100 5 5 --mix-up-meals", "mix up meals stress test")
    }

    if numMin == 0 and numMax == 0:
        numMin = sorted(tests.keys())[0]
        numMax = sorted(tests.keys())[-1]
    elif numMin != 0 and numMax == 0:
        numMax = numMin

    if extra == False and numMax >= 6:
        numMax = 6

    numTests = 0
    numSucceeded = 0

    for key in range(numMin,numMax + 1):
        numTests = numTests + 1

        header = ("Test " + str(key) + ":\n")
        header += ("Command: " + tests[key][0] + "\n")
        header += ("Description: " + tests[key][1])
        print(bcolors.BOLD + header + bcolors.ENDC)
        passed = True
        for testNum in range(0, 20):
            output=""
            timeout = 2
            try:
                output = subprocess.check_output(tests[key][0], timeout=timeout, shell=True).decode("utf-8") 
            except subprocess.TimeoutExpired:
                output = "Timed out after " + str(timeout) + " seconds!"
            if output != "Success!\n":
                print("Test " + bcolors.FAIL + "failed" + bcolors.ENDC +
                    " on iteration " + str(testNum + 1) + " with error message:")
                print(bcolors.FAIL + output + bcolors.ENDC)
                passed = False;
                break;
        if passed:
            print(bcolors.OKGREEN + "Test passed!\n" + bcolors.ENDC)
            numSucceeded = numSucceeded + 1

    summary = (bcolors.BOLD + "Summary:\n" + bcolors.ENDC)
    summary += (str(numSucceeded) + " tests " + bcolors.OKGREEN + "succeeded, " +
        bcolors.ENDC + str(numTests - numSucceeded) + " tests " + bcolors.FAIL + 
        "failed. " + bcolors.ENDC + str(numTests) + " tests total.")
    print(summary)

if len(sys.argv) == 1:
    run_tests()
else:
    curIndex = 1
    extra = False
    if (sys.argv[curIndex] == "-e"):
        extra = True
        curIndex = 2
    if len(sys.argv) > curIndex:
        if "-" in sys.argv[curIndex]:
            run_tests(int(sys.argv[curIndex].split("-")[0]), int(sys.argv[curIndex].split("-")[1]), True)
        else:
            run_tests(int(sys.argv[curIndex]), 0, True)
    else:
        run_tests(0, 0, True)
    

#!/usr/bin/env python3
# cli.py - hacky arg template code using <getopt>
# https://github.com/DrAl-HFS/  ???Hacks.git
# Licence: GPL V3
# (c) Project Contributors August-Sept 2020

# TODO: assess <argparse> class for features
# including automated error handling.

import sys, getopt

def main (la):
    path= "~/default"
    n= 0
    sub= "000,001"
    verbose= False
    # Process args according to flag string
    # (colon signifies arg parameter expected)
    lopt,rem= getopt.getopt(la,"n:s:o:t")
    # Process list of (flag,value) pairs
    # (any remainder args ignored)
    for o,a in lopt:
        if '-n' == o:
            n= int(a)
        if '-s' == o:
            sub= a
        if '-o' == o:
            path= a
        if '-v' == o:
            verbose= True

    lsub= sub.split(',')
    li= list(map(int,lsub))
    print(path,n)
    print(lsub,'->',li)
    print(rem)
# main

if "__main__" == __name__ :
    main(sys.argv[1:])


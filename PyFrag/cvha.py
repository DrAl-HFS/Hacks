#!/usr/bin/env python3
# cvha.py
# OpenCV HSV image analysis

import sys
import cv2
import numpy

def dumpBH (bh):
    sum= 0
    i= 0
    for b in bh:
        if b > 0:
            print(hex(i),':',int(b))
            sum+= int(b)
        i+= 1
    return sum

def info (img,msk):
    sh= img.shape
    n= sh[0]*sh[1]
    print( type(img), sh, "n=", n)
    bh= cv2.calcHist([img],[0],msk,[256],[0,256])
    nh= dumpBH(bh)
    print("total:",nh,"(ex=",n-nh,")")
    return bh

def modelEM (bh,nc): # Not producing anything useful so far...
    #em= cv2.ml.EM(bh, nclusters=nc) #, covMatType=EM::COV_MAT_DIAGONAL,TermCriteria
    em= cv2.ml.EM_create()
    em.setClustersNumber(nc)
    em.trainEM(bh)
    print( em.getMeans() )

if "__main__" == __name__ :
    try:
        imBGR8= cv2.imread(sys.argv[1]) #, cv2.IMREAD_GRAYSCALE)
        imHSV8= cv2.cvtColor(imBGR8, cv2.COLOR_BGR2HSV)
        satMask= 0xFF * (imHSV8[:,:,1] > 0x1F).astype(numpy.uint8)
        bh= info(imHSV8[:,:,0], satMask) # imHSV8[:,:,2])
        modelEM(bh,10)
    except:
        print("Usage:",sys.argv[0],"<image-file>")

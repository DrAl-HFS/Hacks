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

def dumpFH (fh,v0,vs):
    sum= 0
    for f in bh:
        if f > 0:
            print(v0,':',f)
            sum+= f
        v0+= vs
    return sum

def info (img,msk,maxBin=256):
    sh= img.shape
    n= sh[0]*sh[1]
    print( type(img), sh, "n=", n)
    bh= cv2.calcHist([img],[0],msk,[maxBin],[0,maxBin])
    nh= dumpBH(bh)
    print("total:",nh,"(ex=",n-nh,")")
    return bh

def modelEM (bh,nc): # Not producing anything useful so far...
    #em= cv2.ml.EM(bh, nclusters=nc) #, covMatType=EM::COV_MAT_DIAGONAL,TermCriteria
    em= cv2.ml.EM_create()
    em.setClustersNumber(nc)
    em.trainEM(bh)
    print( em.getMeans() )

        #hist,bins= np.histogram(imH32)
        #for h,b in hist,bins:
        #    print(b,':',h)
        #try:
        #    h= np.bincount(imH32,minlength=400)
        #    print(h)
        #except:
        #    print("*ERR:", sys.exc_info()[0])
        #sh= imH8.shape
        #n= sh[0]*sh[1]
        #print(sh,n)
        #try:
        #    xh= cv2.calcHist([imH8],[0],None,[1<<16],[0,1<<16])
        #    nh= dumpBH(xh,0,1)
        #except: # cv2.error as e:
        #print("total:",nh,"(ex=",n-nh,")")
        #info(imHSV32[:,:,0],None)

def myHist (dat,mm,nBins):
    h= numpy.zeros( (nBins) )
    print("myHist() -",dat.shape, mm, "->", h.shape)
    step= (mm[1] - mm[0]) / (nBins - 1)
    rs= 1.0 / s
    print("- step=",step)
    for d in dat:
        i= int((d - mm[0]) * rs)
        h[i]+= 1
    return h, step

if "__main__" == __name__ :
    try:
        imBGR8= cv2.imread(sys.argv[1]) #, cv2.IMREAD_GRAYSCALE)
        #imHSV8= cv2.cvtColor(imBGR8, cv2.COLOR_BGR2HSV)
        #bh= info(imHSV8,None)
        #print("---")
        print("->f32")
        imBGR32= numpy.float32(imBGR8)
        imHSV32= cv2.cvtColor(imBGR32, cv2.COLOR_BGR2HSV)
        # NB: Conversion always generates H range 0.0, 360.0
        imH32= imHSV32[:,:,0]
        mm= ( numpy.min(imH32), numpy.max(imH32) )
        print("H range:", mm)
        fh,w= myHist(imH32, mm, 500)
        print("w=",w)
        dumpFH(fh,mm[0],w)
        print("---")
        imH8= numpy.uint8( imH32 * (0xFF / 360.0) )
        satMask= 0xFF * (imH8 > 0x1F).astype(numpy.uint8)
        bh= info(imH8, satMask)
        modelEM(bh,10)
    except:
        print("Usage:",sys.argv[0],"<image-file>")

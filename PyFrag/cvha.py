#!/usr/bin/env python3
# cvha.py
# OpenCV HSV image analysis

import sys
import cv2
import numpy
#from sklearn import mixture

def cfTest (x):
    from cffi import FFI # NB: install python3-cffi
    ffi= FFI()
    ffi.cdef("int normP1NIF (double r[], const int x[], const int n);")
    #"size_t strlen(const char*);")
    em= ffi.dlopen("/home/al/dev/Hacks/CFrag/em.so")
    r= numpy.zeros( (x.size), dtype=float)
    # TypeError: initializer for ctype 'double *' must be a cdata pointer, not numpy.ndarray
    n= em.normP1NIF(r, x, len(x))
    return r, n

from ctypes import *
def ctTest (x):
    em= CDLL("/home/al/dev/Hacks/CFrag/em.so")
    nrmf= em.normP1NIF
    nrmf.argtypes= [ POINTER(c_double), POINTER(c_int), c_int ]
    nrmf.restype= c_int
    r= numpy.zeros( (x.size), dtype=float)
    n= nrmf(r.ctypes.data_as(POINTER(c_double)), x.ctypes.data_as(POINTER(c_int)), x.size)
    return r, n

def em1DNF (h,nr):
    eml= CDLL("/home/al/dev/Hacks/CFrag/em.so")
    emf= eml.em1DNF
    emf.argtypes= [ POINTER(c_double), c_int, POINTER(c_double), c_int, c_int ]
    emf.restype= c_int
    r= numpy.zeros( nr*3, dtype=numpy.float64)
    #print("em1DNF() - r[", type(r[0]), "h[", type(h[0]) )
    nr= emf(r.ctypes.data_as(POINTER(c_double)), nr, h.ctypes.data_as(POINTER(c_double)), h.size, 0)
    lr= []
    if nr > 0:
        r= numpy.reshape(r,(-1,3))
        for i in range(nr):
            lr.append( tuple(r[i,:]) )
    return lr

#def ctTest (x):
#    import ctypes # FQN=tedious...
#    em= ctypes.CDLL("/home/al/dev/Hacks/CFrag/em.so")
#    nrmf= em.normP1NIF
#    nrmf.argtypes= [ ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_int), ctypes.c_int ]
#    nrmf.restype= ctypes.c_int
#    r= numpy.zeros( (n), dtype=float)
#    n= nrmf(r.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), x.ctypes.data_as(ctypes.POINTER(ctypes.c_int)), x.size)
#    return r, n

def sklModelEM (h,nc):
    em= mixture.GaussianMixture(n_components=nc, covariance_type='spherical') # Doesn't support 1D...
    em.fit(h)
    print(em)

def dumpBH (hist,hvl=1000000000):
    i= 0
    n= 0
    sum= 0
    for f in hist:
        if f > hvl:
            print(hex(i),':',f)
        n+= (f > 0)
        sum+= f
        i+= 1
    print("Occupancy:", ((100.0*n) / len(hist)),"%")
    return sum

def dumpFH (fh,v0,vs):
    sum= 0
    for f in bh:
        if f > 0:
            print(v0,':',f)
            sum+= f
        v0+= vs
    return sum

def cvHist (img,msk,maxBin=256):
    hist= cv2.calcHist([img],[0],msk,[maxBin],[0,maxBin])
    #x= hist[0][0] # numpy.float32 always?
    #print(type(x), x)
    n= img.shape[0] * img.shape[1]
    sh= dumpBH(hist,n/maxBin)
    #print("total:",nh,"(ex=",n-nh,")")
    return hist

def cvModelEM (h,nc): # Not producing anything useful so far...
    #em= cv2.ml.EM(bh, nclusters=nc) #, covMatType=EM::COV_MAT_DIAGONAL,TermCriteria
    em= cv2.ml.EM_create()
    em.setClustersNumber(nc)
    em.trainEM(h)
    print('M:', em.getMeans() )
    print('W:', em.getWeights() )

# No mask provision in this version
def npHist (dat,mm,nBins,hvl=1000000000):
    #hist= numpy.bincount(datI,minlength=nBins) # integral type only
    hist,binE= numpy.histogram(dat,bins=nBins,range=mm,density=True)
    # print(type(hist), type(hist[0]), hist[0]) = <numpy.int32> / <numpy.float64> when density=True
    i= 0
    n= 0
    for h,b in zip(hist,binE):
        if h>hvl:
            print(b,'[',i,']:',h)
        n+= (h > 0)
        i+= 1
    print("Occupancy:", ((100.0*n) / len(hist)),"%")
    return hist

def procCV (imBGR8):
    imHSV8= cv2.cvtColor(imBGR8, cv2.COLOR_BGR2HSV)
    #imH8= numpy.uint8( imH32 * (0xFF / 360.0) ) # 0~0xFF
    imH8= imHSV8[:,:,0] # 0 ~ 179
    satM8= 0xFF * (imHSV8[:,:,1] > 0x1F).astype(numpy.uint8)
    bh= cvHist(imH8, satM8)
    return bh

if "__main__" == __name__ :
    if len(sys.argv) > 1:
        imBGR8= cv2.imread(sys.argv[1]) #, cv2.IMREAD_GRAYSCALE)
    else:
        print("Usage:",sys.argv[0],"<image-file>")
        #imHSV8= cv2.cvtColor(imBGR8, cv2.COLOR_BGR2HSV)
        #bh= info(imHSV8,None)
    procCV(imBGR8)
    print("---")
    print("->f32")
    imHSV32= cv2.cvtColor(numpy.float32(imBGR8), cv2.COLOR_BGR2HSV)
    # NB: Conversion always generates H range 0.0, 360.0 irrespective of input scale.
    # (However S & V normed 0.0~1.0 as might be expected.)
    #print("Sat32:", numpy.min(imHSV32[:,:,1]), numpy.max(imHSV32[:,:,1]) )
    #satM8= 0xFF * (imHSV32[:,:,1] >= 0.125).astype(numpy.uint8)
    imH32= imHSV32[:,:,0]
    sh= imH32.shape
    nSh= sh[0] * sh[1]
    mm= ( numpy.min(imH32), numpy.max(imH32) )
    h= npHist(imH32.ravel(),mm,1000,nSh/100)
    print("---")
    lgm= em1DNF(h,12)
    print("em1DNF() - ", len(lgm))
    for m in lgm:
        print(m)
    if False:
        pmf, n= ctTest(h)
        print(n,pmf)
    if False:
        print("---")
        print("H range:", mm)
        fh,w= myHist(imH32, mm, 500)
        print("w=",w)
        dumpFH(fh,mm[0],w)
        print("---")
    #sklModelEM(h,10)
    #cvModelEM(h,10)

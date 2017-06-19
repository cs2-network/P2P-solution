package com.decoder.util;

public class H264Codec {
    public static native int InitCodec(byte bInOneFrameOnce);
    public static native void UninitCodec();

    public static native int H264Decode(byte[] out_bmp565, byte[] pRawData, int nRawDataSize, int[] out_4para);
    
    static {
        try {
            System.loadLibrary("H264Codec");
        } catch (UnsatisfiedLinkError ule) {
            System.out.println("loadLibrary(H264Codec)," + ule.getMessage());
        }
    }
}



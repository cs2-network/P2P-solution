package com.decoder.util;

public class AdpcmCodec {
    public native static int encode(byte[] pRaw, int nLenRaw, byte[] pDataEncoded);

    public native static int decode(byte[] pDataEncoded, int nLenDataEncoded, byte[] pRaw);

    static {
        try {
            System.loadLibrary("AdpcmCodec");
        } catch (UnsatisfiedLinkError ule) {
            System.out.println("loadLibrary(AdpcmCodec)," + ule.getMessage());
        }
    }
}

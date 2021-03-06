package org.hipacc.demo;

import android.graphics.Bitmap;

public class HipaccFilters {

    static {
        System.loadLibrary("filters");
    }

    public native int runRSBlur(Bitmap in, Bitmap out);
    public native int runFSBlur(Bitmap in, Bitmap out);
    public native int runRSGaussian(Bitmap in, Bitmap out);
    public native int runFSGaussian(Bitmap in, Bitmap out);
    public native int runRSLaplace(Bitmap in, Bitmap out);
    public native int runFSLaplace(Bitmap in, Bitmap out);
    public native int runRSSobel(Bitmap in, Bitmap out);
    public native int runFSSobel(Bitmap in, Bitmap out);
    public native int runRSHarris(Bitmap in, Bitmap out);
    public native int runFSHarris(Bitmap in, Bitmap out);
}

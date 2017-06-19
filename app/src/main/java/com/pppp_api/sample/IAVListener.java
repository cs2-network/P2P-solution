package com.pppp_api.sample;

import android.graphics.Bitmap;

public interface IAVListener {
    public void updateVFrame(Bitmap bmp);

    public void updateAVInfo(int codeInfo, int errCode, String strInfo);

}

package com.pppp_api.sample;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class Monitor extends SurfaceView implements SurfaceHolder.Callback, IAVListener {

    private SurfaceHolder surHolder = null;
    private final Paint m_videoPaint = new Paint();
    private Bitmap m_lastBmp = null;
    private Rect rectVideo = new Rect(0, 0, 10, 10);
    private int vWidth = 10, vHeight = 10;

    public Monitor(Context context, AttributeSet attrs) {
        super(context, attrs);

        surHolder = getHolder();
        surHolder.addCallback(this);
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (vWidth != width || vHeight != height) {
            vWidth = width;
            vHeight = height;

            synchronized (this) {
                rectVideo.set(0, 0, width, height);

                rectVideo.right = 3 * height / 4;
                rectVideo.offset((width - rectVideo.right) / 2, 0);
            }
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void drawVideo() {
        Canvas videoCanvas = null;
        synchronized (this) {
            videoCanvas = surHolder.lockCanvas(null);
            if (videoCanvas != null) {
                videoCanvas.drawColor(Color.BLACK);
                if (m_lastBmp != null)
                    videoCanvas.drawBitmap(m_lastBmp, null, rectVideo, m_videoPaint);
                surHolder.unlockCanvasAndPost(videoCanvas);
                videoCanvas = null;
            }
        }
    }

    @Override
    public void updateVFrame(Bitmap bmp) {
        m_lastBmp = bmp;
        drawVideo();
    }

    @Override
    public void updateAVInfo(int codeInfo, int errCode, String strInfo) {
    }


}

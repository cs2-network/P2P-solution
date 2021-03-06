package com.pppp_api.sample.fragment;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

import com.p2p.pppp_api.PPCS_APIs;
import com.pppp_api.sample.R;
import com.pppp_api.sample.widget.Alert;

/**
 * A placeholder fragment containing a simple view.
 */
public class MainFragment extends Fragment implements IAVListener {
    private final static String TAG = MainFragment.class.getSimpleName();
    private int m_curMode = -1;
    private int m_curOnlineNum = 1;
    private Context mContext;
    private P2PDev m_curCamera = new P2PDev();
    private Handler mHandler;
    private Monitor mMonitor;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        this.mContext = context;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View root = inflater.inflate(R.layout.fragment_main, container, false);
        int n = PPCS_APIs.ms_verAPI;
        final TextView m_text_apiver = (TextView) root.findViewById(R.id.text_apiver);
        m_text_apiver.setText(String.format("API ver: %d.%d.%d.%d", (n >> 24) & 0xff, (n >> 16) & 0xff, (n >> 8) & 0xff, n & 0xff));
        mMonitor = (Monitor) root.findViewById(R.id.monitor_view);
        final TextView m_text_status = (TextView) root.findViewById(R.id.text_status);
        final EditText m_edt_uid = (EditText) root.findViewById(R.id.edt_uid);
        m_edt_uid.setText("PPCS-014921-UKMJJ");
        final Button m_btn_conn = (Button) root.findViewById(R.id.btn_conn);
        final CheckBox m_chk_audio = (CheckBox) root.findViewById(R.id.chk_audio);
        final CheckBox m_chk_video = (CheckBox) root.findViewById(R.id.chk_video);
        final Button m_btn_disconn = (Button) root.findViewById(R.id.btn_disconn);

        m_btn_conn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (m_curCamera.isConnected()) {
                    Alert.showAlert(mContext, "Tips", "This camera is connected.", "Ok");
                    return;
                }
                String sUID = "";
                sUID = m_edt_uid.getText().toString().toUpperCase();
                if (sUID.length() == 0) {
                    Alert.showAlert(mContext, "Tips", "Please fill UID field.", "Ok");
                    return;
                }
                m_curCamera.setData(sUID);
                int nRet = m_curCamera.connectDev();
                if (nRet >= 0) {
                    m_chk_audio.setEnabled(true);
                    m_chk_video.setEnabled(true);
                    m_btn_disconn.setEnabled(true);
                }
                Log.d(TAG, "  m_curCamera.connectDev() = " + nRet);
            }
        });

        m_btn_disconn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!m_curCamera.isConnected()) {
                    Alert.showAlert(mContext, "Tips", "Please first connect this camera.", "Ok");
                    return;
                }
                m_curCamera.disconnDev();
                m_chk_audio.setChecked(false);
                m_chk_video.setChecked(false);
                m_text_status.setText("");
                m_chk_audio.setEnabled(false);
                m_chk_video.setEnabled(false);
                m_btn_disconn.setEnabled(false);
            }
        });

        m_chk_audio.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!m_curCamera.isConnected()) {
                    Alert.showAlert(mContext, "Tips", "Please first connect this camera.", "Ok");
                    return;
                }
                if (m_chk_audio.isChecked()) m_curCamera.startAudio();
                else m_curCamera.stopAudio();
            }
        });

        m_chk_video.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!m_curCamera.isConnected()) {
                    Alert.showAlert(mContext, "Tips", "Please first connect this camera.", "Ok");
                    return;
                }
                if (m_chk_video.isChecked()) m_curCamera.startVideo();
                else m_curCamera.stopVideo();
            }
        });

        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case P2PDev.CODE_INFO_CONNECTING:
                        m_text_status.setText("Connecting...");
                        break;
                    case P2PDev.CODE_INFO_CONNECT_FAIL:
                        m_text_status.setText("Fail: PPCS_Connect(.)=" + msg.arg1);
                        break;
                    case P2PDev.CODE_INFO_PPCS_CHECK_OK:
                        m_curMode = msg.arg1;
                        m_text_status.setText(String.format("%s, Num:%d", (m_curMode == 0) ? "P2P" : "RLY", m_curOnlineNum));
                        break;
                    case P2PDev.CODE_INFO_AV_ONLINENUM:
                    default:
                        m_text_status.setText(String.format("%s, Num:%d", (m_curMode == 0) ? "P2P" : "RLY", msg.arg1));
                        break;
                }
            }
        };
        m_curCamera.regAVListener(this);
        m_curCamera.regAVListener(mMonitor);
        P2PDev.initAll();
        m_chk_audio.setEnabled(false);
        m_chk_video.setEnabled(false);
        m_btn_disconn.setEnabled(false);
        return root;
    }


    @Override
    public void updateVFrame(Bitmap bmp) {
    }

    @Override
    public void updateAVInfo(int codeInfo, int errCode, String strInfo) {
        final Message msg = mHandler.obtainMessage();
        msg.what = codeInfo;
        msg.arg1 = errCode;
        if (strInfo != null) msg.obj = (String) strInfo;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onDestroy() {
        m_curCamera.disconnDev();
        m_curCamera.unregAVListener(this);
        m_curCamera.unregAVListener(mMonitor);
        P2PDev.deinitAll();
        super.onDestroy();
    }
}

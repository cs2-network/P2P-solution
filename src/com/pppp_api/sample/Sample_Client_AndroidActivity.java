package com.pppp_api.sample;

import com.p2p.pppp_api.PPCS_APIs;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

public class Sample_Client_AndroidActivity extends Activity implements IAVListener {
    public static final int APP_STATUS_INIT		 =0;
    public static final int APP_STATUS_CONNECT_OK=1;
    
	TextView m_text_apiver;
	Monitor  m_monitor_view;
	TextView m_text_status;
	EditText m_edt_uid;
	Button	 m_btn_conn;
	CheckBox m_chk_audio;
	CheckBox m_chk_video;
	Button	 m_btn_disconn;
	
	int m_app_status=APP_STATUS_INIT;
	int m_curMode=-1;
	int m_curOnlineNum=1;
	P2PDev   m_curCamera=new P2PDev();
	
    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        findView();
		setListenner();
		
		String apiver;
		int n=PPCS_APIs.ms_verAPI;
		apiver=String.format("API ver: %d.%d.%d.%d", (n>>24)&0xff, (n>>16)&0xff, (n>>8)&0xff, n&0xff);
		m_text_apiver.setText(apiver);
		
		m_curCamera.regAVListener(this);
		m_curCamera.regAVListener(m_monitor_view);
		P2PDev.initAll();
		btnSwitch();
    }
    
    @Override
	protected void onDestroy() {
    	System.out.println("  ---onDestroy()");
    	
		m_curCamera.disconnDev();
		m_curCamera.unregAVListener(this);
		m_curCamera.unregAVListener(m_monitor_view);
		P2PDev.deinitAll();
		
		super.onDestroy();
	}

	private void findView() {
    	m_text_apiver	=(TextView)findViewById(R.id.text_apiver);
    	m_monitor_view	=(Monitor)findViewById(R.id.monitor_view);
    	m_text_status	=(TextView)findViewById(R.id.text_status);
    	m_edt_uid		=(EditText)findViewById(R.id.edt_uid);
    	m_btn_conn		=(Button)findViewById(R.id.btn_conn);
    	m_chk_audio		=(CheckBox)findViewById(R.id.chk_audio);
    	m_chk_video		=(CheckBox)findViewById(R.id.chk_video);
    	m_btn_disconn	=(Button)findViewById(R.id.btn_disconn);
	}
	private void setListenner() {
		m_btn_conn.setOnClickListener(btnConnListener);
		m_chk_audio.setOnClickListener(btnChkAudioListener);
		m_chk_video.setOnClickListener(btnChkVideoListener);
		m_btn_disconn.setOnClickListener(btnDisconnListener);
	}
	
	private View.OnClickListener btnConnListener=new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if(m_curCamera.isConnected()){
				Alert.showAlert(Sample_Client_AndroidActivity.this, "Tips", "This camera is connected.", "Ok");
				return;
			}
			String sUID="";
			sUID=m_edt_uid.getText().toString().toUpperCase();
			if(sUID.length()==0){
				Alert.showAlert(Sample_Client_AndroidActivity.this, "Tips", "Please fill UID field.", "Ok");
				return;
			}

			m_curCamera.setData(sUID);
			int nRet=m_curCamera.connectDev();
			if(nRet>=0){
				m_app_status=APP_STATUS_CONNECT_OK;
				btnSwitch();
			}
			System.out.println("  m_curCamera.connectDev()="+nRet);
		}
	};
	private View.OnClickListener btnChkAudioListener=new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if(!m_curCamera.isConnected()){
				Alert.showAlert(Sample_Client_AndroidActivity.this, "Tips", "Please first connect this camera.", "Ok");
				return;
			}
			if(m_chk_audio.isChecked()) m_curCamera.startAudio();
			else m_curCamera.stopAudio();
		}
	};
	private View.OnClickListener btnChkVideoListener=new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if(!m_curCamera.isConnected()){
				Alert.showAlert(Sample_Client_AndroidActivity.this, "Tips", "Please first connect this camera.", "Ok");
				return;
			}
			if(m_chk_video.isChecked()) m_curCamera.startVideo();
			else m_curCamera.stopVideo();
		}
	};
	private View.OnClickListener btnDisconnListener=new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if(!m_curCamera.isConnected()){
				Alert.showAlert(Sample_Client_AndroidActivity.this, "Tips", "Please first connect this camera.", "Ok");
				return;
			}
			m_curCamera.disconnDev();			
			
			m_chk_audio.setChecked(false);
			m_chk_video.setChecked(false);
			m_app_status=APP_STATUS_INIT;
			m_text_status.setText("");
			btnSwitch();
		}
	};

	private void btnSwitch()
	{
		switch(m_app_status)
		{
			case APP_STATUS_INIT:
				m_chk_audio.setEnabled(false);
				m_chk_video.setEnabled(false);
				m_btn_disconn.setEnabled(false);
				break;
				
			case APP_STATUS_CONNECT_OK:
				m_chk_audio.setEnabled(true);
				m_chk_video.setEnabled(true);
				m_btn_disconn.setEnabled(true);
				break;
				
			default:;
		}		
	}

	@Override
	public void updateVFrame(Bitmap bmp) {
	}

	@Override
	public void updateAVInfo(int codeInfo, int errCode, String strInfo){
		Message msg = handler.obtainMessage();
		msg.what = codeInfo;
		msg.arg1 = errCode;
		if(strInfo!=null) msg.obj=(String)strInfo;
		handler.sendMessage(msg);
	}
	
	private Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg){
			switch(msg.what) {
				case P2PDev.CODE_INFO_CONNECTING:
					m_text_status.setText("Connecting...");
					break;
					
				case P2PDev.CODE_INFO_CONNECT_FAIL:
					m_text_status.setText("Fail: PPCS_Connect(.)="+msg.arg1);
					break;
					
				case P2PDev.CODE_INFO_PPCS_CHECK_OK:{
					m_curMode=msg.arg1;
					String str;
					str=String.format("%s, Num:%d", (m_curMode==0) ? "P2P" : "RLY", m_curOnlineNum);
					m_text_status.setText(str);
					}
					break;
					
				case P2PDev.CODE_INFO_AV_ONLINENUM:{
					m_curOnlineNum=msg.arg1;
					String str;
					str=String.format("%s, Num:%d", (m_curMode==0) ? "P2P" : "RLY", m_curOnlineNum);
					m_text_status.setText(str);
					}
					break;
					
				default:;
			}
		}
	};
}
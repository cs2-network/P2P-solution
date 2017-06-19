package com.pppp_api.sample;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.LinkedList;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import com.decoder.util.AdpcmCodec;
import com.decoder.util.H264Codec;
import com.p2p.pppp_api.AVFrameHead;
import com.p2p.pppp_api.AVIOCtrlHead;
import com.p2p.pppp_api.AVStreamIOHead;
import com.p2p.pppp_api.AVStreamIO_Proto;
import com.p2p.pppp_api.PPCS_APIs;
import com.p2p.pppp_api.st_PPCS_Session;

public class P2PDev {
	public static final int CODE_INFO_CONNECTING	=1;
	public static final int CODE_INFO_CONNECT_FAIL	=2;
	public static final int CODE_INFO_PPCS_CHECK_OK	=3;
	public static final int CODE_INFO_AV_ONLINENUM	=4;
	
	public static final int MAX_SIZE_BUF	=65536; //64*1024;

	public static final byte CHANNEL_DATA	=1;
	public static final byte CHANNEL_IOCTRL	=2;
	
	
	String mDevUID="";
	
	int m_handleSession=-1;
	volatile boolean m_bRunning=false;
	private static int m_nInitH264Decoder=-1;
	private static boolean m_bInitAudio = false;
    private AudioTrack m_AudioTrack = null;
    
	private LinkedList<IAVListener> m_listener=new LinkedList<IAVListener>();
	private ThreadRecvAVData m_threadRecvAVData=null;
	private ThreadPlayAudio	 m_threadPlayAudio=null;
	private FIFO m_fifoAudio=new FIFO();
	
	public P2PDev(){}
	public P2PDev(String sUID){ setData(sUID); }
	
	public void setData(String sUID){
		mDevUID=sUID;
	}
	private boolean isNullField(String str){
		if(str==null || str.length()==0) return true;
		else return false;
	}
	
	public static final byte[] intToByteArray_Little(int value) {
        return new byte[] {
                (byte)value,
                (byte)(value >>> 8),
                (byte)(value >>> 16),                
                (byte)(value >>> 24)
        };
	}
	
	public synchronized boolean initAudioDev(int sampleRateInHz, int channel, int dataBit) {
		if(!m_bInitAudio) {
			int channelConfig= 2;
			int audioFormat = 2;
			int mMinBufSize = 0;

			channelConfig =(channel == AVStreamIO_Proto.ACHANNEL_STERO) ? AudioFormat.CHANNEL_CONFIGURATION_STEREO:AudioFormat.CHANNEL_CONFIGURATION_MONO;
			audioFormat = (dataBit == AVStreamIO_Proto.ADATABITS_16) ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
			mMinBufSize = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
			System.out.println("--audio, mMinBufSize="+mMinBufSize);
			
		    if(mMinBufSize ==AudioTrack.ERROR_BAD_VALUE || mMinBufSize ==AudioTrack.ERROR)  return false;	    
			try {
				m_AudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channelConfig, audioFormat, mMinBufSize,AudioTrack.MODE_STREAM);				
			} catch(IllegalArgumentException iae) {				
				iae.printStackTrace();
				return false; //return----------------------------------------
			}
			m_AudioTrack.play();
			m_bInitAudio = true;					
			return true;
			
		}else return false;
    }
    	
    public synchronized void deinitAudioDev() {
    	if(m_bInitAudio){
    		if (m_AudioTrack != null) {
    			m_AudioTrack.stop();
    			m_AudioTrack.release();
    			m_AudioTrack=null;
    		}
			m_bInitAudio = false;			
		}
    }
    
	public static int initAll(){
		String strPara="EBGAEIBIKHJJGFJKEOGCFAEPHPMAHONDGJFPBKCPAJJMLFKBDBAGCJPBGOLKIKLKAJMJKFDOOFMOBECEJIMM";
		int nRet=PPCS_APIs.PPCS_Initialize(strPara.getBytes());
		m_nInitH264Decoder=H264Codec.InitCodec((byte)1);
		
		return nRet;
	}
	
	public static int deinitAll(){
		int nRet=PPCS_APIs.PPCS_DeInitialize();
		H264Codec.UninitCodec();
		
		return nRet;
	}
	
	public void regAVListener(IAVListener listener){
		synchronized(m_listener){
			if(listener!=null && !m_listener.contains(listener)) m_listener.addLast(listener);
		}
	}
	
	public void unregAVListener(IAVListener listener){
		synchronized(m_listener){
			if(listener!=null && !m_listener.isEmpty()){
				for(int i=0; i<m_listener.size(); i++){
					if(m_listener.get(i)==listener) {
						m_listener.remove(i);
						break;
					}
				}
			}
		}
	}
	
	private void updateAVListenerInfo(int codeInfo, int errCode, String strInfo){
		synchronized(m_listener){
			IAVListener curListener=null;
			for(int i=0; i<m_listener.size(); i++){
				curListener=m_listener.get(i);
				curListener.updateAVInfo(codeInfo, errCode, strInfo);
			}
		}
	}
	private void updateAVListenerVFrame(Bitmap bmp)
	{
		synchronized(m_listener){
			IAVListener curListener=null;
			for(int i=0; i<m_listener.size(); i++){
				curListener=m_listener.get(i);
				curListener.updateVFrame(bmp);
			}
		}
	}
	
	public int sendIOCtrl(int handleSession, int nIOCtrlType, byte[] pIOData, int nIODataSize)
	{
		if(nIODataSize<0 || nIOCtrlType<0) return PPCS_APIs.ER_ANDROID_NULL;

		int nSize=AVStreamIOHead.LEN_HEAD+AVIOCtrlHead.LEN_HEAD+nIODataSize;
		byte[] packet=new byte[nSize];
		Arrays.fill(packet, (byte)0);
		byte[] byt=intToByteArray_Little(AVIOCtrlHead.LEN_HEAD+nIODataSize);
		System.arraycopy(byt, 0, packet, 0, 4);
		packet[3]=(byte)AVStreamIO_Proto.SIO_TYPE_IOCTRL;
		
		packet[4]=(byte)nIOCtrlType;
		packet[5]=(byte)(nIOCtrlType >>> 8);
		
		if(pIOData!=null){
			packet[6]=(byte)nIODataSize;
			packet[7]=(byte)(nIODataSize >>> 8);			
			System.arraycopy(pIOData, 0, packet, 8, nIODataSize);
		}	
		return PPCS_APIs.PPCS_Write(handleSession, CHANNEL_IOCTRL, packet, nSize);
	}
	
	public boolean isConnected() { return (m_handleSession>=0); }
	
	public int connectDev()
	{
		if(isNullField(mDevUID)) return -5000;
		if(m_handleSession<0){
			updateAVListenerInfo(CODE_INFO_CONNECTING, 0, null);
			m_handleSession=PPCS_APIs.PPCS_Connect(mDevUID, (byte)1, 0);
			if(m_handleSession<0){
				updateAVListenerInfo(CODE_INFO_CONNECT_FAIL, m_handleSession, null);
				return m_handleSession;
			}
		}
		
		st_PPCS_Session SInfo=new st_PPCS_Session();
		if(PPCS_APIs.PPCS_Check(m_handleSession, SInfo)==PPCS_APIs.ERROR_PPCS_SUCCESSFUL){
			String str;
			str=String.format("  ----Session Ready: -%s----", (SInfo.getMode()==0) ? "P2P" : "RLY");
			System.out.println(str);
			str=String.format("  Socket: %d", SInfo.getSkt());  System.out.println(str);
			str=String.format("  Remote Addr: %s:%d", SInfo.getRemoteIP(),SInfo.getRemotePort());  System.out.println(str);
			str=String.format("  My Lan Addr: %s:%d", SInfo.getMyLocalIP(),SInfo.getMyLocalPort());System.out.println(str);
			str=String.format("  My Wan Addr: %s:%d", SInfo.getMyWanIP(),SInfo.getMyWanPort());  System.out.println(str);
			str=String.format("  Connection time: %d", SInfo.getConnectTime());  System.out.println(str);
			str=String.format("  DID: %s", SInfo.getDID());  System.out.println(str);
			str=String.format("  I am : %s", (SInfo.getCorD() ==0)? "Client":"Device");  System.out.println(str);
			
			updateAVListenerInfo(CODE_INFO_PPCS_CHECK_OK, SInfo.getMode(), null);
		}
		
		if(m_threadRecvAVData==null){
			m_bRunning=true;
			m_threadRecvAVData=new ThreadRecvAVData();
			m_threadRecvAVData.start();
		}
		return 0;	
	}
	
	public int disconnDev()
	{
		int nRet=PPCS_APIs.ER_ANDROID_NULL;
		m_bRunning=false;
		
		if(m_handleSession>=0){
			sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_AUDIO_STOP, null, 0);
			sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_VIDEO_STOP, null, 0);
			try { Thread.sleep(20); } 
			catch (InterruptedException e) { e.printStackTrace(); }
			
			nRet=PPCS_APIs.PPCS_Close(m_handleSession);
			m_handleSession=-1;
		}
		
		if(m_threadRecvAVData!=null && m_threadRecvAVData.isAlive()){
			try { m_threadRecvAVData.join();}
			catch (InterruptedException e) { e.printStackTrace(); }			
		}
		m_threadRecvAVData=null;
		
		if(m_threadPlayAudio!=null && m_threadPlayAudio.isAlive()){
			try { m_threadPlayAudio.join();}
			catch (InterruptedException e) { e.printStackTrace(); }	
		}
		m_threadPlayAudio=null;
		
		return nRet;		
	}
	
	public int startAudio()
	{
		int nRet=PPCS_APIs.ER_ANDROID_NULL;
		if(m_handleSession<0) return nRet;
		
		if(m_threadPlayAudio==null){
			m_threadPlayAudio=new ThreadPlayAudio();
			m_threadPlayAudio.start();
		}
		nRet=sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_AUDIO_START, null, 0);
		
		return nRet;		
	}
	
	public int stopAudio()
	{
		int nRet=PPCS_APIs.ER_ANDROID_NULL;
		if(m_handleSession<0) return nRet;		
		nRet=sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_AUDIO_STOP, null, 0);
		if(m_threadPlayAudio!=null) {
			m_threadPlayAudio.stopPlay();
			m_threadPlayAudio=null;
		}
		return nRet;		
	}
	
	public int startVideo()
	{
		int nRet=PPCS_APIs.ER_ANDROID_NULL;
		if(m_handleSession<0) return nRet;		
		nRet=sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_VIDEO_START, null, 0);
		return nRet;
	}
	
	public int stopVideo()
	{
		int nRet=PPCS_APIs.ER_ANDROID_NULL;
		if(m_handleSession<0) return nRet;		
		nRet=sendIOCtrl(m_handleSession, AVStreamIO_Proto.IOCTRL_TYPE_VIDEO_STOP, null, 0);
		return nRet;
	}
	
	class ThreadRecvAVData extends Thread
	{
		public static final int MAX_FRAMEBUF=1600000;		
		
		byte[] pAVData=new byte[MAX_SIZE_BUF];
		int[]  nRecvSize=new int[1];
		int    nCurStreamIOType=0, nRet=0;
		AVStreamIOHead pStreamIOHead=new AVStreamIOHead();
		AVFrameHead stFrameHead=new AVFrameHead();
		boolean bFirstFrame_video=true;
		int[]   out_4para =new int[4];
		byte[]  out_bmp565=new byte[MAX_FRAMEBUF];
		
		int 	bmpWidth=0, bmpHeight=0, bmpSizeInBytes=0;
		Bitmap 	bmpLast=Bitmap.createBitmap(640, 480, Config.RGB_565);
		byte[]  bmpBuff = new byte[MAX_FRAMEBUF];
		ByteBuffer bytBuffer = ByteBuffer.wrap(bmpBuff);		
		
		@Override
		public void run() {
			super.run();
			long tick1=System.currentTimeMillis(), tick2=0L;
			int nOnlineNum=1;
			
			do{
				nRecvSize[0]=AVStreamIOHead.LEN_HEAD;
				nRet=PPCS_APIs.PPCS_Read(m_handleSession, CHANNEL_DATA, pAVData, nRecvSize, 0xFFFFFFFF);
				if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
					System.out.println("ThreadRecvIOCtrl: Session TimeOUT!");
					break;

				}else if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_REMOTE){
					System.out.println("ThreadRecvIOCtrl: Session Remote Close!");
					break;

				}else if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_CALLED){
					System.out.println("ThreadRecvIOCtrl: myself called PPCS_Close!");
					break;
				}
				if(nRecvSize[0]>0){
					pStreamIOHead.setData(pAVData);
					nCurStreamIOType=pStreamIOHead.getStreamIOType();
					nRecvSize[0]=pStreamIOHead.getDataSize();
					nRet=PPCS_APIs.PPCS_Read(m_handleSession, CHANNEL_DATA, pAVData, nRecvSize, 0xFFFFFFFF);
					
					if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_TIMEOUT){
						System.out.println("ThreadRecvIOCtrl: Session TimeOUT!");
						break;

					}else if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_REMOTE){
						System.out.println("ThreadRecvIOCtrl: Session Remote Close!");
						break;

					}else if(nRet==PPCS_APIs.ERROR_PPCS_SESSION_CLOSED_CALLED){
						System.out.println("ThreadRecvIOCtrl: myself called PPCS_Close!");
						break;
					}
					
					if(nRecvSize[0]>0){
						tick2=System.currentTimeMillis();
						if((tick2-tick1)>1000){
							tick1=tick2;
							stFrameHead.setData(pAVData);
							nOnlineNum=stFrameHead.getOnlineNum();
							updateAVListenerInfo(CODE_INFO_AV_ONLINENUM, nOnlineNum, null);
						}
						
						if(nRecvSize[0]>=MAX_SIZE_BUF) System.out.println("====nRecvSize>64*1024, nCurStreamIOType="+nCurStreamIOType);
						if(nCurStreamIOType==AVStreamIO_Proto.SIO_TYPE_AUDIO) m_fifoAudio.addLast(pAVData, nRecvSize[0]);
						else if(nCurStreamIOType==AVStreamIO_Proto.SIO_TYPE_VIDEO) myDoVideoData(m_handleSession, pAVData, nRecvSize[0]);
					}
				}
			}while(m_bRunning);
			System.out.println("---ThreadRecvAVData is exit.");
		}
				
		private void myDoVideoData(int handleSession, byte[] pAVData, int nAVDataSize)
		{
			stFrameHead.setData(pAVData);
			switch(stFrameHead.getCodecID()){
				case AVStreamIO_Proto.CODECID_V_H264:
					if(m_nInitH264Decoder>=0){
						if(bFirstFrame_video && stFrameHead.getFlag()!=AVStreamIO_Proto.VFRAME_FLAG_I) break;
						bFirstFrame_video=false;
						
						int consumed_bytes=0;
						int nFrameSize=stFrameHead.getDataSize();
						System.arraycopy(pAVData, AVFrameHead.LEN_HEAD, pAVData, 0, nAVDataSize-AVFrameHead.LEN_HEAD);
						while(nFrameSize>0){
							consumed_bytes=H264Codec.H264Decode(out_bmp565, pAVData, nFrameSize, out_4para);
							if(consumed_bytes<0){
								nFrameSize=0;
								break;
							}
							if(!m_bRunning) break;
							
							if(out_4para[0]>0){
								if(out_4para[2]>0 && out_4para[2]!=bmpWidth){
									if(bmpWidth!=0) H264Codec.H264Decode(out_bmp565, pAVData, nFrameSize, out_4para);
									
									bmpWidth =out_4para[2];
									bmpHeight=out_4para[3];
									bmpSizeInBytes=bmpWidth*bmpHeight*2;
									
									bmpLast=null;
									bmpLast=Bitmap.createBitmap(bmpWidth, bmpHeight, Config.RGB_565);
									bytBuffer=ByteBuffer.wrap(out_bmp565, 0, bmpSizeInBytes);
								}
								//System.out.println("--video, bmpWidth="+bmpWidth);
								
								System.arraycopy(out_bmp565, 0, bmpBuff, 0, bmpSizeInBytes);
								bytBuffer.rewind();
								bmpLast.copyPixelsFromBuffer(bytBuffer);
								
								updateAVListenerVFrame(bmpLast);
							}
							nFrameSize-=consumed_bytes;
							if(nFrameSize>0) System.arraycopy(pAVData, consumed_bytes, pAVData, 0, nFrameSize);
							else nFrameSize=0;
						}
					}
					break;
				default:;
			}
		}
	}
	
	class ThreadPlayAudio extends Thread
	{
		public static final int MAX_AUDIOBUF		  =2560;
		public static final int ADPCM_ENCODE_BYTE_SIZE=160;
		public static final int ADPCM_DECODE_BYTE_SIZE=640;
		
		byte[] pRaw=new byte[MAX_AUDIOBUF];
		byte[] bufTmp=new byte[640];
		AVFrameHead stFrameHead=new AVFrameHead();
		boolean bPlaying=false;
		
		@Override
		public void run()
		{
			byte[] audioData=null, dirVar=new byte[1];
			boolean bFirst=true;
			bPlaying=true;
			int  nLeftData=0, nAudioDataSize=0;
			m_fifoAudio.removeAll();
			dirVar[0]=0;

			while(m_bRunning && bPlaying){
				if(m_fifoAudio.isEmpty()){
					try { Thread.sleep(20); } 
					catch (InterruptedException e) { e.printStackTrace(); }
					continue;
				}
				audioData=m_fifoAudio.removeHead();
				if(audioData!=null) {
					if(bFirst){
						bFirst=false;
						boolean bRet=initAudioDev(8000, AVStreamIO_Proto.ACHANNEL_MONO, AVStreamIO_Proto.ADATABITS_16);
						if(bRet) m_AudioTrack.play();
						System.out.println("--initAudioDev(.)="+bRet);
					}
					nLeftData=m_fifoAudio.getSize();
					//System.out.println("  myDoAudioData, audioData.length="+audioData.length+", m_fifoAudio.getSize()="+nLeftData);
					
					stFrameHead.setData(audioData);
					nAudioDataSize=audioData.length-AVFrameHead.LEN_HEAD;
					System.arraycopy(audioData, AVFrameHead.LEN_HEAD, audioData, 0, nAudioDataSize);
					myDoAudioData(m_handleSession, stFrameHead.getCodecID(), audioData, nAudioDataSize);
				}
			}//while-end
			
			deinitAudioDev();
			System.out.println("---ThreadPlayAudio is exit.");
		}
		
		public void stopPlay(){
			bPlaying=false;
			if(this.isAlive()) {
				try { this.join();}
				catch (InterruptedException e) { e.printStackTrace(); }	
			}
		}
		private void myDoAudioData(int handleSession, int nCodecID, byte[] bytAudioData, int nAudioDataSize)
		{
			switch(nCodecID)
			{
				case AVStreamIO_Proto.CODECID_A_ADPCM:
					{
						int nSize=0, nLeft=nAudioDataSize;
						for(int i=0; i<nAudioDataSize/ADPCM_ENCODE_BYTE_SIZE; i++){
							nLeft=nAudioDataSize-i*ADPCM_ENCODE_BYTE_SIZE;
							System.arraycopy(bytAudioData, i*ADPCM_ENCODE_BYTE_SIZE, bytAudioData, 0, nLeft);
							AdpcmCodec.decode(bytAudioData,ADPCM_ENCODE_BYTE_SIZE, bufTmp);
							System.arraycopy(bufTmp, 0, pRaw, nSize, ADPCM_DECODE_BYTE_SIZE);
							nSize+=ADPCM_DECODE_BYTE_SIZE;
						}						
						m_AudioTrack.write(pRaw, 0, nSize);
					}
					break;
				default:;
			}
		}
	}
	
}

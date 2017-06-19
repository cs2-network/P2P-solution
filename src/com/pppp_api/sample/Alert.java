package com.pppp_api.sample;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.widget.Toast;

public class Alert {

	public static void showAlert(Context context, CharSequence title, CharSequence message, CharSequence btnTitle) {		  
		  AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(context);
		  dlgBuilder.setIcon(android.R.drawable.ic_dialog_alert);
		  dlgBuilder.setTitle(title);
		  dlgBuilder.setMessage(message);
		  dlgBuilder.setPositiveButton(btnTitle, new DialogInterface.OnClickListener(){
				@Override
				public void onClick(DialogInterface dialog, int which) {
				}	    		
	    	}).show();
	    }
	  
	    public static void showToast(Context context, String str){
	    	Toast.makeText(context, str, Toast.LENGTH_SHORT).show();
	    }
}

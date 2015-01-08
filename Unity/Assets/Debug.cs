using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Runtime.InteropServices;


public class Debug : MonoBehaviour {

	private String	ServerAddress = "cli://localhost:7070";
	private PopUnityChannel	mChannel = null;
	private String	JobString = "subscribenewframe serial=isight memfile=false";
	public Texture2D mTexture;
	public Material MaterialForTexture;

	void Start()
	{
		//	need to CREATE a texture. overwriting an existing one doesn't work...
		if (mTexture == null) {
			mTexture = new Texture2D (1280, 1024, TextureFormat.BGRA32, false);

			//	need a material...
			if (MaterialForTexture != null)
				MaterialForTexture.mainTexture = mTexture;
		}

		PopUnity.AssignJobHandler("re:getframe", ((Job) => this.OnGetFrameReply(Job)) );
		PopUnity.AssignJobHandler("newframe", ((Job) => this.OnGetFrameReply(Job)) );
		PopUnity.AssignJobHandler ("newdepth", ((Job) => this.OnGetFrameReply (Job)));
	}

	void Update () {
		PopUnity.Update ();
	}

	void OnGetFrameReply(PopJob Job)
	{
		Job.GetParam("default",mTexture,SoyPixelsFormat.Greyscale);
	}

	Rect StepRect(Rect rect)
	{
		rect.y += rect.height + (rect.height*0.30f);
		return rect;
	}

	void OnGUI()
	{
		Rect rect = new Rect (10, 0, Screen.width - 20, 20);

		if (mChannel == null) {
			rect = StepRect (rect);
			ServerAddress = GUI.TextField (rect, ServerAddress);

			rect = StepRect (rect);
			if (GUI.Button (rect, "Connect to channel")) {
					mChannel = new PopUnityChannel (ServerAddress);
			}			

		} else {
			rect = StepRect (rect);
			GUI.Label (rect, "Channel id: " + mChannel.mChannel);

			rect = StepRect (rect);
			JobString = GUI.TextField (rect, JobString);

			rect = StepRect (rect);
			if (GUI.Button (rect, "Send job")) {
				mChannel.SendJob( JobString );
			}			
		}

		rect = StepRect (rect);
		//	fill
		rect.height = Screen.height - rect.y;

		//string Text = "Hello ";
		//GUI.Label (rect, Text);
		//GUI.DrawTexture (rect, mTexture);
	}

	void OnPostRender()
	{
		GL.IssuePluginEvent (0);
	}
}

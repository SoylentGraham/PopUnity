using UnityEngine;
using System.Collections;
using System;
using System.Runtime.InteropServices;


public class PopUnity
{
	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	private delegate void			DebugLogDelegate(string str);

	[DllImport ("PopUnity")]
	public static extern UInt64 Test();
	
	[DllImport("PopUnity", CallingConvention = CallingConvention.Cdecl)]
	public static extern UInt64 CreateChannel(string ChannelSpec);

	[DllImport("PopUnity", CallingConvention = CallingConvention.Cdecl)]
	public static extern bool SendJob(UInt64 ChannelRef,string Command);

	[DllImport("PopUnity")]
	public static extern void FlushDebug (System.IntPtr FunctionPtr);

	static private DebugLogDelegate	mDebugLogDelegate = new DebugLogDelegate( Log );

	public PopUnity()
	{
	}

	static void Log(string str)
	{
		UnityEngine.Debug.Log("PopUnity: " + str);
	}

	static public void Update()
	{
		FlushDebug (Marshal.GetFunctionPointerForDelegate (mDebugLogDelegate));
	}
};
	

public class PopUnityChannel
{
	public UInt64	mChannel = 0;

	public PopUnityChannel(string Channel)
	{
		mChannel = PopUnity.CreateChannel(Channel);
	}

	public bool SendJob(string Command)
	{
		return PopUnity.SendJob (mChannel, Command);
	}
};


public class Debug : MonoBehaviour {

	private String	ServerAddress = "cli://localhost:7070";
	private PopUnityChannel	mChannel = null;
	private String	JobString = "getframe";

	void Update () {
		PopUnity.Update ();
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

		string Text = "Hello ";
		Text += PopUnity.Test ();
		GUI.Label (rect, Text);
	}
}

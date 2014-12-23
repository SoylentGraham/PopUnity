using UnityEngine;
using System.Collections;
using System;
using System.Runtime.InteropServices;

public class Debug : MonoBehaviour {


	[DllImport ("PopUnity")]
	private static extern UInt64 Test();

	// Use this for initialization
	void Start () {
	
	}
	
	// Update is called once per frame
	void Update () {
	
	}

	void OnGUI()
	{
		string Text = "Hello ";
		Text += Test ();

		Rect rect = new Rect (0, 0, Screen.width, Screen.height );

		GUI.Label (rect, Text);
	}
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(SteamVR_TrackedObject))]
public class TestControllerInput : MonoBehaviour {

    public QuadCameraUpdate mQuadCameraUpdate;
    SteamVR_TrackedController mDevice;
    
    // Use this for initialization
    void Start () {
        //mTrackedObject = GetComponent<SteamVR_TrackedObject>();

        mDevice = GetComponent<SteamVR_TrackedController>();
        if (mDevice == null)
        {
            mDevice = gameObject.AddComponent<SteamVR_TrackedController>();
        }

        mDevice.TriggerClicked += TriggerClickedHandler;
    }
	
    private void TriggerClickedHandler(object sender, ClickedEventArgs e)
    {
        Debug.Log("Left controller trigger pressed. Position: " + mDevice.transform.position);
        mQuadCameraUpdate.SendMessage("LeftControllerTriggerPressed", mDevice.transform.position);
    }
}

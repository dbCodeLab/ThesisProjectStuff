//======= Copyright (c) Valve Corporation, All rights reserved. ===============
//
// Purpose: For controlling in-game objects with tracked devices.
//
//=============================================================================

using UnityEngine;
using Valve.VR;

public class TrackedObjectCustom : MonoBehaviour
{
    public enum EIndex
    {
        None = -1,
        Hmd = (int)OpenVR.k_unTrackedDeviceIndex_Hmd,
        Device1,
        Device2,
        Device3,
        Device4,
        Device5,
        Device6,
        Device7,
        Device8,
        Device9,
        Device10,
        Device11,
        Device12,
        Device13,
        Device14,
        Device15
    }

    public EIndex index;
    public Transform origin; // if not set, relative to parent
    public bool isValid = false;
    SteamVR_Utils.RigidTransform BMatInv;

    private void OnNewPoses(TrackedDevicePose_t[] poses)
    {
        if (index == EIndex.None)
            return;

        var i = (int)index;

        isValid = false;
        if (poses.Length <= i)
            return;

        if (!poses[i].bDeviceIsConnected)
            return;

        if (!poses[i].bPoseIsValid)
            return;

        isValid = true;

        
        var pose = new SteamVR_Utils.RigidTransform(poses[i].mDeviceToAbsoluteTracking);
        pose.Multiply(pose, BMatInv);
        
        if (origin != null)
        {
            transform.position = origin.transform.TransformPoint(pose.pos);
            transform.rotation = origin.rotation * pose.rot;
        }
        else
        {
            transform.localPosition = pose.pos;
            transform.localRotation = pose.rot;
        }
    }

    SteamVR_Events.Action newPosesAction;

    void Awake()
    {
        newPosesAction = SteamVR_Events.NewPosesAction(OnNewPoses);
        HmdMatrix44_t mat = new HmdMatrix44_t();


        /**************************************/
        /*
         -0.102501    0.19387   0.975658   0.690977
        0.00933648   0.980968  -0.193944   0.545085
         -0.994689 -0.0107702   -0.10236  -0.111363
         0 			0			0 			1
        */
        /*
        mat.m0 = -0.102501f;
        mat.m1 = 0.19387f;
        mat.m2 = 0.975658f;
        mat.m3 = 0.690977f;
        mat.m4 = 0.00933648f;
        mat.m5 = 0.980968f;
        mat.m6 = -0.193944f;
        mat.m7 = 0.545085f;
        mat.m8 = -0.994689f;
        mat.m9 = -0.0107702f;
        mat.m10 = -0.10236f;
        mat.m11 = -0.111363f;
        mat.m12 = 0;
        mat.m13 = 0;
        mat.m14 = 0;
        mat.m15 = 1;
        */
        mat.m0 = -0.102501f;
        mat.m4 = 0.19387f;
        mat.m8 = 0.975658f;
        mat.m12 = 0.690977f;
        mat.m1 = 0.00933648f;
        mat.m5 = 0.980968f;
        mat.m9 = -0.193944f;
        mat.m13 = 0.545085f;
        mat.m2 = -0.994689f;
        mat.m6 = -0.0107702f;
        mat.m10 = -0.10236f;
        mat.m14 = -0.111363f;
        mat.m3 = 0;
        mat.m7 = 0;
        mat.m11 = 0;
        mat.m15 = 1;

        /**************************************/

        BMatInv = new SteamVR_Utils.RigidTransform(mat);
        BMatInv.Inverse();
    }

    void OnEnable()
    {
        var render = SteamVR_Render.instance;
        if (render == null)
        {
            enabled = false;
            return;
        }

        newPosesAction.enabled = true;
    }

    void OnDisable()
    {
        newPosesAction.enabled = false;
        isValid = false;
    }

    public void SetDeviceIndex(int index)
    {
        if (System.Enum.IsDefined(typeof(EIndex), index))
            this.index = (EIndex)index;
    }
}


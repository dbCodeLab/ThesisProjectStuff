//======= Copyright (c) Valve Corporation, All rights reserved. ===============
using UnityEngine;

public class SteamVR_TestTrackedCamera : MonoBehaviour
{
	public Material material;
	//public Transform target;
	public bool undistorted = true;
	public bool cropped = true;

    private Matrix4x4 AMat = new Matrix4x4();
    private Matrix4x4 BMat = new Matrix4x4();

    private Matrix4x4 camToHeadMat;// = new Matrix4x4();
    private Vector3 camToHeadPos;
    private Vector3 camToHeadScale;
    private Quaternion camToHeadRotQuat;

    public Texture2D distortionMap;

    private void Start()
    {
        ///*
        // head
        AMat.SetRow(0, new Vector4(-0.114821f, -0.30489f, 0.945441f, 0.937603f));
        AMat.SetRow(1, new Vector4(-0.00263201f, 0.951825f, 0.306629f, 0.584166f));
        AMat.SetRow(2, new Vector4(-0.993383f, 0.0327192f, - 0.110092f, - 0.0892631f));
        AMat.SetRow(3, new Vector4(0, 0, 0, 1));
        // camera
        BMat.SetRow(0, new Vector4(-0.0813817f,   0.186298f,   0.979117f, 1.05795f));
        BMat.SetRow(1, new Vector4(0.01152f ,  0.982486f, -0.185982f, 0.629366f));
        BMat.SetRow(2, new Vector4(-0.996616f, -0.0038561f -0.0821025f, -0.101663f));
        BMat.SetRow(3, new Vector4(0, 0, 0, 1));

        camToHeadMat = Matrix4x4.Inverse(AMat) * BMat;
        //*/

        /* // A^-1*B
        camToHeadMat = new Matrix4x4();
        camToHeadMat.SetRow(0, new Vector4(0.99996991828891091f, 0.0068488870557860092f, 0.0036981684753152472f, 0.028769190211268475f));
        camToHeadMat.SetRow(1, new Vector4(-0.0042364453176018668f, 0.87750024211661348f, -0.47955819355516222f, 0.078758514798765916f));
        camToHeadMat.SetRow(2, new Vector4(-0.0065299042508945027f, 0.47952778164524124f, 0.8775030781983123f, -0.036732876586485164f));
        camToHeadMat.SetRow(3, new Vector4(0, 0, 0, 1));
        */
        camToHeadRotQuat = QuaternionFromMatrix(camToHeadMat);
        camToHeadPos = new Vector3(camToHeadMat[0, 3], camToHeadMat[1, 3], camToHeadMat[2, 3]);
        camToHeadScale = new Vector3(1.0f, 1.0f, 1.0f);
    }

    void OnEnable()
	{
		// The video stream must be symmetrically acquired and released in
		// order to properly disable the stream once there are no consumers.
		var source = SteamVR_TrackedCamera.Source(undistorted);
		source.Acquire();

		// Auto-disable if no camera is present.
		if (!source.hasCamera)
			enabled = false;
	}

	void OnDisable()
	{
		// Clear the texture when no longer active.
		material.mainTexture = null;

		// The video stream must be symmetrically acquired and released in
		// order to properly disable the stream once there are no consumers.
		var source = SteamVR_TrackedCamera.Source(undistorted);
		source.Release();
	}

	void Update()
	{
        var source = SteamVR_TrackedCamera.Source(undistorted);
		var texture = source.texture;
        
        //texture.
        if (texture == null)
		{
            Debug.Log("camera frame not available");
			return;
		}

        if (distortionMap == null)
        {
            Debug.Log("distortionMap frame not available");
            return;
        }

        texture.wrapMode = TextureWrapMode.Clamp;
        texture.filterMode = FilterMode.Point;
        
        distortionMap.wrapMode = TextureWrapMode.Clamp;
        distortionMap.filterMode = FilterMode.Point;
        
        // Apply the latest texture to the material.  This must be performed
        // every frame since the underlying texture is actually part of a ring
        // buffer which is updated in lock-step with its associated pose.
        // (You actually really only need to call any of the accessors which
        // internally call Update on the SteamVR_TrackedCamera.VideoStreamTexture).

        material.SetTexture("_MainTex", texture);
        material.SetTexture("_DistortionTex", distortionMap);

        //material.mainTexture = texture;
        //material.mainTexture.filterMode = FilterMode.Bilinear;
        //material.mainTexture.wrapMode = TextureWrapMode.Clamp;

        // Adjust the height of the quad based on the aspect to keep the texels square.
        var aspect = (float)texture.width / texture.height;

		// The undistorted video feed has 'bad' areas near the edges where the original
		// square texture feed is stretched to undo the fisheye from the lens.
		// Therefore, you'll want to crop it to the specified frameBounds to remove this.
		if (cropped)
		{
			var bounds = source.frameBounds;
            //material.mainTextureOffset = new Vector2(Mathf.PI, Mathf.PI);
            //new Vector2(bounds.uMin, bounds.vMin);

            // during executions we havee vMin > vMax
            //material.mainTextureOffset = new Vector2(bounds.uMin, bounds.vMax); //new Vector2(0, 0);//new Vector2(bounds.uMin, bounds.vMin);
            material.SetTextureOffset("_MainTex", new Vector2(bounds.uMin, bounds.vMax)); // bounds.vMin

            //material.SetTextureOffset("_MainTex", Vector2.zero);
            var du = bounds.uMax - bounds.uMin;
			var dv = bounds.vMax - bounds.vMin;
            //material.mainTextureScale = new Vector2(du, dv);
            material.SetTextureScale("_MainTex", new Vector2(du, -dv));
            //material.SetTextureScale("_MainTex", new Vector2(  1, 1));
           
            aspect *= Mathf.Abs(du / dv);
        }
		else
		{
			//material.mainTextureOffset = Vector2.zero;
            material.SetTextureOffset("_MainTex", Vector2.zero);

            //material.mainTextureScale = new Vector2(1, -1);
            material.SetTextureScale("_MainTex", new Vector2(1, 1));
        }

       
        /*
        transform.localRotation = camToHeadRotQuat;
        transform.localPosition = camToHeadPos;
        transform.localScale = camToHeadScale;
        */
        //transform.localScale = new Vector3(0.5f, 0.5f, 0.5f);

        ///*
        // Apply the pose that this frame was recorded at.
        if (source.hasTracking)
        {
            var t = source.transform;
            transform.position = t.pos;
            transform.rotation = t.rot;

            //Debug.Log("tpos: " + t.pos.x + " " + t.pos.y + " " + t.pos.z);
        }
        //*/

        /*
		target.localScale = new Vector3(1, 1.0f / aspect, 1);

		// Apply the pose that this frame was recorded at.
		if (source.hasTracking)
		{
			var t = source.transform;
			target.localPosition = t.pos;
			target.localRotation = t.rot;
		}
        */
    }

    public static Quaternion QuaternionFromMatrix(Matrix4x4 m)
    {
        // Adapted from: http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
        Quaternion q = new Quaternion();
        q.w = Mathf.Sqrt(Mathf.Max(0, 1 + m[0, 0] + m[1, 1] + m[2, 2])) / 2;
        q.x = Mathf.Sqrt(Mathf.Max(0, 1 + m[0, 0] - m[1, 1] - m[2, 2])) / 2;
        q.y = Mathf.Sqrt(Mathf.Max(0, 1 - m[0, 0] + m[1, 1] - m[2, 2])) / 2;
        q.z = Mathf.Sqrt(Mathf.Max(0, 1 - m[0, 0] - m[1, 1] + m[2, 2])) / 2;
        q.x *= Mathf.Sign(q.x * (m[2, 1] - m[1, 2]));
        q.y *= Mathf.Sign(q.y * (m[0, 2] - m[2, 0]));
        q.z *= Mathf.Sign(q.z * (m[1, 0] - m[0, 1]));
        return q;
    }
}


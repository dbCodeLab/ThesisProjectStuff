using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class QuadCameraUpdate : MonoBehaviour
{
    /***********************************************************/
    // Test external DLL

    //[DllImport("ExternalDLLunity", EntryPoint = "TsaiCameraCalibration")]
    //public static extern void OpenCVSolvePnp(float[] point3fArray, float[] point2fArray, int numPoints, float[] outRvec, float[] outTvec, float[] outRotMat);

    [DllImport("ExternalDLLunity", EntryPoint = "TsaiCameraCalibration")]
    public static extern void TsaiCameraCalibration(float[] point3fArray, float[] point2fArray, int numPoints, float[] cameraEyePos, float[] cameraEyeMat4x4, float[] outTvec, float[] outRotMat);

    /***********************************************************/
    public GameObject cameraEye;
    public Material material;
    public Transform target;
    public bool undistorted = true;
    public bool cropped = true;

    GameObject spriteQuadCamera;

    const float spriteX = 0.4f;
    const float spriteZ = -0.0001f;
    Vector3[] spritePositions = {
        new Vector3(spriteX, spriteX, spriteZ),
        new Vector3(-spriteX, spriteX, spriteZ),
        new Vector3(-spriteX, -spriteX, spriteZ),
        new Vector3(spriteX, -spriteX, spriteZ),

        new Vector3(0, spriteX, spriteZ),
        new Vector3(-spriteX, 0, spriteZ),
        new Vector3(0, -spriteX, spriteZ),
        new Vector3(spriteX, 0, spriteZ),

        new Vector3(spriteX/2, spriteX/2, spriteZ),
        new Vector3(-spriteX/2, spriteX/2, spriteZ),
        new Vector3(-spriteX/2, -spriteX/2, spriteZ),
        new Vector3(spriteX/2, -spriteX/2, spriteZ),

        new Vector3(0, 0, spriteZ)
    };
    int curSpritePosIdx = 0;

    ArrayList controllerWorldPositions = new ArrayList();
    ArrayList imagePointsPositions = new ArrayList();

    void updateSpritePosition()
    {
        spriteQuadCamera.transform.localPosition = spritePositions[curSpritePosIdx];
    } 

    void LeftControllerTriggerPressed(Vector3 worldPos)
    {
        controllerWorldPositions.Add(worldPos);
        imagePointsPositions.Add(spritePositions[curSpritePosIdx]);

        ++curSpritePosIdx;
        if (curSpritePosIdx >= spritePositions.Length) curSpritePosIdx = 0;
        updateSpritePosition();
        //Debug.Log("okkk: " + curSpritePosIdx);
    }

    private void OnApplicationQuit()
    {
        int numPoints = controllerWorldPositions.Count;
        if (numPoints != 13) return;

        Debug.Log("Trasmitting data do TsaiCalibrationExternalDll. #Positions: " + controllerWorldPositions.Count);

        if (controllerWorldPositions.Count != imagePointsPositions.Count)
        {
            Debug.Log("Different number of 2d/3d points (" + controllerWorldPositions.Count + ", " + imagePointsPositions.Count + ").Return immediate.");
        }

        float[] points3f = new float[numPoints * 3];

        int destIdx = 0;
        for (int i = 0; i != numPoints; ++i)
        {
            Vector3 point3f = (Vector3)controllerWorldPositions[i];
            points3f[destIdx++] = point3f.x;
            points3f[destIdx++] = point3f.y;
            points3f[destIdx++] = point3f.z;
        }

        //int numPoints2f = imagePointsPositions.Count;

        float[] points2f = new float[numPoints * 2];

        destIdx = 0;
        int texWidth = material.mainTexture.width / 2;
        int texHeight = material.mainTexture.height / 2;
        for (int i = 0; i != numPoints; ++i)
        {
            Vector3 point2f = (Vector3)imagePointsPositions[i];
            float tx = (point2f.x + 0.5f) * texWidth;
            float ty = texHeight - (0.5f - point2f.y) * texHeight - 1; // flip y coordinate
            points2f[destIdx++] = tx;
            points2f[destIdx++] = ty;
            //Debug.Log(tx  + " " + ty);
        }

        float[] headPos = new float[3];
        headPos[0] = cameraEye.transform.position.x;
        headPos[1] = cameraEye.transform.position.y;
        headPos[2] = cameraEye.transform.position.z;

        // transmit also camera eye matrix
        Matrix4x4 m = Matrix4x4.TRS(cameraEye.transform.position, cameraEye.transform.rotation, cameraEye.transform.localScale);
        
        float[] headMat = new float[16];
        for (int i = 0; i != 4; ++i)
        {
            for (int j = 0; j != 4; ++j)
            {
                headMat[i * 4 + j] = m[i, j];
            }
        }
     
        float[] tvec = new float[3];
        float[] rotMat = new float[9];
 
        //OpenCVSolvePnp(points3f, points2f, numPoints, rvec, tvec, rotMat);
        TsaiCameraCalibration(points3f, points2f, numPoints, headPos, headMat, tvec, rotMat);

        //Debug.Log("rvec: " + rvec[0] + " " + rvec[1] + " " + rvec[2]);
        Debug.Log("tvec: " + tvec[0] + " " + tvec[1] + " " + tvec[2]);
        //Debug.Log("Rot Matrix:");
        Debug.Log("Rotation Matrix: ");
        for (int i = 0; i != 3; ++i)
        {
            Debug.Log(rotMat[i * 3 + 0] + " " + rotMat[i * 3 + 1] + " " + rotMat[i * 3 + 2]);
        }
    }

    private void Awake()
    {
        //Application.targetFrameRate = 25;
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

        spriteQuadCamera = GameObject.Find("SpriteCamera");
        //spriteQuadCamera.transform.localPosition = new Vector3(0.4f, 0.4f, 0.0f);
        updateSpritePosition();
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

    void FixedUpdate()
    {
        //Debug.Log("Eye: x= " + cameraEye.transform.position.x + ", y=" + cameraEye.transform.position.y + ", z=" + +cameraEye.transform.position.z);
        //Debug.Log("Eye: rx= " + cameraEye.transform.eulerAngles.x + ", ry=" + cameraEye.transform.eulerAngles.y + ", rz=" + cameraEye.transform.eulerAngles.z);
        
        var source = SteamVR_TrackedCamera.Source(undistorted);

       // Debug.Log("Camera: x= " + source.transform.pos.x + ", y=" + source.transform.pos.y + ", z=" + source.transform.pos.z);
       
        var texture = source.texture;
        if (texture == null)
        {
            Debug.Log("Frame not captured.");
            return;
        }
              
        // Apply the latest texture to the material.  This must be performed
        // every frame since the underlying texture is actually part of a ring
        // buffer which is updated in lock-step with its associated pose.
        // (You actually really only need to call any of the accessors which
        // internally call Update on the SteamVR_TrackedCamera.VideoStreamTexture).
        material.mainTexture = texture;
        
        // Adjust the height of the quad based on the aspect to keep the texels square.
        var aspect = (float)texture.width / texture.height;

        //texWidth = texture.width;
        //texHeight = texture.height;

        // The undistorted video feed has 'bad' areas near the edges where the original
        // square texture feed is stretched to undo the fisheye from the lens.
        // Therefore, you'll want to crop it to the specified frameBounds to remove this.
        if (cropped)
        {
            var bounds = source.frameBounds;
            material.mainTextureOffset = new Vector2(bounds.uMin, bounds.vMin);
            
            var du = bounds.uMax - bounds.uMin;
            var dv = bounds.vMax - bounds.vMin;
            material.mainTextureScale = new Vector2(du, dv);
            
            aspect *= Mathf.Abs(du / dv);
        }
        else
        {
            material.mainTextureOffset = Vector2.zero;
            material.mainTextureScale = new Vector2(1, -1);
        }

        Vector3 scale = target.localScale;
        target.localScale = new Vector3(scale.x, scale.x / aspect, scale.z);

        spriteQuadCamera.transform.localScale = new Vector3(0.05f, 0.05f*aspect, 0.05f);

    }


}
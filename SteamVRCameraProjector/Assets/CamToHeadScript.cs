using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CamToHeadScript : MonoBehaviour {

    private Matrix4x4 camToHeadMat = new Matrix4x4();
    private Vector3 camToHeadPos;
    private Vector3 camToHeadScale;
    private Quaternion camToHeadRotQuat;

    // Use this for initialization
    void Start() {
        camToHeadMat.SetRow(0, new Vector4(0.99996991828891091f, 0.0068488870557860092f, 0.0036981684753152472f, 0.028769190211268475f));
        camToHeadMat.SetRow(1, new Vector4(-0.0042364453176018668f, 0.87750024211661348f, -0.47955819355516222f, 0.078758514798765916f));
        camToHeadMat.SetRow(2, new Vector4(-0.0065299042508945027f, 0.47952778164524124f, 0.8775030781983123f, -0.036732876586485164f));
        camToHeadMat.SetRow(3, new Vector4(0, 0, 0, 1));
        camToHeadRotQuat = QuaternionFromMatrix(camToHeadMat);
        camToHeadPos = new Vector3(camToHeadMat[0, 3], camToHeadMat[1, 3], camToHeadMat[2, 3]);
        camToHeadScale = new Vector3(1.0f, 1.0f, 1.0f);
    }
	
	// Update is called once per frame
	void Update () {
        //Quaternion.
        transform.localRotation = camToHeadRotQuat;
        transform.localPosition = camToHeadPos;
        transform.localScale = camToHeadScale;
        //transform.localScale = new Vector3(0.5f, 0.5f, 0.5f);
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

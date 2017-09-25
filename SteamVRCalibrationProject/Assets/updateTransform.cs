using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class updateTransform : MonoBehaviour {

    private Matrix4x4 BMatInv = new Matrix4x4();
    private Quaternion quatRot;
    /*
     -0.102501    0.19387   0.975658   0.690977
0.00933648   0.980968  -0.193944   0.545085
 -0.994689 -0.0107702   -0.10236  -0.111363
 0 			0			0 			1
 */
    // Use this for initialization
    void Start () {

        Matrix4x4 BMat = new Matrix4x4();
        BMat.SetRow(0, new Vector4(-0.102501f,    0.19387f,   0.975658f,   0.690977f));
        BMat.SetRow(1, new Vector4(0.00933648f,   0.980968f, -0.193944f,   0.545085f));
        BMat.SetRow(2, new Vector4(-0.994689f, -0.0107702f, -0.10236f, -0.111363f));
        BMat.SetRow(3, new Vector4(0, 0, 0, 1));

        BMatInv = Matrix4x4.Inverse(BMat);
        quatRot = QuaternionFromMatrix(BMatInv);

    }
	
	// Update is called once per frame
	void Update () {
		//this.transform.localRotation.
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

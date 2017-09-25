using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraScript : MonoBehaviour {

 
    public float tspeed = 10.0f;
    //public float rotationSpeed = 100.0F;

    public float rotHSpeed = 2.0F;
    public float rotVSpeed = 2.0F;

    private void Awake()
    {
        //Screen
        //Cursor.visible = false;
        //CursorLockMode.Confined
        //Cursor.lockState = CursorLockMode.Confined;
    }
    // Use this for initialization
    void Start () {
        Cursor.visible = true;
    }
	
	// Update is called once per frame
	void Update () {

        if (Input.GetKey("escape"))
        {
            //Application.Quit();
            UnityEditor.EditorApplication.isPlaying = false;
        }

        float tz = Input.GetAxis("Vertical") * tspeed;
        float tx = Input.GetAxis("Horizontal") * tspeed;
        float ty = Input.GetAxis("UpDown") * tspeed;

        transform.Translate(tx * Time.deltaTime, ty * Time.deltaTime, tz * Time.deltaTime);

        float h = rotHSpeed * Input.GetAxis("Mouse X");
        float v = rotVSpeed * Input.GetAxis("Mouse Y");
        transform.Rotate(new Vector3(0, h, 0), Space.World );
        transform.Rotate(new Vector3(v, 0, 0), Space.Self);

        Cursor.lockState = CursorLockMode.Locked;

    }
}

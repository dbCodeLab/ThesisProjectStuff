using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraEyeScript : MonoBehaviour {

    public GameObject leftController;
    //GameObject rightController;

    public GameObject spriteLeftController;
    //GameObject spriteRightController;

    // Use this for initialization
    void OnEnable()
    { // start ??
      //leftController = GameObject.Find("Controller (left)");
      //rightController = GameObject.Find("Controller (right)");

        //spriteLeftController = GameObject.Find("SpriteLeftController");
        //spriteRightController = GameObject.Find("SpriteRightController");

        //spriteLeftController.transform.localPosition = new Vector3(0.0f, 0.0f, 0.5f);

    }

    // Update is called once per frame
    void Update()
    {
        ///*
        //leftController = GameObject.Find("Controller (left)");
        if (leftController != null)
        {
            spriteLeftController.transform.position = leftController.transform.position;
        }
        //*/
        //spriteLeftController.transform.position = leftController.transform.position;
        //spriteRightController.transform.position = rightController.transform.position;

        //spriteLeftController.transform.position = new Vector3(0.0f, 0.0f, 0.0f);

        //Debug.Log("left controller pos: " + leftController.transform.position);

        //Debug.Log("proasdf");
    }
}

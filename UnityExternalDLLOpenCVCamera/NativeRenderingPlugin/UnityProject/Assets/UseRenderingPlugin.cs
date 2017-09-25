using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;


public class UseRenderingPlugin : MonoBehaviour
{


    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void DebugLogDelegate(string str);
    private static DebugLogDelegate debugLogFunc = msg => Debug.Log(msg);

    [DllImport("RenderingPlugin")]
    private static extern void Set_debug_log_func(IntPtr ptr);

    [DllImport("RenderingPlugin")]
    private static extern void Debug_log_test();


    //public GameObject mainCamera;
    //private new Camera camera;

    // Native plugin rendering events are only called if a plugin is used
    // by some script. This means we have to DllImport at least
    // one function in some active script.
    // For this example, we'll call into plugin's SetTimeFromUnity
    // function and pass the current time so the plugin can animate.

    [DllImport("RenderingPlugin")]
    private static extern void SetProjectionParameters(float FovAngleY, float AspectHByW, float NearZ, float FarZ);

    [DllImport("RenderingPlugin")]
    private static extern void SetCharArrayFromUnity(System.IntPtr charArray);

    [DllImport("RenderingPlugin")]
    private static extern void SetProjectionMatrixFromUnity(System.IntPtr projMatrix);

    [DllImport("RenderingPlugin")]
    private static extern void SetViewMatrixFromUnity(System.IntPtr viewMatrix);


    [DllImport("RenderingPlugin")]


    private static extern void SetTimeFromUnity(float t);


    // We'll also pass native pointer to a texture in Unity.
    // The plugin will fill texture data from native code.

    [DllImport("RenderingPlugin")]
    private static extern void SetTextureFromUnity(System.IntPtr texture, int w, int h);

    // We'll pass native pointer to the mesh vertex buffer.
    // Also passing source unmodified mesh data.
    // The plugin will fill vertex data from native code.
    [DllImport("RenderingPlugin")]
    private static extern void SetMeshBuffersFromUnity(IntPtr vertexBuffer, int vertexCount, IntPtr sourceVertices, IntPtr sourceNormals, IntPtr sourceUVs);

    [DllImport("RenderingPlugin")]
    private static extern IntPtr GetRenderEventFunc();


    void Start()
    {

        CreateCharArrayAndPassToPlugin();
        //CreateTextureAndPassToPlugin();
        //SendMeshBuffersToPlugin();

        SetTimeFromUnity(Time.timeSinceLevelLoad);
        StartCoroutine(OnRender());
        //yield return StartCoroutine("CallPluginAtEndOfFrames");
    }

    private Byte[] buff = new Byte[1000];
    private GCHandle buffHandle;

    private void CreateCharArrayAndPassToPlugin()
    {
        buffHandle = GCHandle.Alloc(buff, GCHandleType.Pinned);
        SetCharArrayFromUnity(buffHandle.AddrOfPinnedObject());
    }

    private void CreateTextureAndPassToPlugin()
    {
        // Create a texture
        Texture2D tex = new Texture2D(256, 256, TextureFormat.ARGB32, false);
        // Set point filtering just so we can see the pixels clearly
        tex.filterMode = FilterMode.Point;
        // Call Apply() so it's actually uploaded to the GPU
        tex.Apply();

        // Set texture onto our material
        GetComponent<Renderer>().material.mainTexture = tex;

        // Pass texture pointer to the plugin
        SetTextureFromUnity(tex.GetNativeTexturePtr(), tex.width, tex.height);
    }

    private void SendMeshBuffersToPlugin()
    {
        var filter = GetComponent<MeshFilter>();
        var mesh = filter.mesh;
        // The plugin will want to modify the vertex buffer -- on many platforms
        // for that to work we have to mark mesh as "dynamic" (which makes the buffers CPU writable --
        // by default they are immutable and only GPU-readable).
        mesh.MarkDynamic();

        // However, mesh being dynamic also means that the CPU on most platforms can not
        // read from the vertex buffer. Our plugin also wants original mesh data,
        // so let's pass it as pointers to regular C# arrays.
        // This bit shows how to pass array pointers to native plugins without doing an expensive
        // copy: you have to get a GCHandle, and get raw address of that.
        var vertices = mesh.vertices;
        var normals = mesh.normals;
        var uvs = mesh.uv;
        GCHandle gcVertices = GCHandle.Alloc(vertices, GCHandleType.Pinned);
        GCHandle gcNormals = GCHandle.Alloc(normals, GCHandleType.Pinned);
        GCHandle gcUV = GCHandle.Alloc(uvs, GCHandleType.Pinned);
        
        SetMeshBuffersFromUnity(mesh.GetNativeVertexBufferPtr(0), mesh.vertexCount, gcVertices.AddrOfPinnedObject(), gcNormals.AddrOfPinnedObject(), gcUV.AddrOfPinnedObject());

        gcVertices.Free();
        gcNormals.Free();
        gcUV.Free();
    }

    private void Update()
    {
        //Debug.Log("asd");
 
    }

    void OnDestroy()
    {
        //ReleaseCamera(camera_);
        Debug.Log("OnDestroy");
    }

    IEnumerator OnRender()
    {
        for (;;)
        {
        
            // Wait until all frame rendering is done
            yield return new WaitForEndOfFrame();

            // Set time for the plugin
            SetTimeFromUnity(Time.timeSinceLevelLoad);


            //SetProjectionParameters(Camera.current.fieldOfView * Mathf.Deg2Rad, Camera.current.aspect, Camera.current.nearClipPlane, Camera.current.farClipPlane);
            ///*
            Matrix4x4 projMatrix = GL.GetGPUProjectionMatrix(Camera.current.projectionMatrix, false); //Camera.current.projectionMatrix;// GL.GetGPUProjectionMatrix(Camera.current.projectionMatrix, false);//camera.projectionMatrix;// GL.GetGPUProjectionMatrix(camera.projectionMatrix, false).transpose;          
            GCHandle projMatrixHandle = GCHandle.Alloc(projMatrix, GCHandleType.Pinned);
            SetProjectionMatrixFromUnity(projMatrixHandle.AddrOfPinnedObject());
            //*/

            //Camera.current.worldToCameraMatrix
            Matrix4x4 viewMatrix = Camera.current.worldToCameraMatrix;
            //viewMatrix.SetRow(2, -viewMatrix.GetRow(2));
            GCHandle viewMatrixHandle = GCHandle.Alloc(viewMatrix, GCHandleType.Pinned);
            SetViewMatrixFromUnity(viewMatrixHandle.AddrOfPinnedObject());

            //Debug.Log("View Matrix:\n" + viewMatrix + "\n");
            /*
            //Debug.Log("Proj Matrix:\n" + projMatrix + "\n");
            Debug.Log("Near: " + camera.nearClipPlane);
            Debug.Log("Far: " + camera.farClipPlane);
            Debug.Log("a: " + camera.aspect);
            Debug.Log("fov: " + camera.fieldOfView + " " + camera.fieldOfView * Mathf.Deg2Rad);
            */




            //string result = System.Text.Encoding.UTF8.GetString(buff);
            //Debug.Log(result);
            //UnityEditor.EditorApplication.isPlaying = false;


            // Issue a plugin event with arbitrary integer identifier.
            // The plugin can distinguish between different
            // things it needs to do based on this ID.
            // For our simple plugin, it does not matter which ID we pass here.
            GL.IssuePluginEvent(GetRenderEventFunc(), 1);

            /*
            var callback = new DebugLogDelegate(debugLogFunc);
            var ptr = Marshal.GetFunctionPointerForDelegate(callback);
            Set_debug_log_func(ptr);
            Debug_log_test();
            */


            //String test = new String(buff);
            //Byte[] buff = { 48, 49 };
            //int a = (int)buff[0];



            //Debug.Log(camera.transform.position.x + " " + camera.transform.position.y + " " + camera.transform.position.z);
            //Debug.Log("View Matrix:\n" + viewMatrix);
            //Debug.Log("View Matrix:\n" + viewMatrix + "\nEye Matrix:" + camera.cameraToWorldMatrix.transpose + "\nGL model view: " + GL.modelview.transpose);
            //Debug.Log("Proj Matrix:\n" + camera.projectionMatrix + "\nGPU Proj Matrix:" + projMatrix);

            //Debug.Log("\n");
            //Debug.Log(camera.worldToCameraMatrix);
        }
    }

    /*
    private IEnumerator CallPluginAtEndOfFrames()
    {
        while (true)
        {
            

        }
    }
    */
}



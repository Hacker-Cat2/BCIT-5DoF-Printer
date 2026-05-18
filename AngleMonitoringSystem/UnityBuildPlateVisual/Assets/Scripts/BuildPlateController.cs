using UnityEngine;
using TMPro;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine.UI;

[ExecuteInEditMode()]

public class BuildPlateController : MonoBehaviour
{
    // ROTATION
    public Transform pitchParent;
    public Transform yawParent;

    public float pitch = 0f;
    // public float yaw = 0f;
    public float commandedPitch = 0f;
    public float commandedYaw = 0f;

    // POSITION
    public float currentX = 0f;
    public float currentY = 0f;
    public float currentZ = 0f;

    public float errorPitch = 0f;
    public float errorYaw = 0f;


    // PERCENT
    public Image percentBarMask;
    public float decValue = 0f;
    private float percentValue = 0f;
    private string pendingStatus = "-";
    public char duetStatus = '-';


    // UI TEXT
    public TextMeshProUGUI pitchText;
    // public TextMeshProUGUI yawText;
    public TextMeshProUGUI comPitchText;
    public TextMeshProUGUI comYawText;
    public TextMeshProUGUI xText;
    public TextMeshProUGUI yText;
    public TextMeshProUGUI zText;
    public TextMeshProUGUI percentText;

    public TextMeshProUGUI errorText;
    public TextMeshProUGUI statusText;

    public Button pauseButton;
    public Button resumeButton;

    public Image statusBG;
    Color badColour  = new Color(0.5f, 0.2f, 0.08f);  // orange-red
    Color goodColour = new Color(0.1f, 0.25f, 0.15f);  // green

    Color pauseColour = new Color(0.1647059f, 0.07843138f, 0.0627451f);  // red
    Color resumeColour = new Color(0.0627451f, 0.1647059f, 0.07843138f);  // orange-red
    Color greyColour = new Color(0.1603774f, 0.1603774f, 0.1603774f);  // grey






    // UDP
    private int udpPort = 1234;
    private string esp32IP = "192.168.4.1";
    private int esp32SendPort = 1234;

    private UdpClient udpClient;
    private UdpClient sendClient;
    private Thread receiveThread;
    private bool running = false;


    void Start() {
        sendClient = new UdpClient();

        try {
            udpClient = new UdpClient(udpPort);
            running = true;

            receiveThread = new Thread(ReceiveData);
            receiveThread.IsBackground = true;
            receiveThread.Start();

            Debug.Log("UDP listener started on port " + udpPort);
            if (statusText != null) statusText.text = "Connecting...";
        }
        catch (System.Exception e) {
            Debug.LogError("Failed to start UDP listener: " + e.Message);
        }
    }

    // Background thread: continuously receives and parses UDP packets from ESP32
    void ReceiveData() {
        IPEndPoint serverInfo = new IPEndPoint(IPAddress.Any, udpPort);
        while (running) {
            try {
                byte[] data = udpClient.Receive(ref serverInfo);
                string message = Encoding.UTF8.GetString(data).Trim();

                if (message.StartsWith("DATA")) {
                    commandedPitch = ParseValue(message, "CP:");
                    pitch          = ParseValue(message, "MP:");
                    commandedYaw   = ParseValue(message, "CY:");
                    currentX       = ParseValue(message, "X:");
                    currentY       = ParseValue(message, "Y:");
                    currentZ       = ParseValue(message, "Z:");
                    decValue       = ParseValue(message, "FP:");
                    errorPitch     = ParseValue(message, "EP:");

                    duetStatus     = parseChar(message, "S:");
                
                }
                else if (message.StartsWith("ERROR")) {
                    pendingStatus = "BAD";  // pitch error exceeded threshold
                }
                else if (message.StartsWith("GOOD")) {
                    pendingStatus = "GOOD"; // pitch within acceptable range
                } else{
                    continue; // Ignore unknown messages
                }
            }
            catch (System.Exception e)
            {
                if (running) Debug.LogError("UDP receive error: " + e.Message);
            }
        }
    }


    float ParseValue(string data, string key)
    {
        int start = data.IndexOf(key);
        if (start == -1) return 0f;

        start += key.Length;
        int end = data.IndexOf(',', start);
        if (end == -1) end = data.Length;

        float val;
        float.TryParse(data.Substring(start, end - start), out val);
        return val;
    }

    char parseChar(string data, string key) {
        int start = data.IndexOf(key);
        if (start == -1) return '-';
        return data[start + key.Length];
    }



    void Update()
    {
        // APPLY ROTATION
        if (pitchParent != null) {
            if (pitch > 50) pitch = 50;
            else if (pitch < 0) pitch = 0;
            pitchParent.localRotation = Quaternion.Euler(0, -pitch, 0);

        }

        if (yawParent != null) {
            // yawParent.localRotation = Quaternion.Euler(0, 0, yaw);
            yawParent.localRotation = Quaternion.Euler(0, 0, commandedYaw );

        }
        // UPDATE PERCENT BAR
        if (percentBarMask != null) {
            percentBarMask.fillAmount = decValue;
            percentValue = Mathf.Clamp(decValue * 100f, 0f, 100f);
        }

        // UPDATE UI TEXT
        if (pitchText   != null) pitchText.text   = $"{pitch:F2}°";
        if (comPitchText != null) comPitchText.text = $"{commandedPitch:F2}°";
        // if (yawText     != null) yawText.text     = $"{yaw:F2}°";
        if (comYawText   != null) comYawText.text   = $"{commandedYaw:F2}°";
        if (xText       != null) xText.text       = $"{currentX:F2} mm";
        if (yText       != null) yText.text       = $"{currentY:F2} mm";
        if (zText       != null) zText.text       = $"{currentZ:F2} mm";
        if (percentText != null) percentText.text = $"{percentValue:F2}%";
        if (errorText != null) errorText.text = $"Pitch error: {errorPitch:F2}°";

        if (pendingStatus != null && pendingStatus != "") {
            statusText.text  = pendingStatus == "BAD" ? "BAD" : "GOOD";
            statusBG.color = pendingStatus == "BAD" ? badColour : goodColour;
        }

        if (pauseButton != null && resumeButton != null) {
            bool isntActive = duetStatus != 'A' && duetStatus != 'D' && duetStatus != 'I';
            bool isntPaused = duetStatus != 'R' && duetStatus != 'P';

            pauseButton.interactable  = isntActive;
            resumeButton.interactable = isntPaused;

            pauseButton.image.color = isntActive ? pauseColour : greyColour;
            resumeButton.image.color = isntPaused ? resumeColour : greyColour;
        }


    }


    void SendCommand(string command)
    {
        try {
            byte[] data = Encoding.UTF8.GetBytes(command);
            sendClient.Send(data, data.Length, esp32IP, esp32SendPort);
            Debug.Log(command);
        }
        catch (System.Exception e) {
            Debug.LogError("Send error: " + e.Message);
        }
    }

    public void PauseButton()  { SendCommand("PAUSE"); }
    public void ResumeButton() { SendCommand("RESUME"); }


    void OnApplicationQuit() {
        running = false;
        udpClient?.Close();
        sendClient?.Close();
    }
}

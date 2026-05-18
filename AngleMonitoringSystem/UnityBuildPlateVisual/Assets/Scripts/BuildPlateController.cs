/*
 * Build Plate Controller
 * Unity C# script for real-time angle monitoring visualization
 *
 * Author: Margareth Sabate
 * Team: Molten Motion (Kassia Ferguson, Keaton, Margareth Sabate)
 * Date: 2026
 *
 * OVERVIEW:
 * Receives UDP packets from ESP32 containing build plate angles, position,
 * and status. Displays real-time 3D visualization of build plate orientation
 * with pitch/yaw rotation. Shows error status with color-coded background
 * (green = good, red = error). Provides pause/resume buttons for manual
 * print control.
 *
 * COMMUNICATION:
 * - ESP32 → Unity : UDP packets at 10 Hz (angles, position, status, error)
 * - Unity → ESP32 : UDP commands (PAUSE, RESUME)
 *
 * UDP PACKET FORMAT:
 * "DATA,CP:xx,MP:xx,CY:xx,X:xx,Y:xx,Z:xx,FP:xx,S:x,EP:xx"
 */

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
    // ROTATION TRANSFORMS
    public Transform pitchParent;  // Rotates around Y-axis for pitch
    public Transform yawParent;    // Rotates around Z-axis for yaw

    // ANGLE DATA
    public float pitch = 0f;           // Measured pitch from BNO055
    public float commandedPitch = 0f;  // Commanded pitch from Duet
    public float commandedYaw = 0f;    // Commanded yaw from Duet

    // POSITION DATA
    public float currentX = 0f;
    public float currentY = 0f;
    public float currentZ = 0f;

    // ERROR DATA
    public float errorPitch = 0f;
    public float errorYaw = 0f;

    // PRINT PROGRESS
    public Image percentBarMask;       // Progress bar fill mask
    public float decValue = 0f;        // Fraction printed (0.0-1.0)
    private float percentValue = 0f;   // Percentage (0-100)
    private string pendingStatus = "-";
    public char duetStatus = '-';      // I=Idle, P=Printing, A=Paused, etc.

    // UI TEXT ELEMENTS
    public TextMeshProUGUI pitchText;
    public TextMeshProUGUI comPitchText;
    public TextMeshProUGUI comYawText;
    public TextMeshProUGUI xText;
    public TextMeshProUGUI yText;
    public TextMeshProUGUI zText;
    public TextMeshProUGUI percentText;
    public TextMeshProUGUI errorText;
    public TextMeshProUGUI statusText;

    // BUTTONS
    public Button pauseButton;
    public Button resumeButton;

    // BACKGROUND COLORS
    public Image statusBG;
    Color badColour    = new Color(0.5f, 0.2f, 0.08f);       // Red (error state)
    Color goodColour   = new Color(0.1f, 0.25f, 0.15f);      // Green (normal state)
    Color pauseColour  = new Color(0.1647059f, 0.07843138f, 0.0627451f);
    Color resumeColour = new Color(0.0627451f, 0.1647059f, 0.07843138f);
    Color greyColour   = new Color(0.1603774f, 0.1603774f, 0.1603774f);

    // UDP SETTINGS
    private int udpPort = 1234;
    private string esp32IP = "192.168.4.1";
    private int esp32SendPort = 1234;

    // UDP CLIENTS AND THREADING
    private UdpClient udpClient;    // Receives data from ESP32
    private UdpClient sendClient;   // Sends commands to ESP32
    private Thread receiveThread;   // Background thread for receiving
    private bool running = false;


    /**
     * Initialize UDP clients and start background receive thread
     */
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

    /**
     * Background thread: continuously receive and parse UDP packets
     * Runs at ~10 Hz based on ESP32 send rate
     */
    void ReceiveData() {
        IPEndPoint serverInfo = new IPEndPoint(IPAddress.Any, udpPort);
        while (running) {
            try {
                byte[] data = udpClient.Receive(ref serverInfo);
                string message = Encoding.UTF8.GetString(data).Trim();

                // Parse DATA packet
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
                // Parse ERROR status
                else if (message.StartsWith("ERROR")) {
                    pendingStatus = "BAD";
                }
                // Parse GOOD status
                else if (message.StartsWith("GOOD")) {
                    pendingStatus = "GOOD";
                } else {
                    continue;
                }
            }
            catch (System.Exception e)
            {
                if (running) Debug.LogError("UDP receive error: " + e.Message);
            }
        }
    }


    /**
     * Parse float value from UDP packet given a key
     * Example: "CP:45.23" returns 45.23
     */
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

    /**
     * Parse char value from UDP packet given a key
     * Example: "S:P" returns 'P'
     */
    char parseChar(string data, string key) {
        int start = data.IndexOf(key);
        if (start == -1) return '-';
        return data[start + key.Length];
    }


    /**
     * Update rotation, UI, and button states every frame
     */
    void Update()
    {
        // Apply pitch rotation (clamped to 0-50°)
        if (pitchParent != null) {
            if (pitch > 50) pitch = 50;
            else if (pitch < 0) pitch = 0;
            pitchParent.localRotation = Quaternion.Euler(0, -pitch, 0);
        }

        // Apply yaw rotation (commanded, not measured)
        if (yawParent != null) {
            yawParent.localRotation = Quaternion.Euler(0, 0, commandedYaw);
        }

        // Update progress bar
        if (percentBarMask != null) {
            percentBarMask.fillAmount = decValue;
            percentValue = Mathf.Clamp(decValue * 100f, 0f, 100f);
        }

        // Update UI text
        if (pitchText    != null) pitchText.text    = $"{pitch:F2}°";
        if (comPitchText != null) comPitchText.text = $"{commandedPitch:F2}°";
        if (comYawText   != null) comYawText.text   = $"{commandedYaw:F2}°";
        if (xText        != null) xText.text        = $"{currentX:F2} mm";
        if (yText        != null) yText.text        = $"{currentY:F2} mm";
        if (zText        != null) zText.text        = $"{currentZ:F2} mm";
        if (percentText  != null) percentText.text  = $"{percentValue:F2}%";
        if (errorText    != null) errorText.text    = $"Pitch error: {errorPitch:F2}°";

        // Update status text and background color
        if (pendingStatus != null && pendingStatus != "") {
            statusText.text = pendingStatus == "BAD" ? "BAD" : "GOOD";
            statusBG.color  = pendingStatus == "BAD" ? badColour : goodColour;
        }

        // Update button interactivity and colors
        if (pauseButton != null && resumeButton != null) {
            bool isntActive = duetStatus != 'A' && duetStatus != 'D' && duetStatus != 'I';
            bool isntPaused = duetStatus != 'R' && duetStatus != 'P';

            pauseButton.interactable  = isntActive;
            resumeButton.interactable = isntPaused;

            pauseButton.image.color  = isntActive ? pauseColour : greyColour;
            resumeButton.image.color = isntPaused ? resumeColour : greyColour;
        }
    }


    /**
     * Send UDP command to ESP32
     */
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

    /**
     * Pause button callback (sends M25 to Duet via ESP32)
     */
    public void PauseButton()  { SendCommand("PAUSE"); }

    /**
     * Resume button callback (sends M24 to Duet via ESP32)
     */
    public void ResumeButton() { SendCommand("RESUME"); }


    /**
     * Cleanup on application quit
     */
    void OnApplicationQuit() {
        running = false;
        udpClient?.Close();
        sendClient?.Close();
    }
}
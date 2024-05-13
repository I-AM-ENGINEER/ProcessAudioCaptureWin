# ProcessAudioCaptureWin
Simple example for capture audio from app PID, find average volume and send as UDP server

Usage is very simple: 
```
volumepid.exe <pid> <dst_port>
<pid> is the process ID to capture or exclude from capture
<dst_port> destination port, for sending volume
```

Supported compiler - only visual studio compiler

Code based on microsoft example:
https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/ApplicationLoopback

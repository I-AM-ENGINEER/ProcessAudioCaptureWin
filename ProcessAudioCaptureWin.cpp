#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include "LoopbackCapture.hpp"
#include <stdio.h>
#include <cstring>

static SOCKET           serverSocket;
static const char       destinationAddr[]   = "127.0.0.1";
static unsigned short   destinationPort     =           0;
static volatile bool    g_bExitRequested    =       false;

BOOL CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
    case CTRL_C_EVENT:
        g_bExitRequested = true;
        return TRUE;
    default:
        return FALSE;
    }
}

bool initWinsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void cleanupWinsock() {
    WSACleanup();
}

SOCKET createUDPSocket(void) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(0); // Bind to port 0 to let the OS choose a free port

    if (bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Get the bound port
    sockaddr_in temp;
    int len = sizeof(temp);
    if (getsockname(sock, (SOCKADDR*)&temp, &len) != 0) {
        std::cerr << "getsockname failed with error: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

bool sendUDPPacket(SOCKET sock, const char* destinationAddr, unsigned short destinationPort, const char* data, int dataSize) {
    sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destinationPort);
    inet_pton(AF_INET, destinationAddr, &destAddr.sin_addr);

    int sentBytes = sendto(sock, data, dataSize, 0, (SOCKADDR*)&destAddr, sizeof(destAddr));
    if (sentBytes == SOCKET_ERROR) {
        std::cerr << "Error sending packet: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void frame_received_callback(UINT16* data, UINT32 frames) {
    char message[16];
    float totalVolume = 0.0f;
    for (UINT32 i = 0; i < frames; i++) {
        INT16 sample = ((UINT16*)data)[i];
        float normalizedSample = static_cast<float>(sample) / 32768.0f;
        float absSample = fabsf(normalizedSample);
        totalVolume += absSample;
    }

    float averageVolume = totalVolume / frames;
    averageVolume *= sqrt(2);
    averageVolume = std::clamp(averageVolume, 0.0f, 1.0f);

    snprintf(message, sizeof(message), "%.4f", averageVolume);
    sendUDPPacket(serverSocket, destinationAddr, destinationPort, message, strlen(message));
}

void usage() {
    std::wcout <<
        L"Usage: volumepid <pid> <dst_port>\n"
        L"\n"
        L"<pid> is the process ID to capture or exclude from capture\n"
        L"<dst_port> destination port, for sending volume\n";
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        usage();
        return 0;
    }

    DWORD processId = wcstoul(argv[1], nullptr, 0);
    if (processId == 0) {
        usage();
        return 0;
    }

    UINT16 port = wcstoul(argv[2], nullptr, 0);
    if (port == 0) {
        usage();
        return 0;
    }
    destinationPort = port;

    if (!initWinsock()) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    serverSocket = createUDPSocket();
    if (serverSocket == INVALID_SOCKET) {
        cleanupWinsock();
        return 1;
    }

    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
        std::cerr << "Error setting up Ctrl handler." << std::endl;
        return 1;
    }

    CLoopbackCapture loopbackCapture;
    HRESULT hr = loopbackCapture.StartCaptureAsync(processId, true);
    if (!FAILED(hr)) {
        loopbackCapture.AttachCallback(frame_received_callback);
        while (!g_bExitRequested) {
            Sleep(100);
        }
        loopbackCapture.StopCaptureAsync();
    }

    closesocket(serverSocket);
    cleanupWinsock();

    return 0;
}

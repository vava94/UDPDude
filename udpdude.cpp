//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************
//-- Local
#include "udpdude.h"

//-- C++
#include <stdio.h>
#include <pthread.h>

#include <utility>
#include <execution>

#define MTU 4096
#define MAX_READ 65536

UDPDude::UDPDude(std::function<void(uint8_t *, size_t, std::string, uint16_t)> callback1,
                 std::function<void(const std::string&, int)> callback2)
{
    callback_dataReceived = std::move(callback1);
    callback_log = std::move(callback2);
    if (callback_log) {
        callback_log("UDPDude started", LOG_INFO);
    }
    #ifdef _WIN32
    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    #endif
    SOCKET _descriptor1 = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET _descriptor2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (_descriptor1 == INVALID_SOCKET || _descriptor2 == INVALID_SOCKET) {
        if(callback_log)
            callback_log("UDPDude: Can't create socket", LOG_ERROR);
    } else {
        if(callback_log)
            callback_log("UDPDude: Socket created successfully", LOG_INFO);
        mTargetSocket = new TargetSocket(_descriptor1);
        mServerSocket = new TargetSocket(_descriptor2);
    }
}

//***************************************************************************************
//--- Servers Data Receive Cycle --------------------------------------------------------
//***************************************************************************************
void UDPDude::ReceiveLoop() {

    auto descriptor = mServerSocket->Descriptor();
    sockaddr_in serverAddress = mServerSocket->SockAddress(),
            clientAddress{};
    std::string clientAddressStr;

    #ifdef _WIN32
    int addrlen,
        received = 0;
    #elif __linux
    socklen_t addrlen;
    ssize_t received = 0;
    #endif

    auto *buffer = new uint8_t[MAX_READ];

    addrlen = sizeof (clientAddress);
    if (::bind(descriptor, reinterpret_cast<sockaddr*>(&serverAddress),
             sizeof(serverAddress)) < 0) {
        if(callback_log)
            callback_log("UDPDude: Bind error", 2);
        return;
    }
    mListening = true;
    if(callback_log) {
        callback_log("UDPDude: Listening started", 0);
    }
    while(mListening) {
        received = recvfrom(descriptor, buffer, MAX_READ, 0,
                             reinterpret_cast<sockaddr*>(&clientAddress), &addrlen);
        if(received > 0) {
            callback_dataReceived(buffer,
                         static_cast<size_t>(received),
                         inet_ntoa(clientAddress.sin_addr),
                         ntohs(clientAddress.sin_port));
        } else {
            break;
        }
    }
    mListening = false;
    if(callback_log) {
        callback_log("UDPDude: Listening stopped", 0);
    }
    mListening = false;
    delete[] buffer;
}

//***************************************************************************************
//--- Function that check server status -------------------------------------------------
///-- Returns TRUE if server is listening -----------------------------------------------
//***************************************************************************************
bool UDPDude::IsListening() const { return mListening; }

//***************************************************************************************
//--- Function for sending messages to the specified address ----------------------------
//***************************************************************************************
bool UDPDude::Send(uint8_t* data, size_t size, std::string address, uint16_t port) {
    bool _result = true;
    sockaddr_in addrIn{};
    addrIn.sin_family = AF_INET;
    addrIn.sin_port = htons(port);
    _result = inet_pton(AF_INET, address.data(), &(addrIn.sin_addr));
    if(!_result) return _result;

    auto *sockAddr = reinterpret_cast<sockaddr*>(&addrIn);
    mTargetSocket->SetAddress(addrIn);

    auto packets = (size - 1) / MTU + 1;
    auto lastSize = size > MTU ? size - (packets - 1) * MTU : size;

    for(ulong _i = 0; _i < packets; _i++) {
        if(sendto(mTargetSocket->Descriptor(), &data[_i * MTU],
                  (_i == (packets - 1)) ? lastSize : MTU,
                  MSG_CONFIRM, sockAddr, sizeof (addrIn)) < 0) {

            _result = false;
            if(callback_log) {
                callback_log("UDPDude: Sending error", LOG_ERROR);
            }
        }
    }
    return _result;
}

//***************************************************************************************
//--- Function that launches UDP server -------------------------------------------------
//***************************************************************************************
void UDPDude::StartServer(uint16_t port) {
    sockaddr_in addrIn{};
    addrIn.sin_family = AF_INET;
    addrIn.sin_port = htons(port);
    addrIn.sin_addr.s_addr = INADDR_ANY;
    mServerSocket->SetAddress(addrIn);
    mServerThread = new std::thread([this] { ReceiveLoop(); });
}

//***************************************************************************************
//--- Function that stops UDP server ----------------------------------------------------
//***************************************************************************************
void UDPDude::StopServer() {
    mListening = false;
    close(mServerSocket->Descriptor());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if(mServerThread != nullptr) {
        mServerThread->join();
    }
}

//***************************************************************************************
//--- Destructor ------------------------------------------------------------------------
//***************************************************************************************

UDPDude::~UDPDude() {
    if(mListening) StopServer();
}

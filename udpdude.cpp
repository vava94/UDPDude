//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************
//-- Local
#include "udpdude.h"

//-- C++
#include <unistd.h>
#include <utility>


UDPDude::UDPDude(std::function<void(uint8_t *, size_t,const std::string&, uint16_t)> callback1,
                 std::function<void(const std::string&, int)> callback2) {
    callback_dataReceived = std::move(callback1);
    callback_log = std::move(callback2);
    if (callback_log) {
        callback_log("UDPDude started", LOG_INFO);
    }
    #ifdef _WIN32
    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    #endif
    SOCKET descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor == INVALID_SOCKET) {
        if(callback_log)
            callback_log("UDPDude: Can't create socket", LOG_ERROR);
    } else {
        if(callback_log)
            callback_log("UDPDude: Socket created successfully", LOG_INFO);
        mSocket = new TargetSocket(descriptor);
        sockaddr_in sockaddrIn{};
        sockaddrIn.sin_family    = AF_INET; // IPv4
        sockaddrIn.sin_addr.s_addr = INADDR_ANY;
    }
}

void UDPDude::ReceiveLoop() {

    bool breakable = listeningTimeout_ms > 0;
    auto descriptor = mSocket->Descriptor();
    sockaddr_in socketAddress = mSocket->SockAddress(),
            targetAddress{};
    std::string clientAddressStr;

    #ifdef _WIN32
    int addrlen,
        received = 0;
    #elif __linux
    socklen_t addrlen;
    ssize_t received = 0;
    #endif

    auto *buffer = new uint8_t[MAX_READ];

    addrlen = sizeof (targetAddress);
    if (mIsServer) {
        if (::bind(descriptor, reinterpret_cast<sockaddr *>(&socketAddress),
                   sizeof(socketAddress)) < 0) {
            if (callback_log)
                callback_log("UDPDude: Bind error", 2);
            return;
        }
    }
    mListening = true;
    if(callback_log) {
        callback_log("UDPDude: Listening started", 0);
    }

    auto timeStart = std::chrono::high_resolution_clock::now();
    while(mListening) {
        received = recvfrom(descriptor, buffer, MAX_READ, 0,
                             reinterpret_cast<sockaddr*>(&targetAddress), &addrlen);
        if(received > 0) {
            callback_dataReceived(buffer,
                         static_cast<size_t>(received),
                         inet_ntoa(targetAddress.sin_addr),
                         ntohs(targetAddress.sin_port));
        } else {
            break;
        }
        if (breakable &&
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - timeStart).count() > listeningTimeout_ms) {
            break;
        }
    }
    mListening = false;
    listeningTimeout_ms = 0;
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
bool UDPDude::Send(uint8_t* data, size_t size, const std::string& address, uint16_t port, long timeout_ms) {

    bool result;
    sockaddr_in addrIn{};
    addrIn.sin_family = AF_INET;
    addrIn.sin_port = htons(port);
    result = inet_pton(AF_INET, address.data(), &(addrIn.sin_addr));
    if(!result) return result;

    auto *cliSockAddr = reinterpret_cast<sockaddr*>(&addrIn);
    auto packets = (size - 1) / MTU + 1;
    auto lastSize = size > MTU ? size - (packets - 1) * MTU : size;

    if (!mIsServer && !mListening) {
        listeningTimeout_ms = timeout_ms;
        mSocketThread = new std::thread([this, timeout_ms] { ReceiveLoop(); });
    } else if (mListening) {
        listeningTimeout_ms += timeout_ms;
    }

    for(ulong _i = 0; _i < packets; _i++) {
        if(sendto(mSocket->Descriptor(), &data[_i * MTU],
                  (_i == (packets - 1)) ? lastSize : MTU,
                  MSG_CONFIRM, cliSockAddr, sizeof (addrIn)) < 0) {

            result = false;
            if(callback_log) {
                callback_log("UDPDude: Sending error", LOG_ERROR);
            }
        }
    }

    return result;
}

//***************************************************************************************
//--- Function that launches UDP server -------------------------------------------------
//***************************************************************************************
void UDPDude::StartServer(uint16_t port) {
    mIsServer = true;
    auto addrIn = mSocket->SockAddress();
    addrIn.sin_port = htons(port);
    mSocket->SetAddress(addrIn);
    mSocketThread = new std::thread([this] { ReceiveLoop(); });
}

//***************************************************************************************
//--- Function that stops UDP server ----------------------------------------------------
//***************************************************************************************
void UDPDude::StopListen() {
    mIsServer = false;
    mListening = false;
    close(mSocket->Descriptor());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if(mSocketThread != nullptr) {
        mSocketThread->join();
    }
}

//***************************************************************************************
//--- Destructor ------------------------------------------------------------------------
//***************************************************************************************

UDPDude::~UDPDude() {
    if(mListening) StopListen();
}

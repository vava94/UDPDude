//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************
//-- Local
#include "udpdude.h"

//-- C++
#include <stdio.h>
#include <pthread.h>

//***************************************************************************************
//--- Local & Static variables ----------------------------------------------------------
//***************************************************************************************

struct TargetSocket {
private:
    SOCKET _descriptor = 0;
    struct sockaddr_in _address;

public:
    TargetSocket(SOCKET descriptor) { _descriptor = descriptor; }
    SOCKET Descriptor() { return _descriptor; }
    string Address() { return inet_ntoa(_address.sin_addr); }
    void SetAddress(sockaddr_in address) { _address = address; }
    uint16_t Port(){ return ntohs(_address.sin_port); }
    sockaddr_in SockAddress() { return  _address; }
};

static TargetSocket *targetSocket, *serverSocket;
static bool listening = false;
static pthread_t serverThread;

#define MTU 4096
#define MAX_READ 65536

#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 3

//***************************************************************************************
//--- Callbacks -------------------------------------------------------------------------
//***************************************************************************************

static void (*DataReceived)(uint8_t* data, size_t size, string address, uint16_t port);
static void (*Log)(string text, int type);

//***************************************************************************************
//--- Constructor -----------------------------------------------------------------------
///-- Arguments are callbacks -----------------------------------------------------------
///-- 1) Callback for received Data -----------------------------------------------------
///-- 2) Callback for logging function (not necessary) ----------------------------------
//***************************************************************************************
UDPDude::UDPDude(void (*DataReceivedCallBack)(uint8_t*, size_t, string, uint16_t),
        void (*LogCallback)(string, int))
{
    DataReceived = DataReceivedCallBack;
    Log = LogCallback;
    Log("UDP Процесс запущен", 0);
    #ifdef _WIN32
    WSADATA              wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    #endif
    SOCKET _descriptor1 = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET _descriptor2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (_descriptor1 == INVALID_SOCKET || _descriptor2 == INVALID_SOCKET) {
        if(Log != nullptr)
            Log("UDP: Invalid socket", LOG_ERROR);
    } else {
        if(Log != nullptr)
            Log("UDP: Socket created successfully", LOG_INFO);
        targetSocket = new TargetSocket(_descriptor1);
        serverSocket = new TargetSocket(_descriptor2);
    }
}

//***************************************************************************************
//--- Servers Data Receive Cycle --------------------------------------------------------
//***************************************************************************************
void *UDPDude::ReceiveLoop(void *arg) {
    auto _serverSocket = reinterpret_cast<TargetSocket*>(arg);
    auto _descriptor = _serverSocket->Descriptor();
    sockaddr_in _serverAddress = _serverSocket->SockAddress(),
            _clientAddress;
    string _clientAddressStr = "";

    #ifdef _WIN32
    int _addrlen,
        _received = 0;
    #elif __linux
    socklen_t _addrlen;
    ssize_t _received = 0;
    #endif

    uint8_t *_buffer = static_cast<uint8_t*>(malloc(MAX_READ));

    _addrlen = sizeof (_clientAddress);
    if (bind(_descriptor, reinterpret_cast<sockaddr*>(&_serverAddress),
             sizeof(_serverAddress)) < 0) {
        if(Log != nullptr)
            Log("UDP: Bind error", 2);
        return nullptr;
    }
    listening = true;
    if(Log != nullptr)
        Log("UDP: Listening started", 0);
    while(listening) {
        _received = recvfrom(_descriptor, _buffer, MAX_READ, 0,
                             reinterpret_cast<sockaddr*>(&_clientAddress), &_addrlen);
        if(_received > 0) {
            DataReceived(_buffer,
                         static_cast<size_t>(_received),
                         inet_ntoa(_clientAddress.sin_addr),
                         ntohs(_clientAddress.sin_port));
        } else {
            break;
        }
    }
    listening = false;
    if(Log != nullptr)
        Log("UDP: Listening stopped",0);
    listening = false;
    delete _buffer;
    delete _serverSocket;
    return nullptr;
}

//***************************************************************************************
//--- Function that check server status -------------------------------------------------
///-- Returns TRUE if server is listening -----------------------------------------------
//***************************************************************************************
bool UDPDude::IsListening() { return listening; }

//***************************************************************************************
//--- Function for sending messages to the specified address ----------------------------
//***************************************************************************************
bool Send(uint8_t* data, size_t size, string address, uint16_t port) {
    bool _result = true;
    sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _result = inet_pton(AF_INET, address.data(), &(_addr.sin_addr));
    if(!_result) return _result;

    sockaddr *_sAddr = reinterpret_cast<sockaddr*>(&_addr);
    targetSocket->SetAddress(_addr);

    auto _packets = (size - 1) / MTU + 1;
    auto _lastSize = size > MTU ? size - (_packets - 1) * MTU : size;

    for(ulong _i = 0; _i < _packets; _i++) {
        if(sendto(targetSocket->Descriptor(), &data[_i * MTU],
                  (_i == (_packets - 1)) ? _lastSize : MTU,
                  MSG_CONFIRM, _sAddr, sizeof (_addr)) < 0) {

            if(Log != nullptr) {
                _result = false;
                Log("UDP: Sending error", LOG_ERROR);
            }
        }
    }
    delete _sAddr;

    return _result;
}

//***************************************************************************************
//--- Function that launches UDP server -------------------------------------------------
//***************************************************************************************
void UDPDude::StartServer(uint16_t port) {
    sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = INADDR_ANY;
    serverSocket->SetAddress(_addr);
    pthread_create(&serverThread, nullptr, ReceiveLoop, serverSocket);
}

//***************************************************************************************
//--- Function that stops UDP server ----------------------------------------------------
//***************************************************************************************
void UDPDude::StopServer() {
    listening = false;
    pthread_cancel(serverThread);
}

//***************************************************************************************
//--- Destructor ------------------------------------------------------------------------
//***************************************************************************************

UDPDude::~UDPDude() {
    if(listening) StopServer();
}

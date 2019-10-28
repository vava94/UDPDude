//***************************************************************************************
//--- Заголовочные ----------------------------------------------------------------------
//***************************************************************************************
//-- Локальные
#include "udpdude.h"

//-- C++
#include <stdio.h>
#include <pthread.h>

//***************************************************************************************
//--- Локальные статические переменные --------------------------------------------------
//***************************************************************************************

struct TargetSocket {
private:
    SOCKET descriptor = 0;
    struct sockaddr_in address;
    uint16_t port = 0;

public:
    TargetSocket(SOCKET descriptor) { this->descriptor = descriptor; }
    SOCKET GetDescriptor() { return descriptor; }
    sockaddr_in GetAddress() { return address; }
    void SetAddress(sockaddr_in address) { this->address = address; }
    uint16_t GetPort(){ return port; }
    void SetPort(uint16_t port){ this->port = port; }
};

static TargetSocket *targetSocket, *servSocket;
static bool listening = false;
static pthread_t listenThread;
static UDPDude *staticThis;

//***************************************************************************************
//--- Функции обратного вызова ----------------------------------------------------------
//***************************************************************************************

static void (*DataReceived)(char* data, long size);
static void (*Log)(string text, int type);

//***************************************************************************************
//--- Конструктор -----------------------------------------------------------------------
///-- Аргументами конструктора является функция обратного вызова для обработки ----------
///-- полученных данных и функция обратного вызова для лога. ----------------------------
///-- Функцией обратного вызова принимаются следующие аргументы: адрес отправителя, -----
///-- порт отправителя, указателя на массив принятых данных, размер этого массива. ------
//***************************************************************************************

UDPDude::UDPDude(void (*DataReceivedCallBack)(char* data, long size),
        void (*LogCallback)(string text, int type))
{
    staticThis= this;
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
        Log("Ошибка создания сокета", 3);
    } else {
        Log("Сокет успешно создан", 0);
        targetSocket = new TargetSocket(_descriptor1);
        servSocket = new TargetSocket(_descriptor2);
    }


}

//***************************************************************************************
//--- Функция цикла приёма данных -------------------------------------------------------
//***************************************************************************************

void *UDPDude::ReceiveLoop(void *arg)
{
    auto _serverSocket = reinterpret_cast<TargetSocket*>(arg);
    auto _descriptor = _serverSocket->GetDescriptor();
    sockaddr_in _addrIn, _otherAddr;
    #ifdef _WIN32
    int _addrlen,
        _received = 0, _totalReceived = 0;
    #elif __linux
    socklen_t _addrlen;
    ssize_t _received = 0, _totalReceived = 0;
    #endif

    char *_buffer = static_cast<char*>(malloc(1024));
    _addrIn.sin_family    = AF_INET;
    _addrIn.sin_addr.s_addr = INADDR_ANY;
    _addrIn.sin_port = htons(targetSocket->GetPort());
    _addrlen = sizeof (_otherAddr);
    if (bind(_descriptor, reinterpret_cast<sockaddr*>(&_addrIn), sizeof(_addrIn)) < 0) {
        Log("Ошибка связки сокета", 2);
        return nullptr;
    }
    listening = true;
    Log("Ожидание данных запущено", 0);
    do {
        _received = recvfrom(_descriptor, _buffer, 1024, 0,
                             reinterpret_cast<sockaddr*>(&_otherAddr), &_addrlen);
        string _s = _buffer;
        _totalReceived += _received;
        DataReceived(_buffer, _received);

    } while (_received > -2);
    Log("Ожидание данных остановлено",0);
    listening = false;
    return nullptr;
}

//***************************************************************************************
//--- Функция установки получателя ------------------------------------------------------
//***************************************************************************************

void UDPDude::SetReceiver(string address, unsigned short port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.data());
    targetSocket->SetAddress(addr);
    targetSocket->SetPort(port);
    if(listening) {
        pthread_cancel(listenThread);
    }
    pthread_create(&listenThread, nullptr, ReceiveLoop, servSocket);
}

//***************************************************************************************
//--- Функция отправки данных -----------------------------------------------------------
///-- Аргуенты: адрес получателя, порт получателя, массив данных, размер массива. -------
//***************************************************************************************

void UDPDude::Send(char *data, size_t size)
{
    auto _descriptor = targetSocket->GetDescriptor();
    auto _addr = targetSocket->GetAddress();
    sendto(_descriptor, data, size, 0,
           reinterpret_cast<sockaddr*>(&_addr), sizeof (_addr));
}

//***************************************************************************************
//--- Деструктор ------------------------------------------------------------------------
//***************************************************************************************

UDPDude::~UDPDude() {

}

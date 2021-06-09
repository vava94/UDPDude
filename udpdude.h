#ifndef UDPDUDE_H
#define UDPDUDE_H
//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************
//-- С++
#include <functional>
#include <string>

#ifdef _WIN32
#include <winsock2.h>

#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

typedef int SOCKET;
#define INVALID_SOCKET  static_cast<SOCKET>(~0)
#endif

///**************************************************************************************
///--- Class for UDP operating ----------------------------------------------------------
///**************************************************************************************
class UDPDude
{

public:
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 3
    ///**************************************************************************************
    ///--- Constructor ----------------------------------------------------------------------
    ///-- Arguments are callbacks -----------------------------------------------------------
    ///-- 1) Callback for received Data -----------------------------------------------------
    ///-- 2) Callback for logging function (not necessary) ----------------------------------
    ///**************************************************************************************
    explicit UDPDude(std::function<void(uint8_t* data,size_t size, std::string address,uint16_t port)> callback_dataReceived,
                     std::function<void(const std::string&, int)> callback_log
    );
    ~UDPDude(); // Destructor

    [[nodiscard]] bool IsListening() const; // return true if server is listening
    // Sending function
    bool Send(uint8_t* data, size_t size, std::string address, uint16_t port);
    void StartServer(uint16_t port); // UDP Server start
    void StopServer(); // UDP Server stop

private:
    class TargetSocket {
    private:
        SOCKET _descriptor = 0;
        struct sockaddr_in _address{};

    public:
        explicit TargetSocket(SOCKET descriptor) { _descriptor = descriptor; }
        [[nodiscard]] SOCKET Descriptor() const { return _descriptor; }
        [[nodiscard]] std::string Address() const { return inet_ntoa(_address.sin_addr); }
        void SetAddress(sockaddr_in address) { _address = address; }
        [[nodiscard]] uint16_t Port() const{ return ntohs(_address.sin_port); }
        sockaddr_in SockAddress() { return  _address; }
    };

    bool mListening = false;
    std::thread *mServerThread;
    TargetSocket *mTargetSocket, *mServerSocket;

    std::function<void(uint8_t*,size_t, std::string,uint16_t)> callback_dataReceived = nullptr;
    std::function<void(const std::string&, int)> callback_log = nullptr;
    void ReceiveLoop();    // Cycle for receiving

 };

#endif // UDPDUDE_H


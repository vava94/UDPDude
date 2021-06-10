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
#include <arpa/inet.h>
#include <sys/socket.h>
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
#define MTU 4096
#define MAX_READ 4096
#define LOG_INFO 0
#define LOG_WARNING 1
#define LOG_ERROR 3
    ///**************************************************************************************
    ///--- Constructor ----------------------------------------------------------------------
    ///-- Arguments are callbacks -----------------------------------------------------------
    ///-- 1) Callback for received Data -----------------------------------------------------
    ///-- 2) Callback for logging function (not necessary) ----------------------------------
    ///**************************************************************************************
    explicit UDPDude(std::function<void(uint8_t* data,size_t size, const std::string& address,uint16_t port)> callback_dataReceived,
                     std::function<void(const std::string&, int)> callback_log
    );
    ~UDPDude(); // Destructor

    [[nodiscard]] bool IsListening() const; // return true if server is listening
    /**
     * Function for Sending data
     * @param data
     * @param size - data size.
     * @param address - destination address.
     * @param port - destination port.
     * @param listenTimeout_ms (WORKING FOR CLIENT ONLY) - timeout waiting for a response in milliseconds. 0 - infinite.
     */
    bool Send(uint8_t* data, size_t size, const std::string& address, uint16_t port, long listenTimeout_ms = 0);
    void StartServer(uint16_t port); // UDP Server start
    void StopListen(); // UDP Server stop

private:

    class TargetSocket {
    private:
        SOCKET mDescriptor = 0;
        struct sockaddr_in mAddress{};

    public:
        explicit TargetSocket(SOCKET descriptor) { mDescriptor = descriptor; }
        [[nodiscard]] SOCKET Descriptor() const { return mDescriptor; }
        [[nodiscard]] std::string Address() const { return inet_ntoa(mAddress.sin_addr); }
        void SetAddress(sockaddr_in address) { mAddress = address; }
        [[nodiscard]] uint16_t Port() const{ return ntohs(mAddress.sin_port); }
        sockaddr_in SockAddress() { return  mAddress; }
    };

    bool mIsServer = false, mListening = false;
    long listeningTimeout_ms = 0;
    std::thread *mSocketThread;
    TargetSocket *mSocket;

    std::function<void(uint8_t*,size_t, const std::string&,uint16_t)> callback_dataReceived = nullptr;
    std::function<void(const std::string&, int)> callback_log = nullptr;
    void ReceiveLoop();    // Cycle for receiving

 };

#endif // UDPDUDE_H


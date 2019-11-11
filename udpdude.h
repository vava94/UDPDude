#ifndef UDPDUDE_H
#define UDPDUDE_H
//***************************************************************************************
//--- Includes --------------------------------------------------------------------------
//***************************************************************************************
//-- С++
#include <string>

#ifdef _WIN32
#include <winsock2.h>

#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int SOCKET;
#define INVALID_SOCKET  static_cast<SOCKET>(~0)
#endif

using namespace std;

//***************************************************************************************
//--- Class for UDP operating -----------------------------------------------------------
//***************************************************************************************
class UDPDude
{

public:
    // Constructor
    UDPDude(void (*DataReceivedCallBack)(uint8_t* data,
                                         size_t size,
                                         string address,
                                         uint16_t port),
            void (*Log)(string text, int type) = nullptr);
    ~UDPDude(); // Destructor
    bool IsListening(); // return true if server is listening
    // Sending function
    bool Send(uint8_t* data, size_t size, string address, uint16_t port);
    void StartServer(uint16_t port); // UDP Server start
    void StopServer(); // UDP Server stop

private:
    static void *ReceiveLoop(void *arg);    // Cycle for receiving

 };

#endif // UDPDUDE_H


#ifndef UDPDUDE_H
#define UDPDUDE_H
//***************************************************************************************
//--- Заголовочные ----------------------------------------------------------------------
//***************************************************************************************
//-- С++
#ifdef _WIN32
#include <winsock2.h>
#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
typedef int SOCKET;
#define INVALID_SOCKET  static_cast<SOCKET>(~0)
#endif
#include <string>

using namespace std;

//***************************************************************************************
//--- Класс приёма и передачи данных по протоколу UDP -----------------------------------
//***************************************************************************************
class UDPDude
{

public:
    // Конструктор
    UDPDude(void (*DataReceivedCallBack)(char* data, long size),
            void (*Log)(string text, int type));
    ~UDPDude(); // Деструктор
    // Функция установки получателя
    static void SetReceiver(string address, unsigned short port);
    // Функция отправки сообщений по указанному адресу
    static void Send(char* data, size_t size);
    // Обратный вызов для обработки полученных данных


private:


    static void *ReceiveLoop(void *arg);    // Функция цикла приёма данных

 };

#endif // UDPDUDE_H


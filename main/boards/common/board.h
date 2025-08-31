#ifndef BOARD_H
#define BOARD_H

#include <http.h>
#include <web_socket.h>
#include <mqtt.h>
#include <udp.h>
#include <string>

#include "backlight.h"

void *create_board();
class AudioCodec;
class Display;
class Board
{
private:
    Board(const Board &) = delete;            // 禁用拷贝构造函数
    Board &operator=(const Board &) = delete; // 禁用赋值操作
    virtual std::string GetBoardJson() = 0;

protected:
    Board();
    std::string GenerateUuid();

    // 软件生成的设备唯一标识
    std::string uuid_;

public:
    // 获取Board类的唯一实例
    // 使用静态成员函数实现单例模式，确保整个程序中只有一个Board实例
    // 返回: Board类的引用，可以用于访问Board类的成员函数和数据
    static Board &GetInstance()
    {
        // 静态局部变量，存储Board实例的指针
        // 使用static_cast转换create_board()的返回值，确保类型安全
        static Board *instance = static_cast<Board *>(create_board());
        return *instance;
    }

    virtual ~Board() = default;
    virtual std::string GetBoardType() = 0;
    virtual std::string GetUuid() { return uuid_; }
    virtual Backlight *GetBacklight() { return nullptr; }
    virtual AudioCodec *GetAudioCodec() = 0;
    virtual Display *GetDisplay();
    virtual Http *CreateHttp() = 0;
    virtual WebSocket *CreateWebSocket() = 0;
    virtual Mqtt *CreateMqtt() = 0;
    virtual Udp *CreateUdp() = 0;
    virtual void StartNetwork() = 0;
    virtual const char *GetNetworkStateIcon() = 0;
    virtual bool GetBatteryLevel(int &level, bool &charging);
    virtual std::string GetJson();
    virtual void SetPowerSaveMode(bool enabled) = 0;
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
    void *create_board()                \
    {                                   \
        return new BOARD_CLASS_NAME();  \
    }

#endif // BOARD_H

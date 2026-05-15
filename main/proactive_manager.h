#ifndef PROACTIVE_MANAGER_H
#define PROACTIVE_MANAGER_H

#include <esp_timer.h>
#include <mutex>
#include <string>
#include <vector>

/**
 * ProactiveManager - 负责宠物的“主动交互”逻辑
 * 
 * 监控设备的闲置时间、情绪状态，并在合适的时机触发主动搭话。
 */
class ProactiveManager {
public:
    static ProactiveManager& GetInstance() {
        static ProactiveManager instance;
        return instance;
    }

    void Initialize();
    
    // 当发生用户交互时调用，重置闲置计时
    void OnUserInteraction();

private:
    ProactiveManager();
    ~ProactiveManager();

    void CheckProactiveAction();
    void PerformMicroBehavior();
    static void TimerCallback(void* arg);
    static void MicroBehaviorTimerCallback(void* arg);

    // 触发主动对话
    void TriggerProactiveChat(const std::string& prompt);

    uint32_t last_interaction_time_ = 0; // 秒级时间戳
    esp_timer_handle_t check_timer_handle_ = nullptr;
    esp_timer_handle_t micro_behavior_timer_handle_ = nullptr;
    mutable std::mutex mutex_;
};

#endif // PROACTIVE_MANAGER_H

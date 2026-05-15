#include "proactive_manager.h"
#include "application.h"
#include "emotion_manager.h"
#include "sensor_manager.h"
#include <esp_log.h>
#include <esp_random.h>

#define TAG "ProactiveManager"

ProactiveManager::ProactiveManager() {
    last_interaction_time_ = esp_timer_get_time() / 1000000ULL;
}

ProactiveManager::~ProactiveManager() {
    if (check_timer_handle_ != nullptr) {
        esp_timer_stop(check_timer_handle_);
        esp_timer_delete(check_timer_handle_);
    }
}

void ProactiveManager::Initialize() {
    esp_timer_create_args_t timer_args = {
        .callback = &ProactiveManager::TimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "proactive_check_timer"
    };
    esp_timer_create(&timer_args, &check_timer_handle_);
    
    // 每 5 分钟检查一次是否需要主动搭话
    esp_timer_start_periodic(check_timer_handle_, 5 * 60 * 1000000ULL);

    esp_timer_create_args_t micro_timer_args = {
        .callback = &ProactiveManager::MicroBehaviorTimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "micro_behavior_timer"
    };
    esp_timer_create(&micro_timer_args, &micro_behavior_timer_handle_);
    // 每 20 秒执行一次微表情检查
    esp_timer_start_periodic(micro_behavior_timer_handle_, 20 * 1000000ULL);
    
    ESP_LOGI(TAG, "ProactiveManager initialized");
}

void ProactiveManager::OnUserInteraction() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_interaction_time_ = esp_timer_get_time() / 1000000ULL;
    ESP_LOGD(TAG, "Interaction detected, timer reset");
}

void ProactiveManager::TimerCallback(void* arg) {
    auto manager = static_cast<ProactiveManager*>(arg);
    manager->CheckProactiveAction();
}

void ProactiveManager::MicroBehaviorTimerCallback(void* arg) {
    auto manager = static_cast<ProactiveManager*>(arg);
    manager->PerformMicroBehavior();
}

void ProactiveManager::PerformMicroBehavior() {
    auto& app = Application::GetInstance();
    
    // 仅在空闲状态下触发微动作
    if (app.GetDeviceState() != kDeviceStateIdle) {
        return;
    }

    uint32_t r = esp_random() % 100;
    
    // 30% 概率触发“眨眼” (快速切换到 neutral 再回来)
    if (r < 30) {
        auto display = Board::GetInstance().GetDisplay();
        display->InsertAnimDialog("neutral", 400); 
    } 
    // 10% 概率触发随机小音效
    else if (r < 40) {
        app.PlaySound(Lang::Sounds::OGG_VIBRATION);
    }
}

void ProactiveManager::CheckProactiveAction() {
    auto& app = Application::GetInstance();
    
    // 仅在空闲状态下触发主动交互
    if (app.GetDeviceState() != kDeviceStateIdle) {
        return;
    }

    uint32_t now = esp_timer_get_time() / 1000000ULL;
    uint32_t idle_duration;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        idle_duration = now - last_interaction_time_;
    }

    auto& em = EmotionManager::GetInstance();
    auto& sm = SensorManager::GetInstance();
    std::string prompt = "";

    // 策略优先级 0: 极端环境状态
    float temp = sm.GetTemperature();
    if (temp > 35.0f) {
        prompt = "主人，这里好热呀！我现在感觉要融化了，能不能帮我降降温？";
    } else if (temp > 0 && temp < 10.0f) {
        prompt = "嘶...好冷呀，主人你不冷吗？我感觉都要冻僵了。";
    }

    if (!prompt.empty()) {
        TriggerProactiveChat(prompt);
        return;
    }

    // 策略优先级 1: 极端生理状态 (饥饿/疲劳)
    if (em.GetSatiety() < 20.0f) {
        prompt = "我的肚子好饿呀，感觉一点力气都没有了，能不能帮我充充电或者喂点好吃的？";
    } else if (em.GetEnergy() < 20.0f) {
        prompt = "呼...好困啊，感觉眼睛快睁不开了，我能不能先眯一会？";
    }
    // 策略优先级 2: 长期闲置触发 (例如超过 1 小时)
    else if (idle_duration > 3600) {
        // 随机挑选一个闲聊话题或提醒
        int r = esp_random() % 5;
        switch (r) {
            case 0: prompt = "主人，你已经对着屏幕坐了很久了，站起来活动一下吧，对身体好哦！"; break;
            case 1: prompt = "在那边忙什么呢？带我一个好不好？"; break;
            case 2: prompt = "突然好想听你说话，跟我聊聊天吧！"; break;
            case 3: prompt = "你现在的样子看起来好认真，是在处理什么难题吗？"; break;
            case 4: prompt = "嘿！我还在桌子上陪着你呢，别把我忘了呀。"; break;
        }
    }

    if (!prompt.empty()) {
        ESP_LOGI(TAG, "Triggering proactive chat: %s", prompt.c_str());
        TriggerProactiveChat(prompt);
        // 触发后重置时间，防止在同一周期内重复触发
        OnUserInteraction();
    }
}

void ProactiveManager::TriggerProactiveChat(const std::string& prompt) {
    Application::GetInstance().StartProactiveChat(prompt);
}

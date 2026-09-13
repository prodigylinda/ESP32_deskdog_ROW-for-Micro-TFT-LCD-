#pragma once

#include "display/lcd_display.h"
#include "anim_player.h"
#include "assets.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

#include <lvgl.h>

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

#include <freertos/semphr.h>


namespace anim {

class EmojiPlayer;

using FlushIoReadyCallback =
    std::function<bool(
        esp_lcd_panel_io_handle_t,
        esp_lcd_panel_io_event_data_t*,
        void*
    )>;

using FlushCallback =
    std::function<void(
        anim_player_handle_t,
        int,
        int,
        int,
        int,
        const void*
    )>;


class EmojiPlayer {
public:
    EmojiPlayer(
        esp_lcd_panel_handle_t panel,
        esp_lcd_panel_io_handle_t panel_io
    );

    ~EmojiPlayer();

    void StartPlayer(
        const std::string& asset_name,
        bool repeat,
        int fps
    );

    void StopPlayer();

    // 保存 AI 英文字幕
    void SetSubtitle(
        const char* text
    );


private:
    static bool OnFlushIoReady(
        esp_lcd_panel_io_handle_t panel_io,
        esp_lcd_panel_io_event_data_t* edata,
        void* user_ctx
    );

    static void OnFlush(
        anim_player_handle_t handle,
        int x_start,
        int y_start,
        int x_end,
        int y_end,
        const void* color_data
    );


    // 从内置英文字体里找字符
    bool ResolveGlyph(
        uint32_t codepoint,
        uint32_t next_codepoint,
        lv_font_glyph_dsc_t& glyph
    );

    // 算整句话有多少像素宽
    int MeasureSubtitleWidthLocked();

    // 把字幕画到底部 16px
    void RenderSubtitle(
        int x_start,
        int y_start,
        int x_end,
        int y_end,
        uint16_t* pixels
    );

    // LCD
    esp_lcd_panel_handle_t panel_ = nullptr;
    // 原来的表情动画
    anim_player_handle_t player_handle_ = nullptr;
    // 字幕数据
    SemaphoreHandle_t subtitle_mutex_ = nullptr;
    // 当前字幕
    std::vector<uint32_t> subtitle_codepoints_;

    // 当前字幕的像素宽度
    int subtitle_width_ = 0;

    // 当前字幕开始显示时间
    // 算滚动位置
    int64_t subtitle_start_us_ = 0;
};


class EmojiWidget : public Display {
public:
    EmojiWidget(
        esp_lcd_panel_handle_t panel,
        esp_lcd_panel_io_handle_t panel_io
    );

    virtual ~EmojiWidget();

    virtual void SetEmotion(
        const char* emotion
    ) override;

    virtual void SetStatus(
        const char* status
    ) override;

    // 接收回复
    virtual void SetChatMessage(
        const char* role,
        const char* content
    ) override;

    virtual void ClearChatMessages() override;


    anim::EmojiPlayer* GetPlayer()
    {
        return player_.get();
    }


private:
    void InitializePlayer(
        esp_lcd_panel_handle_t panel,
        esp_lcd_panel_io_handle_t panel_io
    );

    virtual bool Lock(
        int timeout_ms = 0
    ) override;

    virtual void Unlock() override;

    std::unique_ptr<anim::EmojiPlayer> player_;
};

}
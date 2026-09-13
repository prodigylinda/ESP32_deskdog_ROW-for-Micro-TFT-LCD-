#include <cstring>
#include "display/lcd_display.h"
#include <esp_log.h>
#include "emoji_display.h"
#include "assets/lang_config.h"
#include "assets.h"
#include <algorithm>
#include <unordered_map>
#include <tuple>
#include <esp_timer.h>

#include <esp_lcd_panel_io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

static const char *TAG = "emoji";
LV_FONT_DECLARE(BUILTIN_TEXT_FONT);

namespace anim {

// Emoji asset name mapping based on usage pattern
static const std::unordered_map<std::string, std::string> emoji_asset_name_map = {
    {"connecting", "connecting.aaf"},
    {"wake", "wake.aaf"},
    {"asking", "asking.aaf"},
    {"happy_loop", "happy_loop.aaf"},
    {"sad_loop", "sad_loop.aaf"},
    {"anger_loop", "anger_loop.aaf"},
    {"panic_loop", "panic_loop.aaf"},
    {"blink_quick", "blink_quick.aaf"},
    {"scorn_loop", "scorn_loop.aaf"}
};

bool EmojiPlayer::OnFlushIoReady(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    auto handle =
        static_cast<anim_player_handle_t>(user_ctx);

    anim_player_flush_ready(handle);

    return true;
}

void EmojiPlayer::OnFlush(
    anim_player_handle_t handle,
    int x_start,
    int y_start,
    int x_end,
    int y_end,
    const void *color_data)
{
    // user_data 现在存EmojiPlayer
    auto* self = static_cast<EmojiPlayer*>(
        anim_player_get_user_data(handle)
    );

    if (self == nullptr || self->panel_ == nullptr) {
        return;
    }
    auto* pixels = const_cast<uint16_t*>(
        static_cast<const uint16_t*>(color_data)
    );
    // 画字幕
    self->RenderSubtitle(
        x_start,
        y_start,
        x_end,
        y_end,
        pixels
    );
    // 送LCD
    esp_lcd_panel_draw_bitmap(
        self->panel_,
        x_start,
        y_start,
        x_end,
        y_end,
        pixels
    );
}

EmojiPlayer::EmojiPlayer(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
    : panel_(panel)
{
    ESP_LOGI(TAG, "Create EmojiPlayer, panel: %p, panel_io: %p", panel, panel_io);
    subtitle_mutex_ = xSemaphoreCreateMutex();
    anim_player_config_t player_cfg = {
        .flush_cb = OnFlush,
        .update_cb = NULL,
        .user_data = this,
        .flags = {.swap = true},
        .task = ANIM_PLAYER_INIT_CONFIG()
    };

    player_cfg.task.task_priority = 1;
    player_cfg.task.task_stack = 4096;
    player_handle_ = anim_player_init(&player_cfg);

    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = OnFlushIoReady,
    };
    esp_lcd_panel_io_register_event_callbacks(panel_io, &cbs, player_handle_);
    StartPlayer("connecting", true, 15);
}

EmojiPlayer::~EmojiPlayer()
{
    if (player_handle_) {
        anim_player_update(player_handle_, PLAYER_ACTION_STOP);
        anim_player_deinit(player_handle_);
        player_handle_ = nullptr;
    }
    if (subtitle_mutex_ != nullptr) {
        vSemaphoreDelete(subtitle_mutex_);
        subtitle_mutex_ = nullptr;
    }
}

void EmojiPlayer::StartPlayer(const std::string& asset_name, bool repeat, int fps)
{
    if (player_handle_) {
        uint32_t start, end;
        void *src_data = nullptr;
        size_t src_len = 0;

        auto& assets = Assets::GetInstance();
        std::string filename = emoji_asset_name_map.at(asset_name);
        if (!assets.GetAssetData(filename, src_data, src_len)) {
            ESP_LOGE(TAG, "Failed to get asset data for %s", asset_name.c_str());
            return;
        }

        anim_player_set_src_data(player_handle_, src_data, src_len);
        anim_player_get_segment(player_handle_, &start, &end);
        if(asset_name == "wake"){
            start = 7;
        }
        anim_player_set_segment(player_handle_, start, end, fps, true);
        anim_player_update(player_handle_, PLAYER_ACTION_START);
    }
}

void EmojiPlayer::StopPlayer()
{
    if (player_handle_) {
        anim_player_update(player_handle_, PLAYER_ACTION_STOP);
    }
}

void EmojiPlayer::SetSubtitle(const char* text)
{
    if (subtitle_mutex_ == nullptr) {
        return;
    }

    xSemaphoreTake(
        subtitle_mutex_,
        portMAX_DELAY
    );

    subtitle_codepoints_.clear();

    if (text != nullptr) {
        const unsigned char* p =
            reinterpret_cast<const unsigned char*>(text);

        while (*p) {
            // 当前版本只显示 ASCII 英文
            if (*p < 0x80) {
                uint32_t ch = *p++;

                // 单行字幕
                if (ch == '\n' || ch == '\r') {
                    ch = ' ';
                }

                subtitle_codepoints_.push_back(ch);
            }
            else {
                // 遇到非英文字符时显示一个 ?
                subtitle_codepoints_.push_back('?');

                ++p;

                // 跳过同一个 UTF-8 字符剩余的 continuation bytes
                while ((*p & 0xC0) == 0x80) {
                    ++p;
                }
            }
        }
    }

    // 算这一整句话有多宽
    subtitle_width_ =
        MeasureSubtitleWidthLocked();

    // 记录字幕开始时间，滚动用
    subtitle_start_us_ =
        esp_timer_get_time();

    xSemaphoreGive(
        subtitle_mutex_
    );
}

bool EmojiPlayer::ResolveGlyph(
    uint32_t codepoint,
    uint32_t next_codepoint,
    lv_font_glyph_dsc_t& glyph)
{
    if (
        lv_font_get_glyph_dsc(
            &BUILTIN_TEXT_FONT,
            &glyph,
            codepoint,
            next_codepoint
        ) &&
        !glyph.is_placeholder
    ) {
        return true;
    }

    // 如果找不到就显示 ?
    if (
        codepoint != '?' &&
        lv_font_get_glyph_dsc(
            &BUILTIN_TEXT_FONT,
            &glyph,
            '?',
            0
        )
    ) {
        return true;
    }

    return false;
}

int EmojiPlayer::MeasureSubtitleWidthLocked()
{
    int width = 0;

    for (
        size_t i = 0;
        i < subtitle_codepoints_.size();
        ++i
    ) {
        uint32_t current =
            subtitle_codepoints_[i];

        uint32_t next =
            i + 1 < subtitle_codepoints_.size()
                ? subtitle_codepoints_[i + 1]
                : 0;

        lv_font_glyph_dsc_t glyph{};

        if (
            ResolveGlyph(
                current,
                next,
                glyph
            )
        ) {
            width += glyph.adv_w;
        }
    }

    return width;
}

void EmojiPlayer::RenderSubtitle(
    int x_start,
    int y_start,
    int x_end,
    int y_end,
    uint16_t* pixels)
{
    constexpr int SCREEN_WIDTH = 160;
    constexpr int SCREEN_HEIGHT = 80;
    constexpr int SUBTITLE_TOP = 64;

    if (pixels == nullptr) {
        return;
    }

    if (y_end <= SUBTITLE_TOP) {
        return;
    }

    const int width =
        x_end - x_start;

    if (width <= 0) {
        return;
    }
    // 1. 底部 清黑


    const int clear_top =
        std::max(y_start, SUBTITLE_TOP);

    const int clear_bottom =
        std::min(y_end, SCREEN_HEIGHT);

    for (int y = clear_top; y < clear_bottom; ++y) {
        for (int x = x_start; x < x_end; ++x) {
            int local_x = x - x_start;
            int local_y = y - y_start;

            pixels[
                local_y * width + local_x
            ] = 0x0000;
        }
    }
    // 读取字幕
    if (subtitle_mutex_ == nullptr) {
        return;
    }

    if (
        xSemaphoreTake(
            subtitle_mutex_,
            pdMS_TO_TICKS(5)
        ) != pdTRUE
    ) {
        return;
    }

    if (subtitle_codepoints_.empty()) {
        xSemaphoreGive(subtitle_mutex_);
        return;
    }
    // 3. 计算位置

    constexpr int PADDING = 2;

    int text_x = PADDING;

    if (
        subtitle_width_ <=
        SCREEN_WIDTH - PADDING * 2
    ) {
        // 短句居中
        text_x =
            (
                SCREEN_WIDTH -
                subtitle_width_
            ) / 2;
    }
    else {
        // 长句滚动
        constexpr int SCROLL_MS_PER_PIXEL = 60;
        constexpr int SCROLL_GAP = 20;

        int64_t elapsed_ms =
            (
                esp_timer_get_time() -
                subtitle_start_us_
            ) / 1000;

        int travel =
            subtitle_width_ +
            SCROLL_GAP;

        int offset =
            static_cast<int>(
                (
                    elapsed_ms /
                    SCROLL_MS_PER_PIXEL
                ) %
                travel
            );

        text_x =
            PADDING - offset;
    }

    //一个字符一个字符绘制


    int cursor_x = text_x;

    for (
        size_t i = 0;
        i < subtitle_codepoints_.size();
        ++i
    ) {
        uint32_t current =
            subtitle_codepoints_[i];

        uint32_t next =
            i + 1 < subtitle_codepoints_.size()
                ? subtitle_codepoints_[i + 1]
                : 0;

        lv_font_glyph_dsc_t glyph{};

        if (
            !ResolveGlyph(
                current,
                next,
                glyph
            )
        ) {
            continue;
        }

        const lv_font_t* font =
            glyph.resolved_font;

        if (
            font == nullptr ||
            font->get_glyph_bitmap == nullptr
        ) {
            cursor_x += glyph.adv_w;
            continue;
        }

        int glyph_x =
            cursor_x +
            glyph.ofs_x;

        int glyph_y =
            SUBTITLE_TOP +
            (
                font->line_height -
                font->base_line
            ) -
            glyph.box_h -
            glyph.ofs_y;

        uint8_t old_raw =
            glyph.req_raw_bitmap;

        glyph.req_raw_bitmap = 1;

        const uint8_t* bitmap =
            static_cast<const uint8_t*>(
                font->get_glyph_bitmap(
                    &glyph,
                    nullptr
                )
            );

        glyph.req_raw_bitmap =
            old_raw;

        if (
            bitmap != nullptr &&
            glyph.format ==
                LV_FONT_GLYPH_FORMAT_A1
        ) {
            const int row_bits =
                glyph.stride != 0
                    ? glyph.stride * 8
                    : glyph.box_w;

            for (
                int gy = 0;
                gy < glyph.box_h;
                ++gy
            ) {
                for (
                    int gx = 0;
                    gx < glyph.box_w;
                    ++gx
                ) {
                    int screen_x =
                        glyph_x + gx;

                    int screen_y =
                        glyph_y + gy;

                    // 屏幕裁剪
                    if (
                        screen_x < 0 ||
                        screen_x >= SCREEN_WIDTH ||
                        screen_y < SUBTITLE_TOP ||
                        screen_y >= SCREEN_HEIGHT ||
                        screen_x < x_start ||
                        screen_x >= x_end ||
                        screen_y < y_start ||
                        screen_y >= y_end
                    ) {
                        continue;
                    }

                    int bit_index =
                        gy * row_bits +
                        gx;

                    int byte_index =
                        bit_index / 8;

                    int bit_offset =
                        7 -
                        (bit_index % 8);

                    bool pixel_on =
                        (
                            bitmap[byte_index] >>
                            bit_offset
                        ) & 0x01;

                    if (pixel_on) {
                        int local_x =
                            screen_x -
                            x_start;

                        int local_y =
                            screen_y -
                            y_start;

                        // RGB565 白
                        pixels[
                            local_y * width +
                            local_x
                        ] = 0xFFFF;
                    }
                }
            }
        }

        cursor_x +=
            glyph.adv_w;
    }

    xSemaphoreGive(
        subtitle_mutex_
    );
}

EmojiWidget::EmojiWidget(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
{
    InitializePlayer(panel, panel_io);
}

EmojiWidget::~EmojiWidget()
{

}

void EmojiWidget::SetEmotion(const char* emotion)
{
    if (!player_) {
        return;
    }

    using Param = std::tuple<std::string, bool, int>;
    static const std::unordered_map<std::string, Param> emotion_map = {
        {"happy",       {"happy_loop", true, 25}},
        {"laughing",    {"happy_loop", true, 25}},
        {"funny",       {"happy_loop", true, 25}},
        {"loving",      {"happy_loop", true, 25}},
        {"embarrassed", {"happy_loop", true, 25}},
        {"confident",   {"happy_loop", true, 25}},
        {"delicious",   {"happy_loop", true, 25}},
        {"sad",         {"sad_loop",   true, 25}},
        {"crying",      {"sad_loop",   true, 25}},
        {"sleepy",      {"sad_loop",   true, 25}},
        {"silly",       {"sad_loop",   true, 25}},
        {"angry",       {"anger_loop", true, 25}},
        {"surprised",   {"panic_loop", true, 25}},
        {"shocked",     {"panic_loop", true, 25}},
        {"thinking",    {"happy_loop", true, 25}},
        {"winking",     {"blink_quick", true, 5}},
        {"relaxed",     {"scorn_loop", true, 25}},
        {"confused",    {"scorn_loop", true, 25}},
    };

    auto it = emotion_map.find(emotion);
    if (it != emotion_map.end()) {
        const auto& [aaf, repeat, fps] = it->second;
        player_->StartPlayer(aaf, repeat, fps);
    } else if (strcmp(emotion, "neutral") == 0) {
    }
}

void EmojiWidget::SetStatus(const char* status)
{
    if (player_) {
        if (strcmp(status, Lang::Strings::LISTENING) == 0) {
            player_->StartPlayer("asking", true, 15);
        } else if (strcmp(status, Lang::Strings::STANDBY) == 0) {
            player_->StartPlayer("wake", true, 15);
        }
    }
}

void EmojiWidget::SetChatMessage(
    const char* role,
    const char* content)
{
    if (!player_) {
        return;
    }

    // 空内容直接清掉
    if (
        content == nullptr ||
        content[0] == '\0'
    ) {
        player_->SetSubtitle("");
        return;
    }

    // 用户开始讲话时，把上一条清掉
    if (
        role != nullptr &&
        strcmp(role, "user") == 0
    ) {
        player_->SetSubtitle("");
        return;
    }

    // 只显示 assistant 回复
    if (
        role != nullptr &&
        strcmp(role, "assistant") == 0
    ) {
        player_->SetSubtitle(content);
    }
}


void EmojiWidget::ClearChatMessages()
{
    if (player_) {
        player_->SetSubtitle("");
    }
}

void EmojiWidget::InitializePlayer(esp_lcd_panel_handle_t panel, esp_lcd_panel_io_handle_t panel_io)
{
    player_ = std::make_unique<EmojiPlayer>(panel, panel_io);
}

bool EmojiWidget::Lock(int timeout_ms)
{
    return true;
}

void EmojiWidget::Unlock()
{
}

} // namespace anim

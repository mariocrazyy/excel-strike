#include "Overlays.hpp"

#include "updater/Updater.hpp"
#include "gui/renderer/Renderer.hpp" // Circular dependency
#include "gui/frontend/menu/Menu.hpp" // Circular dependency
#include "assets/fonts/WeaponIcons.h"

bool Overlays::Init() {
    return GetInstance().InitImpl();
}

void Overlays::Render() {
    return GetInstance().RenderImpl();
}

bool Overlays::InitImpl() {
    auto& io = ImGui::GetIO();

    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;

	this->font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 12.0f, &cfg);
	this->font_alt = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 14.0f, &cfg);
    this->font_icons = io.Fonts->AddFontFromMemoryTTF(weapon_icon_font, weapon_icon_font_len, 16.0f,  &cfg);
	
	ImFontConfig merge_icon_cfg{};
	merge_icon_cfg.FontDataOwnedByAtlas = false;
	merge_icon_cfg.MergeMode = true;

	static const ImWchar icon_ranges[] = { 0xE000, 0xE046, 0 };
	io.Fonts->AddFontFromMemoryTTF(weapon_icon_font, weapon_icon_font_len, 12.f, &merge_icon_cfg, icon_ranges);

    // Pre allocate buffer
    this->vel_buffer.resize(static_cast<size_t>(cfg::world::velocity::sample_rate * cfg::world::velocity::sample_length));

    return true;
}

void Overlays::RenderImpl() {
    ImGui::PushFont(this->font);
    {
        RenderWatermark();

        RenderNotice();
        RenderEspStatus();

    #ifdef _DEBUG
        RenderDebugWindow();
    #endif

    }
    ImGui::PopFont();

    ImGui::PushFont(this->font_alt);
    {
        RenderSpectatorList();
        RenderSpeedChart();
        RenderRadar();
        RenderBomb();
    }
    ImGui::PopFont();
}

void Overlays::RenderWatermark() {
    if (!cfg::settings::watermark)
        return;

    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    auto snapshot = Cache::CopySnapshot();
    auto& globals = snapshot.globals;

    static int margin = 10;
    static int padding = 10;
    std::string watermark_string = "Excel-Strike";

    watermark_string += std::format(" | {}fps", (int)io.Framerate);

    if (globals.in_match)
        watermark_string += std::format(" | {}", globals.map_name);

    auto size = ImGui::CalcTextSize(watermark_string.data());

    auto rect_start = ImVec2(io.DisplaySize.x - margin - padding * 2 - size.x, margin);
    auto rect_end = ImVec2(io.DisplaySize.x - margin, margin + size.y + padding);
    auto pos = ImVec2(rect_start.x + padding, rect_start.y + padding * 0.6/* compensate font */);

    d->AddRectFilled(
        rect_start,
        rect_end,
        IM_COL32(0, 0, 0, 200),
        8.f
    );

    d->AddRect(
        rect_start,
        rect_end,
        IM_COL32(100, 100, 100, 200),
        8.f
    );

    d->AddText(
        pos,
        IM_COL32(255, 255, 255, 255),
        watermark_string.data()
    );
}

void Overlays::RenderNotice() {
    static auto status = Updater::GetStatus();
    static bool dismissed = false;

    if (dismissed || (status.notice_title.empty() && status.notice.empty()))
        return;

    if (!Renderer::IsOpen())
        return;
    auto& io = ImGui::GetIO();

    constexpr float margin = 10.f;
    constexpr float padding = 10.f;
    const float available_width = io.DisplaySize.x - margin * 2.f;
    if (available_width <= padding * 2.f)
        return;

    const float box_width = std::min(560.f, available_width);
    const float body_width = box_width - padding * 2.f;
    constexpr float close_button_size = 22.f;
    constexpr float title_gap = 8.f;
    const char* title = status.notice_title.empty() ? "Release Notice" : status.notice_title.c_str();
    ImGui::PushFont(this->font_alt);
    const ImVec2 title_size = ImGui::CalcTextSize(
        title, nullptr, false, body_width - close_button_size - title_gap);
    ImGui::PopFont();

    const ImVec2 body_size = status.notice.empty()
        ? ImVec2(0.f, 0.f)
        : ImGui::CalcTextSize(status.notice.c_str(), nullptr, false, body_width);
    const float title_row_height = std::max(title_size.y, close_button_size);
    const float body_top = margin + padding + title_row_height + (status.notice.empty() ? 0.f : padding);
    const float body_max_height = std::max(0.f,
        std::min(160.f, io.DisplaySize.y - body_top - margin - padding));
    const float body_height = std::min(body_size.y, body_max_height);
    const float box_height = padding * 2.f + title_row_height +
        (status.notice.empty() ? 0.f : padding + body_height);
    const ImVec2 rect_start((io.DisplaySize.x - box_width) * 0.5f, margin);
    const ImVec2 rect_end(rect_start.x + box_width, rect_start.y + box_height);
    const ImVec2 title_pos(rect_start.x + padding, rect_start.y + padding);
    const ImVec2 close_pos(box_width - padding - close_button_size, padding);
    const ImVec2 body_pos(title_pos.x, rect_start.y + padding + title_row_height + padding);

    ImGui::SetNextWindowPos(rect_start);
    ImGui::SetNextWindowSize(ImVec2(box_width, box_height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground;

    if (!ImGui::Begin("##ReleaseNotice", nullptr, flags)) {
        ImGui::End();
        ImGui::PopStyleVar(2);
        return;
    }

    auto* d = ImGui::GetWindowDrawList();
    d->AddRectFilled(
        rect_start,
        rect_end,
        IM_COL32(0, 0, 0, 200),
        10.f
    );

    d->AddRect(
        rect_start,
        rect_end,
        IM_COL32(100, 100, 100, 200),
        10.f
    );

    ImGui::SetCursorPos(close_pos);
    const bool close_hovered = ImGui::IsWindowHovered() &&
        ImGui::IsMouseHoveringRect(
            ImVec2(rect_start.x + close_pos.x, rect_start.y + close_pos.y),
            ImVec2(rect_start.x + close_pos.x + close_button_size,
                rect_start.y + close_pos.y + close_button_size));
    const bool close_clicked = ImGui::InvisibleButton("##CloseNotice", ImVec2(close_button_size, close_button_size));
    const ImU32 close_color = close_hovered
        ? IM_COL32(255, 255, 255, 255)
        : IM_COL32(190, 190, 190, 255);
    const float cross_padding = 7.f;
    const ImVec2 cross_min(rect_start.x + close_pos.x + cross_padding,
        rect_start.y + close_pos.y + cross_padding);
    const ImVec2 cross_max(rect_start.x + close_pos.x + close_button_size - cross_padding,
        rect_start.y + close_pos.y + close_button_size - cross_padding);
    d->AddLine(cross_min, cross_max, close_color, 2.f);
    d->AddLine(ImVec2(cross_max.x, cross_min.y), ImVec2(cross_min.x, cross_max.y), close_color, 2.f);

    if (close_clicked)
        dismissed = true;

    ImGui::PushFont(this->font_alt);
    d->PushClipRect(title_pos, ImVec2(rect_end.x - padding, body_pos.y), true);
    d->AddText(
        this->font_alt,
        this->font_alt->LegacySize,
        title_pos,
        IM_COL32(255, 200, 0, 255),
        title,
        nullptr,
        body_width
    );
    d->PopClipRect();
    ImGui::PopFont();

    if (!status.notice.empty() && body_height > 0.f) {
        d->PushClipRect(body_pos, ImVec2(rect_end.x - padding, body_pos.y + body_height), true);
        d->AddText(
            this->font,
            this->font->LegacySize,
            body_pos,
            IM_COL32(255, 255, 255, 255),
            status.notice.c_str(),
            nullptr,
            body_width
        );
        d->PopClipRect();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

inline Player* FindPlayerByPawnIndex(std::vector<Player>& players, int index) {
    Player* found = nullptr;

    for (auto& p : players) {
        if (p.pawn_controller_addr == index) {
            found = &p;
            break;
        }
    }
    return found;
}

void Overlays::RenderSpectatorList() {
    if (!cfg::world::spectators::enabled)
        return;

    auto snapshot = Cache::CopySnapshot();
    auto& players = snapshot.players;

    const bool is_menu_open = Renderer::IsOpen();
    const bool detailed = cfg::world::spectators::detailed;
    const bool self_only = cfg::world::spectators::self_only;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    ImGuiTableFlags flags_table = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

    bool should_render = false;
    for (Player& p : players) {
        if (auto i = p.observer_services.target) {
            Player* target = FindPlayerByPawnIndex(players, i);

            if (self_only && (!target || !target->localplayer))
                continue;

            should_render = true;
            break;
        }
    }

    if (!should_render && !is_menu_open)
        return;

    // Window
    ImGui::SetNextWindowPos(cfg::world::spectators::pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(150.f, 50.f), ImVec2(FLT_MAX, FLT_MAX));

    if (!ImGui::Begin("Spectator list", nullptr, flags)) {
        ImGui::End();
        return;
    }

    if (is_menu_open)
        cfg::world::spectators::pos = ImGui::GetWindowPos();

    if (!should_render && is_menu_open) {
        ImGui::TextDisabled("No spectators");
        return ImGui::End();
    }

    if (detailed) {
        if (ImGui::BeginTable("##detailed", 3, flags_table)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Mode");
            ImGui::TableSetupColumn("Target");
            ImGui::TableHeadersRow();

            for (Player& player : players) {
                if (player.alive) continue;

                int targetIndex = player.observer_services.target;
                if (targetIndex == 0) continue;

                Player* target = FindPlayerByPawnIndex(players, targetIndex);

                if (self_only && (!target || !target->localplayer))
                    continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", player.name);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", player.observer_services.ToString());

                ImGui::TableSetColumnIndex(2);
                if (self_only) ImGui::Text("You");
                else if (player.observer_services.mode == ObserverMode::Free) ImGui::Text("No One");
                else ImGui::Text("%s", target ? target->name : "Invalid/bomb");
            }

            ImGui::EndTable();
        }
    }
    else {
        for (Player& player : players) {
            if (player.alive) continue;
            int targetIndex = player.observer_services.target;
            if (targetIndex == 0) continue;
            Player* target = FindPlayerByPawnIndex(players, targetIndex);

            if (self_only && (!target || !target->localplayer)) continue;

            ImGui::Text("%s", player.name);
        }
    }

    ImGui::End();
}

void Overlays::RenderSpeedChart() {
    if (!cfg::world::velocity::enabled)
        return;

    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    auto snapshot = Cache::CopySnapshot();
    auto& local = snapshot.local;

    const static float padding = 10.0f;
    const bool is_menu_open = Renderer::IsOpen();

    auto& pos = cfg::world::velocity::pos;
    auto& size = cfg::world::velocity::size;

    int rate = cfg::world::velocity::sample_rate;
    float length = cfg::world::velocity::sample_length;

    static int prev_rate = rate;
    static float prev_length = length;

    float left = pos.x + padding;
    float right = pos.x + size.x - padding;
    float bottom = pos.y + size.y - padding;
    float top = pos.y + padding;

    float width = right - left;
    float height = bottom - top;

    if (!is_menu_open && !local.alive)
        return;

    if (is_menu_open) {
        auto height_padding = 25; // some padding to keep the speed number inside the area
        auto altitude_padding = 10; // so it doesnt go under the titlebar

        ImGui::SetNextWindowBgAlpha(0.1f);
        ImGui::SetNextWindowPos(pos - Vec2_t(0, altitude_padding), ImGuiCond_Once);
        ImGui::SetNextWindowSize(size + Vec2_t(0, height_padding), ImGuiCond_Once);
        if (ImGui::Begin("Velocity Graph", nullptr, ImGuiWindowFlags_NoCollapse))
        {
            pos = ImGui::GetWindowPos() + ImVec2(0, altitude_padding);
            size = ImGui::GetWindowSize() - ImVec2(0, height_padding);
            ImGui::End();
        }
    }

    // Cache menu values and resize when changed
    if (prev_rate != rate || prev_length != length) {
        prev_rate = rate;
        prev_length = length;

        vel_buffer.resize(static_cast<size_t>(rate * length));
    }

    Vec2_t speed_2d(local.vel.x, local.vel.y);
    int speed = floor(speed_2d.len());

    vel_accumulator += io.DeltaTime;
    size_t buff_size = vel_buffer.size();

    std::vector<ImVec2> points;
    points.reserve(buff_size);

    float sample_interval = 1.0f / rate;

    while (vel_accumulator >= sample_interval)
    {
        vel_accumulator -= sample_interval;
        vel_buffer.at(vel_index % buff_size) = speed;
        vel_index = (vel_index + 1) % buff_size;
    }

    int max_speed = 1;
    for (int v : vel_buffer)
        max_speed = std::max(max_speed, v);

    for (size_t i = 0; i < buff_size; ++i) {
        float t = i / float(buff_size - 1);

        float x = left + t * width;

        float normalized =
            vel_buffer[(i + vel_index) % buff_size] / float(max_speed);

        float y = bottom - (normalized * height);

        points.emplace_back(x, y);
    }

    d->AddPolyline(
        points.data(),
        static_cast<int>(points.size()),
        IM_COL32(255, 255, 255, 255),
        ImDrawFlags_None,
        1.0f
    );

    auto center = ImVec2(
        pos.x + size.x / 2,
        pos.y + size.y
    );

    d->AddText(
        center,
        IM_COL32(255, 255, 255, 255),
        std::to_string(speed).c_str());
}

#ifdef _DEBUG
void Overlays::RenderDebugWindow() {
    auto& io = ImGui::GetIO();
    auto d = ImGui::GetBackgroundDrawList();

    auto snapshot = Cache::CopySnapshot();
    auto& game = snapshot.game;
    auto& bomb = snapshot.bomb;
    auto& globals = snapshot.globals;
    auto& players = snapshot.players;

    static int margin = 10;
    static int padding = 10;
    std::string debug_string = "> Game Debug Window\n";

    debug_string += std::format("Map: {}\n", globals.map_name);
    debug_string += std::format("Max Clients: {}\n", globals.max_clients);
    debug_string += std::format("Cache Refresh: {}ms\n", cfg::dev::cache_refresh_rate);

    if (bomb.is_planted) {
        debug_string += "Bomb:\n";
        debug_string += std::format("- Planted Site: {}\n", bomb.site == BombSite::A ? "A" : "B");
    }

    if (!players.empty())
        debug_string += std::format("Players ({}):\n", players.size());

	for (auto& player : players)
		debug_string += std::format(
			"- [{}] {} {}hp {} {}\n", 
			player.index, player.name, 
			player.health, player.weapon.name,
			player.weapon.icon
		);

    auto size = ImGui::CalcTextSize(debug_string.data());

    d->AddRectFilled(
        ImVec2(10 + margin - padding, io.DisplaySize.y - size.y - 20 - margin - padding),
        ImVec2(10 + size.x + margin + padding, io.DisplaySize.y - 20 - margin + padding),
        IM_COL32(0, 0, 0, 200),
        10.f
    );

    d->AddRect(
        ImVec2(10 + margin - padding, io.DisplaySize.y - size.y - 20 - margin - padding),
        ImVec2(10 + size.x + margin + padding, io.DisplaySize.y - 20 - margin + padding),
        IM_COL32(100, 100, 100, 200),
        10.f
    );

    d->AddText(
        ImVec2(10 + margin, io.DisplaySize.y - size.y - 20 - margin),
        IM_COL32(255, 255, 255, 255),
        debug_string.data()
    );
}
#endif

void Overlays::RenderRadar() {
    if (!cfg::world::radar::enabled)
        return;

    auto snapshot = Cache::CopySnapshot();
    auto& local = snapshot.local;
    auto& players = snapshot.players;
    auto& matrix = snapshot.game.view_matrix;

    const bool is_menu_open = Renderer::IsOpen();

    if (!is_menu_open && !local.alive)
        return;

    auto& pos = cfg::world::radar::pos;
    auto& size = cfg::world::radar::size;
    float range = cfg::world::radar::range;

    if (is_menu_open) {
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(size, ImGuiCond_Once);
        if (ImGui::Begin("Radar", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
            pos = ImGui::GetWindowPos();
            size = ImGui::GetWindowSize();
            ImGui::End();
        }
    }

    auto d = ImGui::GetBackgroundDrawList();

    const float cx = pos.x + size.x * 0.5f;
    const float cy = pos.y + size.y * 0.5f;
    const float rx = size.x * 0.5f;
    const float ry = size.y * 0.5f;
    const float radius = std::min(rx, ry);

    d->AddRectFilled(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(0, 0, 0, 50),
        6.f
    );

    d->AddRect(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(80, 80, 80, 100),
        6.f
    );

    d->AddCircle(ImVec2(cx, cy), radius * 0.333f, IM_COL32(50, 50, 50, 120));
    d->AddCircle(ImVec2(cx, cy), radius * 0.666f, IM_COL32(50, 50, 50, 120));
    d->AddLine(ImVec2(pos.x + 4.f, cy), ImVec2(pos.x + size.x - 4.f, cy), IM_COL32(50, 50, 50, 120));
    d->AddLine(ImVec2(cx, pos.y + 4.f), ImVec2(cx, pos.y + size.y - 4.f), IM_COL32(50, 50, 50, 120));

    for (auto& player : players) {
        if (!player.alive)
            continue;

        if (player.localplayer)
            continue;

        Vec3_t delta = player.pos - local.pos;
        float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

        if (dist > range)
            continue;

        float nx = delta.x / range;
        float ny = delta.y / range;

        float sx, sy;
        if (!cfg::world::radar::no_rotate) {
            float rx = matrix[0][0];
            float ry = matrix[0][1];
            float len = sqrtf(rx * rx + ry * ry);
            if (len > 0.001f) { rx /= len; ry /= len; }
            float fx = -ry;
            float fy =  rx;
            float rad_x = nx * rx + ny * ry;
            float rad_y = nx * fx + ny * fy;
            sx = cx + rad_x * (size.x * 0.5f - 6.f);
            sy = cy - rad_y * (size.y * 0.5f - 6.f);
        } else {
            sx = cx + nx * (size.x * 0.5f - 6.f);
            sy = cy - ny * (size.y * 0.5f - 6.f);
        }

        bool mate = player.team == local.team;
        ImU32 col = mate
            ? IM_COL32(0, 220, 80, 255)
            : IM_COL32(220, 50, 50, 255);

        d->AddCircleFilled(ImVec2(sx, sy), 4.f, col);
        d->AddCircle(ImVec2(sx, sy), 4.f, IM_COL32(0, 0, 0, 180));
    }

    d->AddCircleFilled(ImVec2(cx, cy), 5.f, IM_COL32(100, 180, 255, 255));
    d->AddCircle(ImVec2(cx, cy), 5.f, IM_COL32(0, 0, 0, 180));

    d->AddText(ImVec2(pos.x + 6.f, pos.y + 4.f), IM_COL32(180, 180, 180, 200), "Radar");
}

void Overlays::RenderBomb() {
    if (!cfg::world::bomb::location && !cfg::world::bomb::timer)
        return;

    auto& io = ImGui::GetIO();
    auto snapshot = Cache::CopySnapshot();

    auto& bomb = snapshot.bomb;
    auto& local = snapshot.local;
    auto& matrix = snapshot.game.view_matrix;

    const bool is_menu_open = Renderer::IsOpen();

    float width = 20.f;
    float height = 20.f;
    float rounding = 4.f;

    static int margin = 4;
    static int padding = 6;
    float bar_height = 3.f;
    float element_gap = 6.f;

    auto duration_str = std::format("{}s", bomb.is_planted ? bomb.time_left : 40.0f);
    auto bombsite_str = std::string(!bomb.is_planted || bomb.site == BombSite::A ? "A" : "B");

    std::string bomb_string = "";

    if (cfg::world::bomb::location)
        bomb_string += "SITE " + bombsite_str;

    if (cfg::world::bomb::timer)
    {
        if (cfg::world::bomb::location)
            bomb_string += " | ";

        bomb_string += duration_str;
    }

    auto text_size = ImGui::CalcTextSize(bomb_string.data());

    ImGui::PushFont(this->font_icons);
    auto icon_size = ImGui::CalcTextSize(WeaponIcons::C4);
    ImGui::PopFont();

    float content_width = icon_size.x + element_gap + text_size.x;
    float content_height = std::max(icon_size.y, text_size.y);

    width = content_width + (padding * 2);
    height = content_height + (padding * 2) + (cfg::world::bomb::timer ? bar_height + 2.f : 0.f);

    if (is_menu_open) {
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        float title_bar_height = ImGui::GetFrameHeight();
        ImGui::SetNextWindowPos(cfg::world::bomb::pos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(width, height + title_bar_height), ImGuiCond_Always);

        if (ImGui::Begin("Bomb Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
            cfg::world::bomb::pos = ImGui::GetWindowPos();
            ImGui::End();
        }

        ImGui::PopStyleVar();
    }

    if (!bomb.is_planted && !is_menu_open)
        return;

    if (bomb.is_planted && !bomb.pos.length() && !is_menu_open)
        return;

    if (!local.alive && !is_menu_open)
        return;

    Vec2_t screen_pos;
    bool on_top = bomb.is_planted ? matrix.wts(bomb.pos, io.DisplaySize, screen_pos) : false;

    auto dist = local.pos.dist_to(bomb.pos);

    if (is_menu_open) on_top = false;
    if (dist > 1500.f) on_top = false;

    float render_x = 0.f;
    float render_y = 0.f;

    // If we use bomb esp the overlay will be sticky
    if (on_top && bomb.is_planted && !cfg::esp::bomb) {
        render_x = screen_pos.x - (width * 0.5f);
        render_y = screen_pos.y + margin;
    }
    else {
        float title_bar_height = ImGui::GetFrameHeight();
        render_x = cfg::world::bomb::pos.x;
        render_y = cfg::world::bomb::pos.y + title_bar_height;
    }

    auto d = ImGui::GetBackgroundDrawList();

    d->AddRectFilled(
        ImVec2(render_x, render_y),
        ImVec2(render_x + width, render_y + height),
        IM_COL32(15, 15, 15, 220),
        rounding
    );

    d->AddRect(
        ImVec2(render_x, render_y),
        ImVec2(render_x + width, render_y + height),
        IM_COL32(45, 45, 45, 255),
        rounding
    );

    d->AddText(
        this->font_icons,
        16.0f,
        ImVec2(render_x + padding, render_y + padding + (content_height - icon_size.y) * 0.5f),
        IM_COL32(255, 60, 60, 255),
        WeaponIcons::C4
    );

    d->AddText(
        ImVec2(render_x + padding + icon_size.x + element_gap, render_y + padding + (content_height - text_size.y) * 0.5f),
        IM_COL32(240, 240, 240, 255),
        bomb_string.data()
    );

    if (cfg::world::bomb::timer)
    {
        float time_left = bomb.is_planted ? bomb.time_left : 40.f;
        float progress = std::clamp(time_left / 40.f, 0.f, 1.f);

        ImU32 bar_color = progress > 0.5f
            ? IM_COL32((int)((1.f - progress) * 2.f * 255), 220, 50, 255)
            : IM_COL32(220, (int)(progress * 2.f * 220), 50, 255);

        ImVec2 bar_start(render_x + rounding, render_y + height - bar_height - 4.f);
        ImVec2 bar_end(render_x + width - rounding, render_y + height - bar_height - 4.f);
        float max_bar_width = bar_end.x - bar_start.x;

        d->AddLine(bar_start, bar_end, IM_COL32(40, 40, 40, 255), bar_height);

        if (progress > 0.f)
        {
            ImVec2 bar_filled_end(bar_start.x + (max_bar_width * progress), bar_end.y);
            d->AddLine(bar_start, bar_filled_end, bar_color, bar_height);
        }
    }
}

void Overlays::RenderEspStatus() {
    if (cfg::enabled || !Renderer::IsOpen())
        return;

    auto d = ImGui::GetBackgroundDrawList();
    auto menu_pos = Menu::GetPos();
    auto menu_size = Menu::GetSize();

    constexpr int margin = 10;
    constexpr int padding = 10;
    constexpr auto message = "Settings can be changed now and will take effect when ESP is enabled.";

    auto max_width = menu_size.x - padding * 2;
    auto size = ImGui::CalcTextSize(message, nullptr, false, max_width);
    auto rect_start = ImVec2(menu_pos.x, menu_pos.y + menu_size.y + margin);
    auto rect_end = ImVec2(menu_pos.x + menu_size.x, rect_start.y + padding * 2 + size.y);
    auto pos = ImVec2(rect_start.x + padding, rect_start.y + padding);

    d->AddRectFilled(rect_start, rect_end, IM_COL32(0, 0, 0, 200), 10.f);
    d->AddRect(rect_start, rect_end, IM_COL32(100, 100, 100, 200), 10.f);
    //d->AddText(pos - ImVec2(0, padding + padding * 0.5f), IM_COL32(255, 200, 0, 255), "ESP disabled");
    d->AddText(this->font, this->font->LegacySize, pos, IM_COL32(255, 255, 255, 255), message, nullptr, max_width);
}

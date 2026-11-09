#include "Menu.hpp"

#include "core/engine/cache/Cache.hpp"
#include "gui/renderer/Renderer.hpp" // Circular dependency
#include "gui/renderer/window/Window.hpp" // Circular dependency
#include "assets/fonts/Icons.h"
#include "updater/Updater.hpp"

namespace {
	constexpr const char* bitcoin_wallet_address = "bc1q7c95ee3h6ynwug7uxqvpvcz78dy6s8afkqml5q";
	constexpr const char* ethereum_wallet_address = "0xabac244a9030ed43e2392282a7b8feefb85f2288";
	constexpr const char* solana_wallet_address = "G22j5qLoSxdbEsZv4WyHGfPvMQA1s6nbn3GZAfe4Y483";

	struct SearchEntry {
		const char* label;
		const char* location;
		const char* keywords;
		Tab tab;
	};

	const SearchEntry search_entries[] = {
		{ "Player", "Tab", "visuals esp flags", Tab::PLAYER },
		{ "Visuals", "Player", "esp box skeleton tracker tracers health armor spotted team", Tab::PLAYER },
		{ "Box", "Player / Visuals", "team enemy color", Tab::PLAYER },
		{ "Team box color", "Player / Visuals", "box", Tab::PLAYER },
		{ "Enemy box color", "Player / Visuals", "box", Tab::PLAYER },
		{ "Skeleton", "Player / Visuals", "team enemy color", Tab::PLAYER },
		{ "Team skeleton color", "Player / Visuals", "skeleton", Tab::PLAYER },
		{ "Enemy skeleton color", "Player / Visuals", "skeleton", Tab::PLAYER },
		{ "Head Tracker", "Player / Visuals", "team enemy color", Tab::PLAYER },
		{ "Team head tracker color", "Player / Visuals", "tracker", Tab::PLAYER },
		{ "Enemy head tracker color", "Player / Visuals", "tracker", Tab::PLAYER },
		{ "Tracers", "Player / Visuals", "team enemy color", Tab::PLAYER },
		{ "Team tracer color", "Player / Visuals", "tracers", Tab::PLAYER },
		{ "Enemy tracer color", "Player / Visuals", "tracers", Tab::PLAYER },
		{ "Health", "Player / Visuals", "number", Tab::PLAYER },
		{ "Health Number", "Player / Visuals", "health", Tab::PLAYER },
		{ "Armor", "Player / Visuals", "health", Tab::PLAYER },
		{ "Spotted", "Player / Visuals", "visible", Tab::PLAYER },
		{ "Show Team", "Player / Visuals", "teammates", Tab::PLAYER },
		{ "Flags", "Player", "flashed reloading defusing scoped c4 name money weapon ammo ping", Tab::PLAYER },
		{ "Flashed", "Player / Flags", "team enemy color", Tab::PLAYER },
		{ "Team flashed color", "Player / Flags", "flashed", Tab::PLAYER },
		{ "Enemy flashed color", "Player / Flags", "flashed", Tab::PLAYER },
		{ "Reloading", "Player / Flags", "team enemy color", Tab::PLAYER },
		{ "Team reloading color", "Player / Flags", "reloading", Tab::PLAYER },
		{ "Enemy reloading color", "Player / Flags", "reloading", Tab::PLAYER },
		{ "Defusing", "Player / Flags", "team enemy color", Tab::PLAYER },
		{ "Team defusing color", "Player / Flags", "defusing", Tab::PLAYER },
		{ "Enemy defusing color", "Player / Flags", "defusing", Tab::PLAYER },
		{ "Scoped", "Player / Flags", "team enemy color", Tab::PLAYER },
		{ "Team scoped color", "Player / Flags", "scoped", Tab::PLAYER },
		{ "Enemy scoped color", "Player / Flags", "scoped", Tab::PLAYER },
		{ "Has C4", "Player / Flags", "bomb", Tab::PLAYER },
		{ "Team C4 color", "Player / Flags", "c4", Tab::PLAYER },
		{ "Enemy C4 color", "Player / Flags", "c4", Tab::PLAYER },
		{ "Name", "Player / Flags", "player", Tab::PLAYER },
		{ "Money", "Player / Flags", "cash", Tab::PLAYER },
		{ "Weapon", "Player / Flags", "gun", Tab::PLAYER },
		{ "Ammo", "Player / Flags", "ammunition", Tab::PLAYER },
		{ "Ping", "Player / Flags", "latency", Tab::PLAYER },
		{ "World", "Tab", "bomb spectators crosshair velocity radar", Tab::WORLD },
		{ "Bomb", "World", "esp location timer color", Tab::WORLD },
		{ "Bomb ESP", "World / Bomb", "color", Tab::WORLD },
		{ "Bomb color", "World / Bomb", "esp", Tab::WORLD },
		{ "Bomb Location", "World / Bomb", "position", Tab::WORLD },
		{ "Bomb Timer", "World / Bomb", "countdown", Tab::WORLD },
		{ "Spectator list", "World", "watchers players", Tab::WORLD },
		{ "Spectator list Enable", "World / Spectator list", "enabled", Tab::WORLD },
		{ "Detailed", "World / Spectator list", "spectators", Tab::WORLD },
		{ "Only Self", "World / Spectator list", "spectators watching you", Tab::WORLD },
		{ "Crosshair", "World / Misc", "center", Tab::WORLD },
		{ "Velocity Graph", "World / Misc", "speed movement", Tab::WORLD },
		{ "Radar", "World", "range rotation", Tab::WORLD },
		{ "Radar Range", "World / Radar", "distance", Tab::WORLD },
		{ "Disable Rotation", "World / Radar", "rotate", Tab::WORLD },
		{ "Settings", "Tab", "preferences options", Tab::SETTINGS },
		{ "Streamproof", "Settings / Misc", "capture recording", Tab::SETTINGS },
		{ "Watermark", "Settings / Misc", "overlay", Tab::SETTINGS },
		{ "VSync", "Settings / Misc", "vertical sync", Tab::SETTINGS },
		{ "Free CPU", "Settings / Misc", "performance resources latency", Tab::SETTINGS },
		{ "Log date format", "Settings / Misc", "european american dd mm yyyy", Tab::SETTINGS },
		{ "Notes", "Settings", "performance lag vsync latency", Tab::SETTINGS },
#ifdef _DEBUG
		{ "Sample rate", "World / Velocity Graph", "sample", Tab::WORLD },
		{ "Sample length", "World / Velocity Graph", "sample", Tab::WORLD },
		{ "Console", "Settings / Dev", "developer", Tab::SETTINGS },
		{ "Open Menu Key", "Settings / Dev", "keyboard shortcut", Tab::SETTINGS },
		{ "Cache Refresh Rate", "Settings / Dev", "milliseconds", Tab::SETTINGS },
		{ "Force Show Flags", "Settings / Dev", "developer", Tab::SETTINGS },
#endif
		{ "Donate", "Tab", "support bitcoin ethereum solana wallet", Tab::DONATE },
		{ "Bitcoin", "Donate", "btc wallet address", Tab::DONATE },
		{ "Ethereum", "Donate", "eth wallet address", Tab::DONATE },
		{ "Solana", "Donate", "sol wallet address", Tab::DONATE },
		{ "About", "Tab", "version release updates github", Tab::ABOUT },
		{ "Installed version", "About", "current app", Tab::ABOUT },
		{ "Latest stable release", "About", "update", Tab::ABOUT },
		{ "Release notes", "About", "changelog", Tab::ABOUT },
		{ "Enable", "Sidebar", "master toggle", Tab::PLAYER },
	};

	bool MatchesSettingsSearch(std::string_view label, std::string_view query) {
		if (query.empty())
			return true;

		std::string normalized_label(label);
		std::transform(normalized_label.begin(), normalized_label.end(), normalized_label.begin(),
			[](unsigned char value) { return static_cast<char>(std::tolower(value)); });
		return normalized_label.find(query) != std::string::npos;
	}

	bool MatchesMenuSearch(const SearchEntry& entry, std::string_view query) {
		return MatchesSettingsSearch(entry.label, query) ||
			MatchesSettingsSearch(entry.location, query) ||
			MatchesSettingsSearch(entry.keywords, query);
	}
}


bool Menu::Init() {
	return GetInstance().InitImpl();
}

void Menu::Render() {
	return GetInstance().RenderImpl();
}

void Menu::RenderStartupHelp() {
	return GetInstance().RenderStartupHelpImpl();
}

ImVec2 Menu::GetPos() {
	return GetInstance().pos;
}

ImVec2 Menu::GetSize() {
	return GetInstance().size;
}

bool Menu::InitImpl() {
	SetupStyles();

	LOGF(INFO, "Successfully initialized menu...");
	return true;
}

void Menu::RenderImpl() {
	if (!isSetup)
		return;

	static auto io = ImGui::GetIO();
	static auto screen = io.DisplaySize;
	static auto color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None;

#ifdef _DEBUG
	static auto title = "Excel-Strike [DEV]";
#else
	static auto title = "Excel-Strike";
#endif

	ImGui::SetNextWindowSize(ImVec2(600, 370), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(screen.x / 2 - 300, screen.y / 2 - 150), ImGuiCond_FirstUseEver);

	ImGui::GetWindowPos();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
		this->pos = ImGui::GetWindowPos();
		this->size = ImGui::GetWindowSize();

		static int active_tab = 0;
		static char global_search_buffer[64]{};
		static bool reset_tab_scroll = false;
		static const char* target_section = nullptr;

		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##GlobalSearch", "Search anything...", global_search_buffer, sizeof(global_search_buffer));
		const ImVec2 search_input_min = ImGui::GetItemRectMin();
		const ImVec2 search_input_max = ImGui::GetItemRectMax();
		std::string global_search_query(global_search_buffer);
		std::transform(global_search_query.begin(), global_search_query.end(), global_search_query.begin(),
			[](unsigned char value) { return static_cast<char>(std::tolower(value)); });

		if (!global_search_query.empty()) {
			ImGui::SetNextWindowPos(
				ImVec2(search_input_min.x, search_input_max.y),
				ImGuiCond_Always);
			ImGui::SetNextWindowSize(
				ImVec2(search_input_max.x - search_input_min.x, 180.f),
				ImGuiCond_Always);
			if (ImGui::IsItemActive())
				ImGui::OpenPopup("##GlobalSearchDropdown");
		}

		if (ImGui::BeginPopup("##GlobalSearchDropdown",
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus)) {
			bool has_results = false;
			for (size_t i = 0; i < IM_ARRAYSIZE(search_entries); ++i) {
				const auto& entry = search_entries[i];
				if (!MatchesMenuSearch(entry, global_search_query))
					continue;

				has_results = true;
				const std::string result_label =
					std::string(entry.label) + "  -  " + entry.location + "##" + std::to_string(i);
				if (ImGui::Selectable(result_label.c_str())) {
					active_tab = entry.tab;
					global_search_buffer[0] = '\0';
					reset_tab_scroll = true;
					const std::string_view location(entry.location);
					const size_t section_start = location.find("/ ");
					target_section = section_start == std::string_view::npos
						? nullptr
						: entry.location + section_start + 2;
					ImGui::CloseCurrentPopup();
				}
			}
			if (!has_results)
				ImGui::TextDisabled("No matching options");
			ImGui::EndPopup();
		}

		auto section_heading = [](const char* label) {
			ImGui::Text("%s", label);
			if (target_section && std::string_view(target_section) == label) {
				ImGui::SetScrollHereY(0.f);
				target_section = nullptr;
			}
			ImGui::Separator();
		};

		if (ImGui::BeginChild("##main_split"))
		{
			auto size = ImGui::GetContentRegionAvail();

			ImGui::BeginChild("##tab_buttons", ImVec2(120, size.y), true);
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f, 0.5f));
				for (const auto& tab : tabs)
				{
					bool is_active = (active_tab == tab.id);

					if (is_active)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.28f, 0.16f, 0.84f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.46f, 0.25f, 0.94f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.58f, 0.32f, 0.98f));
					}

					if (ImGui::Button((tab.icon + " " + tab.label).c_str(), ImVec2(-1, 28)))
						active_tab = tab.id;

					if (is_active) 
						ImGui::PopStyleColor(3);
				}
				ImGui::PopStyleVar(1);

				auto space = ImGui::GetContentRegionAvail().y;
				ImGui::SetCursorPosY(ImGui::GetCursorPos().y + space - 16.f * 5);

				const bool donate_is_active = active_tab == Tab::DONATE;
				if (donate_is_active) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.28f, 0.16f, 0.84f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.46f, 0.25f, 0.94f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.58f, 0.32f, 0.98f));
				}
				if (ImGui::Button("Donate", ImVec2(-1, 28)))
					active_tab = Tab::DONATE;
				if (donate_is_active)
					ImGui::PopStyleColor(3);

				ImGui::Dummy(ImVec2(11, 0)); ImGui::SameLine();
				ImGui::TextLinkOpenURL(Icons::DISCORD, "https://discord.gg/mFVRxPGV3");
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(11, 0)); ImGui::SameLine();
				ImGui::TextLinkOpenURL(Icons::GITHUB, "https://github.com/mariocrazyy/excel-strike");
				ImGui::Separator();

				ImGui::Checkbox("Enable", &cfg::enabled);
			}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("##tab_content", ImVec2(0, size.y), true);
			{
				if (reset_tab_scroll) {
					ImGui::SetScrollY(0.f);
					reset_tab_scroll = false;
				}

				if (active_tab == Tab::PLAYER)
				{
					section_heading("Visuals");

					ImGui::BeginGroup();
					{
						ImGui::Checkbox("Box", &cfg::esp::box);
						ImGui::BeginDisabled(!cfg::esp::box);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team box color", cfg::esp::colors::box_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy box color", cfg::esp::colors::box_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Skeleton", &cfg::esp::skeleton);
						ImGui::BeginDisabled(!cfg::esp::skeleton);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team skeleton color", cfg::esp::colors::skeleton_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy skeleton color", cfg::esp::colors::skeleton_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Head Tracker", &cfg::esp::head_tracker);
						ImGui::BeginDisabled(!cfg::esp::head_tracker);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team head tracker color", cfg::esp::colors::tracker_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy head tracker color", cfg::esp::colors::tracker_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Tracers", &cfg::esp::tracers);
						ImGui::BeginDisabled(!cfg::esp::tracers);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team tracer color", cfg::esp::colors::tracer_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy tracer color", cfg::esp::colors::tracer_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();
					}
					ImGui::EndGroup();

					ImGui::SameLine();

					ImGui::BeginGroup();
					{
						ImGui::Checkbox("Health", &cfg::esp::health);
						if (cfg::esp::health)
							ImGui::Checkbox("Health Number", &cfg::esp::health_number);
						ImGui::Checkbox("Armor", &cfg::esp::armor);

						ImGui::Checkbox("Spotted", &cfg::esp::spotted);
						ImGui::SetItemTooltip("Esp will only be visible if the player has been spotted by you");

						ImGui::Checkbox("Show Team", &cfg::esp::team);
					}
					ImGui::EndGroup();

					//ImGui::SameLine();
					ImGui::Spacing();

					section_heading("Flags");

					ImGui::BeginGroup();
					{
						ImGui::Checkbox("Flashed", &cfg::esp::flags::flashed);
						ImGui::BeginDisabled(!cfg::esp::flags::flashed);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team flashed color", cfg::esp::colors::flags::flashed_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy flashed color", cfg::esp::colors::flags::flashed_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Reloading", &cfg::esp::flags::reloading);
						ImGui::BeginDisabled(!cfg::esp::flags::reloading);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team reloading color", cfg::esp::colors::flags::reloading_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy reloading color", cfg::esp::colors::flags::reloading_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Defusing", &cfg::esp::flags::defusing);
						ImGui::BeginDisabled(!cfg::esp::flags::defusing);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team defusing color", cfg::esp::colors::flags::defusing_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy defusing color", cfg::esp::colors::flags::defusing_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Scoped", &cfg::esp::flags::scoped);
						ImGui::BeginDisabled(!cfg::esp::flags::scoped);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team scoped color", cfg::esp::colors::flags::scoped_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy scoped color", cfg::esp::colors::flags::scoped_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();

						ImGui::Checkbox("Has C4", &cfg::esp::flags::has_c4);
						ImGui::BeginDisabled(!cfg::esp::flags::has_c4);
						{
							ImGui::SameLine();
							ImGui::ColorEdit4("Team C4 color", cfg::esp::colors::flags::c4_team.data(), color_flags);
							ImGui::SameLine();
							ImGui::ColorEdit4("Enemy C4 color", cfg::esp::colors::flags::c4_enemy.data(), color_flags);
						}
						ImGui::EndDisabled();
					}
					ImGui::EndGroup();

					ImGui::SameLine();

					ImGui::BeginGroup();
					{
						ImGui::Checkbox("Name", &cfg::esp::flags::name);
						ImGui::Checkbox("Money", &cfg::esp::flags::money);
						ImGui::Checkbox("Weapon", &cfg::esp::flags::weapon);
						ImGui::Checkbox("Ammo", &cfg::esp::flags::ammo);
						ImGui::Checkbox("Ping", &cfg::esp::flags::ping);
					}
					ImGui::EndGroup();
				}
				else if (active_tab == Tab::WORLD)
				{
					section_heading("Bomb");
					{
						ImGui::Checkbox("Bomb ESP", &cfg::esp::bomb);
						ImGui::SameLine();
						ImGui::ColorEdit4("Bomb color", cfg::esp::colors::bomb.data(), color_flags);
					}
					ImGui::Checkbox("Bomb Location", &cfg::world::bomb::location);
					ImGui::Checkbox("Bomb Timer", &cfg::world::bomb::timer);

					ImGui::Spacing();

					section_heading("Spectator list");

					ImGui::Checkbox("Enable", &cfg::world::spectators::enabled);
					if (cfg::world::spectators::enabled) {
						ImGui::Checkbox("Detailed", &cfg::world::spectators::detailed);
						ImGui::Checkbox("Only Self", &cfg::world::spectators::self_only);
						ImGui::SetItemTooltip("Only display users spectating you");
					}

					ImGui::Spacing();

					section_heading("Misc");

					ImGui::Checkbox("Crosshair", &cfg::world::crosshair::enabled);
					ImGui::Checkbox("Velocity Graph", &cfg::world::velocity::enabled);
				#ifdef _DEBUG // Part of the velocity graph for developers
					if (cfg::world::velocity::enabled) {
						ImGui::SliderInt("Sample rate", &cfg::world::velocity::sample_rate, 1, 100);
						ImGui::SliderFloat("Sample length", &cfg::world::velocity::sample_length, 1, 20, "%.1f");
					}
				#endif
					ImGui::Spacing();

					section_heading("Radar");

					ImGui::Checkbox("Radar", &cfg::world::radar::enabled);
					ImGui::BeginDisabled(!cfg::world::radar::enabled);
					{
						ImGui::SameLine();
						ImGui::SliderFloat("Range", &cfg::world::radar::range, 100.f, 8000.f, "%.0f u");
						ImGui::Checkbox("Disable Rotation", &cfg::world::radar::no_rotate);
					}
					ImGui::EndDisabled();
				}
				else if (active_tab == Tab::SETTINGS)
				{
					section_heading("Misc");

					if (ImGui::Checkbox("Streamproof", &cfg::settings::streamproof))
					{
						Window::SetAffinity(
							Window::hwnd,
							cfg::settings::streamproof ? WindowAffinity::Invisible : WindowAffinity::Disabled
						);
					}

					ImGui::Checkbox("Watermark", &cfg::settings::watermark);

					if (ImGui::Checkbox("VSync", &cfg::settings::vsync))
						Window::vsync = cfg::settings::vsync;

					ImGui::Checkbox("Free CPU", &cfg::settings::free_cpu);
					ImGui::SetItemTooltip("Let the CPU sleep to Free Resources\nNOTE: might cause performance issues in lower end computers!");

					{
						const char* date_format_options[] = {
							"European (DD-MM-YYYY)",
							"American (MM-DD-YYYY)"
						};
						int date_format = cfg::settings::log_date_format == cfg::LogDateFormat::AMERICAN ? 1 : 0;
						if (ImGui::Combo("Log date format", &date_format, date_format_options, IM_ARRAYSIZE(date_format_options)))
							cfg::settings::log_date_format = date_format == 1
								? cfg::LogDateFormat::AMERICAN
								: cfg::LogDateFormat::EUROPEAN;
					}

					section_heading("Notes");
					ImGui::TextWrapped(
						"If you experience bad performance/lag try the following:\n"
						"\t- Disable ESP VSync: Look up > VSync: Un-Check\n"
						"\t- Disable VSync in game: ...Advanced Video > V-Sync: Disabled\n"
						"\t- Last Resort: Disable \"Free CPU\" option, it will inpact on your overall performace, but improve latency\n"
					);

#ifdef _DEBUG
					section_heading("Dev");

					if (ImGui::Checkbox("Console", &cfg::dev::console))
						if (!cfg::dev::console) LogHelper::HideConsole();

					static int key_out;
					if (ImGui::Button("Open Menu Key"))
					{
						for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++)
						{
							if (ImGui::IsKeyPressed((ImGuiKey)i))
							{
								key_out = i;
								LOGF(VERBOSE, "Changed the open menu key to {}", key_out);
								break;
							}
						}
					}

					ImGui::SliderInt("Cache Refresh Rate", &cfg::dev::cache_refresh_rate, 0, 100, "%dms");
					ImGui::Checkbox("Force Show Flags", &cfg::dev::force_show_flags);
#endif
				}
				else if (active_tab == Tab::DONATE)
				{
					ImGui::Text("Support Excel-Strike");
					ImGui::TextWrapped("Send a donation using one of the wallet addresses below.");
					const auto wallet_row = [](const char* ticker, const char* name, const char* address, ImVec4 color) {
						ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.03f, 0.09f, 0.06f, 0.68f));
						ImGui::BeginChild(ticker, ImVec2(0, 76), true);
						ImGui::PushStyleColor(ImGuiCol_Button, color);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
						ImGui::Button(ticker, ImVec2(48, 30));
						ImGui::PopStyleColor(3);
						ImGui::SameLine();
						ImGui::BeginGroup();
						ImGui::Text("%s", name);
						ImGui::TextWrapped("%s", address);
						ImGui::EndGroup();
						if (ImGui::Button(("Copy##" + std::string(ticker)).c_str()))
							ImGui::SetClipboardText(address);
						ImGui::EndChild();
						ImGui::PopStyleColor();
					};

					wallet_row("BTC", "Bitcoin", bitcoin_wallet_address, ImVec4(0.72f, 0.42f, 0.12f, 0.95f));
					wallet_row("ETH", "Ethereum", ethereum_wallet_address, ImVec4(0.28f, 0.38f, 0.62f, 0.95f));
					wallet_row("SOL", "Solana", solana_wallet_address, ImVec4(0.34f, 0.28f, 0.58f, 0.95f));
				}
				else if (active_tab == Tab::ABOUT)
				{
					const Status status = Updater::GetStatus();
					ImGui::Text("About Excel-Strike");
					ImGui::Separator();
					ImGui::Text("Installed version: %s",
						status.current_version.empty() ? "Unknown" : status.current_version.c_str());
					ImGui::Text("Latest stable release: %s",
						status.update_check_succeeded && !status.latest_version.empty()
							? status.latest_version.c_str()
							: "Unavailable");
					if (!status.update_check_succeeded)
						ImGui::TextDisabled("The latest release could not be checked this session.");

					if (status.update_check_succeeded && !status.latest_version.empty() &&
						status.latest_version != status.current_version) {
						ImGui::TextColored(ImVec4(1.f, 0.8f, 0.25f, 1.f),
							"An update or downgrade is available.");
					}

					ImGui::TextLinkOpenURL("Open GitHub releases",
						"https://github.com/mariocrazyy/excel-strike/releases");

					ImGui::Spacing();
					ImGui::Text("Latest release notes");
					ImGui::Separator();
					if (status.notice.empty())
						ImGui::TextDisabled("No release notes are available.");
					else {
						if (!status.notice_title.empty())
							ImGui::TextColored(ImVec4(1.f, 0.8f, 0.25f, 1.f), "%s", status.notice_title.c_str());
						ImGui::TextWrapped("%s", status.notice.c_str());
					}
				}
			}
			ImGui::EndChild();
			ImGui::EndChild();
		}

	}

	ImGui::End();
}

void Menu::SetupStyles() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.12f, 0.08f, 0.82f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.03f, 0.09f, 0.06f, 0.68f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.16f, 0.10f, 0.94f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.13f, 0.45f, 0.27f, 0.72f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.24f, 0.16f, 0.78f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.38f, 0.23f, 0.90f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.52f, 0.30f, 0.92f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.16f, 0.09f, 0.88f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.24f, 0.13f, 0.96f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.10f, 0.05f, 0.68f);

	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.17f, 0.10f, 0.82f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.10f, 0.06f, 0.54f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.12f, 0.39f, 0.22f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.16f, 0.55f, 0.30f, 0.92f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.20f, 0.68f, 0.37f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.78f, 0.45f, 1.00f);

	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.66f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.14f, 0.50f, 0.27f, 1.00f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.09f, 0.28f, 0.16f, 0.84f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.46f, 0.25f, 0.94f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.58f, 0.32f, 0.98f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.08f, 0.25f, 0.14f, 0.78f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.40f, 0.21f, 0.90f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.52f, 0.28f, 0.96f);
	style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.56f, 0.32f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.24f, 0.72f, 0.39f, 0.95f);

	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.29f, 0.30f, 0.31f, 0.95f);

	style.Colors[ImGuiCol_Tab] = ImVec4(0.04f, 0.16f, 0.09f, 0.76f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.12f, 0.39f, 0.22f, 0.88f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.10f, 0.32f, 0.18f, 0.94f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.04f, 0.13f, 0.07f, 0.78f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.07f, 0.22f, 0.12f, 0.88f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.18f, 0.58f, 0.32f, 0.45f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.30f, 0.78f, 0.45f, 1.00f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.24f, 0.68f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	style.FrameBorderSize = 1.0f;

	// Window & Frame
	style.WindowRounding = 12.f;
	style.ChildRounding = 10.f;

	style.FrameRounding = 5.f;
	style.PopupRounding = 5.f;

	style.GrabRounding = 3.f;

	auto& io = ImGui::GetIO();

	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f);

	ImFontConfig merge_icon_cfg{};
	merge_icon_cfg.FontDataOwnedByAtlas = false;
	merge_icon_cfg.MergeMode = true;
	merge_icon_cfg.GlyphOffset = Vec2_t(0, 3.5f);

	// the icons will use the size specified when getting added so it ignores the base size
	static const ImWchar icon_ranges[] = { 0xE100, 0xE108, 0 };
	io.Fonts->AddFontFromMemoryTTF(icons_font, icons_font_len, 20.f, &merge_icon_cfg, icon_ranges);
}

void Menu::RenderStartupHelpImpl() {
	static bool has_opened_menu = false;

	if (has_opened_menu)
		return;

	auto& io = ImGui::GetIO();
	auto screen = io.DisplaySize;
	auto d = ImGui::GetBackgroundDrawList();

	if (Renderer::IsOpen())
		has_opened_menu = true;

	auto help = "To OPEN the menu, press Right Shift"
		"\n\t\t\t\tTo CLOSE, press the End key";
	auto size = ImGui::CalcTextSize(help);

	d->AddText(
		ImVec2(screen.x / 2 - size.x / 2, 80),
		IM_COL32(255, 255, 255, 255),
		help
	);
}

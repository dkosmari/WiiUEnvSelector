/*
 *  Wii U Env Selector
 *  Copyright (C) 2026  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <stdexcept>
#include <vector>

#include <strings.h> // strcasecmp()

#include <coreinit/launch.h>
#include <coreinit/memory.h>
#include <coreinit/title.h>
#include <gx2/registers.h>
#include <gx2/swap.h>
#include <padscore/kpad.h>
#include <sndcore2/core.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/gfx.h>
#include <whb/proc.h>

#include <imgui.h>
#include <imgui_raii.h>
#include <imgui_stdlib.h>
#ifdef IMGUI_ENABLE_FREETYPE
#include <imgui_freetype.h>
#endif

#include <imgui_impl_wiiu.h>
#include <imgui_impl_gx2.h>

#include <mocha/mocha.h>

#include "ButtonHBox.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using namespace std::literals;
using std::cout;
using std::cerr;
using std::endl;


struct mocha {

    struct error : std::runtime_error {
        error(MochaUtilsStatus code) :
            std::runtime_error{Mocha_GetStatusStr(code)}
        {}
    };


    mocha()
    {
        if (auto e = Mocha_InitLibrary())
            throw error{e};
    }


    ~mocha()
    {
        Mocha_DeInitLibrary();
    }


    std::filesystem::path
    get_environment()
        const
    {
        std::vector<char> buffer(1024);
        if (auto e = Mocha_GetEnvironmentPath(buffer.data(), buffer.size()))
            throw error{e};
        return std::filesystem::path{buffer.data()};
    }

}; // struct rpxloader_init


const std::filesystem::path env_root = "fs:/vol/external01/wiiu/environments";
const std::filesystem::path default_cfg_path = env_root / "default.cfg";

constexpr std::uint64_t HBL_TITLE_ID           = 0x00050000'13374842;
constexpr std::uint64_t MII_MAKER_EUR_TITLE_ID = 0x00050010'1004A200;
constexpr std::uint64_t MII_MAKER_JPN_TITLE_ID = 0x00050010'1004A000;
constexpr std::uint64_t MII_MAKER_USA_TITLE_ID = 0x00050010'1004A100;

std::vector<std::filesystem::path> env_list{""};
std::size_t sel_idx;

std::filesystem::path cur_env;
std::filesystem::path default_env;

bool from_hbl = false;


void
load_font(OSSharedDataType font, bool merge)
{
    static const char* names[OS_SHAREDDATATYPE_FONT_MAX] = {
        [OS_SHAREDDATATYPE_FONT_CHINESE]   = "CafeCn.ttf",
        [OS_SHAREDDATATYPE_FONT_KOREAN]    = "CafeKr.ttf",
        [OS_SHAREDDATATYPE_FONT_STANDARD]  = "CafeStd.ttf",
        [OS_SHAREDDATATYPE_FONT_TAIWANESE] = "CafeCn.ttf",
    };

    assert(font < OS_SHAREDDATATYPE_FONT_MAX);

    auto& io = ImGui::GetIO();
    ImFontConfig config;
    config.MergeMode = merge;
    config.EllipsisChar = U'…';
    config.FontDataOwnedByAtlas = false;
#ifdef IMGUI_ENABLE_FREETYPE
    config.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
    config.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_Bitmap;
    config.ExtraSizeScale = 0.75f;
    // NOTE: use ImGui::SeparatorText() to find a good Y alignment value.
    config.GlyphOffset.y = -10.0f;
#endif

    void* font_data = nullptr;
    uint32_t font_size = 0;

    auto& style = ImGui::GetStyle();

    if (OSGetSharedData(font,
                        0,
                        &font_data,
                        &font_size)) {
        std::snprintf(config.Name, sizeof config.Name, names[font]);
        io.Fonts->AddFontFromMemoryTTF(font_data,
                                       font_size,
                                       style.FontSizeBase,
                                       &config);
    }
}


void
load_fonts()
{
#ifdef IMGUI_ENABLE_FREETYPE
    auto& io = ImGui::GetIO();
    io.Fonts->FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
    io.Fonts->FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_Bitmap;
#endif
    load_font(OS_SHAREDDATATYPE_FONT_STANDARD, false);
}


bool
detect_hbl()
{
    std::uint64_t titleID = OSGetTitleID();
    switch (titleID) {
        case HBL_TITLE_ID:
        case MII_MAKER_EUR_TITLE_ID:
        case MII_MAKER_JPN_TITLE_ID:
        case MII_MAKER_USA_TITLE_ID:
            return true;
        default:
            return false;
    }
}


bool
safe_equivalent(const std::filesystem::path& a,
                const std::filesystem::path& b)
{
    if (a == b)
        return true;
    if (a.empty() || b.empty())
        return false;
    return equivalent(a, b);
}


std::string
get_env_label(const std::filesystem::path& env_path)
{
    if (env_path.empty())
        return "<none>";
    return env_path.filename().string();
}


void
scan_environments()
{
    try {

        try {
            mocha m;
            cur_env = m.get_environment();
        }
        catch (std::exception& e) {
            cout << "Could not detect current environment: " << e.what() << endl;
        }

        for (auto dir : std::filesystem::directory_iterator{env_root}) {
            if (!dir.is_directory())
                continue;
            env_list.push_back(dir.path());
        }

        std::ranges::sort(
            env_list,
            [](const std::filesystem::path& a,
               const std::filesystem::path& b) -> bool
            {
                int r = strcasecmp(a.c_str(), b.c_str());
                return r < 0;
            }
        );

        if (exists(default_cfg_path)) {
            std::ifstream cfg_file{default_cfg_path};
            std::string line;
            if (getline(cfg_file, line) && !line.empty())
                default_env = env_root / line;
        }

        // Make sel_idx point to the entry that corresponds to the default_env
        for (auto [idx, env] : env_list | std::views::enumerate) {
            if (default_env == env || safe_equivalent(default_env, env)) {
                sel_idx = idx;
                break;
            }
        }
    }
    catch (std::exception& e) {
        cerr << "scan_environments(): " << e.what() << endl;
    }
}


void
set_default_cfg()
{
    const auto& sel_env = env_list.at(sel_idx);
    if (default_env == sel_env || safe_equivalent(sel_env, default_env)) {
        cout << "No change necessary." << endl;
        return;
    }

    if (sel_env.empty()) {
        if (exists(default_cfg_path))
            remove(default_cfg_path);
    } else {
        std::string name = sel_env.filename().string();
        std::ofstream cfg_file{default_cfg_path};
        if (cfg_file)
            cfg_file << name;
    }

    default_env = sel_env;
}


void
show_ui()
try {
    using namespace ImGui::RAII;

    auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
    if (Window main_window{PACKAGE_STRING,
                           nullptr,
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoResize}) {

        ImGui::FormatText("Current: {}",
                          get_env_label(cur_env));

        ImGui::FormatText("Default: {}",
                          get_env_label(default_env));

        // ImGui::SeparatorText("{Align test}");

        ButtonHBox buttons;
        buttons.spread = true;

        buttons.add(
            "Quit",
            false,
            []
            {
                if (!from_hbl)
                    SYSLaunchMenu();
                else
                    WHBProcStopRunning();
            }
        );

        buttons.add(
            "Reboot",
            false,
            []
            {
                OSLaunchTitlel(OS_TITLE_ID_REBOOT, 0);
            }
        );

        buttons.add(
            "Apply",
            false,
            []
            {
                set_default_cfg();
            }
        );

        buttons.add(
            "Apply & Reboot",
            true,
            []
            {
                set_default_cfg();
                OSLaunchTitlel(OS_TITLE_ID_REBOOT, 0);
            }
        );

        if (Child content{"content",
                          {0, -buttons.get_height_with_spacing()},
                          ImGuiChildFlags_Borders |
                          ImGuiChildFlags_NavFlattened}) {
            for (auto [idx, env] : env_list | std::views::enumerate)
                ImGui::RadioButton(get_env_label(env),
                                   sel_idx,
                                   static_cast<std::size_t>(idx));
        }

        buttons.show();
    }
}
catch (std::exception& e) {
    cerr << "Exception in show_ui(): " << e.what() << endl;
}


void
initialize_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.LogFilename = nullptr;
    io.IniFilename = nullptr;

    io.ConfigDragScroll = true;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.MouseDragThreshold = 30;
    // io.ConfigInputTrickleEventQueue = false;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(3.0f);
    style.FontSizeBase = 36;
    style.ItemSpacing = {18, 12};

    style.FrameRounding = 12;

    style.WindowBorderSize = 0;

    load_fonts();

    ImGui_ImplWiiU_Init();
    ImGui_ImplGX2_Init();
}


void
initialize()
{
    from_hbl = detect_hbl();

    cout << "Initializing " << PACKAGE_STRING << endl;
    cout << "HBL detected: " << (from_hbl ? "yes" : "no") << endl;

    // Briefly initialize the audio system to stop the boot sound.
    AXInitParams ax_params{};
    ax_params.renderer = AX_INIT_RENDERER_32KHZ;
    ax_params.pipeline = AX_INIT_PIPELINE_SINGLE;
    AXInitWithParams(&ax_params);
    AXQuit();

    KPADInit();
    WPADEnableURCC(true);

    WHBProcInit();

    WHBGfxInit();

    initialize_imgui();

    scan_environments();
}


void
finalize_imgui()
{
    ImGui_ImplGX2_Shutdown();
    ImGui_ImplWiiU_Shutdown();
    ImGui::DestroyContext();
}


void
finalize()
{
    finalize_imgui();

    WHBGfxShutdown();

    WHBProcShutdown();

    KPADShutdown();

    cout << "Finalizing " << PACKAGE_STRING << endl;
}


int
main()
{
    try {
        initialize();

        VPADStatus vpad;
        KPADStatus kpad[4];
        ImGui_ImplWiiU_ControllerInput wiiu_input;

        ImVec4 bg_color = {0, 0, 0, 1};

        auto& io = ImGui::GetIO();

        while (WHBProcIsRunning()) {

            // Input processing.

            wiiu_input = {};

            {
                auto ch = VPAD_CHAN_0;
                VPADReadError error;
                auto r = VPADRead(ch, &vpad, 1, &error);
                if (r == 1 && error == VPAD_READ_SUCCESS) {
                    wiiu_input.vpad = &vpad;
                }
            }

            for (int ch = 0; ch < 4; ++ch) {
                auto r = KPADRead(static_cast<KPADChan>(ch), &kpad[ch], 1);
                if (r == 1)
                    wiiu_input.kpad[ch] = &kpad[ch];
            }

            ImGui_ImplWiiU_ProcessInput(&wiiu_input);

            // GUI processing.

            GX2ColorBuffer* cb = WHBGfxGetTVColourBuffer();

            ImGui_ImplWiiU_NewFrame(cb);
            ImGui_ImplGX2_NewFrame();
            ImGui::NewFrame();

            show_ui();

            ImGui::EndFrame();

            ImGui::Render();

            WHBGfxBeginRender();

            WHBGfxBeginRenderTV();

            GX2SetViewport(0, 0,
                           io.DisplaySize.x, io.DisplaySize.y,
                           0.0f, 1.0f);
            WHBGfxClearColor(bg_color.x, bg_color.y, bg_color.z, bg_color.w);
            ImGui_ImplGX2_RenderDrawData(ImGui::GetDrawData());

            ImGui_ImplWiiU_DrawKeyboardOverlay(ImGui_KeyboardOverlay_Auto);

            WHBGfxFinishRenderTV();

            GX2CopyColorBufferToScanBuffer(WHBGfxGetTVColourBuffer(),
                                           GX2_SCAN_TARGET_DRC);

            WHBGfxFinishRender();
        }

        finalize();
    }
    catch (std::exception& e) {
        cerr << "ERROR in main(): " << e.what() << endl;
        return 1;
    }
}

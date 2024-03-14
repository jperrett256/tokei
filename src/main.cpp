#define UNICODE

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <assert.h>

#define array_count(x) (sizeof(x) / sizeof(*(x)))

struct DX11State
{
    ID3D11Device * device;
    ID3D11DeviceContext * device_context;
    IDXGISwapChain * swapchain;
    UINT screen_width, screen_height;
    ID3D11RenderTargetView * render_target_view;
};

static DX11State dx11_state;

void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param);

#ifdef JDP_DEBUG_CONSOLE
int main(void)
{
    int show_code = SW_SHOWDEFAULT;
    HINSTANCE instance = GetModuleHandle(NULL);
    return WinMain(instance, NULL, NULL, show_code);
}
#endif

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    (void) prev_instance; (void) command_line;

    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSW window_class = {};
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = main_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.lpszClassName = L"Tokei";
    RegisterClass(&window_class);
    HWND window = CreateWindowExW(
        0, window_class.lpszClassName, L"時計", WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 800,
        NULL, NULL, window_class.hInstance, NULL
    );

    {
        DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
        swapchain_desc.BufferCount = 2;
        swapchain_desc.BufferDesc.Width = 0;
        swapchain_desc.BufferDesc.Height = 0;
        swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.BufferDesc.RefreshRate.Numerator = 60; // TODO what's this?
        swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
        swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.OutputWindow = window;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // TODO given we're targeting Windows 11 we could use the newer flip model swapchain type

        UINT create_device_flags = 0;
#ifdef JDP_DEBUG_DX11
        create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL feature_level;
        const D3D_FEATURE_LEVEL feature_levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

        HRESULT result;

        result = D3D11CreateDeviceAndSwapChain(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, create_device_flags,
            feature_levels, array_count(feature_levels), D3D11_SDK_VERSION, &swapchain_desc,
            &dx11_state.swapchain, &dx11_state.device, &feature_level, &dx11_state.device_context
        );

        if (result == DXGI_ERROR_UNSUPPORTED)
        {
            // try high-performance WARP software driver if hardware is not available
            result = D3D11CreateDeviceAndSwapChain(
                NULL, D3D_DRIVER_TYPE_WARP, NULL, create_device_flags,
                feature_levels, array_count(feature_levels), D3D11_SDK_VERSION, &swapchain_desc,
                &dx11_state.swapchain, &dx11_state.device, &feature_level, &dx11_state.device_context
            );
        }

        if (FAILED(result))
        {
            // TODO create message box displaying error
            OutputDebugStringA("Failed to create D3D11 device.");
            ExitProcess(1);
        }

        CreateRenderTarget();
    }

    ShowWindow(window, show_code);
    UpdateWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: Experimental. THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    //io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI: Experimental.

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(dx11_state.device, dx11_state.device_context);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (dx11_state.screen_width != 0 && dx11_state.screen_height != 0)
        {
            CleanupRenderTarget();

            dx11_state.swapchain->ResizeBuffers(0, dx11_state.screen_width, dx11_state.screen_height, DXGI_FORMAT_UNKNOWN, 0);
            dx11_state.screen_width = dx11_state.screen_height = 0;

            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", (double) (1000.0f / io.Framerate), (double) io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        dx11_state.device_context->OMSetRenderTargets(1, &dx11_state.render_target_view, NULL);
        dx11_state.device_context->ClearRenderTargetView(dx11_state.render_target_view, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        dx11_state.swapchain->Present(1, 0); // vsync enabled
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    {
        CleanupRenderTarget();
        assert(dx11_state.swapchain);
        dx11_state.swapchain->Release();
        assert(dx11_state.device_context);
        dx11_state.device_context->Release();
        assert(dx11_state.device);
        dx11_state.device->Release();
    }

    return 0;
}

void CreateRenderTarget()
{
    HRESULT result;

    ID3D11Texture2D * backbuffer;
    result = dx11_state.swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    assert(SUCCEEDED(result));
    result = dx11_state.device->CreateRenderTargetView(backbuffer, NULL, &dx11_state.render_target_view);
    assert(SUCCEEDED(result));
    backbuffer->Release();
}

void CleanupRenderTarget()
{
    if (dx11_state.render_target_view)
    {
        dx11_state.render_target_view->Release();
        dx11_state.render_target_view = NULL;
    }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI main_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param))
        return true;

    switch (message)
    {
    case WM_SIZE:
        if (w_param == SIZE_MINIMIZED)
            return 0;
        dx11_state.screen_width = (UINT) LOWORD(l_param); // Queue resize
        dx11_state.screen_height = (UINT) HIWORD(l_param);
        return 0;
    case WM_SYSCOMMAND:
        if ((w_param & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(w_param);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT * suggested_rect = (RECT *) l_param;
            SetWindowPos(window, NULL,
                suggested_rect->left, suggested_rect->top,
                suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return DefWindowProcW(window, message, w_param, l_param);
}

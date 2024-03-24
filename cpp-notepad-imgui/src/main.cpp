#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3d9.h>

// Data
static LPDIRECT3D9				g_pD3D = nullptr;
static LPDIRECT3DDEVICE9		g_pd3dDevice = nullptr;
static UINT						g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS	g_d3dpp = {};

// Helper functions
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool ProcessMessages();
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

// WinMain loop
int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	const wchar_t *mainClassName = L"ImGui Notepad App";

	//Create app window
	WNDCLASSEXW wndClass = {};
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_CLASSDC;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = nullptr;
	wndClass.hCursor = nullptr;
	wndClass.hbrBackground = nullptr;
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = mainClassName;
	wndClass.hIconSm = nullptr;
	::RegisterClassExW(&wndClass);

	HWND hWnd = ::CreateWindowW(
		wndClass.lpszClassName,
		L"ImGui Notepad",
		WS_OVERLAPPEDWINDOW,
		100, 100,
		1280, 720,
		nullptr, nullptr,
		wndClass.hInstance,
		nullptr
		);

	// Init Direct3D
	// if the func below returns false then program dies
	if (!CreateDeviceD3D(hWnd)) {
		std::cerr << "Error: Failed to create Direct3D Device";
		CleanupDeviceD3D();
		::UnregisterClassW(wndClass.lpszClassName, wndClass.hInstance);
		return 1;
	}

	// Show window
	::ShowWindow(hWnd, SW_SHOW);
	::UpdateWindow(hWnd);

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	// casting to void to avoid unnecessary warning about unused variables
	(void)io;
	io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;

	// Setup ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends <- idk what this mean
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	// Load Fonts
	io.Fonts->AddFontDefault();
	ImFont *fontArial = io.Fonts->AddFontFromFileTTF("src/fonts/arial.ttf", 16.0f);
	io.Fonts->Build();

	unsigned char* tex_pixels = nullptr;
	int tex_width, tex_height;
	io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_width, &tex_height);

	// Define variables
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove ;// | ImGuiWindowFlags_NoBackground;


	// Main loop
	bool running = true;
	while (running)
	{
		if (!ProcessMessages())
			running = false;
		if (!running)
			break;

		// Handle window resize
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			g_d3dpp.BackBufferWidth = g_ResizeWidth;
			g_d3dpp.BackBufferHeight = g_ResizeHeight;
			io.DisplaySize.x = g_ResizeWidth;
			io.DisplaySize.y = g_ResizeHeight;
			g_ResizeWidth = g_ResizeHeight = 0;
			ResetDevice();
		}

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);

		// Start ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::PushFont(fontArial);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

		// Show menubar
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// Show fullscreen window (see WindowFlags in Data section)
		ImGui::SetWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
		ImGui::SetWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
		ImGui::Begin("Fullscreen", nullptr, flags);
		ImGui::Text("Hello World!");
		ImGui::End();

		ImGui::PopFont();
		ImGui::PopStyleVar();

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}

	// Shutdown
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//CleanupDeviceD3D();
	::DestroyWindow(hWnd);
	::UnregisterClassW(wndClass.lpszClassName, wndClass.hInstance);

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool ProcessMessages()
{
	MSG msg = {};
	while (::PeekMessage(&msg, nullptr, 0u, 0u, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	return true;
}


bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr) { return false; }

	// Create D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // w/VSync

	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,D3DCREATE_HARDWARE_VERTEXPROCESSING, 
		&g_d3dpp, &g_pd3dDevice))
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) {g_pd3dDevice->Release(); g_pd3dDevice = nullptr;}
	if (g_pD3D){g_pD3D->Release(); g_pD3D = nullptr;}
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}


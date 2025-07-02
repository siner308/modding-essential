// Dummy ImGui header file for demonstration

namespace ImGui {
    void Init(void* hwnd, void* device, void* context);
    void NewFrame();
    void Render();
    void EndFrame();
    void Shutdown();

    void Begin(const char* name);
    void Text(const char* text);
    void End();
}

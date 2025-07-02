// Dummy ImGui DX11 implementation header

namespace ImGui_ImplDX11 {
    void Init(void* device, void* context);
    void NewFrame();
    void RenderDrawData(void* draw_data);
    void Shutdown();
}

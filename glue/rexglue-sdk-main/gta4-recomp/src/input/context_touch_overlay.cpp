#include "input/context_touch_overlay.h"
#include <rex/platform.h>
#if REX_PLATFORM_ANDROID
#include "../android/android_frame_metrics.h"
#include <rex/cvar.h>
#include <cstdio>
#include <cfloat>
REXCVAR_DECLARE(bool, gta4_android_frame_metrics);
#endif

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>
#include <rex/input/absolute_pointer.h>
#include <rex/ui/image_decode.h>

#include "input/context_touch_settings.h"
#include "input/gamepad_touch_overlay.h"

namespace gta4::input {

ContextTouchOverlay::ContextTouchOverlay(rex::ui::ImGuiDrawer* drawer,
                                         rex::ui::ImmediateDrawer* immediate,
                                         std::filesystem::path icon_directory,
                                         rex::ui::Window* window)
    : ImGuiDialog(drawer), immediate_(immediate), icon_directory_(std::move(icon_directory)) {
  if (window) editor_mouse_ = std::make_unique<ContextTouchEditorMouse>(window);
}

void ContextTouchOverlay::SetIconResolver(ContextTouchIconResolver resolver, void* context) noexcept {
  icon_resolver_ = resolver;
  icon_context_ = context;
}

bool ContextTouchOverlay::ResolveIcon(void* context, std::string_view id, ContextTouchIcon* icon) {
  auto& self = *static_cast<ContextTouchOverlay*>(context);
  if (!icon || !self.immediate_ || self.icon_directory_.empty() || id.empty() ||
      !std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; })) return false;
  const std::string key(id);
  auto found = self.icons_.find(key);
  if (found == self.icons_.end()) {
    if (self.icons_.size() >= 256) return false;
    std::unique_ptr<rex::ui::ImmediateTexture> texture;
    std::ifstream file(self.icon_directory_ / (key + ".png"), std::ios::binary | std::ios::ate);
    if (file) {
      const auto length = file.tellg();
      if (length > 0 && length <= 4194304) {
        std::vector<uint8_t> bytes(static_cast<size_t>(length));
        file.seekg(0);
        if (file.read(reinterpret_cast<char*>(bytes.data()), length)) {
          int width = 0, height = 0;
          const auto rgba = rex::ui::DecodeImageRGBA(bytes.data(), bytes.size(), width, height, 1024, 1024);
          if (!rgba.empty() && width > 0 && height > 0 &&
              rgba.size() <= 33554432 - self.decoded_icon_bytes_) {
            texture = self.immediate_->CreateTexture(static_cast<uint32_t>(width),
                static_cast<uint32_t>(height), rex::ui::ImmediateTextureFilter::kLinear, false, rgba.data());
            if (texture) self.decoded_icon_bytes_ += rgba.size();
          }
        }
      }
    }
    found = self.icons_.emplace(key, std::move(texture)).first;
  }
  if (!found->second) return false;
  icon->texture = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(found->second.get()));
  icon->aspect = float(found->second->width) / float(found->second->height);
  return true;
}

void ContextTouchOverlay::OnDraw(ImGuiIO& io) {
  (void)FlushContextTouchSettings();
  if (GamepadTouchLayoutSelected() || CutsceneTouchLayoutActive()) {
    repaint_needed_ = UpdateAndDrawGamepadTouchOverlay(
        ImGui::GetForegroundDrawList(), ImGui::GetFont(), ImGui::GetFontSize(), io.DisplaySize.x,
        io.DisplaySize.y);
    return;
  }
  // Reset the fallback's held state immediately when contextual gameplay returns.
  UpdateAndDrawGamepadTouchOverlay(nullptr, nullptr, 0.0f, 0.0f, 0.0f);
  const auto snapshot = GetContextTouchDrawableOverlaySnapshot();
#if REX_PLATFORM_ANDROID
  if (snapshot.visible && !snapshot.editor && REXCVAR_GET(gta4_android_frame_metrics)) {
    const auto fps = gta4::android_metrics::completed_fps_tenths.load(std::memory_order_relaxed);
    if (fps) {
      char label[40];
      std::snprintf(label, sizeof(label), "GAME %.1f FPS", fps / 10.0);
      const float size = std::max(11.0f, io.DisplaySize.y * 0.021f);
      const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, label);
      const ImVec2 pos(io.DisplaySize.x * 0.5f - extent.x * 0.5f,
                       io.DisplaySize.y * 0.11f);
      auto* draw = ImGui::GetForegroundDrawList();
      draw->AddRectFilled(ImVec2(pos.x-5, pos.y-3), ImVec2(pos.x+extent.x+5,pos.y+extent.y+3),
                          IM_COL32(0,0,0,140), 3.0f);
      draw->AddText(ImGui::GetFont(), size, pos, IM_COL32(240,240,240,220), label);
    }
  }
#endif
  repaint_needed_ = snapshot.visible && snapshot.layout.mode != ContextTouchMode::kFrontend &&
                    snapshot.layout.mode != ContextTouchMode::kMap;
  DrawContextTouchOverlay(ImGui::GetForegroundDrawList(), ImGui::GetFont(), ImGui::GetFontSize(),
                          snapshot, io.DisplaySize.x, io.DisplaySize.y,
                          icon_resolver_ ? icon_resolver_ : ResolveIcon,
                          icon_resolver_ ? icon_context_ : this);
}

}  // namespace gta4::input

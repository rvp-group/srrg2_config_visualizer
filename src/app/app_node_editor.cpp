#include <imgui_node_editor_internal.h>

#include "srrg_imgui-node-editor_app.h"
#include <imgui_node_editor.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "srrg_config_visualizer/configurable_node_manager.h"
#include <atomic>
#include <ax/Builders.h>
#include <ax/Math2D.h>
#include <ax/Widgets.h>
#include <fstream>
#include <imgui_internal.h>
#include <srrg_config/configurable_shell.h>
#include <srrg_system_utils/parse_command_line.h>
#include <thread>

#define TEST_LOG std::cerr << "test_config_node| "
namespace ed   = ax::NodeEditor;
namespace util = ed::Utilities;

using ax::Widgets::IconType;
using namespace srrg2_core;

std::string config_file = "";
static ConfigurableNodeManager manager;
static std::vector<std::string> types;
static float types_max_size_x;
static bool open_node_selector = false;
static bool setup              = true;

static const char* banner[] = {"Load a configuration and visualize the graph",
                               "w/ imgui-node-editor",
                               0};

// static inline ImRect ImGui_GetItemRect() {
//  return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
//}
//
// static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y) {
//  auto result = rect;
//  result.Min.x -= x;
//  result.Min.y -= y;
//
//  result.Max.x += x;
//  result.Max.y += y;
//  return result;
//}

static ed::Detail::EditorContext* g_Context = nullptr;

void displayMenuBar() {
  static std::string file_to_open;

  bool open_load_popup = false;
  bool open_save_popup = false;
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load config...")) {
        open_load_popup = true;
        file_to_open    = config_file;
      }

      if (ImGui::MenuItem("Save config...")) {
        open_save_popup = true;
        file_to_open    = config_file;
      }

      if (ImGui::MenuItem("Clear Workspace")) {
        manager.clear();
      }
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        //        quit = true;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  if (open_load_popup) {
    ImGui::OpenPopup("Load a config");
  }
  if (open_save_popup) {
    ImGui::OpenPopup("Save a config");
  }

  char buff[512];
  bool dummy_open = true;
  if (ImGui::BeginPopupModal("Load a config", &dummy_open)) {
    ImGui::Text("Path to configuration");
    ImGui::PushItemWidth(512);
    std::strcpy(buff, file_to_open.c_str());
    if (ImGui::InputText("", buff, 512)) {
      file_to_open = std::string(buff);
    }
    ImGui::PopItemWidth();

    if (ImGui::Button("Load")) {
      if (manager.load(file_to_open)) {
        config_file = file_to_open;
        std::cerr << "loaded file " << config_file << std::endl;
      }
      ImGui::CloseCurrentPopup();
      open_load_popup = false;
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopupModal("Save a config", &dummy_open)) {
    ImGui::Text("Path to configuration");
    ImGui::PushItemWidth(512);
    std::strcpy(buff, file_to_open.c_str());
    if (ImGui::InputText("", buff, 512)) {
      file_to_open = std::string(buff);
    }
    ImGui::PopItemWidth();

    if (ImGui::Button("Save")) {
      config_file = file_to_open;
      manager.write(config_file);
      ImGui::CloseCurrentPopup();
      open_save_popup = false;
    }
    ImGui::EndPopup();
  }
}

ImVec2 clicked_mouse;
void bgContextMenu() {
  if (ImGui::BeginPopup("bg_context_menu")) {
    clicked_mouse = ImGui::GetIO().MouseClickedPos[1];

    if (ImGui::MenuItem("Add Node")) {
      open_node_selector = true;
    }
    if (ImGui::MenuItem("Refresh")) {
      manager.refreshView(ImVec2(100, 100));
    }
    ImGui::EndPopup();
  }
  if (open_node_selector) {
    ImGui::OpenPopup("node_selector");
  }

  bool dummy_open = true;

  types_max_size_x = std::max<float>(ImGui::CalcTextSize("Select the type of node").x + 100 +
                                       ImGui::CalcTextSize("search").x,
                                     types_max_size_x);
  ImGui::SetNextWindowSize(ImVec2(types_max_size_x + 50, 300), ImGuiCond_FirstUseEver);
  if (ImGui::BeginPopupModal("node_selector", &dummy_open)) {
    ImGui::Text("Select the type of node");
    ImGui::SameLine(ImGui::GetWindowWidth() - 90 - ImGui::CalcTextSize("search").x);
    static ImGuiTextFilter filter;
    filter.Draw("search", 70);
    ImGui::SetNextWindowSize(ImVec2(types_max_size_x, 200), ImGuiCond_FirstUseEver);
    ImGui::BeginChild("ASD", ImVec2(0, 0), true);
    std::vector<bool> selected(types.size(), false);
    for (size_t i = 0; i < types.size(); ++i) {
      if (filter.PassFilter(types[i].c_str())) {
        if (ImGui::Selectable(types[i].c_str(), selected[i], ImGuiCond_FirstUseEver)) {
          if (ImGui::IsMouseDoubleClicked(0)) {
            selected[i]      = !selected[i];
            ImVec2 scrolling = g_Context->GetView().Origin;
            ImVec2 pos       = (clicked_mouse - scrolling) * ed::GetCurrentZoom();

            //            std::cerr << "pos   : " << clicked_mouse.x << " " << clicked_mouse.y <<
            //            std::endl;

            manager.createConfig(types[i], pos);

            ImGui::CloseCurrentPopup();
            open_node_selector = false;
          }
        }
      }
    }

    if (!ImGui::IsPopupOpen("node_selector")) {
      open_node_selector = false;
    }

    ImGui::EndChild();
    ImGui::EndPopup();
  }
}
static ed::NodeId contextNodeId = 0;
static ed::LinkId contextLinkId = 0;
static ed::PinId contextPinId   = 0;

static std::map<void*, PropertyContainerIdentifiablePtr> nodeid_config_map;
void nodeContextMenu() {
  std::vector<ed::NodeId> selectedNodes;
  std::vector<ed::LinkId> selectedLinks;
  selectedNodes.resize(ed::GetSelectedObjectCount());
  selectedLinks.resize(ed::GetSelectedObjectCount());

  ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
  ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

  if (ImGui::BeginPopup("node_context_menu")) {
    if (ImGui::MenuItem("Delete Node")) {
      nodeid_config_map.clear();
      for (auto& n : manager.nodes()) {
        nodeid_config_map.insert(std::make_pair(n.second->ID().AsPointer(), n.first));
      }

      for (const auto& node_id : selectedNodes) {
        manager.deleteConfigurable(nodeid_config_map.at(node_id.AsPointer()));
      }
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopup("pin_context_menu")) {
    if (ImGui::MenuItem("Delete Links")) {
      manager.deleteLinksByPin(contextPinId);
    }
    ImGui::EndPopup();
  }
}

void DrawPinIcon(PinPtr pin, bool connected, int alpha) {
  IconType iconType;
  ImColor color;
  switch (pin->type()) {
    case Pin::PinType::Config:
      iconType = IconType::Circle;
      color    = ImColor(30, 127, 255);
      break;
    case Pin::PinType::ConfigVector:
      iconType = IconType::Square;
      color    = ImColor(255, 30, 30);
      break;
    default:
      return;
  }

  color.Value.w = alpha / 255.0f;
  ax::Widgets::Icon(ImVec2(20, 20), iconType, connected, color, ImColor(32, 32, 32, alpha));
};
char buff[512];

void displayContextMenu() {
  // Open context menu

  ed::Suspend();

  if (ed::ShowNodeContextMenu(&contextNodeId)) {
    ImGui::OpenPopup("node_context_menu");
  } else if (ed::ShowPinContextMenu(&contextPinId)) {
    ImGui::OpenPopup("pin_context_menu");
  } else if (ed::ShowBackgroundContextMenu()) {
    ImGui::OpenPopup("bg_context_menu");
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

  bgContextMenu();
  nodeContextMenu();

  ImGui::PopStyleVar();

  ed::Resume();
}

void displayEditor() {
  ed::Begin("My Editor", ImVec2(0.0, 0.0f));

  if (setup && config_file.length()) {
    manager.load(config_file);
    setup = false;
  }

  displayContextMenu();

  manager.showNodes();
  manager.createLink();
  manager.showLinks();

  ed::End();
}

const char* srrg2_ine_Application_GetName() {
  return "SRRG Config Visualizer";
}

// static std::atomic<bool> shell_ready;
// static ConfigurableShell* shell;
// std::thread shell_thread;

void srrg2_ine_Application_Initialize() {
  ed::Config config;
  config.SettingsFile = nullptr;

  g_Context = reinterpret_cast<ed::Detail::EditorContext*>(ed::CreateEditor(&config));

  auto& io       = ImGui::GetIO();
  io.IniFilename = nullptr;

  ParseCommandLine cmd_line(srrg_argv, banner);
  ArgumentString file(
    &cmd_line, "c", "conf_filename", "generates a config file", "test_config_node.config");
  ArgumentString dl_stub_file(
    &cmd_line, "dlc", "dl-config", "stub where to read/write the stub", "");
  cmd_line.parse();
  if (dl_stub_file.isSet()) {
    std::ifstream is(dl_stub_file.value());
    if (is.good()) {
      ConfigurableManager::initFactory(dl_stub_file.value());
    } else {
      ConfigurableManager::makeFactoryStub(dl_stub_file.value());
    }
  }

  ConfigurableNodeManager::initFactory();
  types       = ConfigurableNodeManager::listTypes();
  config_file = file.value();
  // // start the shell thread
  // shell_ready=false;
  // shell=new ConfigurableShell(manager);
  // shell_thread=std::thread(ConfigurableShell::runStatic, StringVector(), &shell_ready);
}

void srrg2_ine_Application_Finalize() {
  ed::SetCurrentEditor(reinterpret_cast<ed::EditorContext*>(g_Context));
  TEST_LOG << "manager clear\n";
  manager.clear();
  TEST_LOG << "final checkout\n";

  ed::SetCurrentEditor(nullptr);

  ed::DestroyEditor(reinterpret_cast<ed::EditorContext*>(g_Context));

  //  shell_thread.join();
}

void srrg2_ine_Application_Frame() {
  ed::SetCurrentEditor(reinterpret_cast<ed::EditorContext*>(g_Context));
  ImGui::PushItemWidth(120.0f);

  displayMenuBar();
  displayEditor();

  ImGui::PopItemWidth();
  ed::SetCurrentEditor(nullptr);
  // ImGui::ShowMetricsWindow();
}

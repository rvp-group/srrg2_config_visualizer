#include "config_node.h"
#include "configurable_node_manager.h"
#include <srrg2_proslam_adaptation/intensity_feature_extractor_base.h>
#include <srrg_property/property_eigen.h>
#include <srrg_property/property_identifiable.h>

#define VERTICAL_SPACING 8.0f
#define HALF_VERTICAL_SPACING 4.0f
#define PADDING 14.0f
#define HALF_PADDING 7.0f
#define ITEM_WIDTH 180.0f

namespace ed = ax::NodeEditor;

namespace srrg2_core {
  using namespace srrg2_slam_interfaces; // ds feature extractor
  int ConfigNode::ed_counter = 1;

  ConfigNode::ConfigNode(PropertyContainerIdentifiablePtr configurable_) :
    _configurable(configurable_),
    _id(ed_counter++) {
    _pins.push_back(PinPtr(new Pin(ed_counter++, ed::PinKind::Input)));
    for (auto prop : _configurable->properties()) {
      PropertyConfigurableBase* pc = dynamic_cast<PropertyConfigurableBase*>(prop.second);
      if (!pc) {
        continue;
      }
      if (dynamic_cast<PropertyConfigurableVector*>(pc)) {
        _pins.push_back(PinPtr(
          new Pin(ed_counter++, ed::PinKind::Output, pc->name(), Pin::PinType::ConfigVector)));
      } else {
        _pins.push_back(PinPtr(new Pin(ed_counter++, ed::PinKind::Output, pc->name())));
      }
    }
  }

  ed::Utilities::BlueprintNodeBuilder ConfigNode::builder = ed::Utilities::BlueprintNodeBuilder();
  void ConfigNode::internals() {
    auto* window = ImGui::GetCurrentWindow();

    builder.Begin(_id);
    builder.Header(ImColor(100, 50, 55));
    ImGui::Spring(1);
    ImGui::TextUnformatted(name());
    ImGui::Spring(1);
    ImGui::Dummy(ImVec2(0, 30));
    ImGui::Spring(0);
    builder.EndHeader();

    builder.Input(inputPin()->ID());
    ax::Widgets::Icon(ImVec2(20, 20),
                      ax::Drawing::IconType::Circle,
                      false,
                      ImColor(30, 127, 255),
                      ImColor(32, 32, 32));
    ImGui::Spring(0);
    builder.EndInput();

    builder.Middle();
    ImGui::Spring(-1);

    std::map<PropertyBase*, ImVector<ImGuiID>> prop_id_map;

    for (auto field : _configurable->properties()) {
      if (dynamic_cast<PropertyIdentifiablePtrInterfaceBase*>(field.second)) {
        continue;
      }
      const char* name       = field.first.c_str();
      const char* popup_name = field.second->name().c_str();
      ImVec2 text_size       = ImGui::CalcTextSize(name);
      ImVec2 text_padding(15, HALF_VERTICAL_SPACING);
      ImGui::PushItemWidth(ITEM_WIDTH);

      char buff[512];

      if (auto p = dynamic_cast<PropertyBool*>(field.second)) {
        bool& b = p->value();
        ImGui::Checkbox(name, &b);
        ImGui::SameLine();
        if (b) {
          ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "True");
        } else {
          ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "False");
        }
      } else if (auto p = dynamic_cast<PropertyDouble*>(field.second)) {
        double& d = p->value();
        ImGui::DragScalar(name, ImGuiDataType_Double, &d, 0.05);
      } else if (auto p = dynamic_cast<PropertyString*>(field.second)) {
        if (dynamic_cast<FeatureExtractor*>(_configurable.get())) {
          if (field.first == "descriptor_type") {
            static std::vector<std::string> descriptor_types =
              FeatureExtractor::available_descriptors;
            std::sort(descriptor_types.begin(), descriptor_types.end());
            const size_t factor_classes_size = descriptor_types.size();
            const char* items[factor_classes_size];
            for (size_t i = 0; i < factor_classes_size; ++i) {
              items[i] = descriptor_types[i].c_str();
            }
            size_t index = std::distance(
              descriptor_types.begin(),
              std::find(descriptor_types.begin(), descriptor_types.end(), p->value()));
            const char* item_current = items[index == factor_classes_size ? 0 : index];

            if (ImGui::BeginCombo(name, item_current, 0)) {
              for (size_t n = 0; n < factor_classes_size; n++) {
                bool is_selected = (item_current == items[n]);
                if (ImGui::Selectable(items[n], is_selected)) {
                  item_current = items[n];
                  p->setValue(std::string(item_current));
                }
                if (is_selected) {
                  ImGui::SetItemDefaultFocus();
                }
              }
              ImGui::EndCombo();
            }

          } else if (field.first == "detector_type") {
            static std::vector<std::string> detector_type = FeatureExtractor::available_detectors;
            std::sort(detector_type.begin(), detector_type.end());
            const size_t factor_classes_size = detector_type.size();
            const char* items[factor_classes_size];
            for (size_t i = 0; i < factor_classes_size; ++i) {
              items[i] = detector_type[i].c_str();
            }
            size_t index =
              std::distance(detector_type.begin(),
                            std::find(detector_type.begin(), detector_type.end(), p->value()));
            const char* item_current = items[index == factor_classes_size ? 0 : index];

            if (ImGui::BeginCombo(name, item_current, 0)) {
              for (size_t n = 0; n < factor_classes_size; n++) {
                bool is_selected = (item_current == items[n]);
                if (ImGui::Selectable(items[n], is_selected)) {
                  item_current = items[n];
                  p->setValue(std::string(item_current));
                }
                if (is_selected) {
                  ImGui::SetItemDefaultFocus();
                }
              }
              ImGui::EndCombo();
            }
          }

        } else if (field.first == "factor_classname" || field.first == "factor_class_name") {
          const char* label      = "Select the type of factor";
          float types_max_size_x = 100.0;
          static std::vector<std::string> factor_classes =
            ConfigurableNodeManager::listFactorTypes();
          const size_t factor_classes_size = factor_classes.size();
          const char* items[factor_classes_size];
          for (size_t i = 0; i < factor_classes_size; ++i) {
            items[i]         = factor_classes[i].c_str();
            types_max_size_x = ImGui::CalcTextSize(items[i]).x;
          }
          types_max_size_x = std::max<float>(
            ImGui::CalcTextSize(label).x + 100 + ImGui::CalcTextSize("search").x, types_max_size_x);

          size_t index =
            std::distance(factor_classes.begin(),
                          std::find(factor_classes.begin(), factor_classes.end(), p->value()));
          const char* item_current = items[index == factor_classes_size ? 0 : index];

          if (ImGui::BeginCombo(name, item_current, 0)) {
            for (size_t n = 0; n < factor_classes_size; n++) {
              bool is_selected = (item_current == items[n]);
              if (ImGui::Selectable(items[n], is_selected)) {
                item_current = items[n];
                p->setValue(std::string(item_current));
              }
              if (is_selected) {
                ImGui::SetItemDefaultFocus();
              }
            }
            ImGui::EndCombo();
          }
        } else {
          std::strcpy(buff, p->value().c_str());
          if (ImGui::InputText(name, buff, 512)) {
            p->setValue(std::string(buff));
          }
        }
      } else if (auto p = dynamic_cast<PropertyFloat*>(field.second)) {
        float& f = p->value();
        ImGui::DragScalar(name, ImGuiDataType_Float, &f, 0.05);
      } else if (auto p = dynamic_cast<PropertyUInt8*>(field.second)) {
        uint8_t& u8       = p->value();
        const uint8_t min = 0, max = 255;
        ImGui::SliderScalar(name, ImGuiDataType_U8, &u8, &min, &max, "%u");
      } else if (auto p = dynamic_cast<PropertyUnsignedInt*>(field.second)) {
        uint64_t& i = p->value();
        ImGui::DragScalar(name, ImGuiDataType_U64, &i, 1);
      } else if (auto p = dynamic_cast<PropertyInt*>(field.second)) {
        int& i = p->value();
        ImGui::DragScalar(name, ImGuiDataType_S32, &i, 1);
      } else if (dynamic_cast<PropertyVector_<int>*>(field.second)) {
        prop_id_map.insert(std::make_pair(field.second, window->IDStack));
        if (ImGui::Button(name)) {
          ed::Suspend();
          ImGui::OpenPopup(popup_name);
          ed::Resume();
        }
      } else if (dynamic_cast<PropertyVector_<std::string>*>(field.second)) {
        prop_id_map.insert(std::make_pair(field.second, window->IDStack));
        if (ImGui::Button(name)) {
          ed::Suspend();
          ImGui::OpenPopup(popup_name);
          ed::Resume();
        }
      } else if (auto p = dynamic_cast<PropertyEigenBase*>(field.second)) {
        const int rows = p->rows();
        const int cols = p->cols();
        const int size = rows * cols;
        if (!size) {
          throw std::runtime_error("Eigen property has no rows or cols");
        }

        prop_id_map.insert(std::make_pair(field.second, window->IDStack));
        if (ImGui::Button(name, text_size + text_padding)) {
          ed::Suspend();
          ImGui::OpenPopup(popup_name);
          ed::Resume();
        }
      } else {
        std::cerr << "type of property " << field.first << " is not handled" << std::endl;
        std::cerr << "please contact the maintainers" << std::endl;
      }
    }

    ImGui::Spring(1);

    for (auto& output : outputPins()) {
      builder.Output(output->ID());
      ImGui::Spring(0);
      ImGui::TextUnformatted(output->paramName().c_str());
      ImGui::Spring(0);
      ax::Widgets::IconType iconType;
      ImColor color;
      switch (output->type()) {
        case Pin::PinType::Config:
          iconType = ax::Widgets::IconType::Circle;
          color    = ImColor(30, 127, 255);
          break;
        case Pin::PinType::ConfigVector:
          iconType = ax::Widgets::IconType::Square;
          color    = ImColor(255, 30, 30);
          break;
        default:
          return;
      }

      ax::Widgets::Icon(ImVec2(20, 20), iconType, false, color, ImColor(32, 32, 32));
      builder.EndOutput();
    }

    builder.Footer();
    ImGui::Spring(1);

    const char* buff_name = _configurable->name().c_str();
    if (ImGui::InputText("name", (char*) buff_name, 256)) {
      _configurable->setName(std::string(buff_name));
    }
    ImGui::Spring(1);

    builder.EndFooter();
    builder.End();

    for (auto elem : prop_id_map) {
      const char* popup_name = elem.first->name().c_str();
      std::swap(window->IDStack, elem.second);

      if (auto p = dynamic_cast<PropertyVector_<int>*>(elem.first)) {
        std::vector<int> values_int = p->value();
        uint64_t size               = values_int.size();

        ed::Suspend();
        if (ImGui::BeginPopup(popup_name)) {
          ImGui::PushItemWidth(70);
          ImGui::InputScalar("vector size", ImGuiDataType_U64, &size, NULL, NULL, "%u");
          if (values_int.size() != size) {
            values_int.resize(size);
          }
          for (uint64_t i = 0; i < size; ++i) {
            std::string s = "val " + std::to_string(i);
            ImGui::DragScalar(s.c_str(), ImGuiDataType_S32, &values_int[i], 1);
          }
          if (!ImGui::IsPopupOpen(popup_name)) {
            p->setValue(values_int);
          }
          ImGui::PopItemWidth();
          ImGui::EndPopup();
        }
        ed::Resume();
      } else if (auto p = dynamic_cast<PropertyVector_<std::string>*>(elem.first)) {
        std::vector<std::string> values_string = p->value();
        values_string.reserve(100);
        uint64_t size = values_string.size();
        ed::Suspend();
        if (ImGui::BeginPopup(popup_name)) {
          ImGui::PushItemWidth(70);
          ImGui::InputScalar("vector size", ImGuiDataType_U64, &size, NULL, NULL, "%u");
          ImGui::PopItemWidth();
          if (values_string.size() != size) {
            values_string.resize(size);
          }
          for (uint64_t i = 0; i < size; ++i) {
            std::string s = "val " + std::to_string(i);
            char buff_internal[256];
            std::strcpy(buff_internal, values_string[i].c_str());
            //            const char* buff_internal = values_string[i].c_str();
            if (ImGui::InputText(s.c_str(), buff_internal, 256)) {
              values_string[i] = buff_internal;
              p->setValue(i, values_string[i]);
            }
          }
          if (!ImGui::IsPopupOpen(popup_name)) {
            p->setValue(values_string);
          }
          ImGui::EndPopup();
        }
        ed::Resume();
      } else if (auto p = dynamic_cast<PropertyEigenBase*>(elem.first)) {
        const int rows = p->rows();
        const int cols = p->cols();
        const int size = rows * cols;
        std::vector<float> values_float(size);
        for (int r = 0; r < rows; ++r) {
          for (int c = 0; c < cols; ++c) {
            values_float[r * cols + c] = p->valueAt(r, c);
          }
        }
        ed::Suspend();
        if (ImGui::BeginPopup(popup_name)) {
          ImGui::PushItemWidth(70);
          for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
              char s[16];
              sprintf(s, "##%u,%u", r, c);
              if (ImGui::DragScalar(s, ImGuiDataType_Float, &values_float[r * cols + c], 0.05)) {
                p->setValueAt(r, c, values_float[r * cols + c]);
              }
              ImGui::SameLine();
            }
            ImGui::NewLine();
          }

          if (!ImGui::IsPopupOpen(popup_name)) {
            for (int r = 0; r < rows; ++r) {
              for (int c = 0; c < cols; ++c) {
                p->setValueAt(r, c, values_float[r * cols + c]);
              }
            }
          }
          ImGui::PopItemWidth();
          ImGui::EndPopup();
        }
        ed::Resume();
      }

      std::swap(window->IDStack, elem.second);
    }
  }

} // namespace srrg2_core

#pragma once
#include "srrg_config/property_configurable_vector.h"

#include <imgui_node_editor_internal.h>

#include <ax/Builders.h>
#include <ax/Drawing.h>
#include <ax/Widgets.h>

namespace srrg2_core {

  using PropertyConfigurableBase   = PropertyIdentifiablePtrInterfaceBase;
  using PropertyConfigurable       = PropertyIdentifiablePtrInterface;
  using PropertyConfigurableVector = PropertyIdentifiablePtrVectorInterface;

  class ConfigNode;

  class Pin {
  public:
    enum class PinType {
      Config,
      ConfigVector,
    };

    Pin() = delete;
    Pin(int id_,
        ax::NodeEditor::PinKind kind_,
        const std::string& param_name_ = "",
        PinType type_                  = PinType::Config) :
      _id(id_),
      _param_name(param_name_),
      _type(type_),
      _direction(kind_) {
    }

    const ax::NodeEditor::PinId& ID() const {
      return _id;
    }

    const std::string& paramName() const {
      return _param_name;
    }

    const PinType& type() const {
      return _type;
    }

    const ax::NodeEditor::PinKind direction() const {
      return _direction;
    }

  protected:
    ax::NodeEditor::PinId _id;
    std::string _param_name            = "";
    PinType _type                      = PinType::Config;
    ax::NodeEditor::PinKind _direction = ax::NodeEditor::PinKind::Input;
  };

  using PinPtr = std::shared_ptr<Pin>;

  class NodeLink;
  using NodeLinkPtr = std::shared_ptr<NodeLink>;

  struct BoundingBox {
    ImVec2 pos  = ImVec2(0, 0);
    ImVec2 size = ImVec2(0, 0);
  };

  class ConfigNode {
  public:
    ConfigNode(PropertyContainerIdentifiablePtr configurable_);

    const ax::NodeEditor::NodeId& ID() {
      return _id;
    }
    inline const char* name() const {
      return _configurable->className().c_str();
    }

    PropertyContainerIdentifiablePtr configurable() {
      return _configurable;
    }

    virtual ~ConfigNode() {
      // TODO rilasciare nodeID
      releaseConnections();
    }

    void computeSize() {
      node_bb.size = ax::NodeEditor::GetNodeSize(_id);
    }

    void releaseConnections() {
      _input_links.clear();
      _output_links.clear();
    }

    void internals();

    const PinPtr inputPin() const {
      return _pins[0];
    }
    std::vector<PinPtr> outputPins() const {
      return std::vector<PinPtr>(_pins.begin() + 1, _pins.end());
    }

    BoundingBox node_bb;

    const std::multimap<std::string, NodeLinkPtr>& outputLinks() const {
      return _output_links;
    }

    const std::vector<NodeLinkPtr>& inputLinks() const {
      return _input_links;
    }

    friend class ConfigurableNodeManager;
    friend class NodeLink;

  protected:
    static ax::NodeEditor::Utilities::BlueprintNodeBuilder builder;
    std::vector<PinPtr> _pins;
    std::vector<NodeLinkPtr> _input_links;
    std::multimap<std::string, NodeLinkPtr> _output_links;

    PropertyContainerIdentifiablePtr _configurable = nullptr;
    //    const int _id;
    ax::NodeEditor::NodeId _id;

    static int ed_counter;
    static void _resetCouter() {
      ed_counter = 1;
    }
  };
  using ConfigNodePtr = std::shared_ptr<ConfigNode>;

} // namespace srrg2_core

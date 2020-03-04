#pragma once
#include "config_node.h"
#include <srrg_config/configurable_manager.h>
#include <srrg_data_structures/matrix.h>
#include <srrg_system_utils/shell_colors.h>
#include <srrg_system_utils/system_utils.h>

namespace srrg2_core {
  using NodeMap = std::map<PropertyContainerIdentifiablePtr, ConfigNodePtr>;

  class NodeLink {
  public:
    NodeLink() = delete;
    NodeLink(ax::NodeEditor::LinkId id_,
             std::shared_ptr<ConfigNode> parent_,
             const std::string& param_name_) :
      _id(id_),
      _parent(parent_),
      _param_name(param_name_) {
    }

    ~NodeLink() = default;

    const ax::NodeEditor::LinkId& ID() {
      return _id;
    }

    void reset() {
      child   = nullptr;
      _parent = nullptr;
    }

    void release() {
      auto& child_links  = child->_input_links;
      auto& parent_links = _parent->_output_links;

      for (size_t i = 0; i < child_links.size(); ++i) {
        if (child_links[i].get() == this) {
          child_links.erase(child_links.begin() + i);
          break;
        }
      }

      for (auto o_it = parent_links.begin(); o_it != parent_links.end(); ++o_it) {
        if (o_it->second.get() == this) {
          parent_links.erase(o_it);
          break;
        }
      }
    }

    bool isPresent() {
      return (bool) (_parent && child);
    }
    const std::shared_ptr<ConfigNode> parent() const {
      return _parent;
    }
    std::shared_ptr<ConfigNode> parent() {
      return _parent;
    }
    const std::string& paramName() const {
      return _param_name;
    }
    std::shared_ptr<ConfigNode> child = nullptr;

  protected:
    ax::NodeEditor::LinkId _id;
    std::shared_ptr<ConfigNode> _parent = nullptr;
    const std::string _param_name;
  };

  using NodeLinkPtr = std::shared_ptr<NodeLink>;

  class ConfigurableNodeManager : public ConfigurableManager {
  public:
    ~ConfigurableNodeManager() {
      clear();
    }

    static std::vector<std::string> listFactorTypes();

    bool load(const std::string& file_) {
      std::cerr << "loading file: " << file_ << std::endl;
      if (file_.length() && srrg2_core::isAccessible(file_)) {
        clear();
        this->read(file_);
        _buildConfigNodes();
        return true;
      }
      std::cerr << "file not present" << std::endl;
      return false;
    }

    bool updateConnection(const NodeLinkPtr link_, std::shared_ptr<ConfigNode> new_child_);

    void createConfig(const std::string& type_, ImVec2 pos_);
    void addConfig(const PropertyContainerIdentifiablePtr instance_, ImVec2 pos_);

    void refreshView(ImVec2 pos_);

    void clear();

    void deleteConfigurable(PropertyContainerIdentifiablePtr configurable_);

    inline void showNodes() {
      for (const auto& node : _nodes) {
        if (!node.second) {
          continue;
        }
        node.second->internals();
      }
    }

    inline void showLinks() {
      // srrg questo e' na mmerda
      for (auto l : _links) {
        for (auto op : l->parent()->outputPins()) {
          if (op->paramName() == l->paramName()) {
            ax::NodeEditor::Link(l->ID(), op->ID(), l->child->inputPin()->ID());
            break;
          }
        }
      }
    }

    void createLink();

    inline const NodeMap& nodes() const {
      return _nodes;
    }

    void deleteLinksByPin(ax::NodeEditor::PinId pin_);

  protected:
    NodeMap _nodes;
    float _curr_y_bb;
    std::vector<NodeLinkPtr> _links;

    static int ed_counter_links;

    void _computeHierarchy(ImVec2 pos);
    void _computeSources(const Matrix_<int>& matrix_,
                         const std::map<PropertyContainerIdentifiablePtr, size_t>& bookkeeping_,
                         int level_,
                         std::multimap<int, ConfigNodePtr>& sources_);

    void _buildConfigNodes();
    void _computeNodesPose(ImVec2 init_pose_, const std::multimap<int, ConfigNodePtr>& sources_);
    void _computeSortedNodePoses(const std::vector<BoundingBox>& bb_,
                                 const std::multimap<int, ConfigNodePtr>& sources_);
    void _computeSortedNodePoses(ConfigNodePtr parent_node_,
                                 const size_t& lvl_,
                                 const std::vector<BoundingBox>& bb_,
                                 const std::multimap<int, ConfigNodePtr>& sources_);

    void _clearNodes() {
      std::cerr << "\n";
      for (auto c_pair : _nodes) {
        PropertyContainerIdentifiablePtr c = c_pair.first;
        std::cerr << "ConfigurableNodeManager::_clearNodes|destroying node [ "
                  << FG_RED(static_cast<void*>(c.get())) << " ] CN [ " << c->className()
                  << " ] -- ";
        erase(c);
        std::cerr << "[ " << FG_GREEN("SUCCESS") << " ] -- releasing connections ";
        c_pair.second->releaseConnections();
        std::cerr << "[ " << FG_GREEN("SUCCESS") << " ]\n";
      }

      _nodes.clear();
      std::cerr << "ConfigurableNodeManager::_clearNodes|container cleaned\n";
      ConfigNode::_resetCouter();
      std::cerr << "ConfigurableNodeManager::_clearNodes|counter reset\n";
    }

    void _clearLinks();

    inline void _releaseLinks(ConfigNodePtr node_) {
      const auto& i_links = node_->inputLinks();

      for (auto it : i_links) {
        it->release();

        ax::NodeEditor::DeleteLink(it->ID());
        auto l_to_del = std::find(_links.begin(), _links.end(), it);
        if (l_to_del != _links.end()) {
          _links.erase(l_to_del);
        }
      }

      const auto& o_links = node_->outputLinks();
      for (auto it = o_links.begin(); it != o_links.end(); ++it) {
        it->second->release();

        ax::NodeEditor::DeleteLink(it->second->ID());
        auto l_to_del = std::find(_links.begin(), _links.end(), it->second);
        if (l_to_del != _links.end()) {
          _links.erase(l_to_del);
        }
      }
      node_->releaseConnections();
    }

    inline std::pair<ConfigNodePtr, PinPtr> _findPin(ax::NodeEditor::PinId id) {
      if (!id) {
        return std::make_pair(nullptr, nullptr);
      }

      for (auto node : _nodes) {
        if (node.second->inputPin()->ID() == id) {
          return std::make_pair(node.second, node.second->inputPin());
        }
        for (auto& pin : node.second->outputPins()) {
          if (pin->ID() == id) {
            return std::make_pair(node.second, pin);
          }
        }
      }
      return std::make_pair(nullptr, nullptr);
    }
  };

} // namespace srrg2_core

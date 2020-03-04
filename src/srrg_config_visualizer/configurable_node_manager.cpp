#include "configurable_node_manager.h"
#include <srrg_solver/solver_core/factor_base.h>
#include <srrg_system_utils/system_utils.h>

namespace srrg2_core {
  int ConfigurableNodeManager::ed_counter_links = 1e5;

  void ConfigurableNodeManager::createLink() {
    namespace ed = ax::NodeEditor;

    if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f)) {
      ed::PinId parent_pin_id = 0, child_pin_id = 0;
      if (ed::QueryNewLink(&parent_pin_id, &child_pin_id)) {
        auto parent_pair = _findPin(parent_pin_id);
        auto child_pair  = _findPin(child_pin_id);

        ConfigNodePtr parent_node = parent_pair.first;
        PinPtr parent_pin         = parent_pair.second;

        ConfigNodePtr child_node = child_pair.first;
        PinPtr child_pin         = child_pair.second;

        if (parent_pin->direction() == ed::PinKind::Input) {
          std::swap(parent_pin, child_pin);
          std::swap(parent_node, child_node);
        }

        if (parent_pin && child_pin) {
          if (parent_node == child_node) {
            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
          } else if (parent_pin->direction() == child_pin->direction()) {
            ed::RejectNewItem(ImColor(255, 50, 25), 2.0f);
          } else {
            if (ed::AcceptNewItem(ImColor(255, 255, 255), 2.0f)) {
              NodeLinkPtr l(new NodeLink(ed_counter_links++, parent_node, parent_pin->paramName()));
              if (!ImGui::IsMouseDown(0) && updateConnection(l, child_node)) {
                if (std::find_if(_links.begin(), _links.end(), [l](NodeLinkPtr other_l) {
                      return l->parent() == other_l->parent() && l->child == other_l->child;
                    }) == _links.end()) {
                  _links.emplace_back(l);
                }
              }
            }
          }
        }
      }
    }
    ed::EndCreate();
  }

  void ConfigurableNodeManager::deleteLinksByPin(ax::NodeEditor::PinId pin_) {
    auto node_pin_pair        = _findPin(pin_);
    ConfigNodePtr parent_node = node_pin_pair.first;
    PinPtr pin                = node_pin_pair.second;
    const auto& param_name    = pin->paramName();
    auto prop_it              = parent_node->configurable()->properties().find(param_name);
    if (prop_it == parent_node->configurable()->properties().end()) {
      std::cerr << "class [" << parent_node->configurable()->className()
                << "] does not have field [" << param_name << "]" << std::endl;
      return;
    }
    // srrg remove the actual connection from the configs
    if (auto pcv = dynamic_cast<PropertyConfigurableVector*>(prop_it->second)) {
      pcv->assign(std::vector<PropertyContainerIdentifiablePtr>());
    } else if (auto pc = dynamic_cast<PropertyConfigurable*>(prop_it->second)) {
      pc->assign(PropertyContainerIdentifiablePtr());
    }

    // srrg get all the links connected to the pin
    auto lu_it = parent_node->outputLinks().equal_range(param_name);
    std::vector<NodeLinkPtr> tmp_vec;
    for (auto it = lu_it.first; it != lu_it.second; ++it) {
      tmp_vec.emplace_back(it->second);
    }

    // srrg remove link from the childs
    for (auto l : tmp_vec) {
      l->release();
      auto l_it = std::find(_links.begin(), _links.end(), l);
      if (l_it != _links.end()) {
        _links.erase(l_it);
        ax::NodeEditor::DeleteLink(l->ID());
      }
    }
  }

  bool ConfigurableNodeManager::updateConnection(const NodeLinkPtr link_,
                                                 ConfigNodePtr new_child_) {
    if (!new_child_ || !link_) {
      return false;
    }

    const auto& parent_node = link_->parent();
    const auto& param_name  = link_->paramName();
    auto prop_it            = parent_node->configurable()->properties().find(param_name);
    if (prop_it == parent_node->configurable()->properties().end()) {
      std::cerr << "class [" << parent_node->configurable()->className()
                << "] does not have field [" << param_name << "]" << std::endl;
      return false;
    }

    if (auto pcv = dynamic_cast<PropertyConfigurableVector*>(prop_it->second)) {
      PropertyContainerIdentifiablePtr val = new_child_->configurable();

      if (!pcv->canAssign(val)) {
        return false;
      }

      bool is_present = false;
      for (size_t i = 0; i < pcv->size(); ++i) {
        if (pcv->getSharedPtr(i) == val) {
          is_present = true;
        }
      }

      if (!is_present) {
        pcv->pushBack(val);
      }

      is_present = false;
      auto lu_it = parent_node->outputLinks().equal_range(param_name);
      for (auto it = lu_it.first; it != lu_it.second; ++it) {
        if (it->second->child->configurable() == new_child_->configurable()) {
          is_present = true;
        }
      }

      if (!is_present) {
        parent_node->_output_links.insert(std::make_pair(param_name, link_));
      }

      new_child_->_input_links.push_back(link_);
      link_->child = new_child_;
      return true;
    }

    if (auto pc = dynamic_cast<PropertyConfigurable*>(prop_it->second)) {
      PropertyContainerIdentifiablePtr val = new_child_->configurable();
      // srrg can I assign the module to the actual property?
      if (!pc->canAssign(val)) {
        return false;
      }

      // srrg release the connection to the old child
      for (const auto& pin : parent_node->outputPins()) {
        if (pin->paramName() == param_name) {
          deleteLinksByPin(pin->ID());
          break;
        }
      }

      // srrg set the connection to the actual one
      pc->assign(val);
      new_child_->_input_links.push_back(link_);
      link_->child = new_child_;
      parent_node->_output_links.insert(std::make_pair(pc->name(), link_));
      return true;
    }
    return false;
  }

  void ConfigurableNodeManager::addConfig(const PropertyContainerIdentifiablePtr instance_,
                                          ImVec2 pos_) {
    NodeMap created_nodes;
    if (_nodes.find(instance_) == _nodes.end()) {
      ConfigNodePtr node(new ConfigNode(instance_));
      node->node_bb.pos = pos_;
      created_nodes.insert(std::make_pair(instance_, node));
      _nodes.insert(std::make_pair(instance_, node));
    }

    std::set<PropertyContainerIdentifiablePtr> connected_configs;
    instance_->getReacheableContainers(connected_configs);

    for (const auto& config : connected_configs) {
      if (_nodes.find(config) != _nodes.end()) {
        continue;
      }
      ConfigNodePtr node(new ConfigNode(config));
      created_nodes.insert(std::make_pair(config, node));
      _nodes.insert(std::make_pair(config, node));
    }
    int adjac_matrix_size = created_nodes.size();
    std::map<PropertyContainerIdentifiablePtr, size_t> bookkeeping;
    size_t i = 0;

    for (auto n : created_nodes) {
      n.second->internals();
      n.second->computeSize();
      bookkeeping.insert(std::make_pair(n.first, i));
      ++i;
    }

    srrg2_core::Matrix_<int> config_adjacency_matrix;
    config_adjacency_matrix.resize(adjac_matrix_size, adjac_matrix_size);

    // mc create connections
    for (const auto& node_pair : created_nodes) {
      ConfigNodePtr parent = node_pair.second;
      std::multimap<std::string, PropertyContainerIdentifiablePtr> connected_configs;
      parent->configurable()->getConnectedContainers(connected_configs);
      for (const auto& elem : connected_configs) {
        PropertyContainerIdentifiablePtr c = elem.second;
        if (!c) {
          continue;
        }
        if (_nodes.find(c) == _nodes.end()) {
          std::cerr << "Please add this module manually to the config: " << std::endl;
          std::cerr << "  " << node_pair.first->className() << "->" << c->className() << std::endl;
          continue;
        }
        auto child = _nodes.at(c);
        NodeLinkPtr link(new NodeLink(ed_counter_links++, parent, elem.first));
        updateConnection(link, child);
        _links.emplace_back(link);
        config_adjacency_matrix(bookkeeping[parent->configurable()],
                                bookkeeping[child->configurable()]) = 1;
      }
    }
    std::multimap<int, ConfigNodePtr> sources;
    _computeSources(config_adjacency_matrix, bookkeeping, 0, sources);
    _computeNodesPose(pos_, sources);
  }

  void ConfigurableNodeManager::createConfig(const std::string& type_, ImVec2 pos_) {
    PropertyContainerIdentifiablePtr instance = this->create(type_);
    addConfig(instance, pos_);
    //    _buildConfigNodes();
    //    if (pos_.x == -1 && pos_.y == -1) {
    //      return;
    //    }
    //    _nodes.find(instance)->second->node_bb.pos = pos_;
  }

  std::vector<std::string> ConfigurableNodeManager::listFactorTypes() {
    // now we try a cast to initiate the types
    std::vector<std::string> available;
    std::vector<std::string> class_names = srrg2_core::getClassNames();
    for (const std::string& name : class_names) {
      // std::cerr << *it << " ";
      srrg2_core::Serializable* ser = srrg2_core::Serializable::createInstance(name);
      srrg2_solver::FactorBase* f   = dynamic_cast<srrg2_solver::FactorBase*>(ser);
      if (f) {
        available.push_back(name);
      }
      delete ser;
    }
    return available;
  }

  void ConfigurableNodeManager::refreshView(ImVec2 pos_) {
    _computeHierarchy(pos_);
  }

  void ConfigurableNodeManager::clear() {
    std::cerr << "ConfigurableNodeManager::clear|destroying links ... ";
    _clearLinks();
    std::cerr << "[ " << FG_GREEN("SUCCESS") << " ]\n";

    std::cerr << "ConfigurableNodeManager::clear|destroying nodes ... ";
    _clearNodes();
    std::cerr << "[ " << FG_GREEN("SUCCESS") << " ]\n";
  }

  void ConfigurableNodeManager::deleteConfigurable(PropertyContainerIdentifiablePtr configurable_) {
    _releaseLinks(_nodes[configurable_]);
    _nodes.erase(configurable_);
    erase(configurable_);
  }

  void ConfigurableNodeManager::_computeHierarchy(ImVec2 pos_) {
    _clearLinks();
    int adjac_matrix_size = _nodes.size();
    std::map<PropertyContainerIdentifiablePtr, size_t> bookkeeping;
    size_t i = 0;
    for (auto n : _nodes) {
      n.second->internals();
      n.second->computeSize();
      bookkeeping.insert(std::make_pair(n.first, i));
      ++i;
    }
    srrg2_core::Matrix_<int> config_adjacency_matrix;
    config_adjacency_matrix.resize(adjac_matrix_size, adjac_matrix_size);
    // mc create connections
    for (const auto& node_pair : _nodes) {
      ConfigNodePtr parent = node_pair.second;
      std::multimap<std::string, PropertyContainerIdentifiablePtr> connected_configs;
      parent->configurable()->getConnectedContainers(connected_configs);
      for (const auto& elem : connected_configs) {
        PropertyContainerIdentifiablePtr c = elem.second;
        if (!c) {
          continue;
        }
        if (_nodes.find(c) == _nodes.end()) {
          std::cerr << "Please add this module manually to the config: " << std::endl;
          std::cerr << "  " << node_pair.first->className() << "->" << c->className() << std::endl;
          continue;
        }
        auto child = _nodes.at(c);
        NodeLinkPtr link(new NodeLink(ed_counter_links++, parent, elem.first));
        updateConnection(link, child);
        _links.emplace_back(link);
        config_adjacency_matrix(bookkeeping[parent->configurable()],
                                bookkeeping[child->configurable()]) = 1;
      }
    }

    std::multimap<int, ConfigNodePtr> sources;
    _computeSources(config_adjacency_matrix, bookkeeping, 0, sources);
    _computeNodesPose(pos_, sources);
  }

  void ConfigurableNodeManager::_computeSources(
    const Matrix_<int>& matrix_,
    const std::map<PropertyContainerIdentifiablePtr, size_t>& bookkeeping_,
    int level_,
    std::multimap<int, ConfigNodePtr>& sources_) {
    std::vector<PropertyContainerIdentifiablePtr> sources_to_delete;
    //    for (size_t r = 0; r < matrix_.rows(); ++r) {
    //      for (size_t c = 0; c < matrix_.cols(); ++c) {
    //        std::cerr << matrix_(r, c) << " ";
    //      }
    //      std::cerr << std::endl;
    //    }
    //    std::cerr << "level " << level_ << " sources:\n";
    for (size_t c = 0; c < matrix_.cols(); ++c) {
      int c_sum = 0;
      for (size_t r = 0; r < matrix_.rows(); ++r) {
        c_sum += matrix_(r, c);
      }
      if (c_sum == 0) {
        PropertyContainerIdentifiablePtr to_delete;
        for (auto b : bookkeeping_) {
          if (b.second == c) {
            to_delete = b.first;
            break;
          }
        }
        //        std::cerr << "\t" << _nodes[to_delete]->name() << " " <<
        //        _nodes[to_delete]->id()
        //        << " col: " << c << " conf: " << to_delete << std::endl;
        sources_.insert(std::make_pair(level_, _nodes[to_delete]));
        sources_to_delete.push_back(to_delete);
      }
    }
    //    std::cerr << std::endl;
    size_t submatrix_size = matrix_.rows() - sources_to_delete.size();
    if (!submatrix_size) {
      return;
    }
    std::map<PropertyContainerIdentifiablePtr, size_t> bookkeeping;
    std::vector<size_t> sources_idx;
    int i = 0;
    for (auto b : bookkeeping_) {
      if (std::find(sources_to_delete.begin(), sources_to_delete.end(), b.first) ==
          sources_to_delete.end()) {
        bookkeeping.insert(std::make_pair(b.first, i++));
      } else {
        //        std::cerr << b.first->className() << " " << b.first << " c: "
        //        << b.second << std::endl;
        sources_idx.push_back(b.second);
      }
    }
    if (submatrix_size < 2) {
      ConfigNodePtr n = _nodes[bookkeeping.begin()->first];
      sources_.insert(std::make_pair(++level_, n));
      //      std::cerr << "level " << level_ << " sources:\n";
      //      std::cerr << "\t" << n->name() << " " << n->id() << " col: " << 0
      //      << std::endl;
      return;
    }
    Matrix_<int> other_matrix(submatrix_size, submatrix_size);
    int k = 0;
    for (size_t r = 0; r < matrix_.rows(); ++r) {
      for (size_t c = 0; c < matrix_.cols(); ++c) {
        bool add_to_new_matrix = false;
        for (size_t i = 0; i < sources_idx.size(); ++i) {
          const size_t& idx_to_delete = sources_idx[i];
          if (r == idx_to_delete || c == idx_to_delete) {
            add_to_new_matrix = false;
            break;
          } else {
            add_to_new_matrix = true;
          }
        }
        if (add_to_new_matrix) {
          other_matrix.data()[k++] = matrix_(r, c);
        }
      }
    }
    _computeSources(other_matrix, bookkeeping, ++level_, sources_);
  }

  void ConfigurableNodeManager::_buildConfigNodes() {
    for (PropertyContainerIdentifiablePtr config : _instances) {
      if (_nodes.find(config) != _nodes.end()) {
        continue;
      }
      ConfigNodePtr node(new ConfigNode(config));
      _nodes.insert(std::make_pair(config, node));
    }
    refreshView(ImVec2(100, 100));
  }

  void
  ConfigurableNodeManager::_computeNodesPose(ImVec2 init_pose_,
                                             const std::multimap<int, ConfigNodePtr>& sources_) {
    static const ImVec2 padding = ImVec2(20, 20);
    int levels                  = 0;
    for (auto it = sources_.begin(); it != sources_.end(); ++it) {
      levels = std::max<int>(levels, it->first);
    }

    std::vector<BoundingBox> bb_per_level;
    bb_per_level.resize(levels + 1);
    int prev_level = 0;
    for (auto it = sources_.begin(); it != sources_.end(); ++it) {
      const int& level   = it->first;
      ConfigNodePtr node = it->second;

      // srrg compute level bounding boxes
      if (it == sources_.begin()) {
        bb_per_level[level].pos  = init_pose_;
        _curr_y_bb               = init_pose_.y;
        bb_per_level[level].size = node->node_bb.size + padding;
        continue;
      }

      if (prev_level != level) {
        bb_per_level[level].pos = bb_per_level[prev_level].pos;
        bb_per_level[level].pos.x += bb_per_level[prev_level].size.x;
        bb_per_level[level].size = node->node_bb.size + padding;
      } else {
        bb_per_level[level].size.x =
          std::max<float>(bb_per_level[level].size.x, node->node_bb.size.x + padding.x);
      }
      prev_level = level;
    }

    float max_bb_y = 0;
    for (int i = 0; i < levels + 1; ++i) {
      //      std::cerr << "level[" << i << "]: " << bb_per_level[i].pos.x << " " <<
      //      bb_per_level[i].pos.y
      //                << " s: " << bb_per_level[i].size.x << "x" << bb_per_level[i].size.y << " "
      //                << std::endl;
      max_bb_y = std::max(max_bb_y, bb_per_level[i].size.y);
      ImRect rect;
    }
    // srrg set node poses
    _computeSortedNodePoses(bb_per_level, sources_);

    for (auto node : sources_) {
      //      ImVec2 fixed_pos = node.second->node_bb.pos;
      //      node.second->node_bb.pos.y -= bb_per_level[0].pos.y * .5;
      //      node.second->node_bb.pos.y += node.second->node_bb.size.y * .5;
      ax::NodeEditor::SetNodePosition(node.second->ID(), node.second->node_bb.pos);
    }
  }

  void ConfigurableNodeManager::_computeSortedNodePoses(
    ConfigNodePtr parent_node_,
    const size_t& lvl_,
    const std::vector<BoundingBox>& bb_,
    const std::multimap<int, ConfigNodePtr>& sources_) {
    static const ImVec2 padding      = ImVec2(20, 20);
    static const ImVec2 half_padding = ImVec2(8, 8);

    const float prev_y_bb = _curr_y_bb;

    for (auto con : parent_node_->outputLinks()) {
      auto cn = con.second->child;
      _computeSortedNodePoses(cn, lvl_ + 1, bb_, sources_);
    }

    parent_node_->node_bb.pos.x = bb_[lvl_].pos.x + half_padding.x;
    if (parent_node_->outputLinks().size()) {
      parent_node_->node_bb.pos.y =
        prev_y_bb + (_curr_y_bb - prev_y_bb) * .5f - parent_node_->node_bb.size.y * .5f;
      float tmp = prev_y_bb + parent_node_->node_bb.size.y + padding.y;
      if (tmp > _curr_y_bb) {
        parent_node_->node_bb.pos.y = prev_y_bb + half_padding.y;
        _curr_y_bb                  = tmp;
      }
    } else {
      parent_node_->node_bb.pos.y = prev_y_bb + half_padding.y;
      _curr_y_bb                  = prev_y_bb + parent_node_->node_bb.size.y + padding.y;
    }
  }

  void ConfigurableNodeManager::_computeSortedNodePoses(
    const std::vector<BoundingBox>& bb_,
    const std::multimap<int, ConfigNodePtr>& sources_) {
    std::vector<ConfigNodePtr> sorted_sources(sources_.count(0));
    for (size_t i = 0; i < sorted_sources.size(); ++i) {
      sorted_sources[i] = (std::next(sources_.begin(), i))->second;
    }

    std::sort(sorted_sources.begin(), sorted_sources.end(), [](ConfigNodePtr a_, ConfigNodePtr b_) {
      return std::string(a_->name()) < std::string(b_->name());
    });
    for (auto s_s : sorted_sources) {
      _computeSortedNodePoses(s_s, 0, bb_, sources_);
    }
  }

  void ConfigurableNodeManager::_clearLinks() {
    for (auto l : _links) {
      ax::NodeEditor::DeleteLink(l->ID());
    }
    for (auto n : _nodes) {
      n.second->releaseConnections();
    }
    _links.clear();
    ed_counter_links = 1e5;
  }

} // namespace srrg2_core

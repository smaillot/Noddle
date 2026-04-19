#pragma once

#include <QtNodes/Definitions>
#include <vector>
#include <unordered_map>

namespace QtNodes {
class DataFlowGraphModel;
}

namespace noddle {

using ExecutionLevel = std::vector<QtNodes::NodeId>;

struct GraphSchedule
{
    std::vector<ExecutionLevel> levels;
    std::unordered_map<QtNodes::NodeId, std::size_t> nodeLevels;
    std::vector<QtNodes::NodeId> sourceNodes;

    struct Edge {
        QtNodes::NodeId targetNode;
        QtNodes::PortIndex outPort;
        QtNodes::PortIndex inPort;
    };
    std::unordered_map<QtNodes::NodeId, std::vector<Edge>> adjacency;

    /// Reverse adjacency: nodeId → incoming edges (sourceNode, outPort, inPort).
    std::unordered_map<QtNodes::NodeId, std::vector<Edge>> incomingEdges;

    [[nodiscard]] bool isValid() const { return !levels.empty(); }
};

[[nodiscard]] GraphSchedule buildSchedule(QtNodes::DataFlowGraphModel const &model);

} // namespace noddle

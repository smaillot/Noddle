#include "engine/GraphSchedule.hpp"

#include <QtNodes/DataFlowGraphModel>

#include <algorithm>
#include <queue>
#include <unordered_set>

namespace noddle {

GraphSchedule buildSchedule(QtNodes::DataFlowGraphModel const &model)
{
    auto const nodeIds = model.allNodeIds();
    if (nodeIds.empty())
        return {};

    GraphSchedule schedule;

    // Track which nodes have incoming connections.
    std::unordered_set<QtNodes::NodeId> hasIncoming;

    // In-degree per node (for Kahn's algorithm).
    std::unordered_map<QtNodes::NodeId, std::size_t> inDegree;
    for (auto id : nodeIds)
        inDegree[id] = 0;

    // Build adjacency and compute in-degrees from connections.
    for (auto id : nodeIds) {
        auto const connections = model.allConnectionIds(id);
        for (auto const &cn : connections) {
            if (cn.outNodeId == id) {
                schedule.adjacency[id].push_back(
                    {cn.inNodeId, cn.outPortIndex, cn.inPortIndex});
                schedule.incomingEdges[cn.inNodeId].push_back(
                    {id, cn.outPortIndex, cn.inPortIndex});
                ++inDegree[cn.inNodeId];
                hasIncoming.insert(cn.inNodeId);
            }
        }
    }

    // Source nodes: those with no incoming connections.
    for (auto id : nodeIds) {
        if (!hasIncoming.count(id))
            schedule.sourceNodes.push_back(id);
    }

    // Kahn's BFS to assign topological levels.
    std::queue<QtNodes::NodeId> queue;
    for (auto id : schedule.sourceNodes) {
        schedule.nodeLevels[id] = 0;
        queue.push(id);
    }

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        std::size_t currentLevel = schedule.nodeLevels[current];

        auto it = schedule.adjacency.find(current);
        if (it == schedule.adjacency.end())
            continue;

        for (auto const &edge : it->second) {
            auto targetId = edge.targetNode;

            // Track maximum level from all predecessors.
            auto [lvlIt, inserted] = schedule.nodeLevels.try_emplace(
                targetId, currentLevel + 1);
            if (!inserted && lvlIt->second < currentLevel + 1)
                lvlIt->second = currentLevel + 1;

            if (--inDegree[targetId] == 0)
                queue.push(targetId);
        }
    }

    // Determine number of levels and group nodes.
    std::size_t maxLevel = 0;
    for (auto const &[id, lvl] : schedule.nodeLevels) {
        if (lvl > maxLevel)
            maxLevel = lvl;
    }

    schedule.levels.resize(maxLevel + 1);
    for (auto const &[id, lvl] : schedule.nodeLevels)
        schedule.levels[lvl].push_back(id);

    return schedule;
}

} // namespace noddle

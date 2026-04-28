#pragma once
#include "../core/graph/CampusGraph.h"
#include "../core/graph/Algorithms.h"
#include <string>
#include <vector>

class NavigationService {
public:
    explicit NavigationService(CampusGraph& graph);

    TraversalResult runDfs(const std::string& start, bool mobilityReduced) const;
    TraversalResult runBfs(const std::string& start, bool mobilityReduced) const;
    PathResult findPathDfs(const std::string& from, const std::string& to, bool mobilityReduced) const;
    PathResult findPath(const std::string& from, const std::string& to, bool mobilityReduced) const;
    bool checkConnectivity() const;
    bool checkConnectivity(bool respectBlockedEdges) const;
    std::vector<std::vector<std::string>> getComponents() const;
    std::vector<std::vector<std::string>> getComponents(bool respectBlockedEdges) const;

private:
    CampusGraph& graph_;
};

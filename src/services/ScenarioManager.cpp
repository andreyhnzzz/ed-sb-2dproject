#include "ScenarioManager.h"
#include <algorithm>
#include <cctype>

ScenarioManager::ScenarioManager() = default;

void ScenarioManager::setMobilityReduced(bool mr) { mobility_reduced_ = mr; }
void ScenarioManager::setStudentType(StudentType st) { student_type_ = st; }

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

static bool containsAll(const std::string& haystack, const std::vector<std::string>& needles) {
    for (const auto& needle : needles) {
        if (haystack.find(needle) == std::string::npos) return false;
    }
    return true;
}

std::vector<std::string> ScenarioManager::applyProfile(const CampusGraph& graph,
                                                       const std::string& origin,
                                                       const std::string& destination) const {
    std::vector<std::string> waypoints;
    if (origin.empty() || destination.empty()) return waypoints;
    waypoints.push_back(origin);

    if (student_type_ == StudentType::NEW_STUDENT) {
        constexpr const char* mandatoryStops[] = {"biblio", "interiorcafeteria"};
        for (const char* stop : mandatoryStops) {
            if (!graph.hasNode(stop)) continue;
            if (origin == stop || destination == stop) continue;
            if (std::find(waypoints.begin(), waypoints.end(), stop) == waypoints.end()) {
                waypoints.push_back(stop);
            }
        }
    }

    if (waypoints.empty() || waypoints.back() != destination) {
        waypoints.push_back(destination);
    }
    return waypoints;
}

PathResult ScenarioManager::buildProfiledPath(const CampusGraph& graph,
                                              const std::string& origin,
                                              const std::string& destination) const {
    PathResult merged;
    const auto waypoints = applyProfile(graph, origin, destination);
    if (waypoints.size() < 2) return merged;

    merged.found = true;
    for (size_t i = 1; i < waypoints.size(); ++i) {
        const PathResult segment = Algorithms::findPath(
            graph, waypoints[i - 1], waypoints[i], mobility_reduced_, false);
        if (!segment.found || segment.path.empty()) {
            return {};
        }

        merged.total_weight += segment.total_weight;
        if (merged.path.empty()) {
            merged.path = segment.path;
        } else {
            merged.path.insert(merged.path.end(), segment.path.begin() + 1, segment.path.end());
        }
    }
    return merged;
}

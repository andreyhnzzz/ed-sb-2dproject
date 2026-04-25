#pragma once
#include "../core/graph/CampusGraph.h"
#include "../core/graph/Algorithms.h"
#include <string>
#include <vector>

enum class StudentType { NEW_STUDENT, VETERAN_STUDENT, DISABLED_STUDENT };

class ScenarioManager {
public:
    ScenarioManager();

    void setMobilityReduced(bool mr);
    void setStudentType(StudentType st);
    void setReferenceWaypoints(std::vector<std::string> referenceWaypoints);
    std::vector<std::string> applyProfile(const CampusGraph& graph,
                                          const std::string& origin,
                                          const std::string& destination) const;
    PathResult buildProfiledPath(const CampusGraph& graph,
                                 const std::string& origin,
                                 const std::string& destination) const;

    bool isMobilityReduced() const;
    StudentType getStudentType() const { return student_type_; }

private:
    bool mobility_reduced_{false};
    StudentType student_type_{StudentType::VETERAN_STUDENT};
    std::vector<std::string> reference_waypoints_;
};

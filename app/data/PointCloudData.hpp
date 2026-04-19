#pragma once

#include <QtNodes/NodeData>
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

struct Point3D {
    float x, y, z;
};

class PointCloudData : public NodeData
{
public:
    explicit PointCloudData(std::vector<Point3D> points)
        : m_points(std::move(points))
    {}

    NodeDataType type() const override
    {
        return {"pointcloud", "Point Cloud"};
    }

    std::vector<Point3D> const &points() const { return m_points; }
    std::size_t size() const { return m_points.size(); }

private:
    std::vector<Point3D> m_points;
};

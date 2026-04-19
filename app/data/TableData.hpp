#pragma once

#include <QtNodes/NodeData>
#include <vector>
#include <string>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

class TableData : public NodeData
{
public:
    TableData(std::vector<std::vector<float>> rows,
              std::vector<std::string> headers = {})
        : m_rows(std::move(rows))
        , m_headers(std::move(headers))
    {}

    NodeDataType type() const override
    {
        return {"table", "Table"};
    }

    std::vector<std::vector<float>> const &rows() const { return m_rows; }
    std::vector<std::string> const &headers() const { return m_headers; }

    std::size_t rowCount() const { return m_rows.size(); }
    std::size_t colCount() const { return m_rows.empty() ? 0 : m_rows[0].size(); }

private:
    std::vector<std::vector<float>> m_rows;
    std::vector<std::string> m_headers;
};

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include <memory>

// Minimal source node: 0 inputs, 1 output
class DummySourceModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummySource"); }
    QString name() const override { return QStringLiteral("DummySource"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::Out) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return {}; }
    QWidget *embeddedWidget() override { return nullptr; }
};

// Minimal processing node: 1 input, 1 output
class DummyProcessModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummyProcess"); }
    QString name() const override { return QStringLiteral("DummyProcess"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::In || pt == QtNodes::PortType::Out) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return {}; }
    QWidget *embeddedWidget() override { return nullptr; }
};

// Merge node: 2 inputs, 1 output
class DummyMergeModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummyMerge"); }
    QString name() const override { return QStringLiteral("DummyMerge"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        if (pt == QtNodes::PortType::In) return 2;
        if (pt == QtNodes::PortType::Out) return 1;
        return 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return {}; }
    QWidget *embeddedWidget() override { return nullptr; }
};

// Sink node: 1 input, 0 outputs
class DummySinkModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummySink"); }
    QString name() const override { return QStringLiteral("DummySink"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::In) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return {}; }
    QWidget *embeddedWidget() override { return nullptr; }
};

// ── Integration-test models ────────────────────────────────────

#include <vector>
#include <utility>
#include <mutex>

// Simple NodeData payload for integration tests
class DummyNodeData : public QtNodes::NodeData {
public:
    QtNodes::NodeDataType type() const override { return {"dummy", "Dummy"}; }
};

// Source that stores output data and can fire dataUpdated(0) on demand.
class DummyDataSourceModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummyDataSource"); }
    QString name() const override { return QStringLiteral("DummyDataSource"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::Out) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex) override {}
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return m_data; }
    QWidget *embeddedWidget() override { return nullptr; }

    Q_INVOKABLE void refreshWidgets() {}

    void emitData() {
        m_data = std::make_shared<DummyNodeData>();
        Q_EMIT dataUpdated(0);
    }

private:
    std::shared_ptr<DummyNodeData> m_data;
};

// Recorder sink: 1 input, 0 outputs. Records all setInData calls.
class DummyRecorderModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummyRecorder"); }
    QString name() const override { return QStringLiteral("DummyRecorder"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::In) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex port) override {
        std::lock_guard lock(m_mutex);
        m_received.push_back({data, port});
    }
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override { return {}; }
    QWidget *embeddedWidget() override { return nullptr; }

    Q_INVOKABLE void refreshWidgets() {}

    std::vector<std::pair<std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex>> received() const {
        std::lock_guard lock(m_mutex);
        return m_received;
    }

    void clearReceived() {
        std::lock_guard lock(m_mutex);
        m_received.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::vector<std::pair<std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex>> m_received;
};

// Processing node that records and forwards: 1 input, 1 output.
class DummyRecorderProcessModel : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    QString caption() const override { return QStringLiteral("DummyRecorderProcess"); }
    QString name() const override { return QStringLiteral("DummyRecorderProcess"); }
    unsigned int nPorts(QtNodes::PortType pt) const override {
        return (pt == QtNodes::PortType::In || pt == QtNodes::PortType::Out) ? 1 : 0;
    }
    QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override {
        return {"dummy", "Dummy"};
    }
    void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex port) override {
        std::lock_guard lock(m_mutex);
        m_received.push_back({data, port});
        m_lastData = data;
    }
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex) override {
        std::lock_guard lock(m_mutex);
        return m_lastData;
    }
    QWidget *embeddedWidget() override { return nullptr; }

    Q_INVOKABLE void refreshWidgets() {}

    std::vector<std::pair<std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex>> received() const {
        std::lock_guard lock(m_mutex);
        return m_received;
    }

    void clearReceived() {
        std::lock_guard lock(m_mutex);
        m_received.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<QtNodes::NodeData> m_lastData;
    std::vector<std::pair<std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex>> m_received;
};

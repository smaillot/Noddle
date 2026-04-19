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

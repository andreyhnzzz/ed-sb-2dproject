#include "ConnectivityTab.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

ConnectivityTab::ConnectivityTab(NavigationService& nav, const CampusGraph& graph, QWidget* parent)
    : QWidget(parent), nav_(nav), graph_(graph)
{
    auto* layout = new QVBoxLayout(this);
    auto* buttonRow = new QHBoxLayout();
    btn_check_practical_ = new QPushButton("Conectividad Practica", this);
    btn_check_theoretical_ = new QPushButton("Conexidad Teorica", this);
    buttonRow->addWidget(btn_check_practical_);
    buttonRow->addWidget(btn_check_theoretical_);
    layout->addLayout(buttonRow);
    lbl_result_ = new QLabel(this);
    lbl_result_->setWordWrap(true);
    layout->addWidget(lbl_result_);
    layout->addWidget(new QLabel("Componentes:"));
    list_components_ = new QListWidget(this);
    layout->addWidget(list_components_);

    connect(btn_check_practical_, &QPushButton::clicked,
            this, &ConnectivityTab::onCheckPracticalConnectivity);
    connect(btn_check_theoretical_, &QPushButton::clicked,
            this, &ConnectivityTab::onCheckTheoreticalConnectivity);
}

void ConnectivityTab::onCheckPracticalConnectivity() {
    runConnectivityCheck(true);
}

void ConnectivityTab::onCheckTheoreticalConnectivity() {
    runConnectivityCheck(false);
}

void ConnectivityTab::runConnectivityCheck(bool respectBlockedEdges) {
    const bool connected = nav_.checkConnectivity(respectBlockedEdges);
    const auto components = nav_.getComponents(respectBlockedEdges);
    const QString modeLabel = respectBlockedEdges ? "Conectividad practica" : "Conexidad teorica";

    if (connected) {
        lbl_result_->setText(QString("%1: el grafo ES conexo. Todos los nodos son accesibles desde cualquier punto.")
                                 .arg(modeLabel));
        lbl_result_->setStyleSheet("color: green; font-weight: bold;");
    } else {
        lbl_result_->setText(QString("%1: el grafo NO es conexo. Hay %2 componentes.")
                                 .arg(modeLabel)
                                 .arg(components.size()));
        lbl_result_->setStyleSheet("color: red; font-weight: bold;");
    }

    list_components_->clear();
    for (size_t ci = 0; ci < components.size(); ++ci) {
        const auto& comp = components[ci];
        QString names;
        for (auto& id : comp) {
            try {
                names += QString::fromStdString(graph_.getNode(id).name) + ", ";
            } catch (...) {}
        }
        if (names.endsWith(", ")) names.chop(2);
        list_components_->addItem(QString("Componente %1 (%2 nodos): %3")
                                      .arg(ci + 1).arg(comp.size()).arg(names));
    }
}

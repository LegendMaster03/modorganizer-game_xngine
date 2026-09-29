#include "xnginerecordgraph.h"

#include <QSet>

bool XngineRecordGraph::addNode(const Node& node, QStringList* diagnostics)
{
  if (node.id == 0) {
    return false;
  }

  if (m_Nodes.contains(node.id)) {
    if (diagnostics != nullptr) {
      diagnostics->push_back(QString("Duplicate record ID %1 in record graph").arg(node.id));
    }
    return false;
  }

  m_Nodes.insert(node.id, node);
  return true;
}

bool XngineRecordGraph::contains(quint32 id) const
{
  return m_Nodes.contains(id);
}

qint32 XngineRecordGraph::typeForId(quint32 id) const
{
  const auto it = m_Nodes.constFind(id);
  return (it == m_Nodes.cend()) ? -1 : it->type;
}

XngineRecordGraph::TraceResult XngineRecordGraph::traceFromParent(quint32 parentId,
                                                                  quint32 ancestorId) const
{
  TraceResult result;
  QSet<quint32> visited;
  quint32 current = parentId;

  while (current != 0) {
    if (current == ancestorId) {
      result.chain.push_back(current);
      result.reachesAncestor = true;
      return result;
    }

    if (visited.contains(current)) {
      result.chain.push_back(current);
      result.cycleDetected = true;
      return result;
    }
    visited.insert(current);
    result.chain.push_back(current);

    const auto it = m_Nodes.constFind(current);
    if (it == m_Nodes.cend()) {
      result.missingReference = true;
      result.missingId = current;
      return result;
    }

    current = it->parentId;
  }

  return result;
}

XngineRecordGraph::TraceResult XngineRecordGraph::traceToAncestor(quint32 startId,
                                                                  quint32 ancestorId) const
{
  if (startId == ancestorId && startId != 0) {
    TraceResult result;
    result.reachesAncestor = true;
    result.chain.push_back(startId);
    return result;
  }

  const auto it = m_Nodes.constFind(startId);
  if (it == m_Nodes.cend()) {
    TraceResult result;
    result.missingReference = true;
    result.missingId = startId;
    return result;
  }

  return traceFromParent(it->parentId, ancestorId);
}

XngineRecordGraph::TraceResult XngineRecordGraph::traceParentToAncestor(
    quint32 parentId, quint32 ancestorId) const
{
  return traceFromParent(parentId, ancestorId);
}

QVector<quint32> XngineRecordGraph::descendantsOf(quint32 ancestorId) const
{
  QVector<quint32> result;
  for (auto it = m_Nodes.cbegin(); it != m_Nodes.cend(); ++it) {
    if (it.key() == ancestorId) {
      continue;
    }
    if (traceFromParent(it->parentId, ancestorId).reachesAncestor) {
      result.push_back(it.key());
    }
  }
  return result;
}

QStringList XngineRecordGraph::validateLinks() const
{
  QStringList diagnostics;
  QSet<QString> seen;

  for (auto it = m_Nodes.cbegin(); it != m_Nodes.cend(); ++it) {
    const Node& node = it.value();
    if (node.parentId != 0) {
      const auto parent = m_Nodes.constFind(node.parentId);
      if (parent == m_Nodes.cend()) {
        const QString message =
            QString("Record %1 references missing parent %2").arg(node.id).arg(node.parentId);
        if (!seen.contains(message)) {
          diagnostics.push_back(message);
          seen.insert(message);
        }
      } else if (node.expectedParentType >= 0 && parent->type != node.expectedParentType) {
        const QString message =
            QString("Record %1 expects parent type %2 but parent %3 has type %4")
                .arg(node.id)
                .arg(node.expectedParentType)
                .arg(node.parentId)
                .arg(parent->type);
        if (!seen.contains(message)) {
          diagnostics.push_back(message);
          seen.insert(message);
        }
      }
    }

    QSet<quint32> visited;
    quint32 current = node.id;
    while (current != 0) {
      if (visited.contains(current)) {
        const QString message = QString("Record graph cycle detected from record %1 at %2")
                                    .arg(node.id)
                                    .arg(current);
        if (!seen.contains(message)) {
          diagnostics.push_back(message);
          seen.insert(message);
        }
        break;
      }
      visited.insert(current);
      const auto currentNode = m_Nodes.constFind(current);
      if (currentNode == m_Nodes.cend()) {
        break;
      }
      current = currentNode->parentId;
    }
  }

  return diagnostics;
}

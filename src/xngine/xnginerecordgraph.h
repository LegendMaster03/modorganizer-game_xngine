#ifndef XNGINE_RECORDGRAPH_H
#define XNGINE_RECORDGRAPH_H

#include <QHash>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

// Track candidate save record IDs and parent links independently of a parser.
// Graph traversal can be tested directly; whether extracted parent fields
// correspond to real Battlespire ownership remains a separate research claim.
class XngineRecordGraph
{
public:
  struct Node
  {
    quint32 id = 0;
    quint32 parentId = 0;
    qint32 type = -1;
    qint32 expectedParentType = -1;
  };

  // The chain includes IDs visited while looking for an ancestor; distinct
  // flags distinguish success, a cycle, and an unresolved reference.
  struct TraceResult
  {
    bool reachesAncestor = false;
    bool cycleDetected = false;
    bool missingReference = false;
    quint32 missingId = 0;
    QVector<quint32> chain;
  };

  // Duplicate or zero IDs are rejected rather than replacing a prior node.
  bool addNode(const Node& node, QStringList* diagnostics = nullptr);
  bool contains(quint32 id) const;
  qint32 typeForId(quint32 id) const;

  // traceToAncestor starts at a record, while traceParentToAncestor starts
  // directly with a parent ID, for callers already holding that relation.
  TraceResult traceToAncestor(quint32 startId, quint32 ancestorId) const;
  TraceResult traceParentToAncestor(quint32 parentId, quint32 ancestorId) const;
  QVector<quint32> descendantsOf(quint32 ancestorId) const;
  QStringList validateLinks() const;

private:
  TraceResult traceFromParent(quint32 parentId, quint32 ancestorId) const;

private:
  QHash<quint32, Node> m_Nodes;
};

#endif  // XNGINE_RECORDGRAPH_H

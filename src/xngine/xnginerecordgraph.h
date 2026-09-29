#ifndef XNGINE_RECORDGRAPH_H
#define XNGINE_RECORDGRAPH_H

#include <QHash>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

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

  struct TraceResult
  {
    bool reachesAncestor = false;
    bool cycleDetected = false;
    bool missingReference = false;
    quint32 missingId = 0;
    QVector<quint32> chain;
  };

  bool addNode(const Node& node, QStringList* diagnostics = nullptr);
  bool contains(quint32 id) const;
  qint32 typeForId(quint32 id) const;

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

#ifndef REDGUARDSSCRIPTREADER_H
#define REDGUARDSSCRIPTREADER_H

#include <QBuffer>
#include <QList>
#include <QMap>
#include <QString>

#include "redguardsscriptinstruction.h"

class RedguardsMapDatabase;
class RedguardsMapHeader;

class RedguardsScriptReader
{
public:
  RedguardsScriptReader(RedguardsMapDatabase* mapDatabase, RedguardsMapHeader* header);

  QString read();

private:
  enum class ValueMode { MAIN, LHS, RHS, PARAMETER, REFERENCE, FORMULA };
  enum class ParameterType { NORMAL, DIALOGUE, MAP, ITEM };

  void getValue(ValueMode mode);
  void getBlock(int lastPos);
  void getTaskText(int taskType);
  void getIf();
  void getLabel(const QString& prefix);
  QString addLabel(int label);
  void getFormula();
  bool getOperator();
  void getObjectName();
  void getReferenceName();
  QString readIntFlexible();

  int readByte();
  int readShort();
  int readInt();
  QString readString();

  RedguardsMapDatabase* mMapDatabase;
  RedguardsMapHeader* mHeader;
  QBuffer mInput;
  QList<RedguardsScriptInstruction> mInstructions;
  RedguardsScriptInstruction* mCurrentInstruction = nullptr;
  QList<int> mLabels;
  int mPos = 0;
  int mIndentLevel = 0;
  QString mCurrentTask;
  int mTaskParamNum = 0;

  static const QStringList COMPARISONS;
  static const QStringList OPERATORS;
  static const QStringList OBJECT_NAMES;
  static const QMap<QString, ParameterType> PARAMETER_TYPES;
};

#endif  // REDGUARDSSCRIPTREADER_H

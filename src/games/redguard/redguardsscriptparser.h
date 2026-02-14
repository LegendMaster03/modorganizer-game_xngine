#ifndef REDGUARDSSCRIPTPARSER_H
#define REDGUARDSSCRIPTPARSER_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QString>

class RedguardsMapDatabase;
class RedguardsParsedMapHeader;

class RedguardsScriptParser
{
public:
  RedguardsScriptParser(RedguardsMapDatabase* mapDatabase, const QString& script);

  QList<RedguardsParsedMapHeader> parse();
  int totalScriptLength() const { return mTotalScriptLength; }

private:
  enum class ValueMode { MAIN, LHS, RHS, PARAMETER, REFERENCE, FORMULA };

  void initReverseMaps();
  void preParse(const QString& script);
  void parseBlock();
  void parseValue(const QString& value, ValueMode mode);
  void parseTask(const QString& line, bool writeBytes);
  void parseIf(const QStringList& lineSplit);
  int parseLabel(const QString& label, bool writeBytes, bool savePos);
  void parseFormula(const QString& line);
  bool parseOperator(const QString& line);
  void parseObjectName(const QString& line);
  void parseReferenceName(const QString& line);

  void addString(const QString& str);
  void addInt(int num, bool littleEndian);
  void addShort(int num, bool littleEndian);
  void addByte(int num);

  RedguardsMapDatabase* mMapDatabase;
  QStringList mLines;
  int mLineIndex = 0;

  QList<RedguardsParsedMapHeader> mParsedHeaders;
  RedguardsParsedMapHeader* mCurrentHeader = nullptr;
  QList<unsigned char> mCurrentScriptBytes;
  QMap<int, QList<int>> mLabels;
  int mPos = 0;
  QString mCurrentTask;
  int mTotalScriptLength = 0;

  static const QMap<QString, int> OBJECT_NAME_VALUES;
  static const QMap<QString, int> OPERATOR_VALUES;
  static const QMap<QString, int> COMPARISON_VALUES;
  static const QSet<QString> DIALOGUE_FUNCTIONS;

  QMap<QString, int> mMapIds;
  QMap<QString, int> mFunctionIds;
  QMap<QString, int> mFlagIds;
  QMap<QString, int> mItemIds;
  QMap<QString, int> mReferences;
  QMap<QString, int> mAttributes;
};

#endif  // REDGUARDSSCRIPTPARSER_H

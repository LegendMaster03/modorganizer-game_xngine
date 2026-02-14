#include "redguardsscriptparser.h"

#include "redguardsmapdatabase.h"
#include "redguardsmapfile.h"
#include "redguardsparsedmapheader.h"
#include "redguardssoupfunction.h"
#include "redguardssoupflag.h"
#include "redguardsitem.h"
#include "redguardsutils.h"

#include <QByteArray>
#include <QRegularExpression>

const QMap<QString, int> RedguardsScriptParser::OBJECT_NAME_VALUES = {
    {"Me", 0}, {"Player", 1}, {"Camera", 2}
};

const QMap<QString, int> RedguardsScriptParser::OPERATOR_VALUES = {
    {"", 0}, {"+", 1}, {"-", 2}, {"/", 3}, {"*", 4}, {"<<", 5}, {">>", 6},
    {"&", 7}, {"|", 8}, {"^", 9}, {"++", 10}, {"--", 11}
};

const QMap<QString, int> RedguardsScriptParser::COMPARISON_VALUES = {
    {"=", 0}, {"!=", 1}, {"<", 2}, {">", 3}, {"<=", 4}, {">=", 5}
};

const QSet<QString> RedguardsScriptParser::DIALOGUE_FUNCTIONS = {
    "ACTIVATE", "AddLog", "AmbientRtx", "menuAddItem", "RTX", "rtxAnim",
    "RTXp", "RTXpAnim", "TorchActivate"};

RedguardsScriptParser::RedguardsScriptParser(RedguardsMapDatabase* mapDatabase,
                                             const QString& script)
    : mMapDatabase(mapDatabase)
{
  initReverseMaps();
  preParse(script);
}

void RedguardsScriptParser::initReverseMaps()
{
  for (auto* mapFile : mMapDatabase->mapFiles()) {
    if (!mapFile->ids().isEmpty()) {
      mMapIds.insert("<" + mapFile->name() + ">", mapFile->ids().first());
    }
  }

  for (int i = 0; i < mMapDatabase->functions().size(); ++i) {
    mFunctionIds.insert(mMapDatabase->functions()[i]->name(), i);
  }

  for (int i = 0; i < mMapDatabase->flags().size(); ++i) {
    mFlagIds.insert(mMapDatabase->flags()[i]->name(), i);
  }

  for (int i = 0; i < mMapDatabase->items().size(); ++i) {
    mItemIds.insert("<" + mMapDatabase->items()[i]->name() + ">", i);
  }

  for (int i = 0; i < mMapDatabase->references().size(); ++i) {
    mReferences.insert(mMapDatabase->references()[i], i);
  }

  for (int i = 0; i < mMapDatabase->attributes().size(); ++i) {
    mAttributes.insert(mMapDatabase->attributes()[i], i);
  }
}

void RedguardsScriptParser::preParse(const QString& script)
{
  QString text = script;
  text.replace(QRegularExpression(", +"), ",");
  text.replace(QRegularExpression(" +//.*"), "");
  text.replace(QRegularExpression("(\\+\\+|--)"), " \\1");

  QRegularExpression pattern("<[a-zA-Z0-9 ()']+>");
  QRegularExpressionMatchIterator it = pattern.globalMatch(text);
  QString result;
  int lastPos = 0;
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    result.append(text.mid(lastPos, match.capturedStart() - lastPos));
    const QString token = match.captured(0);
    if (mMapIds.contains(token)) {
      result.append(QString::number(mMapIds.value(token)));
    } else if (mItemIds.contains(token)) {
      result.append(QString::number(mItemIds.value(token)));
    } else {
      result.append(token);
    }
    lastPos = match.capturedEnd();
  }
  result.append(text.mid(lastPos));

  mLines = result.split('\n');
  mLineIndex = 0;
}

QList<RedguardsParsedMapHeader> RedguardsScriptParser::parse()
{
  for (int i = 0; i < 3 && mLineIndex < mLines.size(); ++i) {
    mLineIndex++;
  }

  QByteArray attributeBytes(256, 0);
  while (mLineIndex < mLines.size()) {
    QString line = mLines[mLineIndex++].trimmed();
    if (!line.isEmpty() && !line.startsWith("var")) {
      if (line.contains('=')) {
        QStringList split = line.split(QRegularExpression(" *= *"));
        if (split.size() == 2 && mAttributes.contains(split[0])) {
          int index = mAttributes.value(split[0]);
          attributeBytes[index] = static_cast<char>(split[1].toInt());
        }
      } else {
        QStringList split = line.split(QRegularExpression(" +"));
        mParsedHeaders.append(RedguardsParsedMapHeader(split[0]));
        mCurrentHeader = &mParsedHeaders.last();
        mCurrentHeader->setAttributeBytes(attributeBytes);
        attributeBytes = QByteArray(256, 0);

        if (split.size() >= 5) {
          int label = parseLabel(split[4].left(split[4].length() - 1), false, false);
          mCurrentHeader->setScriptPC(label);
        }

        parseBlock();

        for (auto it = mLabels.begin(); it != mLabels.end(); ++it) {
          const QList<int>& positions = it.value();
          QByteArray bytes = RedguardsUtils::shortToByteArray(static_cast<int16_t>(positions.first()), true);
          for (int i = 1; i < positions.size(); ++i) {
            int position = positions[i];
            mCurrentScriptBytes[position] = bytes[0];
            mCurrentScriptBytes[position + 1] = bytes[1];
          }
        }

        QByteArray scriptBytes;
        scriptBytes.resize(mCurrentScriptBytes.size());
        for (int i = 0; i < mCurrentScriptBytes.size(); ++i) {
          scriptBytes[i] = static_cast<char>(mCurrentScriptBytes[i]);
        }
        mCurrentHeader->setScriptBytes(scriptBytes);
        mCurrentHeader->setScriptDataOffset(mTotalScriptLength);
        mTotalScriptLength += mCurrentScriptBytes.size();

        mLabels.clear();
        mCurrentScriptBytes.clear();
        mPos = 0;
      }
    }
  }

  return mParsedHeaders;
}

void RedguardsScriptParser::parseBlock()
{
  if (mLineIndex < mLines.size()) {
    mLineIndex++;
  }
  QString line = (mLineIndex < mLines.size()) ? mLines[mLineIndex++].trimmed() : QString();
  while (line != "}" && mLineIndex <= mLines.size()) {
    if (!line.isEmpty()) {
      parseValue(line, ValueMode::MAIN);
    }
    if (mLineIndex >= mLines.size()) {
      break;
    }
    line = mLines[mLineIndex++].trimmed();
  }
}

void RedguardsScriptParser::parseValue(const QString& value, ValueMode mode)
{
  QStringList valueSplit = value.split(QRegularExpression(" +"));

  if (value.startsWith('#')) {
    parseLabel(value, false, true);
  } else if (value.contains('(') && valueSplit[0].contains('(')) {
    parseTask(value, true);
  } else if (valueSplit.size() > 1 && valueSplit[1] == "<ScriptRv>") {
    addByte(30);
    addByte(1);
    parseBlock();
  } else if (valueSplit[0] == "if") {
    addByte(3);
    parseIf(valueSplit);
  } else if (valueSplit[0] == "Goto") {
    addByte(4);
    if (valueSplit.size() == 1) {
      addInt(0, false);
    } else {
      parseLabel(valueSplit[1], true, true);
    }
  } else if (valueSplit[0] == "End") {
    addByte(5);
    if (valueSplit.size() == 1) {
      addInt(0, false);
    } else {
      parseLabel(valueSplit[1], true, true);
    }
  } else if (mFlagIds.contains(valueSplit[0])) {
    addByte(6);
    addShort(mFlagIds.value(valueSplit[0]), true);
    if (mode == ValueMode::MAIN) {
      parseFormula(value.mid(valueSplit[0].length()).trimmed());
    } else if (mode == ValueMode::LHS || mode == ValueMode::RHS) {
      parseOperator(valueSplit.size() > 1 ? valueSplit[1] : "");
    } else if (mode == ValueMode::PARAMETER) {
      addShort(0, false);
    }
  } else if (value.startsWith('"') && value.endsWith('"') &&
             !DIALOGUE_FUNCTIONS.contains(mCurrentTask)) {
    addByte(21);
    QString str = value.mid(1, value.length() - 2);
    addInt(mCurrentHeader->addString(str), true);
  } else if (value.startsWith('"') || QRegularExpression("^-?\\d+$").match(value).hasMatch()) {
    if (mode == ValueMode::PARAMETER || mode == ValueMode::RHS) {
      if (mCurrentTask == "TestGlobalFlag" || mCurrentTask == "SetGlobalFlag" ||
          mCurrentTask == "ResetGlobalFlag") {
        addByte(22);
      } else {
        addByte(7);
      }
      if (value.startsWith('"')) {
        addString(value.mid(1, value.length() - 2));
      } else {
        addInt(value.toInt(), true);
      }
    } else {
      addByte(7);
      addInt(value.toInt(), true);
    }
  } else if (value.startsWith("var")) {
    addByte(10);
    addByte(valueSplit[0].mid(3).toInt());
    if (mode == ValueMode::MAIN) {
      parseFormula(value.mid(valueSplit[0].length()).trimmed());
    } else if (mode == ValueMode::LHS || mode == ValueMode::RHS) {
      parseOperator(value.mid(valueSplit[0].length()).trimmed());
    } else if (mode == ValueMode::PARAMETER) {
      addByte(0);
      addByte(0);
      addByte(0);
    }
  } else if (valueSplit[0] == "Gosub") {
    addByte(17);
    parseLabel(valueSplit[1], true, true);
  } else if (value == "Return") {
    addByte(18);
  } else if (value == "Endint") {
    addByte(19);
  } else if (value.startsWith("<Anchor>=")) {
    QStringList equalSplit = value.split('=');
    addByte(equalSplit[1].toInt());
  } else if (value.startsWith("<TaskPause")) {
    addByte(27);
    QStringList parenSplit = value.split('(');
    QString numStr = parenSplit[1].left(parenSplit[1].length() - 2);
    parseLabel(numStr, true, true);
  } else if (value.contains('.')) {
    QStringList dotSplit = value.split('.');
    int parenIndex = dotSplit[1].indexOf('(');
    if (parenIndex > 0) {
      QString name = dotSplit[1].left(parenIndex);
      if (name.startsWith('@')) {
        name = name.mid(1);
      }
      if (mFunctionIds.contains(name)) {
        if (mMapDatabase->functions()[mFunctionIds.value(name)]->type() == "function") {
          addByte(26);
        } else {
          addByte(25);
        }
        parseObjectName(dotSplit[0]);
        if (mode == ValueMode::MAIN) {
          parseValue(value.mid(dotSplit[0].length() + 1), ValueMode::REFERENCE);
        } else {
          parseTask(value.mid(dotSplit[0].length() + 1), false);
        }
      }
    } else {
      addByte(20);
      parseObjectName(dotSplit[0]);
      parseReferenceName(valueSplit[0].mid(dotSplit[0].length() + 1));
      if (mode == ValueMode::MAIN) {
        parseFormula(value.mid(valueSplit[0].length()).trimmed());
      } else if (mode == ValueMode::LHS) {
        parseOperator(value.mid(valueSplit[0].length()).trimmed());
      }
    }
  }
}

void RedguardsScriptParser::parseTask(const QString& line, bool writeBytes)
{
  QStringList split = line.split('(');
  mCurrentTask = split[0];
  bool multitask = false;
  if (mCurrentTask.startsWith('@')) {
    mCurrentTask = mCurrentTask.mid(1);
    multitask = true;
  }

  int functionId = mFunctionIds.value(mCurrentTask, 0);
  auto* function = mMapDatabase->functions().value(functionId);

  if (writeBytes) {
    if (multitask) {
      addByte(1);
    } else {
      addByte((function && function->type() == "task") ? 0 : 2);
    }
  }
  addShort(functionId, true);

  int paramNum = function ? function->paramCount() : 0;
  addByte(paramNum);
  if (paramNum > 0) {
    QString params = split[1].left(split[1].length() - 1);
    QStringList paramList = params.split(',');
    for (int i = 0; i < paramNum && i < paramList.size(); ++i) {
      parseValue(paramList[i], ValueMode::PARAMETER);
    }
  }

  mCurrentTask.clear();
}

void RedguardsScriptParser::parseIf(const QStringList& lineSplit)
{
  int conjunction;
  int counter = 1;
  do {
    conjunction = 0;
    parseValue(lineSplit[counter], ValueMode::LHS);
    counter++;
    addByte(COMPARISON_VALUES.value(lineSplit[counter]));
    counter++;
    parseValue(lineSplit[counter], ValueMode::RHS);
    counter++;
    if (lineSplit.size() > counter) {
      if (lineSplit[counter] == "and") {
        conjunction = 1;
        addByte(1);
      } else {
        conjunction = 2;
        addByte(2);
      }
      counter++;
    } else {
      addByte(0);
    }
  } while (conjunction != 0);

  int arrayPos = mPos;
  addInt(0, false);
  parseBlock();
  QByteArray lengthBytes = RedguardsUtils::intToByteArray(mPos, true);
  for (int i = 0; i < lengthBytes.size(); ++i) {
    mCurrentScriptBytes[arrayPos + i] = static_cast<unsigned char>(lengthBytes[i]);
  }
}

int RedguardsScriptParser::parseLabel(const QString& label, bool writeBytes, bool savePos)
{
  int endIndex = label.indexOf(':');
  QString value = label.mid(1, (endIndex > 0) ? endIndex - 1 : label.length() - 1);
  bool ok = false;
  int labelNum = value.toInt(&ok, 16);
  if (!ok) {
    return 0;
  }

  if (!mLabels.contains(labelNum)) {
    mLabels.insert(labelNum, QList<int>());
  }
  if (savePos) {
    if (endIndex > 0) {
      mLabels[labelNum].prepend(mPos);
    } else {
      mLabels[labelNum].append(mPos);
    }
  }
  if (writeBytes) {
    addInt(0, false);
  }

  return labelNum;
}

void RedguardsScriptParser::parseFormula(const QString& line)
{
  QStringList lineSplit = line.split(QRegularExpression(" +"));
  int counter = 1;
  do {
    parseValue(lineSplit[counter++], ValueMode::FORMULA);
  } while (parseOperator(counter < lineSplit.size() ? lineSplit[counter++] : ""));
}

bool RedguardsScriptParser::parseOperator(const QString& line)
{
  bool wantsValue = false;
  int op = OPERATOR_VALUES.value(line, 0);
  addByte(op);
  if (op == 10 || op == 11) {
    addByte(0);
  }
  if (op > 0 && op < 10) {
    wantsValue = true;
  }
  return wantsValue;
}

void RedguardsScriptParser::parseObjectName(const QString& line)
{
  if (OBJECT_NAME_VALUES.contains(line)) {
    addByte(OBJECT_NAME_VALUES.value(line));
    addByte(0);
  } else if (QRegularExpression("^[a-z0-9._]+$").match(line).hasMatch()) {
    addByte(4);
    addByte(mCurrentHeader->addString(line));
  } else {
    addByte(10);
  }
}

void RedguardsScriptParser::parseReferenceName(const QString& line)
{
  addShort(mReferences.value(line), true);
}

void RedguardsScriptParser::addString(const QString& str)
{
  mPos += 4;
  const QByteArray bytes = str.toLatin1();
  for (char b : bytes) {
    mCurrentScriptBytes.append(static_cast<unsigned char>(b));
  }
}

void RedguardsScriptParser::addInt(int num, bool littleEndian)
{
  mPos += 4;
  QByteArray bytes = RedguardsUtils::intToByteArray(num, littleEndian);
  for (char b : bytes) {
    mCurrentScriptBytes.append(static_cast<unsigned char>(b));
  }
}

void RedguardsScriptParser::addShort(int num, bool littleEndian)
{
  mPos += 2;
  QByteArray bytes = RedguardsUtils::shortToByteArray(static_cast<int16_t>(num), littleEndian);
  for (char b : bytes) {
    mCurrentScriptBytes.append(static_cast<unsigned char>(b));
  }
}

void RedguardsScriptParser::addByte(int num)
{
  mPos += 1;
  mCurrentScriptBytes.append(static_cast<unsigned char>(num));
}

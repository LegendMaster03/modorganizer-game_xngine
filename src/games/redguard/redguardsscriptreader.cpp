#include "redguardsscriptreader.h"

#include "redguardsmapdatabase.h"
#include "redguardsmapheader.h"
#include "redguardsscriptinstruction.h"
#include "redguardssoupfunction.h"
#include "redguardssoupflag.h"
#include "redguardsitem.h"
#include "redguardsmapfile.h"
#include "redguardsutils.h"

#include <QByteArray>

const QStringList RedguardsScriptReader::COMPARISONS = {
    " = ", " != ", " < ", " > ", " <= ", " >= "
};

const QStringList RedguardsScriptReader::OPERATORS = {
    "", " + ", " - ", " / ", " * ", " << ", " >> ", " & ", " | ", " ^ ", "++", "--"
};

const QStringList RedguardsScriptReader::OBJECT_NAMES = {"Me", "Player", "Camera"};

const QMap<QString, RedguardsScriptReader::ParameterType>
    RedguardsScriptReader::PARAMETER_TYPES = {
        {"ACTIVATE0", ParameterType::DIALOGUE},
        {"AddLog0", ParameterType::DIALOGUE},
        {"AmbientRtx0", ParameterType::DIALOGUE},
        {"menuAddItem0", ParameterType::DIALOGUE},
        {"RTX0", ParameterType::DIALOGUE},
        {"rtxAnim0", ParameterType::DIALOGUE},
        {"RTXp0", ParameterType::DIALOGUE},
        {"RTXpAnim0", ParameterType::DIALOGUE},
        {"TorchActivate0", ParameterType::DIALOGUE},
        {"LoadWorld0", ParameterType::MAP},
        {"ActiveItem9", ParameterType::ITEM},
        {"AddItem0", ParameterType::ITEM},
        {"DropItem0", ParameterType::ITEM},
        {"HandItem0", ParameterType::ITEM},
        {"HaveItem0", ParameterType::ITEM},
        {"SelectItem0", ParameterType::ITEM},
        {"ShowItem0", ParameterType::ITEM},
        {"ShowItemNoRtx0", ParameterType::ITEM}};

RedguardsScriptReader::RedguardsScriptReader(RedguardsMapDatabase* mapDatabase,
                                             RedguardsMapHeader* header)
    : mMapDatabase(mapDatabase), mHeader(header)
{
  mInput.setData(header->scriptBytes());
  mInput.open(QIODevice::ReadOnly);
}

QString RedguardsScriptReader::read()
{
  getBlock(mHeader->scriptLength());

  QString script;
  script.append(mHeader->name());
  if (mHeader->scriptPC() > 0) {
    script.append(" (Execution starts at ");
    script.append(addLabel(mHeader->scriptPC()));
    script.append(")");
  }
  script.append("\n{\n");

  mIndentLevel = 1;
  for (const auto& instruction : mInstructions) {
    instruction.build(script, mIndentLevel, mLabels.contains(instruction.address()));
    mIndentLevel = instruction.indentLevel();
  }
  script.append("}");

  return script;
}

void RedguardsScriptReader::getValue(ValueMode mode)
{
  int valueType = readByte();
  switch (valueType) {
  case 0:
  case 1:
  case 2:
    getTaskText(valueType);
    break;
  case 3:
    getIf();
    break;
  case 4:
    getLabel("Goto");
    break;
  case 5:
    getLabel("End");
    break;
  case 6: {
    int flagIndex = readShort();
    auto* flag = mMapDatabase->flags().value(flagIndex);
    if (flag) {
      mCurrentInstruction->appendText(flag->name());
      if (mode == ValueMode::MAIN) {
        getFormula();
        mCurrentInstruction->setComment(flag->comment());
      } else if (mode == ValueMode::LHS || mode == ValueMode::RHS) {
        getOperator();
      } else if (mode == ValueMode::PARAMETER) {
        readShort();
      }
    }
    break;
  }
  case 7:
  case 22:
    if (mode == ValueMode::PARAMETER || mode == ValueMode::RHS) {
      mCurrentInstruction->appendText(readIntFlexible());
    } else {
      mCurrentInstruction->appendText(QString::number(readInt()));
    }
    break;
  case 10: {
    int varIndex = readByte();
    mCurrentInstruction->appendText("var")->appendText(QString::number(varIndex));
    if (mode == ValueMode::MAIN) {
      getFormula();
    } else if (mode == ValueMode::LHS || mode == ValueMode::RHS) {
      getOperator();
    } else if (mode == ValueMode::PARAMETER) {
      readByte();
      readByte();
      readByte();
    }
    break;
  }
  case 15:
  case 16:
    getObjectName();
    getReferenceName();
    mCurrentInstruction->appendText((valueType == 15) ? "++" : "--");
    break;
  case 17:
    mCurrentInstruction->appendText("Gosub " + addLabel(readInt()));
    break;
  case 18:
    mCurrentInstruction->appendText("Return");
    break;
  case 19:
    mCurrentInstruction->appendText("Endint");
    break;
  case 20:
    getObjectName();
    getReferenceName();
    if (mode == ValueMode::MAIN) {
      getFormula();
    } else if (mode == ValueMode::LHS) {
      getOperator();
    }
    break;
  case 21: {
    int stringIndex = readInt();
    if (stringIndex >= 0 && stringIndex < mHeader->strings().size()) {
      mCurrentInstruction->appendText("\"");
      mCurrentInstruction->appendText(mHeader->strings()[stringIndex]);
      mCurrentInstruction->appendText("\"");
    }
    break;
  }
  case 23: {
    int anchor = readByte();
    mCurrentInstruction->appendText("<Anchor>=" + QString::number(anchor));
    break;
  }
  case 25:
  case 26:
    getObjectName();
    if (mode == ValueMode::MAIN) {
      getValue(ValueMode::REFERENCE);
    } else {
      getTaskText(26);
    }
    break;
  case 27: {
    int taskNum = readInt();
    QString label = addLabel(taskNum);
    mCurrentInstruction->appendText("<TaskPause(" + label + ")>");
    break;
  }
  case 30: {
    int value = readByte();
    mCurrentInstruction->appendText("if <ScriptRv> = " + QString::number(value));
    getBlock(mPos + 4);
    break;
  }
  default:
    break;
  }
}

void RedguardsScriptReader::getBlock(int lastPos)
{
  mIndentLevel++;
  while (mPos < lastPos) {
    mInstructions.append(RedguardsScriptInstruction(mPos, mIndentLevel));
    mCurrentInstruction = &mInstructions.last();
    getValue(ValueMode::MAIN);
  }
  mIndentLevel--;
}

void RedguardsScriptReader::getTaskText(int taskType)
{
  int id = readShort();
  if (taskType == 1) {
    mCurrentInstruction->appendText("@");
  }
  auto* function = mMapDatabase->functions().value(id);
  if (!function) {
    return;
  }

  mCurrentTask = function->name();
  mCurrentInstruction->appendText(mCurrentTask + "(");
  int numParams = (id == 0) ? 0 : readByte();
  if (numParams > 0) {
    for (int i = 0; i < numParams; ++i) {
      if (i != 0) {
        mCurrentInstruction->appendText(", ");
      }
      mTaskParamNum = i;
      getValue(ValueMode::PARAMETER);
    }
    mTaskParamNum = 9;
  }
  mCurrentInstruction->appendText(")");
}

void RedguardsScriptReader::getIf()
{
  mCurrentInstruction->appendText("if ");
  int conjunction;
  do {
    getValue(ValueMode::LHS);
    int comparison = readByte();
    if (comparison >= 0 && comparison < COMPARISONS.size()) {
      mCurrentInstruction->appendText(COMPARISONS[comparison]);
    }
    getValue(ValueMode::RHS);
    conjunction = readByte();
    if (conjunction > 0) {
      mCurrentInstruction->appendText((conjunction == 1) ? " and " : " or ");
    }
  } while (conjunction != 0);

  int end = readInt();
  getBlock(end);
}

void RedguardsScriptReader::getLabel(const QString& prefix)
{
  int label = readInt();
  mCurrentInstruction->appendText(prefix);
  if (label != 0) {
    mCurrentInstruction->appendText(" " + addLabel(label));
  }
}

QString RedguardsScriptReader::addLabel(int label)
{
  mLabels.append(label);
  return RedguardsScriptInstruction::makeLabel(label);
}

void RedguardsScriptReader::getFormula()
{
  mCurrentInstruction->appendText(" = ");
  do {
    getValue(ValueMode::FORMULA);
  } while (getOperator());
}

bool RedguardsScriptReader::getOperator()
{
  int op = readByte();
  if (op == 10 || op == 11) {
    mCurrentInstruction->appendText(OPERATORS[op]);
    readByte();
    return false;
  }
  bool wantsValue = false;
  if (op > 0 && op < 10) {
    mCurrentInstruction->appendText(OPERATORS[op]);
    wantsValue = true;
  }
  return wantsValue;
}

void RedguardsScriptReader::getObjectName()
{
  int paramValue = readByte();
  if (paramValue >= 0 && paramValue < OBJECT_NAMES.size()) {
    mCurrentInstruction->appendText(OBJECT_NAMES[paramValue]);
    readByte();
  } else if (paramValue == 4) {
    mCurrentInstruction->appendText(mHeader->strings().value(readByte()));
  } else if (paramValue == 10) {
    int idx = readByte();
    mCurrentInstruction->appendText(QString::number(mHeader->variables().value(idx)));
  }
  mCurrentInstruction->appendText(".");
}

void RedguardsScriptReader::getReferenceName()
{
  int offset = readShort() & 0xff;
  mCurrentInstruction->appendText(mMapDatabase->references().value(offset));
}

QString RedguardsScriptReader::readIntFlexible()
{
  ParameterType paramType = PARAMETER_TYPES.value(mCurrentTask + QString::number(mTaskParamNum),
                                                  ParameterType::NORMAL);
  switch (paramType) {
  case ParameterType::DIALOGUE: {
    QString text = readString();
    QString subtitle = mMapDatabase->rtxEntry(text);
    if (!subtitle.isEmpty()) {
      mCurrentInstruction->setComment("Dlg " + text + " = " + subtitle);
    }
    mCurrentTask.clear();
    return '"' + text + '"';
  }
  case ParameterType::ITEM: {
    int itemId = readInt();
    auto* item = mMapDatabase->items().value(itemId);
    mCurrentTask.clear();
    return item ? "<" + item->name() + ">" : QString::number(itemId);
  }
  case ParameterType::MAP: {
    int mapId = readInt();
    auto* mapFile = mMapDatabase->mapFileFromId(mapId);
    mCurrentTask.clear();
    return mapFile ? "<" + mapFile->name() + ">" : QString::number(mapId);
  }
  default:
    break;
  }

  return QString::number(readInt());
}

int RedguardsScriptReader::readByte()
{
  mPos += 1;
  return RedguardsUtils::readUnsignedByte(&mInput);
}

int RedguardsScriptReader::readShort()
{
  mPos += 2;
  return RedguardsUtils::readLittleEndianShort(&mInput);
}

int RedguardsScriptReader::readInt()
{
  mPos += 4;
  return RedguardsUtils::readLittleEndianInt(&mInput);
}

QString RedguardsScriptReader::readString()
{
  mPos += 4;
  return RedguardsUtils::readString(&mInput, 4);
}

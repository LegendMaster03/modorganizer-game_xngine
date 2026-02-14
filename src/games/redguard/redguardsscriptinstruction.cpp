#include "redguardsscriptinstruction.h"

const QString RedguardsScriptInstruction::INDENT = "  ";

RedguardsScriptInstruction::RedguardsScriptInstruction(int address, int indentLevel)
    : mAddress(address), mIndentLevel(indentLevel)
{
}

void RedguardsScriptInstruction::build(QString& output, int previousIndentLevel,
                                       bool addLabel) const
{
  if (mIndentLevel > previousIndentLevel) {
    output.append(INDENT.repeated(previousIndentLevel));
    output.append("{\n");
  } else {
    int level = previousIndentLevel;
    while (mIndentLevel < level) {
      output.append(INDENT.repeated(level - 1));
      output.append("}\n");
      --level;
    }
  }

  if (addLabel) {
    output.append("\n");
    output.append(INDENT.repeated(mIndentLevel));
    output.append(makeLabel(mAddress));
    output.append(":\n");
  }

  output.append(INDENT.repeated(mIndentLevel));
  output.append(mText);
  if (!mComment.isEmpty()) {
    output.append(" // ");
    output.append(mComment);
  }
  output.append("\n");
}

QString RedguardsScriptInstruction::makeLabel(int address)
{
  return QString("#%1").arg(address, 2, 16, QLatin1Char('0')).toUpper();
}

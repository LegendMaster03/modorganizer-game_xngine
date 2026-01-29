#include "ScriptInstruction.h"

const QString ScriptInstruction::INDENT = "  ";

ScriptInstruction::ScriptInstruction(int address, int indentLevel)
    : mAddress(address), mIndentLevel(indentLevel)
{
}

void ScriptInstruction::build(QString& output, int previousIndentLevel, bool addLabel) const
{
  // Handle indent changes
  if (mIndentLevel > previousIndentLevel) {
    for (int i = 0; i < previousIndentLevel; ++i) {
      output.append(INDENT);
    }
    output.append("{\n");
  } else {
    while (mIndentLevel < previousIndentLevel) {
      previousIndentLevel--;
      for (int i = 0; i < previousIndentLevel; ++i) {
        output.append(INDENT);
      }
      output.append("}\n");
    }
  }

  // Add label if needed
  if (addLabel) {
    output.append("\n");
    for (int i = 0; i < mIndentLevel; ++i) {
      output.append(INDENT);
    }
    output.append(makeLabel(mAddress)).append(":\n");
  }

  // Add instruction text with indentation
  for (int i = 0; i < mIndentLevel; ++i) {
    output.append(INDENT);
  }
  output.append(mText);

  // Add comment if present
  if (!mComment.isEmpty()) {
    output.append(" // ").append(mComment);
  }
  output.append("\n");
}

QString ScriptInstruction::makeLabel(int address)
{
  return QString("#%1").arg(address, 2, 16, QChar('0')).toUpper();
}

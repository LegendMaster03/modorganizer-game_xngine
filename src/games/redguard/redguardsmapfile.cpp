#include "redguardsmapfile.h"

#include "redguardsmapchanges.h"
#include "redguardsmapdatabase.h"
#include "redguardsmapheader.h"
#include "redguardsparsedmapheader.h"
#include "redguardsscriptparser.h"
#include "redguardsscriptreader.h"
#include "redguardsutils.h"

#include <QDataStream>
#include <QFile>
#include <QSet>

RedguardsMapFile::RedguardsMapFile(RedguardsMapDatabase* mapDatabase, const QString& name)
    : mMapDatabase(mapDatabase), mName(name), mFullName("MAPS/" + name + ".RGM")
{
}

RedguardsMapFile::~RedguardsMapFile()
{
  qDeleteAll(mMapHeaders);
}

bool RedguardsMapFile::readMap(const QString& filePath)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }

  mRecords.clear();
  qDeleteAll(mMapHeaders);
  mMapHeaders.clear();

  QDataStream in(&file);
  in.setByteOrder(QDataStream::BigEndian);

  while (!in.atEnd()) {
    QByteArray nameBytes(4, 0);
    if (in.readRawData(nameBytes.data(), 4) != 4) {
      break;
    }
    QString section = QString::fromLatin1(nameBytes);
    if (section == "END ") {
      break;
    }

    qint32 length = 0;
    in >> length;
    if (length < 0) {
      break;
    }

    QByteArray data(length, 0);
    if (in.readRawData(data.data(), length) != length) {
      break;
    }
    mRecords.insert(section, data);
  }

  file.close();
  parseMapHeaders();
  return true;
}

bool RedguardsMapFile::writeMap(const QString& filePath, const QString& script)
{
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  RedguardsScriptParser parser(mMapDatabase, script);
  QList<RedguardsParsedMapHeader> parsedHeaders = parser.parse();

  QDataStream out(&file);
  out.setByteOrder(QDataStream::BigEndian);

  // RAHD
  out.writeRawData("RAHD", 4);
  const int rahdLength = 8 + parsedHeaders.size() * 165;
  out << static_cast<qint32>(rahdLength);
  QByteArray rahdData(8, 0);
  QByteArray headerCountBytes = RedguardsUtils::intToByteArray(parsedHeaders.size(), true);
  rahdData.replace(0, 4, headerCountBytes);
  rahdData[4] = 27;
  rahdData[5] = static_cast<char>(128);
  rahdData[6] = 55;
  rahdData[7] = 0;
  out.writeRawData(rahdData.data(), rahdData.size());

  for (int i = 0; i < mMapHeaders.size() && i < parsedHeaders.size(); ++i) {
    RedguardsParsedMapHeader& parsedHeader = parsedHeaders[i];
    QByteArray data = mMapHeaders[i]->data();

    QByteArray scriptLengthBytes = RedguardsUtils::intToByteArray(parsedHeader.scriptBytes().size(), true);
    data.replace(77, 4, scriptLengthBytes);

    int dataOffset = mScriptDataOffset + parsedHeader.scriptDataOffset();
    QByteArray scriptDataOffsetBytes = RedguardsUtils::intToByteArray(dataOffset, true);
    data.replace(81, 4, scriptDataOffsetBytes);

    QByteArray scriptPCBytes = RedguardsUtils::intToByteArray(parsedHeader.scriptPC(), true);
    data.replace(85, 4, scriptPCBytes);

    out.writeRawData(data.data(), data.size());
  }

  // RAFS
  out.writeRawData("RAFS", 4);
  out << static_cast<qint32>(1);
  out.writeRawData("\0", 1);

  // RAST
  QSet<QString> stringSet;
  QStringList strings;
  for (const auto& header : parsedHeaders) {
    for (const QString& str : header.strings()) {
      if (!stringSet.contains(str)) {
        stringSet.insert(str);
        strings.append(str);
      }
    }
  }

  QByteArray rastBytes;
  for (const QString& str : strings) {
    rastBytes.append(str.toLatin1());
    rastBytes.append('\0');
  }
  out.writeRawData("RAST", 4);
  out << static_cast<qint32>(rastBytes.size());
  out.writeRawData(rastBytes.data(), rastBytes.size());

  // RASB
  QByteArray rasbBytes;
  for (const auto& header : parsedHeaders) {
    for (const QString& str : header.strings()) {
      int offset = rastBytes.indexOf(str.toLatin1());
      QByteArray offsetBytes = RedguardsUtils::intToByteArray(offset, true);
      rasbBytes.append(offsetBytes);
    }
  }
  out.writeRawData("RASB", 4);
  out << static_cast<qint32>(rasbBytes.size());
  out.writeRawData(rasbBytes.data(), rasbBytes.size());

  // RAVA
  QByteArray ravaBytes;
  ravaBytes.append(QByteArray(4, 0));
  for (const auto* header : mMapHeaders) {
    for (int instance = 0; instance < header->instances(); ++instance) {
      for (int32_t var : header->variables()) {
        ravaBytes.append(RedguardsUtils::intToByteArray(var, true));
      }
    }
  }
  out.writeRawData("RAVA", 4);
  out << static_cast<qint32>(ravaBytes.size());
  out.writeRawData(ravaBytes.data(), ravaBytes.size());

  // RASC
  out.writeRawData("RASC", 4);
  out << static_cast<qint32>(mScriptDataOffset + parser.totalScriptLength());
  out.writeRawData(QByteArray(mScriptDataOffset, 0).data(), mScriptDataOffset);
  for (const auto& header : parsedHeaders) {
    out.writeRawData(header.scriptBytes().data(), header.scriptBytes().size());
  }

  auto writeRecord = [&out](const QString& name, const QByteArray& data) {
    out.writeRawData(name.toLatin1().data(), 4);
    out << static_cast<qint32>(data.size());
    out.writeRawData(data.data(), data.size());
  };

  // Records between script and attributes
  const QStringList preAttr = {"RAHK", "RALC", "RAEX"};
  for (const QString& record : preAttr) {
    writeRecord(record, mRecords.value(record));
  }

  // RAAT
  out.writeRawData("RAAT", 4);
  out << static_cast<qint32>(parsedHeaders.size() * 256);
  for (const auto& header : parsedHeaders) {
    out.writeRawData(header.attributeBytes().data(), header.attributeBytes().size());
  }

  // Remaining records
  const QStringList postAttr = {"RAAN", "RAGR", "RANM", "MPOB", "MPRP", "MPSO",
                                "MPSL", "MPSF", "MPMK", "MPSZ", "WDNM", "FLAT"};
  for (const QString& record : postAttr) {
    writeRecord(record, mRecords.value(record));
  }

  out.writeRawData("END ", 4);
  file.close();
  return true;
}

QString RedguardsMapFile::getScript() const
{
  QString output;
  output.append("Maps\\");
  output.append(mName);
  output.append(".RGM\nID");
  if (mIds.size() > 1) {
    output.append("s");
  }
  output.append(": ");
  for (int i = 0; i < mIds.size(); ++i) {
    if (i > 0) {
      output.append(", ");
    }
    output.append(QString::number(mIds[i]));
  }
  output.append("\n\n");

  for (int i = 0; i < mMapHeaders.size(); ++i) {
    const RedguardsMapHeader* mapHeader = mMapHeaders[i];
    if (i > 0) {
      output.append("\n\n");
    }

    if (mapHeader->variables().size() > 4) {
      for (int j = 2; j < mapHeader->variables().size() - 2; ++j) {
        output.append("var");
        output.append(QString::number(j));
        output.append(" = ");
        output.append(QString::number(mapHeader->variables()[j]));
        output.append("\n");
        if (j == mapHeader->variables().size() - 3) {
          output.append("\n");
        }
      }
    }

    bool hasAttribute = false;
    const QByteArray& attrBytes = mapHeader->attributeBytes();
    for (int j = 0; j < attrBytes.size(); ++j) {
      const int value = static_cast<unsigned char>(attrBytes[j]);
      if (value != 0) {
        const QString attrName = mMapDatabase->attributes().value(j);
        output.append(attrName);
        output.append(" = ");
        output.append(QString::number(value));
        output.append("\n");
        hasAttribute = true;
      }
    }
    if (hasAttribute) {
      output.append("\n");
    }

    output.append(mapHeader->script());
  }

  return output;
}

QString RedguardsMapFile::getModifiedScript(const RedguardsMapChanges& mapChanges) const
{
  QString output;
  const QStringList scriptLines = getScript().split('\n');
  
  int changeCount = 0;
  int deletionCount = 0;
  int insertionCount = 0;

  for (int pos = 0; pos < scriptLines.size(); ++pos) {
    const QList<QString>* lines = mapChanges.lineChangesAt(mName, pos);
    if (!lines) {
      output.append(scriptLines[pos]);
      output.append("\n");
    } else {
      changeCount++;
      qInfo().noquote() << "[GameRedguard] Map" << mName << "position" << pos << "has" << lines->size() << "changes";
      
      // Show context: 3 lines before
      qInfo().noquote() << "[GameRedguard]   --- Context (3 lines before) ---";
      for (int i = qMax(0, pos - 3); i < pos; ++i) {
        qInfo().noquote() << "[GameRedguard]   " << QString::number(i) + ":" << scriptLines[i].left(80);
      }
      
      qInfo().noquote() << "[GameRedguard]   >>> Position" << pos << "(ORIGINAL):" << scriptLines[pos].left(80);
      qInfo().noquote() << "[GameRedguard]   First change:" << (lines->isEmpty() ? "EMPTY" : lines->first().left(80));
      
      // Output original line FIRST (unless first change is "null" = deletion marker)
      qInfo().noquote() << "[GameRedguard]   === OUTPUT START ===";
      if (lines->first() != "null") {
        output.append(scriptLines[pos]);
        output.append("\n");
        qInfo().noquote() << "[GameRedguard]   + KEPT ORIG:" << scriptLines[pos].left(80);
      } else {
        deletionCount++;
        qInfo().noquote() << "[GameRedguard]   - DELETED:" << scriptLines[pos].left(80);
      }
      
      // Then insert change lines AFTER the original line
      for (const QString& line : *lines) {
        if (line != "null") {
          insertionCount++;
          output.append(line);
          output.append("\n");
          qInfo().noquote() << "[GameRedguard]   + INSERTED:" << line.left(80);
        }
      }
      qInfo().noquote() << "[GameRedguard]   === OUTPUT END ===";
      
      // Show context: 3 lines after
      qInfo().noquote() << "[GameRedguard]   --- Context (3 lines after) ---";
      for (int i = pos + 1; i < qMin(scriptLines.size(), pos + 4); ++i) {
        qInfo().noquote() << "[GameRedguard]   " << QString::number(i) + ":" << scriptLines[i].left(80);
      }
      qInfo().noquote() << "";
    }
  }
  
  if (changeCount > 0) {
    qInfo().noquote() << "[GameRedguard] Map" << mName << "applied" << changeCount << "position changes:" 
             << deletionCount << "deletions," << insertionCount << "insertions";
  }

  return output;
}

void RedguardsMapFile::parseMapHeaders()
{
  const QByteArray headerBytes = mRecords.value("RAHD");
  if (headerBytes.isEmpty()) {
    return;
  }

  int numMapHeaders = RedguardsUtils::byteRangeToInt(headerBytes, 0, 4, true);
  for (int i = 0; i < numMapHeaders; ++i) {
    int start = 8 + i * 165;
    QByteArray subrecord = headerBytes.mid(start, 165);
    mMapHeaders.append(new RedguardsMapHeader(subrecord));
  }

  QString allStrings = QString::fromLatin1(mRecords.value("RAST"));
  QByteArray stringOffsets = mRecords.value("RASB");

  QByteArray variableBytes = mRecords.value("RAVA");
  QList<int32_t> variables;
  for (int i = 0; i < variableBytes.size() / 4; ++i) {
    variables.append(RedguardsUtils::byteRangeToInt(variableBytes, i * 4, 4, true));
  }

  QByteArray scriptBytes = mRecords.value("RASC");
  if (mMapHeaders.isEmpty()) {
    return;
  }
  mScriptDataOffset = mMapHeaders.first()->scriptDataOffset();
  int scriptPos = mScriptDataOffset;

  for (auto* header : mMapHeaders) {
    header->initStrings(allStrings, stringOffsets);
    header->initVariables(variables);

    QByteArray headerScript = scriptBytes.mid(scriptPos, header->scriptLength());
    scriptPos += header->scriptLength();
    header->setScriptBytes(headerScript);
  }

  QByteArray attributeBytes = mRecords.value("RAAT");
  for (int i = 0; i < mMapHeaders.size(); ++i) {
    QByteArray attr = attributeBytes.mid(i * 256, 256);
    mMapHeaders[i]->setAttributeBytes(attr);
  }

  for (auto* header : mMapHeaders) {
    RedguardsScriptReader reader(mMapDatabase, header);
    header->setScript(reader.read());
  }
}

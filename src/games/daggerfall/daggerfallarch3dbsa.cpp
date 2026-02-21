#include "daggerfallarch3dbsa.h"
#include "daggerfallformatutils.h"

#include "xnginebsaformat.h"

#include <cmath>

namespace {

using Daggerfall::FormatUtil::readLE16U;
using Daggerfall::FormatUtil::readLE32;
using Daggerfall::FormatUtil::readLE32U;
using Daggerfall::FormatUtil::setError;

bool readLE16S(const QByteArray& data, qsizetype offset, qint16& value)
{
  quint16 raw = 0;
  if (!readLE16U(data, offset, raw)) {
    return false;
  }
  value = static_cast<qint16>(raw);
  return true;
}

QString readVersion(const QByteArray& data)
{
  if (data.size() < 4) {
    return {};
  }
  return QString::fromLatin1(data.constData(), 4);
}

bool isKnownVersion(const QString& version)
{
  return version == "v2.7" || version == "v2.6" || version == "v2.5";
}

int pointIndexForOffset(const QString& version, qint32 pointOffset)
{
  if (pointOffset < 0) {
    return -1;
  }
  if (version == "v2.5") {
    if ((pointOffset % 3) != 0) {
      return -1;
    }
    return pointOffset / 3;
  }
  if ((pointOffset % 12) != 0) {
    return -1;
  }
  return pointOffset / 12;
}

qint16 decodeSigned12Bit(qint16 fromFile)
{
  qint16 v = static_cast<qint16>(fromFile & 0x0FFF);
  if ((v & 0x0800) != 0) {
    v = static_cast<qint16>(v | 0xF000);
  }
  return v;
}

struct DVec3
{
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

DVec3 toVec3(const DaggerfallArch3dBsa::Point& p)
{
  return {static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(p.z)};
}

DVec3 sub(const DVec3& a, const DVec3& b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

double dot(const DVec3& a, const DVec3& b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool solvePlaneCoordinates(const DaggerfallArch3dBsa::Point& p0,
                           const DaggerfallArch3dBsa::Point& p1,
                           const DaggerfallArch3dBsa::Point& p2,
                           const DaggerfallArch3dBsa::Point& p,
                           double& outS, double& outT)
{
  const DVec3 a = toVec3(p0);
  const DVec3 b = toVec3(p1);
  const DVec3 c = toVec3(p2);
  const DVec3 q = toVec3(p);
  const DVec3 e1 = sub(b, a);
  const DVec3 e2 = sub(c, a);
  const DVec3 d = sub(q, a);

  const double a11 = dot(e1, e1);
  const double a12 = dot(e1, e2);
  const double a22 = dot(e2, e2);
  const double b1 = dot(d, e1);
  const double b2 = dot(d, e2);
  const double det = a11 * a22 - a12 * a12;
  if (std::abs(det) < 1.0e-12) {
    return false;
  }

  outS = (b1 * a22 - b2 * a12) / det;
  outT = (a11 * b2 - a12 * b1) / det;
  return true;
}

bool solveLinear4x4(double a[4][4], double b[4], double x[4])
{
  for (int col = 0; col < 4; ++col) {
    int pivot = col;
    double best = std::abs(a[col][col]);
    for (int r = col + 1; r < 4; ++r) {
      const double v = std::abs(a[r][col]);
      if (v > best) {
        best = v;
        pivot = r;
      }
    }
    if (best < 1.0e-12) {
      return false;
    }
    if (pivot != col) {
      for (int c = col; c < 4; ++c) {
        std::swap(a[col][c], a[pivot][c]);
      }
      std::swap(b[col], b[pivot]);
    }

    const double div = a[col][col];
    for (int c = col; c < 4; ++c) {
      a[col][c] /= div;
    }
    b[col] /= div;

    for (int r = 0; r < 4; ++r) {
      if (r == col) {
        continue;
      }
      const double factor = a[r][col];
      if (std::abs(factor) < 1.0e-20) {
        continue;
      }
      for (int c = col; c < 4; ++c) {
        a[r][c] -= factor * a[col][c];
      }
      b[r] -= factor * b[col];
    }
  }

  for (int i = 0; i < 4; ++i) {
    x[i] = b[i];
  }
  return true;
}

bool solveAffineUvFromFourAnchors(const DaggerfallArch3dBsa::Point& p0,
                                  const DaggerfallArch3dBsa::Point& p1,
                                  const DaggerfallArch3dBsa::Point& p2,
                                  const DaggerfallArch3dBsa::Point& p3,
                                  double u0, double u1, double u2, double u3,
                                  double v0, double v1, double v2, double v3,
                                  double uCoeff[4], double vCoeff[4])
{
  double mU[4][4] = {
      {static_cast<double>(p0.x), static_cast<double>(p0.y), static_cast<double>(p0.z), 1.0},
      {static_cast<double>(p1.x), static_cast<double>(p1.y), static_cast<double>(p1.z), 1.0},
      {static_cast<double>(p2.x), static_cast<double>(p2.y), static_cast<double>(p2.z), 1.0},
      {static_cast<double>(p3.x), static_cast<double>(p3.y), static_cast<double>(p3.z), 1.0},
  };
  double rhsU[4] = {u0, u1, u2, u3};
  if (!solveLinear4x4(mU, rhsU, uCoeff)) {
    return false;
  }

  double mV[4][4] = {
      {static_cast<double>(p0.x), static_cast<double>(p0.y), static_cast<double>(p0.z), 1.0},
      {static_cast<double>(p1.x), static_cast<double>(p1.y), static_cast<double>(p1.z), 1.0},
      {static_cast<double>(p2.x), static_cast<double>(p2.y), static_cast<double>(p2.z), 1.0},
      {static_cast<double>(p3.x), static_cast<double>(p3.y), static_cast<double>(p3.z), 1.0},
  };
  double rhsV[4] = {v0, v1, v2, v3};
  return solveLinear4x4(mV, rhsV, vCoeff);
}

void evalAffineUv(const DaggerfallArch3dBsa::Point& p, const double coeff[4],
                  double& outValue)
{
  outValue = coeff[0] * static_cast<double>(p.x) +
             coeff[1] * static_cast<double>(p.y) +
             coeff[2] * static_cast<double>(p.z) + coeff[3];
}

void finalizePlaneUv(DaggerfallArch3dBsa::Plane& plane,
                     const QVector<DaggerfallArch3dBsa::Point>& points,
                     QString& warning)
{
  const int count = plane.points.size();
  if (count <= 0) {
    return;
  }

  // 0: absolute, 1: delta from 0, 2: delta from 1, 3: absolute.
  plane.points[0].uAbsolute = plane.points[0].u;
  plane.points[0].vAbsolute = plane.points[0].v;

  if (count >= 2) {
    plane.points[1].uAbsolute = static_cast<qint32>(plane.points[0].uAbsolute) + plane.points[1].u;
    plane.points[1].vAbsolute = static_cast<qint32>(plane.points[0].vAbsolute) + plane.points[1].v;
  }
  if (count >= 3) {
    plane.points[2].uAbsolute = static_cast<qint32>(plane.points[1].uAbsolute) + plane.points[2].u;
    plane.points[2].vAbsolute = static_cast<qint32>(plane.points[1].vAbsolute) + plane.points[2].v;
  }
  if (count >= 4) {
    plane.points[3].uAbsolute = plane.points[3].u;
    plane.points[3].vAbsolute = plane.points[3].v;
  }

  // Reconstruct 5+ from first 4 anchors using affine 3D->UV mapping when possible.
  bool usedAffine = false;
  if (count >= 5) {
    const int i0 = plane.points[0].pointIndex;
    const int i1 = plane.points[1].pointIndex;
    const int i2 = plane.points[2].pointIndex;
    const int i3 = plane.points[3].pointIndex;
    if (i0 >= 0 && i0 < points.size() &&
        i1 >= 0 && i1 < points.size() &&
        i2 >= 0 && i2 < points.size() &&
        i3 >= 0 && i3 < points.size()) {
      double uCoeff[4] = {};
      double vCoeff[4] = {};
      if (solveAffineUvFromFourAnchors(points.at(i0), points.at(i1), points.at(i2), points.at(i3),
                                       static_cast<double>(plane.points[0].uAbsolute),
                                       static_cast<double>(plane.points[1].uAbsolute),
                                       static_cast<double>(plane.points[2].uAbsolute),
                                       static_cast<double>(plane.points[3].uAbsolute),
                                       static_cast<double>(plane.points[0].vAbsolute),
                                       static_cast<double>(plane.points[1].vAbsolute),
                                       static_cast<double>(plane.points[2].vAbsolute),
                                       static_cast<double>(plane.points[3].vAbsolute),
                                       uCoeff, vCoeff)) {
        for (int i = 4; i < count; ++i) {
          const int pi = plane.points[i].pointIndex;
          if (pi < 0 || pi >= points.size()) {
            continue;
          }
          double u = 0.0;
          double v = 0.0;
          evalAffineUv(points.at(pi), uCoeff, u);
          evalAffineUv(points.at(pi), vCoeff, v);
          plane.points[i].uAbsolute = static_cast<qint32>(std::llround(u));
          plane.points[i].vAbsolute = static_cast<qint32>(std::llround(v));
          plane.points[i].uvReconstructed = true;
        }
        usedAffine = true;
        plane.uvMode = DaggerfallArch3dBsa::UvReconstructionMode::Affine4;
      }
    }
  }

  // Fallback: reconstruct 5+ from first triangle in model space.
  if (count >= 5 && !usedAffine) {
    const int i0 = plane.points[0].pointIndex;
    const int i1 = plane.points[1].pointIndex;
    const int i2 = plane.points[2].pointIndex;
    if (i0 >= 0 && i0 < points.size() &&
        i1 >= 0 && i1 < points.size() &&
        i2 >= 0 && i2 < points.size()) {
      const auto& p0 = points.at(i0);
      const auto& p1 = points.at(i1);
      const auto& p2 = points.at(i2);
      const double u0 = static_cast<double>(plane.points[0].uAbsolute);
      const double v0 = static_cast<double>(plane.points[0].vAbsolute);
      const double du1 = static_cast<double>(plane.points[1].uAbsolute - plane.points[0].uAbsolute);
      const double dv1 = static_cast<double>(plane.points[1].vAbsolute - plane.points[0].vAbsolute);
      const double du2 = static_cast<double>(plane.points[2].uAbsolute - plane.points[0].uAbsolute);
      const double dv2 = static_cast<double>(plane.points[2].vAbsolute - plane.points[0].vAbsolute);

      for (int i = 4; i < count; ++i) {
        const int pi = plane.points[i].pointIndex;
        if (pi < 0 || pi >= points.size()) {
          continue;
        }
        double s = 0.0;
        double t = 0.0;
        if (!solvePlaneCoordinates(p0, p1, p2, points.at(pi), s, t)) {
          continue;
        }
        const double u = u0 + s * du1 + t * du2;
        const double v = v0 + s * dv1 + t * dv2;
        plane.points[i].uAbsolute = static_cast<qint32>(std::llround(u));
        plane.points[i].vAbsolute = static_cast<qint32>(std::llround(v));
        plane.points[i].uvReconstructed = true;
      }
      plane.uvMode = DaggerfallArch3dBsa::UvReconstructionMode::Triangle3;
    } else if (warning.isEmpty()) {
      warning = "Some plane UVs could not be reconstructed (invalid point index)";
    }
  }

  for (int i = 0; i < count; ++i) {
    plane.points[i].uPixels = static_cast<double>(plane.points[i].uAbsolute) / 16.0;
    plane.points[i].vPixels = static_cast<double>(plane.points[i].vAbsolute) / 16.0;
  }
}

bool parseMeshRecordData(const QByteArray& data, quint16 recordId,
                         DaggerfallArch3dBsa::MeshRecord& outMesh, QString* errorMessage)
{
  outMesh = {};
  outMesh.recordId = recordId;

  if (data.size() < 64) {
    return setError(errorMessage, "ARCH3D record is smaller than 64-byte header");
  }

  outMesh.version = readVersion(data);
  if (!isKnownVersion(outMesh.version)) {
    return setError(errorMessage,
                    QString("Unsupported ARCH3D record version: %1").arg(outMesh.version));
  }

  if (!readLE32(data, 4, outMesh.pointCount) ||
      !readLE32(data, 8, outMesh.planeCount) ||
      !readLE32(data, 12, outMesh.radius) ||
      !readLE32(data, 24, outMesh.planeDataOffset) ||
      !readLE32(data, 28, outMesh.objectDataOffset) ||
      !readLE32(data, 32, outMesh.objectDataCount) ||
      !readLE32U(data, 36, outMesh.unknown2) ||
      !readLE32(data, 48, outMesh.pointListOffset) ||
      !readLE32(data, 52, outMesh.normalListOffset) ||
      !readLE32U(data, 56, outMesh.unknown3) ||
      !readLE32(data, 60, outMesh.planeListOffset)) {
    return setError(errorMessage, "Failed reading ARCH3D record header fields");
  }

  if (outMesh.pointCount < 0 || outMesh.planeCount < 0 || outMesh.objectDataCount < 0) {
    return setError(errorMessage, "ARCH3D record reports negative element counts");
  }
  if (outMesh.pointListOffset < 0 || outMesh.planeListOffset < 0 ||
      outMesh.normalListOffset < 0 || outMesh.planeDataOffset < 0 ||
      outMesh.objectDataOffset < 0) {
    return setError(errorMessage, "ARCH3D record reports negative section offsets");
  }

  const qsizetype pointListOffset = static_cast<qsizetype>(outMesh.pointListOffset);
  const qsizetype pointBytes = static_cast<qsizetype>(outMesh.pointCount) * 12;
  if (pointListOffset + pointBytes > data.size()) {
    return setError(errorMessage, "ARCH3D point list overflows record");
  }
  outMesh.points.reserve(outMesh.pointCount);
  for (qint32 i = 0; i < outMesh.pointCount; ++i) {
    const qsizetype off = pointListOffset + static_cast<qsizetype>(i) * 12;
    DaggerfallArch3dBsa::Point p;
    if (!readLE32(data, off + 0, p.x) || !readLE32(data, off + 4, p.y) ||
        !readLE32(data, off + 8, p.z)) {
      return setError(errorMessage, "Failed reading ARCH3D point");
    }
    outMesh.points.push_back(p);
  }

  const qsizetype normalListOffset = static_cast<qsizetype>(outMesh.normalListOffset);
  const qsizetype normalBytes = static_cast<qsizetype>(outMesh.planeCount) * 12;
  if (normalListOffset + normalBytes > data.size()) {
    return setError(errorMessage, "ARCH3D normal list overflows record");
  }
  outMesh.normals.reserve(outMesh.planeCount);
  for (qint32 i = 0; i < outMesh.planeCount; ++i) {
    const qsizetype off = normalListOffset + static_cast<qsizetype>(i) * 12;
    DaggerfallArch3dBsa::Point n;
    if (!readLE32(data, off + 0, n.x) || !readLE32(data, off + 4, n.y) ||
        !readLE32(data, off + 8, n.z)) {
      return setError(errorMessage, "Failed reading ARCH3D normal");
    }
    outMesh.normals.push_back(n);
  }

  const qsizetype planeDataOffset = static_cast<qsizetype>(outMesh.planeDataOffset);
  const qsizetype planeDataBytes = static_cast<qsizetype>(outMesh.planeCount) * 24;
  if (planeDataOffset + planeDataBytes <= data.size()) {
    outMesh.planeData.reserve(outMesh.planeCount);
    for (qint32 i = 0; i < outMesh.planeCount; ++i) {
      const qsizetype off = planeDataOffset + static_cast<qsizetype>(i) * 24;
      outMesh.planeData.push_back(data.mid(off, 24));
    }
  } else {
    outMesh.warning = "PlaneDataList range is invalid for this record";
  }

  // Variable-size list of plane records.
  qsizetype planePos = static_cast<qsizetype>(outMesh.planeListOffset);
  outMesh.planes.reserve(outMesh.planeCount);
  for (qint32 i = 0; i < outMesh.planeCount; ++i) {
    if (planePos + 8 > data.size()) {
      return setError(errorMessage, "ARCH3D plane header overflows record");
    }

    DaggerfallArch3dBsa::Plane plane;
    plane.pointCount = static_cast<quint8>(data.at(planePos + 0));
    plane.unknown1 = static_cast<quint8>(data.at(planePos + 1));
    if (!readLE16U(data, planePos + 2, plane.textureRaw) ||
        !readLE32U(data, planePos + 4, plane.unknown2)) {
      return setError(errorMessage, "Failed reading ARCH3D plane header");
    }
    plane.texture.imageIndex = static_cast<int>(plane.textureRaw & 0x7f);
    plane.texture.fileIndex = static_cast<int>(plane.textureRaw >> 7);
    planePos += 8;

    const qsizetype planePointBytes = static_cast<qsizetype>(plane.pointCount) * 8;
    if (planePos + planePointBytes > data.size()) {
      return setError(errorMessage, "ARCH3D plane point list overflows record");
    }
    plane.points.reserve(plane.pointCount);
    for (quint8 p = 0; p < plane.pointCount; ++p) {
      DaggerfallArch3dBsa::PlanePoint pp;
      if (!readLE32(data, planePos + 0, pp.pointOffset) ||
          !readLE16S(data, planePos + 4, pp.u) ||
          !readLE16S(data, planePos + 6, pp.v)) {
        return setError(errorMessage, "Failed reading ARCH3D plane point");
      }
      pp.u = decodeSigned12Bit(pp.u);
      pp.v = decodeSigned12Bit(pp.v);
      pp.pointIndex = pointIndexForOffset(outMesh.version, pp.pointOffset);
      plane.points.push_back(pp);
      planePos += 8;
    }

    finalizePlaneUv(plane, outMesh.points, outMesh.warning);
    switch (plane.uvMode) {
      case DaggerfallArch3dBsa::UvReconstructionMode::Affine4:
        ++outMesh.planesUvAffine4;
        break;
      case DaggerfallArch3dBsa::UvReconstructionMode::Triangle3:
        ++outMesh.planesUvTriangle3;
        break;
      default:
        ++outMesh.planesUvNone;
        break;
    }
    outMesh.planes.push_back(plane);
  }

  // Variable-size object-data list.
  qsizetype objectPos = static_cast<qsizetype>(outMesh.objectDataOffset);
  if (objectPos > data.size()) {
    outMesh.warning = "ObjectDataList offset points outside record";
    return true;
  }

  outMesh.objectData.reserve(outMesh.objectDataCount);
  for (qint32 i = 0; i < outMesh.objectDataCount; ++i) {
    if (objectPos + 18 > data.size()) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "ObjectDataList appears truncated in this record";
      }
      break;
    }

    DaggerfallArch3dBsa::ObjectData obj;
    if (!readLE32(data, objectPos + 0, obj.number0) ||
        !readLE32(data, objectPos + 4, obj.number1) ||
        !readLE32(data, objectPos + 8, obj.number2) ||
        !readLE32(data, objectPos + 12, obj.number3)) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "Failed decoding ObjectData header values";
      }
      break;
    }
    quint16 count = 0;
    if (!readLE16U(data, objectPos + 16, count)) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "Failed decoding ObjectData subrecord count";
      }
      break;
    }
    obj.subrecordCount = count;
    objectPos += 18;

    const qsizetype valuesBytes = static_cast<qsizetype>(obj.subrecordCount) * 6;
    if (objectPos + valuesBytes > data.size()) {
      if (outMesh.warning.isEmpty()) {
        outMesh.warning = "ObjectData subrecord list overflows record";
      }
      break;
    }

    obj.values.reserve(obj.subrecordCount);
    for (quint16 v = 0; v < obj.subrecordCount; ++v) {
      obj.values.push_back(data.mid(objectPos, 6));
      objectPos += 6;
    }
    outMesh.objectData.push_back(obj);
  }

  return true;
}

}  // namespace

bool DaggerfallArch3dBsa::listRecordIds(const QString& arch3dBsaPath,
                                        QVector<quint16>& outRecordIds,
                                        QString* errorMessage)
{
  outRecordIds.clear();

  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NumberRecord) {
    return setError(errorMessage, "ARCH3D.BSA is not a NumberRecord archive");
  }

  outRecordIds.reserve(archive.entries.size());
  for (const auto& e : archive.entries) {
    outRecordIds.push_back(e.recordId);
  }
  return true;
}

bool DaggerfallArch3dBsa::loadMeshRecord(const QString& arch3dBsaPath, quint16 recordId,
                                         MeshRecord& outMesh, QString* errorMessage)
{
  XngineBSAFormat::Archive archive;
  if (!XngineBSAFormat::readArchive(arch3dBsaPath, archive, errorMessage)) {
    return false;
  }
  if (archive.type != XngineBSAFormat::IndexType::NumberRecord) {
    return setError(errorMessage, "ARCH3D.BSA is not a NumberRecord archive");
  }

  const XngineBSAFormat::Entry* target = nullptr;
  for (const auto& e : archive.entries) {
    if (e.recordId == recordId) {
      target = &e;
      break;
    }
  }
  if (target == nullptr) {
    return setError(errorMessage, QString("ARCH3D record %1 not found").arg(recordId));
  }

  return parseMeshRecordData(target->data, target->recordId, outMesh, errorMessage);
}

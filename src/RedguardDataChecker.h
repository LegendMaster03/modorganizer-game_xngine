#ifndef REDGUARDDATACHECKER_H
#define REDGUARDDATACHECKER_H

#include <moddatachecker.h>
#include <QString>
#include <QStringList>

class RedguardDataChecker : public MOBase::ModDataChecker
{
public:
  RedguardDataChecker() = default;
  
  virtual CheckReturn dataLooksValid(std::shared_ptr<const MOBase::IFileTree> fileTree) const override;
  
  virtual std::shared_ptr<MOBase::IFileTree> fix(std::shared_ptr<MOBase::IFileTree> fileTree) const override;

private:
  // Check if this is Format 1 (patch-based) mod
  bool isFormat1Mod(std::shared_ptr<const MOBase::IFileTree> tree) const;
  
  // Check if this is Format 2 (file replacement) mod
  bool isFormat2Mod(std::shared_ptr<const MOBase::IFileTree> tree) const;
  
  // Find the actual mod root when nested in a single subdirectory
  std::shared_ptr<const MOBase::IFileTree> findModRoot(std::shared_ptr<const MOBase::IFileTree> tree) const;
};

#endif // REDGUARDDATACHECKER_H

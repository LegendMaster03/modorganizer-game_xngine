#ifndef BATTLESPIREDATACHECKER_H
#define BATTLESPIREDATACHECKER_H

#include <xnginemoddatachecker.h>

#include <QString>

class GameBattlespire;

class BattlespiresModDataChecker : public XngineModDataChecker
{
public:
  BattlespiresModDataChecker(GameBattlespire const* game) : XngineModDataChecker(game) {}

protected:
  QString getModCheckFolders() const override;
  QString getModCheckExtensions() const override;
};

#endif  // BATTLESPIREDATACHECKER_H

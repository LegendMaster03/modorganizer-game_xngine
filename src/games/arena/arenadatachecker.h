#ifndef ARENADATACHECKER_H
#define ARENADATACHECKER_H

#include <xnginemoddatachecker.h>
#include <QString>

class GameArena;

class ArenaModDataChecker : public XngineModDataChecker
{
public:
  ArenaModDataChecker(GameArena const* game) : XngineModDataChecker(game) {}

protected:
  QString getModCheckFolders() const override;
  QString getModCheckExtensions() const override;
};

#endif  // ARENADATACHECKER_H

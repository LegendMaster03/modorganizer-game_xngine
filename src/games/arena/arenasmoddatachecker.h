#ifndef ARENASMODDATACHECKER_H
#define ARENASMODDATACHECKER_H

#include <xnginemoddatachecker.h>

#include <QString>

class GameArena;

class ArenasModDataChecker : public XngineModDataChecker
{
public:
  ArenasModDataChecker(GameArena const* game) : XngineModDataChecker(game) {}

protected:
  QString getModCheckFolders() const override;
  QString getModCheckExtensions() const override;
};

#endif  // ARENASMODDATACHECKER_H

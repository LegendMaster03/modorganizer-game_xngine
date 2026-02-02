#ifndef XGINESCRIPTEXTENDER_H
#define XGINESCRIPTEXTENDER_H

#include "scriptextender.h"

class GameXngine;

class XngineScriptExtender : public MOBase::ScriptExtender
{
public:
  XngineScriptExtender(GameXngine const* game);

  virtual ~XngineScriptExtender();

  virtual QString loaderName() const override;

  virtual QString loaderPath() const override;

  virtual QString savegameExtension() const override;

  virtual bool isInstalled() const override;

  virtual QString getExtenderVersion() const override;

  virtual WORD getArch() const override;

protected:
  GameXngine const* const m_Game;
};

#endif  // XGINESCRIPTEXTENDER_H

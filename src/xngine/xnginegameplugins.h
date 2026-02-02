#ifndef XNGINEGAMEPLUGINS_H
#define XNGINEGAMEPLUGINS_H

#include <QDateTime>
#include <QStringList>
#include <gameplugins.h>
#include <imoinfo.h>

class XngineGamePlugins : public MOBase::GamePlugins
{
public:
  XngineGamePlugins(MOBase::IOrganizer* organizer);

  virtual void writePluginLists(const MOBase::IPluginList* pluginList) override;
  virtual void readPluginLists(MOBase::IPluginList* pluginList) override;
  virtual QStringList getLoadOrder() override;

protected:
  MOBase::IOrganizer* organizer() const { return m_Organizer; }

  virtual void writePluginList(const MOBase::IPluginList* pluginList,
                               const QString& filePath);
  virtual void writeLoadOrderList(const MOBase::IPluginList* pluginList,
                                  const QString& filePath);
  virtual QStringList readLoadOrderList(MOBase::IPluginList* pluginList,
                                        const QString& filePath);
  virtual QStringList readPluginList(MOBase::IPluginList* pluginList);

protected:
  MOBase::IOrganizer* m_Organizer;
  QDateTime m_LastRead;

private:
  void writeList(const MOBase::IPluginList* pluginList, const QString& filePath,
                 bool loadOrder);
};

#endif  // XNGINEGAMEPLUGINS_H

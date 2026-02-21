#include "daggerfallbiocodes.h"

#include <QRegularExpression>

namespace {

QHash<int, QString> parseIdNameTable(const QString& raw)
{
  QHash<int, QString> out;
  const QStringList lines = raw.split('\n');
  static const QRegularExpression rx("^\\s*(\\d+)\\s+(.+?)\\s*$");

  for (const QString& line : lines) {
    const QRegularExpressionMatch m = rx.match(line);
    if (!m.hasMatch()) {
      continue;
    }
    const int id = m.captured(1).toInt();
    const QString name = m.captured(2).trimmed();
    if (id <= 0 || name.isEmpty() || out.contains(id)) {
      continue;
    }
    out.insert(id, name);
  }

  return out;
}

}  // namespace

QVector<DaggerfallBioCodes::PredefinedClass> DaggerfallBioCodes::predefinedClasses()
{
  return {
      {0, "Mage"},       {1, "Spellsword"}, {2, "Battlemage"}, {3, "Sorceror"},
      {4, "Healer"},     {5, "Nightblade"}, {6, "Bard"},       {7, "Burglar"},
      {8, "Rogue"},      {9, "Acrobat"},    {10, "Thief"},     {11, "Assassin"},
      {12, "Monk"},      {13, "Archer"},    {14, "Ranger"},    {15, "Barbarian"},
      {16, "Warrior"},   {17, "Knight"},
  };
}

QString DaggerfallBioCodes::predefinedClassName(int gamma)
{
  const QVector<PredefinedClass> values = predefinedClasses();
  for (const PredefinedClass& pc : values) {
    if (pc.gamma == gamma) {
      return pc.name;
    }
  }
  return {};
}

QString DaggerfallBioCodes::biographyFileNameForGamma(int gamma)
{
  if (gamma < 0 || gamma > 99) {
    return {};
  }
  return QString("BIOG%1T0.TXT").arg(gamma, 2, 10, QLatin1Char('0'));
}

QHash<int, QString> DaggerfallBioCodes::skillCodeNames()
{
  return {
      {0, "Medical"},        {1, "Etiquette"},    {2, "Streetwise"},  {3, "Jumping"},
      {4, "Orcish"},         {5, "Harpy"},        {6, "Giantish"},    {7, "Dragonish"},
      {8, "Nymph"},          {9, "Daedric"},      {10, "Spriggan"},   {11, "Centaurian"},
      {12, "Impish"},        {13, "Lockpicking"}, {14, "Mercantile"}, {15, "Pickpocket"},
      {16, "Stealth"},       {17, "Swimming"},    {18, "Climbing"},   {19, "Backstabbing"},
      {20, "Dodging"},       {21, "Running"},     {22, "Destruction"},{23, "Restoration"},
      {24, "Illusion"},      {25, "Alteration"},  {26, "Thaumaturgy"},{27, "Mysticism"},
      {28, "Short Blade"},   {29, "Long Blade"},  {30, "Hand-to-Hand"},{31, "Axe"},
      {32, "Blunt Weapon"},  {33, "Archery"},     {34, "Critical Strike"},
  };
}

QString DaggerfallBioCodes::skillName(int code)
{
  return skillCodeNames().value(code);
}

QHash<int, QString> DaggerfallBioCodes::demographicNames()
{
  return {
      {0, "Commoners"},
      {1, "Merchants"},
      {2, "Scholars"},
      {3, "Nobility"},
      {4, "Underworld"},
      {5, "Generic Temple and Lord Coulder"},
      {6, "Supernatural"},
      {7, "Knight Order, Temple, or Vampire Clan"},
  };
}

QString DaggerfallBioCodes::demographicName(int index)
{
  return demographicNames().value(index);
}

QHash<int, QString> DaggerfallBioCodes::knownFactionNames()
{
  static const QString raw = R"(1 Clavicus Vile
2 Mehrunes Dagon
3 Molag Bal
4 Hircine
5 Sanguine
6 Peryite
7 Malacath
8 Hermaeus Mora
9 Sheogorath
10 Boethiah
11 Namira
12 Meridia
13 Vaernima
14 Nocturnal
15 Mephala
16 Azura
17 Oblivion
21 Arkay
22 Zen
24 Mara
25 Ebonarm
26 Akatosh
27 Julianos
29 Dibella
33 Stendarr
35 Kynareth
36 The Temple of Kynareth
37 The Kynaran Order
40 The Mages Guild
41 The Fighters Guild
42 The Thieves Guild
60 The Academics
61 The Patricians
62 The Travelers League
63 The Mercenary Mages
64 The Isolationists
65 The Utility Mages
66 The Cabal
67 The Order of the Lamp
68 The Archmagister
69 The Guildmagister
70 The Master of Academia
71 The Master of Incunabula
72 The Master at Arms
73 The Palatinus
74 The Master of the Scry
76 The Master of Initiates (Mages Guild)
77 The Guildmaster
82 The Order of Arkay
83 The Knights of the Circle
84 The Resolution of Z'en
85 The Knights of Iron
88 The Benevolence of Mara
89 The Maran Knights
90 The Citadel of Ebonarm
91 The Battlelords
92 The Akatosh Chantry
93 The Order of the Hour
94 The School of Julianos
95 The Knights Mentor
98 The House of Dibella
99 The Order of the Lily
106 The Temple of Stendarr
107 The Crusaders
108 The Dark Brotherhood
129 The Blades
150 The Vraseth
151 The Haarvenu
152 The Thrafey
153 The Lyrezi
154 The Montalion
155 The Khulari
156 The Garlythi
157 The Anthotis
158 The Selenu
194 People of Glenumbra Moors
195 Court of Glenumbra Moors
196 Glenumbra Moors
198 People of Balfiera
199 Isle of Balfiera
200 Dwynnen
201 Daggerfall
202 Glenpoint
203 Betony
204 Sentinel
205 Anticlere
206 Lainlyn
207 Wayrest
208 Northmoor
209 Menevia
210 Alcaire
211 Koegria
212 Bhoraine
213 Kambria
214 Phrygia
215 Urvaius
216 Ykalon
217 Daenia
218 Shalgora
219 Abibon-Gora
220 Kairou
221 Pothago
222 Myrkwasa
223 Ayasofya
224 Tigonus
225 Kozanset
226 Satakalaam
227 Totambu
228 Mournoth
229 Ephesus
230 Santaki
231 Antiphyllos
232 Bergama
233 Gavaudon
234 Tulune
235 Ilessan Hills
236 Cybiades
240 Temple Missionaries
241 Teachers of Arkay
242 Random Noble
243 Teachers of z'en
244 Court of Balfiera
245 Teachers of Mara
246 Court of Lainlyn
247 Teachers of Akatosh
249 Teachers of Julianos
250 Teachers of Dibella
252 Teachers of Stendarr
254 Teachers of Kynareth
301 The Oracle
302 The Acolyte
303 Nulfaga
304 Skakmat
305 King of Worms
306 The Necromancers
350 The Septim Empire
351 The Great Knight
352 Lady Brisienna
353 The Underking
354 Agents of The Underking
355 Lord Harth
356 The Night Mother
357 Gortwog
358 Orsinium
359 Lord Plessington
360 Lord Kilbar
361 Chulmore Quill
362 The Quill Circus
363 Medora
364 King Gothryd
365 Queen Aubk-i
366 Mynisera
367 Lord Bridwell
368 The Knights of the Dragon
369 Popudax
370 Lord Coulder
371 Mobar
372 The Royal Guard
373 The Crow
374 Thyr Topfield
375 Lord Bertram Spode
376 Baltham Greyman
377 Lady Bridwell
378 Sylch Greenwood
379 Baron Shrike
380 Queen Akorithi
381 Prince Greklith
382 Prince Lhotun
383 Lord Vhosek
384 The Royal Guards
385 Charvek-si
386 Lord K'avar
387 Lord Provlith
388 Thaik
389 Whitka
390 King Eadwyre
391 Queen Barenziah
392 Princess Elysana
393 Prince Helseth
394 Princess Morgiah
395 Lord Castellian
396 Karethys
397 Lord Darkworth
398 Lord Woodborne
399 The Squid
400 Lady Doryanna Flyte
401 Lord Auberon Flyte
402 Lord Quistley
403 Farrington
404 Lord Perwright
405 Baroness Dh'emka
406 Lord Khane
407 Br'itsa
408 The Order of the Candle
409 The Knights of the Rose
410 The Knights of the Flame
411 The Host of the Horn
412 The Host of the True Horn
413 The Knights of the Owl
414 The Order of the Raven
415 The Knights of the Wheel
416 The Order of the Scarab
417 The Knights of the Hawk
418 The Order of the Cup
419 The Glenmoril Witches
420 The Dust Witches
421 The Witches of Devilrock
422 The Tamarilyn Witches
423 The Sisters of the Bluff
424 The Daughters of Wroth
425 The Skeffington Witches
426 The Witches of the Marsh
427 The Mountain Witches
428 The Daggerfall Witches
429 The Beldama
430 The Sisters of Kykos
431 The Tide Witches
432 The Witches of Alcaire
450 Generic Temple
453 Apothecaries of Arkay
454 Mixers of Arkay
455 Binders of Arkay
456 Summoners of Arkay
462 Apothecaries of Z'en
463 Mixers of Z'en
464 Summonists of Z'en
468 Apothecaries of Mara
469 Mixers of Mara
470 Summoners of Mara
473 Apothecaries of Akatosh
474 Mixers of Akatosh
475 Summoners of Akatosh
480 Crafters of Julianos
481 Smiths of Julianos
482 Summoners of Julianos
485 Apothecaries of Dibella
487 Mixers of Dibella
488 Summoners of Dibella
490 Apothecaries of Stendarr
491 Mixers of Stendarr
492 Summoners of Stendarr
496 Enchanters of Kynareth
497 Spellsmiths of Kynareth
498 Summoners of Kynareth
499 Court of Wayrest
500 Cyndassa
501 Lady Northbridge
502 Wrothgaria
503 Court of Wrothgaria
504 People of Wrothgaria
505 Dragontail
506 Court of Dragontail
507 People of Dragontail
508 Alik'ra
509 Court of Alik'ra
510 The Merchants
511 The Bards
512 The Prostitutes
513 The Fey
514 Children
515 Dancers
516 Court of Dwynnen
517 People of Dwynnen
518 People of Daggerfall
519 Court of Betony
520 People of Betony
521 Court of Glenpoint
522 People of Glenpoint
523 People of Sentinel
524 People of Anticlere
525 People of Lainlyn
526 People of Wayrest
527 Court of Northmoor
528 People of Northmoor
529 Court of Menevia
530 People of Menevia
531 Court of Alcaire
532 People of Alcaire
533 Court of Koegria
534 People of Koegria
535 Court of Bhoraine
536 People of Bhoraine
537 Court of Kambria
538 People of Kambria
539 Court of Phrygia
540 People of Phrygia
541 Court of Urvaius
542 People of Urvaius
543 Court of Ykalon
544 People of Ykalon
545 Court of Daenia
546 People of Daenia
547 Court of Koegria
548 People of Koegria
549 Court of Abibon-Gora
550 People of Abibon-Gora
551 Court of Kairou
552 People of Kairou
553 Court of Pothago
554 People of Pothago
555 Court of Myrkwasa
556 People of Myrkwasa
557 Court of Ayasofya
558 People of Ayasofya
559 Court of Tigonus
560 People of Tigonus
561 Court of Kozanset
562 People of Kozanset
563 Court of Satakalaam
564 People of Satakalaam
565 Court of Totambu
566 People of Totambu
567 Court of Mournoth
568 People of Mournoth
569 Court of Ephesus
570 People of Ephesus
571 Court of Santaki
572 People of Santaki
573 Court of Antiphyllos
574 People of Antiphyllos
575 Court of Bergama
576 People of Bergama
577 Court of Gavaudon
578 People of Gavaudon
579 Court of Tulune
580 People of Tulune
581 Court of Ilessen Hills
582 People of Ilessen Hills
583 Court of Cybiades
584 People of Cybiades
590 People of Alik'ra
591 Dak'fron
592 Court of Dak'fron
593 People of Dak'fron
595 Court of Daggerfall
596 Court of Sentinel
597 Court of Anticlere
598 Court of Orsinium
599 People of Orsinium
801 The Odylic Mages
802 The Crafters
803 The Shadow Trainers
804 The Shadow Schemers
805 The Shadow Appraisers
806 The Shadow Spies
807 Dark Slayers
810 Temple Treasurers
811 Temple Blessers
813 Temple Healers
839 Dark Trainers
840 Dark Mixers
841 Venom Masters
842 Dark Plotters
843 Dark Binders
844 Generic Knightly Order
845 Smiths
846 Questers
847 Healers
848 Seneschal
849 Fighter Trainers
850 Fighter Equippers
851 Fighter Questers
852 Random Ruler
853 Random Knight
977 Secret of Oblivion)";

  return parseIdNameTable(raw);
}

QString DaggerfallBioCodes::factionName(int factionId)
{
  return knownFactionNames().value(factionId);
}

#ifndef TGAMEDETAILS_H
#define TGAMEDETAILS_H
/***************************************************************************
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "utils.h" // For qsl(...)

#include "pre_guard.h"
#include <QString>
#include <QList>
#include "post_guard.h"

struct GameDetail
{
    QString name;
    QString hostUrl;
    int port = 0;
    bool tlsEnabled = false;
    QString websiteInfo;
    QString icon;
    QString description;
};

class TGameDetails
{
public:
    inline static const QList<GameDetail>::const_iterator findGame(const QString& key)
    {
        QList<GameDetail>::const_iterator i;
        for (i = scmDefaultGames.constBegin(); i != scmDefaultGames.constEnd(); ++i) {
            if ((*i).name == key) {
                return i;
            }
        }
        // Return the past-the-end iterator if not found:
        return i;
    }

    inline static const QStringList keys()
    {
        QStringList result;
        for (auto i = scmDefaultGames.constBegin(); i != scmDefaultGames.constEnd(); ++i) {
            result << (*i).name;
        }
        return result;
    }

    // clang-format off
    // games are to be added here in alphabetical order
    inline static const QList<GameDetail> scmDefaultGames = {
            {qsl("Avalon.de"), // Name
             qsl("avalon.mud.de"), // address to connect to
             23, // port to connect on
             false, // secure connection possible?
             qsl("<a href='http://avalon.mud.de'>http://avalon.mud.de</a>"), // Website or other URLs
             qsl(":/icons/avalon.png"), // path to the profile icon
             QString()}, // Text to use for description

            {qsl("Achaea"),
             qsl("achaea.com"),
             23,
             false,
             qsl("<a href='http://www.achaea.com/'>http://www.achaea.com</a>"),
             qsl(":/icons/achaea_120_30.png"),
             QString()},

            {qsl("3Kingdoms"),
             qsl("3k.org"),
             3000,
             false,
             qsl("<a href='http://www.3k.org/'>http://www.3k.org</a>"),
             qsl(":/icons/3klogo.png"),
             qsl("Simple enough to learn, yet complex enough to challenge you for years, 3Kingdoms "
                 "is a colossal adventure through which many years of active and continued "
                 "development by its dedicated coding staff.  Based around the mighty town of "
                 "Pinnacle, three main realms beckon the player to explore. These kingdoms are known "
                 "as: Fantasy, a vast medieval realm full of orcs, elves, dragons, and a myriad of "
                 "other creatures; Science, a post-apocalyptic, war-torn world set in the not-so-"
                 "distant future; and Chaos, a transient realm where the enormous realities of "
                 "Fantasy and Science collide to produce creatures so bizarre that they have yet to "
                 "be categorized.  During their exploration of the realms, players have the "
                 "opportunity to join any of well over a dozen different guilds, which grant "
                 "special, unique powers to the player, furthering their abilities as they explore "
                 "the vast expanses of each realm. Add in the comprehensive skill system that 3K "
                 "offers and you are able to extensively customize your characters.")},

            {qsl("3Scapes"),
             qsl("3k.org"),
             3200,
             false,
             qsl("<a href='http://www.3scapes.org/'>http://www.3scapes.org</a>"),
             qsl(":/icons/3slogo.png"),
             qsl("3Scapes is an alternative dimension to 3Kingdoms, similar in many respects, but "
                 "unique and twisted in so many ways.  3Scapes offers a faster pace of play, along "
                 "with an assortment of new guilds, features, and areas.")},

            {qsl("Lusternia"),
             qsl("lusternia.com"),
             23,
             false,
             qsl("<a href='http://www.lusternia.com/'>http://www.lusternia.com</a>"),
             qsl(":/icons/lusternia_120_30.png"),
             QString()},

            {qsl("BatMUD"),
             qsl("batmud.bat.org"),
             23,
             false,
             qsl("<a href='http://www.bat.org'>http://www.bat.org</a>"),
             qsl(":/icons/batmud_mud.png"),
             QString()},

            {qsl("God Wars II"),
             qsl("godwars2.org"),
             3000,
             false,
             qsl("<a href='http://www.godwars2.org'>http://www.godwars2.org</a>"),
             qsl(":/icons/gw2.png"),
             qsl("God Wars II is a fast and furious combat mud, designed to test player skill in "
                 "terms of pre-battle preparation and on-the-spot reflexes, as well as the ability "
                 "to adapt quickly to new situations. Take on the role of a godlike supernatural "
                 "being in a fight for supremacy.\n\nRoomless world. Manual combat. Endless "
                 "possibilities.")},

            {qsl("Slothmud"),
             qsl("slothmud.org"),
             6101,
             false,
             qsl("<a href='http://www.slothmud.org/'>http://www.slothmud.org/</a>"),
             qsl(":/icons/Slothmud.png"),
             qsl("SlothMUD... the ultimate in DIKUMUD! The most active, intricate, exciting FREE MUD "
                 "of its kind. This text based multiplayer free online rpg game and is enjoyed "
                 "continuously by players worldwide. With over 27,500 uniquely described rooms, "
                 "9,300 distinct creatures, 14,200 characters, and 87,100 pieces of equipment, "
                 "charms, trinkets and other items, our online rpg world is absolutely enormous and "
                 "ready to explore.")},

            {qsl("Aardwolf"),
             qsl("aardmud.org"),
             4000,
             false,
             qsl("<a href='http://www.aardwolf.com/'>http://www.aardwolf.com</a>"),
             qsl(":/icons/aardwolf_mud.png"),
             QString()},

            {qsl("Materia Magica"),
             qsl("materiamagica.com"),
             23,
             false,
             qsl("<a href='http://www.materiamagica.com'>http://www.materiamagica.com</a>"),
             qsl(":/materiaMagicaIcon"),
             QString()},

            {qsl("Mudren"),
             qsl("mud.ren"),
             6666,
             false,
             qsl("<a href='https://mud.ren/'>https://mud.ren/</a>"),
             qsl(":/icons/mudren.png"),
             /* English translation (courtesy of Google NOT the originator):
 *           "The world is out of my generation."
 *           "Emperor Tu Baye talked about laughter, it was drunk in life."
 *           "The sword rides on the ride, and the white bone is like a mountain bird to fly."
 *           "The dust is like a tide, and only a few people in the rivers and lakes."
 *           "\n\n"
 *           "Chinese open source martial arts MUD Yan Huang Qunxia biography, the game includes 25 masters and 5 major families. Laughing, grudge."
 */
             qsl("天下风云出我辈，一入江湖岁月催。\n"
                 "皇图霸业谈笑中，不胜人生一场醉。\n"
                 "提剑跨骑挥鬼雨，白骨如山鸟惊飞。\n"
                 "尘事如潮人如水，只叹江湖几人回。"
                 "\n\n"
                 "中文开源武侠MUD炎黄群侠传，游戏包括25大门派和5大世家，正邪只在一念间；近千门武学等你学习，上百种任务随你体验；让自己成为一代宗师，江湖笑，恩怨了。")},

            {qsl("Realms of Despair"),
             qsl("realmsofdespair.com"),
             4000,
             false,
             qsl("<a href='http://www.realmsofdespair.com/'>http://www.realmsofdespair.com</a>"),
             qsl(":/icons/120x30RoDLogo.png"),
             qsl("The Realms of Despair is the original SMAUG MUD and is FREE to play. We have an "
                 "active Roleplaying community, an active player-killing (deadly) community, and "
                 "a very active peaceful community. Players can choose from 13 classes (including "
                 "a deadly-only class) and 13 races. Character appearances are customizable on "
                 "creation and we have a vast collection of equipment that is level, gender, "
                 "class, race and alignment specific. We boast well over 150 original, exclusive "
                 "areas, with a total of over 20,000 rooms. Mob killing, or 'running' is one of "
                 "our most popular activities, with monster difficulties varying from easy one-"
                 "player kills to difficult group kills. We have four deadly-only Clans, twelve "
                 "peaceful-only Guilds, eight Orders, and fourteen Role-playing Nations that "
                 "players can join to interact more closely with other players. We have two mortal "
                 "councils that actively work toward helping players: The Symposium hears ideas "
                 "for changes, and the Newbie Council assists new players. Our team of Immortals "
                 "are always willing to answer questions and to help out however necessary. Best "
                 "of all, playing the Realms of Despair is totally FREE!")},

            {qsl("ZombieMUD"),
             qsl("zombiemud.org"),
             3000,
             false,
             qsl("<a href='http://www.zombiemud.org/'>http://www.zombiemud.org</a>"),
             qsl(":/icons/zombiemud.png"),
             qsl("Since 1994, ZombieMUD has been on-line and bringing orc-butchering fun to the "
                 "masses from our home base in Oulu, Finland. We're a pretty friendly bunch, with "
                 "players logging in from all over the globe to test their skill in our medieval "
                 "role-playing environment. With 15 separate guilds and 41 races to choose from, "
                 "as a player the only limitation to your achievements on the game is your own "
                 "imagination and will to succeed.")},

            {qsl("Aetolia"),
             qsl("aetolia.com"),
             23,
             false,
             qsl("<a href='http://www.aetolia.com/'>http://www.aetolia.com</a>"),
             qsl(":/icons/aetolia_120_30.png"),
             QString()},

            {qsl("Imperian"),
             qsl("imperian.com"),
             23,
             false,
             qsl("<a href='http://www.imperian.com/'>http://www.imperian.com</a>"),
             qsl(":/icons/imperian_120_30.png"),
             QString()},

            {qsl("WoTMUD"),
             qsl("game.wotmud.org"),
             2224,
             false,
             qsl("<a href='http://www.wotmud.org/'>Main website</a><br>"
                 "<a href='http://www.wotmod.org/'>Forums</a>"),
             qsl(":/icons/wotmudicon.png"),
             qsl("WoTMUD is the most popular on-line game based on the late Robert Jordan's epic "
                 "Wheel of Time fantasy novels."
                 "\n\n"
                 "Not only totally FREE to play since it started in 1993 it was officially "
                 "sanctioned by the Author himself."
                 "\n\n"
                 "Explore a World very like that of Rand al'Thor's; from the Blight in the North "
                 "down to the Isle of Madmen far, far south."
                 "\n\n"
                 "Wander around in any of the towns from the books such as Caemlyn, Tar Valon or "
                 "Tear, or start your adventure in the Two Rivers area, not YET the home of the "
                 "Dragon Reborn."
                 "\n\n"
                 "Will you join one of the Clans working for the triumph of the Light over the "
                 "creatures and minions of the Dark One; or will you be one of the returning "
                 "invaders in the South West, descendants of Artur Hawkwing's long-thought lost "
                 "Armies; or just maybe you are skilled enough to be a hideous Trolloc, creature of "
                 "the Dark, who like Humans - but only as a source of sustenance."
                 "\n\n"
                 "Very definitely a Player Verses Player (PvP) world but with strong Role Playing "
                 "(RP) too; nowhere is totally safe but some parts are much more dangerous than "
                 "others - once you enter you may never leave...")},

            {qsl("Midnight Sun 2"),
             qsl("midnightsun2.org"),
             3000,
             false,
             qsl("<a href='http://midnightsun2.org/'>http://midnightsun2.org/</a>"),
             qsl(":/icons/midnightsun2.png"),
             qsl("Midnight Sun is a medieval fantasy LPmud that has been around since 1991. We are a "
                 "non-PK, hack-and-slash game, cooperative rather than competitive in nature, and "
                 "with a strong sense of community.")},

            {qsl("Luminari"),
             qsl("luminarimud.com"),
             4100,
             false,
             qsl("<a href='http://www.luminarimud.com/'>http://www.luminarimud.com/</a>"),
             qsl(":/icons/luminari_icon.png"),
             qsl("Luminari is a deep, engaging game set in the world of the Luminari - A place where "
                 "magic is entwined with the fabric of reality and the forces of evil and "
                 "destruction are rising from a long slumber to again wreak havoc on the realm.  "
                 "The gameplay of Luminari will be familiar to anyone who has played Dungeons and "
                 "Dragons, Pathfinder or any of the many RPG systems based on the d20 ruleset.")},

            {qsl("StickMUD"),
             qsl("stickmud.com"),
             7670,
             true,
             qsl("<a href='http://www.stickmud.com/'>stickmud.com</a>"),
             qsl(":/icons/stickmud_icon.jpg"),
             qsl("StickMUD is a free, medieval fantasy game with a graphical user interface and a "
                 "depth of features. You are welcomed into the game world with maps and dashboards "
                 "to complement your imagination. Newbies escape quickly into game play with minimal "
                 "study time. Awaken under the wondrous Mallorn Tree in the center of Newbie Park "
                 "and learn by playing. Challenge non-player characters to gain experience, advance "
                 "level and maximize your stats. Between battles, sit on the enchanted bench under "
                 "the Tree to rapidly heal and reduce wait time. Signs in the park present game "
                 "features such as races, clans and guilds. Read up on teasers about the adventures "
                 "on the path ahead like dragons, castles and sailing. Join a guild and learn the "
                 "ways of a Bard, Fighter, Mage, Necromancer, Ninja, Thief, Healer or Priest. Train "
                 "skills in both craft and combat aligned with your guild. Participate in frequent "
                 "game-wide events to earn points exchanged for gold, experience or skill training. "
                 "Heroes and villains alike are invited! Role play is optional and player vs. player "
                 "combat is allowed in much of the game. StickMUD was born in Finland in June 1991 "
                 "and is now hosted in Canada. Our diverse community of players and active game "
                 "engineers are ready to welcome new players like you to one of the best text-based "
                 "multi-player games ever!")},

            {qsl("Clessidra"),
             qsl("mud.clessidra.it"),
             4000,
             false,
             qsl("<a href='http://www.clessidra.it/'>http://www.clessidra.it</a>"),
             qsl(":/icons/clessidra.jpg"),
             /* English translation, provided by Game:
 *           "Clessidra is the first all italian MUD ever created! On Clessidra you may find "
 *           "only original Areas, all in italian! Many features make Clessidra one of the best, "
 *           "or the best MUD in Italy: Advanced travel mode, fight one to one versus your "
 *           "friend, or enemy, The Arena and its fight, the Mortal Challenge, the intelligent "
 *           "MOBs and their Quest and fighting style, a random automatic mission assignament "
 *           "and for you and your friends you must try the advanced Clan system that allows "
 *           "wars and conquest. A mercenary system to help playing when few players are online, "
 *           "a crafting system to create special object and a graphical user interface to help "
 *           "newbie and expert players have a better experience. A MUD that evolves with new "
 *           "challenge, new rules, new skills!"
 */
             qsl("Clessidra e' il primo MUD completamente in italiano mai creato. Su Clessidra "
                 "potrete trovare solo aree originali ed in italiano. Molte caratteristiche rendono "
                 "Clessidra uno dei migliori, se non il migliore, MUD in Italia : Avanzati sistemi "
                 "di spostamento, sfide uno-contro-uno contro gli amici, o i nemici, L'arena e i "
                 "combattimenti, Le sfide all'ultimo sangue e i MOB intelligenti con le loro Quest e "
                 "tecniche di combattimento, un sistema di assegnazione di missioni casuali e un "
                 "avanzatissimo sistema di Clan che permettera' guerre e conquiste. Disponibilità di "
                 "mercenari in caso di poca utenza, sistema di produzione/mercato per ottenere "
                 "esclusivi oggetti, un interfaccia grafica per aiutarti a giocare, sia per i novizi "
                 "che gli esperti. Un MUD che si evolve di continuo.")},

            {qsl("Reinos de Leyenda"),
             qsl("reinosdeleyenda.es"),
             23,
             false,
             qsl("<a href='https://www.reinosdeleyenda.es/'>Sitio web principal</a><br>"
                 "<a href='https://www.reinosdeleyenda.es/foro/'>Foros</a><br>"
                 "<a href='https://wiki.reinosdeleyenda.es/'>Wiki</a>"),
             qsl(":/icons/reinosdeleyenda_mud.png"),
             /* English translation, provided by Game:
 *           "The oldest Spanish free mud with more than 20 years of running history."
 *           "\n\n"
 *           "Reinos de Leyenda takes place in the ever changing world of Eirea, ravaged by the "
 *           "mischiefs of the gods after more than a thousand years of contempt and hideous war "
 *           "amongst their zealous mortal pawns."
 *           "\n\n"
 *           "History is written on a day per day basis, taking into consideration the players' "
 *           "choices to decide the irreversible aftermath of this everlasting struggle."
 *           "\n\n"
 *           "This is a PvP MUD which allows the player to set how high are the stakes: the more "
 *           "you risk losing upon death, the more glory to be earned by your heroism. RP, while "
 *           "not enforced, is rewarded with non-PvP oriented perks and unique treasure."
 *           "\n\n"
 *           "A powerful character customization system allows you to choose your deity –or "
 *           "fully disregard the gods– and join one of the player-run realms that govern the "
 *           "land to explore a breathing world, delve into the secrets of the oceans, shape "
 *           "your legacy, craft forgotten marvels for you –or your allies– and fight for faith, "
 *           "glory or coin."
 */
             qsl("El mud Español gratis con más de 20 años de historia."
                 "\n\n"
                 "Reinos de Leyenda toma lugar en el siempre cambiante mundo de Eirea, devastado por "
                 "las intrigas de los dioses tras más de un millar de años de desprecio y cruenta "
                 "guerra entre sus fanáticos peones mortales."
                 "\n\n"
                 "La historia se escribe día a día, tomando en consideración las elecciones de los "
                 "jugadores para decidir las consecuencias irreversibles de este conflicto "
                 "imperecedero."
                 "\n\n"
                 "Éste es un MUD con PvP que permite al jugador establecer cuánto quiere arriesgar "
                 "al morir: a más riesgo, más gloria ganará por sus heroicidades. La interpretación "
                 "(Rol) no está obligada, pero si recompensada con habilidades especiales -no "
                 "orientadas al combate- y tesoros únicos."
                 "\n\n"
                 "El detallado creador del juego te permitirá elegir tu deidad -o renegar "
                 "completamente de los dioses- y unirte a uno de los reinos que los jugadores se "
                 "encargan de gobernar para explorar un mundo viviente, sumergirte en los misterios "
                 "del océano, dar forma a tu legado, forjar maravillas olvidadas para ti -o tus "
                 "aliados- y luchar por fe, gloria o dinero.")},

            {qsl("Fierymud"),
             qsl("fierymud.org"),
             4000,
             false,
             qsl("<a href='https://www.fierymud.org/'>https://www.fierymud.org</a>"),
             qsl(":/icons/fiery_mud.png"),
             qsl("The original vision of FieryMUD was to create a challanging MUD for advanced "
                 "players. This new reborne Fiery is a hope to bring back the goals of the past by "
                 "inflicting certain death on unsuspecting players. FieryMUD will continue to grow "
                 "and change through the coming years and those players who seek challenge and "
                 "possess imagination will come in search of what the 3D world fails to offer them.")},

            {qsl("Mudlet self-test"),
             qsl("mudlet.org"),
             23,
             false,
             qsl("<a href='https://www.mudlet.org'>www.mudlet.org"),
             QString(), // TODO: https://github.com/Mudlet/Mudlet/issues/6443
             qsl("This isn't a game profile, but a special one for testing Mudlet itself using "
                 "Busted. You can also use it as a starting point to create automated tests for your "
                 "own profiles!")},

            {qsl("Carrion Fields"),
             qsl("carrionfields.net"),
             4449,
             false,
             qsl("<a href='http://www.carrionfields.net'>www.carrionfields.net</a>"),
             qsl(":/icons/carrionfields.png"),
             qsl("Carrion Fields is a unique blend of high-caliber roleplay and complex, hardcore "
                 "player-versus-player combat that has been running continuously, and 100% free, "
                 "for over 25 years."
                 "\n\n"
                 "Choose from among 21 races, 17 highly customizable classes, and several cabals "
                 "and religions to suit your playstyle and the story you want to tell. Our "
                 "massive, original world is full of secrets and envied limited objects that take "
                 "skill to acquire and great care to keep."
                 "\n\n"
                 "We like to think of ourselves as the Dark Souls of MUDs, with a community that "
                 "is supportive of new players - unforgiving though our world may be. Join us for a "
                 "real challenge and real rewards: adrenalin-pumping battles, memorable quests run "
                 "by our volunteer immortal staff, and stories that will stick with you for a "
                 "lifetime.")},

            {qsl("Cleft of Dimensions"),
             qsl("cleftofdimensions.net"),
             4354,
             false,
             qsl("<a href='https://www.cleftofdimensions.net/'>cleftofdimensions.net</a><br>"
                 "<a href='https://discord.gg/cSqkpbu'>Discord Guild</a>"),
             qsl(":/icons/cleftofdimensions.png"),
             qsl("Do you have a soft spot for an old SNES RPG? Are you a fan of retro gaming? The "
                 "Cleft of Dimensions is an adventure-driven MUD with content inspired by a variety "
                 "of classic video games. Do you want to jump on goombas? Maybe you'd rather "
                 "immolate them with lava or bombard them with meteors. Then again, why fight when "
                 "enslavement's an option? If that doesn't work out, you've got this motorcycle you "
                 "could crash into them. The Cleft has 16 character classes, each with a "
                 "distinctive playstyle."
                 "\n\n"
                 "Gameplay in the Cleft features exploration, puzzles, quests, and combat. At time "
                 "of writing, the world contains 98 areas. Quests range from deciphering treasure "
                 "maps and committing industrial espionage to seeking the blessings of the mana "
                 "spirits or just going fishing. A remort system facilitates repeat playthroughs to "
                 "find content you missed the first time around."
                 "\n\n"
                 "The Cleft opened in July 2000 and has been in active development ever since. We're "
                 "always innovating. Recent features include Discord integration "
                 "and areas written with artificial intelligence. Check us out!")},

            {qsl("Legends of the Jedi"),
             qsl("legendsofthejedi.com"),
             5656,
             false,
             qsl("<a href='https://www.legendsofthejedi.com/'>legendsofthejedi.com</a>"),
             qsl(":/icons/legendsofthejedi_120x30.png"),
             qsl("Legends of the Jedi is a text-based roleplaying experience that immerses players "
                 "in a multiplayer world where they can rewrite classic Star Wars stories with their "
                 "own heroes, villains, battles, and endings. Over the course of each two-year "
                 "timeline, the game explores all the key eras of the Star Wars Expanded Universe."
                 "\n\n"
                 "Take and hold planets as an Imperial Stormtrooper, command the Rebel navy and "
                 "liberate the galaxy, pursue targets as a bounty hunter, or shape things on a "
                 "larger scale as a member of the Galactic Senate. Maybe you'll even be one of the "
                 "few born with force sensitivity, destined to be trained by Jedi or Sith."
                 "\n\n"
                 "The game offers an extensive crafting system for engineers to supply weapons, "
                 "armor, and ships to the galaxy. Develop new, cutting-edge armaments to give your "
                 "side an edge, or open a shop in a bustling commercial district and become wealthy "
                 "as part of a powerful engineering conglomerate."
                 "\n\n"
                 "LOTJ offers full PVP in both ground and space combat, governed by a set of rules to "
                 "minimize griefing and ensure that all kills have sufficient in-character cause."
                 "\n\n"
                 "What role will you play? The legend awaits!")},

            {qsl("CoreMUD"),
             qsl("coremud.org"),
             4020,
             true,
             qsl("<a href='https://coremud.org/'>coremud.org</a>"),
             qsl(":/icons/coremud_icon.jpg"),
             qsl("Welcome to Core Mud, an interactive text MUD set on the planet formal star-charts "
                 "refer to as Hermes 571-G, but that everyone in the know refers to simply as \"Core\"."
                 "\n\n"
                 "Core is one of the most distant settlements known to mankind, most famous for its "
                 "lucrative yet oppressive mines, but more than mankind can be found here..."
                 "\n\n"
                 "Core is a diverse group of 9 races in total, all vying for recognition or profits, "
                 "or both, working for The Company, the megalithic entity running the colony itself."
                 "\n\n"
                 "To The Company, everything is secondary to profits."
                 "\n\n"
                 "It is up to you to determine how best to survive in this environment, whether that "
                 "be through combat training, superior mining skills, or technical prowess."
                 "\n\n"
                 "Core MUD is always free to play and features a fun and supportive atmosphere. "
                 "Roleplaying is encouraged but not mandatory."
                 "\n\n"
                 "Mining is your primary source of income, but there are multiple ways to scrape "
                 "together a few credits... or a few million."
                 "\n\n"
                 "Core Mud also features an economy which is player-driven.  Players own "
                 "merchandise shops featuring energy weaponry or useful tools, pubs featuring "
                 "assorted alcoholic (of course) and non-alcoholic beverages, and clinics for "
                 "healing, to name a few."
                 "\n\n"
                 "Come join us today!")},

            {qsl("Multi-Users in Middle-earth"),
             qsl("mume.org"),
             4242,
             true,
             qsl("<a href='https://mume.org/'>mume.org</a>"),
             qsl(":/icons/mume.png"),
             qsl("Multi-Users in Middle-earth (MUME) is a highly competitive world PvP DikuMUD, set "
                 "in J. R. R. Tolkien’s fictional world of Middle-earth, as described in The Hobbit "
                 "and The Lord of the Rings, where players may choose to join the epic war between "
                 "the forces of Sauron and the armies of the Free peoples. In MUME players can "
                 "explore, role-play, acquire achievements, and complete quests across many "
                 "challenging locations across Middle-earth such as Lothlórien, the Shire, Bree, "
                 "Rivendell, Goblin-town, Mirkwood, Dol Guldur, and the Mines of Moria. The game is "
                 "completely at no cost to play and has been continually enhanced since its "
                 "inception in the fall of 1991.")},
            };
    // clang-format on
};

#endif // TGAMEDETAILS_H

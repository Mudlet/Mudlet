/*
 * Copyright (C) 2008-2016 The Communi Project
 *
 * This test is free, and not covered by the BSD license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#ifndef TST_EUIRC_H
#define TST_EUIRC_H

static const char* euirc_welcome =
        ":irc.rbx.fr.euirc.net 001 communi :Welcome to the euIRCnet IRC Network communi!~communi@hidd.en\n"
        ":irc.rbx.fr.euirc.net 002 communi :Your host is irc.rbx.fr.euirc.net, running version euIRCd 1.3.4-e544f33+debug\n"
        ":irc.rbx.fr.euirc.net 003 communi :This server was created Mon Jul 08 2013 at 07:09:56 CEST\n"
        ":irc.rbx.fr.euirc.net 004 communi irc.rbx.fr.euirc.net euIRCd 1.3.4-e544f33+debug oOiRwhsSaHANCrxjWqBZ1dtcpPUnTI vhoaqpsmtinrRQKVHOAYCNcSXUTxW5beIwklfLBuFM\n"
        ":irc.rbx.fr.euirc.net 005 communi NETWORK=euIRCnet WATCH=128 SAFELIST PREFIX=(qaohv)*!@%+ CHANMODES=bewI,k,flBL,cimnprstxACHKNOQRiTSVWXY5 CHANTYPES=#&+ KICKLEN=307 KNOCK MAP MAXLIST=bewI:100 MODES=6 NICKLEN=30 SILENCE=5 TOPICLEN=307 AWAYLEN=307 WALLCHOPS CHANNELLEN=32 MAXCHANNELS=30 MAXTARGETS=20 INVEX=I EXCEPT=e :are available on this server\n"
        ":irc.rbx.fr.euirc.net 005 communi STARTTLS :are available on this server\n"
        ":irc.rbx.fr.euirc.net 251 communi :There are 14 users and 2768 invisible on 10 servers\n"
        ":irc.rbx.fr.euirc.net 252 communi 24 :operator(s) online\n"
        ":irc.rbx.fr.euirc.net 253 communi 1 :unknown connection(s)\n"
        ":irc.rbx.fr.euirc.net 254 communi 1671 :channels formed\n"
        ":irc.rbx.fr.euirc.net 255 communi :I have 1060 clients and 1 servers\n"
        ":irc.rbx.fr.euirc.net 265 communi :Current Local Users: 1060  Max: 1221\n"
        ":irc.rbx.fr.euirc.net 266 communi :Current Global Users: 2782  Max: 11082\n"
        ":irc.rbx.fr.euirc.net 375 communi :- irc.rbx.fr.euirc.net Message of the Day -\n"
        ":irc.rbx.fr.euirc.net 376 communi :End of /MOTD command\n";

static const char* euirc_join =
        ":communi!~communi@hidd.en JOIN :#euirc\n"
        ":irc.rbx.fr.euirc.net 332 communi #euirc :Welcome to euIRC || Problems? Join #opers || www.euirc.net || SSL and S/MIME authentication now available on all servers (port 6697) || euIRC meets facebook: www.facebook.com/euirc\n"
        ":irc.rbx.fr.euirc.net 333 communi #euirc Renne 1370272649\n"
        ":irc.rbx.fr.euirc.net 353 communi = #euirc :communi Guest774 burning_rabbit %aleksandr netsplit Jerry Brueggus charly6 HermiNe %brue Mercutio stephan48 NeinnHomer Luthandorius Technomagier |Baron| Laknu_ Polizist1 !alamar rhonabwy @Vampi Mayday @Road radic MorkiTorki PRoTaGoNiST Simik|ZzZZ @][flat][ statsbot7 !mensch holygoth firefly Kanibal Luchs xinator CR|Noah|Away MrWolf !specon CR|Dani %leni CR|Sven @Renne Revi Arovin pinGUUin Vampire666 gastgast_\n"
        ":irc.rbx.fr.euirc.net 353 communi = #euirc :konqui Tina-chan_onAir picoFF @medice SLXViper !TC sb Kn0p3XX SlySing faZe Icedream Der_Orwischer Goggy g00fy Burle klaxa [Chaos|Krieger] Guest14697 Tehlak icefly Herr_Vorragend Ding Nothing4You Sven|Off Zarquod !jun|per scaba meister Hikaru-Shindo Alx Kinji-san Guest13553 !Chibisuke\n"
        ":irc.rbx.fr.euirc.net 366 communi #euirc :End of /NAMES list.\n";

static const char* euirc_names =
        "communi Guest774 burning_rabbit aleksandr netsplit Jerry Brueggus charly6 HermiNe brue Mercutio stephan48 NeinnHomer Luthandorius Technomagier |Baron| Laknu_ Polizist1 alamar rhonabwy Vampi Mayday Road radic MorkiTorki PRoTaGoNiST Simik|ZzZZ ][flat][ statsbot7 mensch holygoth firefly Kanibal Luchs xinator CR|Noah|Away MrWolf specon CR|Dani leni CR|Sven Renne Revi Arovin pinGUUin Vampire666 gastgast_ "
        "konqui Tina-chan_onAir picoFF medice SLXViper TC sb Kn0p3XX SlySing faZe Icedream Der_Orwischer Goggy g00fy Burle klaxa [Chaos|Krieger] Guest14697 Tehlak icefly Herr_Vorragend Ding Nothing4You Sven|Off Zarquod jun|per scaba meister Hikaru-Shindo Alx Kinji-san Guest13553 Chibisuke";

static const char* euirc_admins = "alamar mensch specon TC jun|per Chibisuke";
static const char* euirc_ops = "Vampi Road ][flat][ Renne medice";
static const char* euirc_halfops = "aleksandr brue leni";
static const char* euirc_voices = "";

#endif // TST_EUIRC_H

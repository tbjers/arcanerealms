-- phpMyAdmin SQL Dump
-- version 3.5.2.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Aug 06, 2012 at 04:09 AM
-- Server version: 5.1.61
-- PHP Version: 5.3.3

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `arcanere_g`
--

-- --------------------------------------------------------

--
-- Table structure for table `boot_index`
--

DROP TABLE IF EXISTS `boot_index`;
CREATE TABLE IF NOT EXISTS `boot_index` (
  `znum` bigint(20) unsigned NOT NULL,
  `mini` tinyint(1) unsigned DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `configuration`
--

DROP TABLE IF EXISTS `configuration`;
CREATE TABLE IF NOT EXISTS `configuration` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) DEFAULT NULL,
  `type` varchar(255) DEFAULT NULL,
  `field` text,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `cultures_list`
--

DROP TABLE IF EXISTS `cultures_list`;
CREATE TABLE IF NOT EXISTS `cultures_list` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `Abbreviation` varchar(255) DEFAULT NULL,
  `Name` varchar(255) DEFAULT NULL,
  `Selectable` enum('No','Yes') DEFAULT NULL,
  `Cost` int(11) DEFAULT NULL,
  `Description` text,
  `Const` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `Name` (`Name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Table structure for table `events`
--

DROP TABLE IF EXISTS `events`;
CREATE TABLE IF NOT EXISTS `events` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `title` varchar(255) NOT NULL,
  `short_desc` text NOT NULL,
  `long_desc` text NOT NULL,
  `startdate` bigint(20) NOT NULL,
  `enddate` bigint(20) NOT NULL,
  `assigned` varchar(255) NOT NULL,
  `active` tinyint(1) unsigned NOT NULL,
  `approved` tinyint(1) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `guild_index`
--

DROP TABLE IF EXISTS `guild_index`;
CREATE TABLE IF NOT EXISTS `guild_index` (
  `name` int(11) NOT NULL,
  `gossip_name` varchar(255) NOT NULL,
  `gl_title` varchar(255) NOT NULL,
  `guildie_titles` text NOT NULL,
  `type` int(10) unsigned NOT NULL DEFAULT '0',
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `flags` varchar(255) NOT NULL DEFAULT '0',
  `guildwalk_room` int(10) unsigned NOT NULL DEFAULT '0',
  `gold` int(10) unsigned NOT NULL DEFAULT '0',
  `description` text NOT NULL,
  `requirements` text NOT NULL,
  `gossip` varchar(255) NOT NULL,
  `gchan_name` varchar(255) NOT NULL,
  `gchan_color` varchar(255) NOT NULL,
  `gchan_type` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `help_categories`
--

DROP TABLE IF EXISTS `help_categories`;
CREATE TABLE IF NOT EXISTS `help_categories` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `rights` varchar(255) NOT NULL DEFAULT '0',
  `text` text NOT NULL,
  `sortorder` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `help_index`
--

DROP TABLE IF EXISTS `help_index`;
CREATE TABLE IF NOT EXISTS `help_index` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `menutitle` varchar(255) NOT NULL,
  `brief` varchar(255) NOT NULL,
  `syntax` varchar(255) NOT NULL,
  `modified` bigint(20) unsigned NOT NULL,
  `category` int(11) unsigned NOT NULL,
  `imm_info` varchar(255) NOT NULL,
  `see_also` varchar(255) NOT NULL,
  `body` varchar(255) NOT NULL,
  `rights` varchar(255) NOT NULL,
  `keyword` varchar(255) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Table structure for table `lib_text`
--

DROP TABLE IF EXISTS `lib_text`;
CREATE TABLE IF NOT EXISTS `lib_text` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `name` varchar(255) DEFAULT NULL,
  `text` text,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=23 ;

-- --------------------------------------------------------

--
-- Table structure for table `MCL`
--

DROP TABLE IF EXISTS `MCL`;
CREATE TABLE IF NOT EXISTS `MCL` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `command` varchar(255) NOT NULL,
  `sort_as` varchar(255) NOT NULL,
  `minimum_position` int(10) unsigned NOT NULL DEFAULT '0',
  `command_num` int(10) unsigned NOT NULL DEFAULT '0',
  `rights` varchar(255) NOT NULL DEFAULT '0',
  `subcmd` int(10) unsigned NOT NULL DEFAULT '0',
  `copyover` int(10) unsigned NOT NULL DEFAULT '0',
  `enabled` int(10) unsigned NOT NULL DEFAULT '0',
  `reserved` enum('No','Yes') NOT NULL DEFAULT 'No',
  PRIMARY KEY (`id`),
  UNIQUE KEY `command` (`command`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=245 ;

-- --------------------------------------------------------

--
-- Table structure for table `mob_index`
--

DROP TABLE IF EXISTS `mob_index`;
CREATE TABLE IF NOT EXISTS `mob_index` (
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `short_description` text NOT NULL,
  `long_description` text NOT NULL,
  `description` text NOT NULL,
  `flags` varchar(255) NOT NULL DEFAULT '0',
  `aff_flags` varchar(255) NOT NULL DEFAULT '0',
  `alignment` int(10) unsigned NOT NULL DEFAULT '0',
  `difficulty` int(10) unsigned NOT NULL DEFAULT '0',
  `hitroll` int(10) unsigned NOT NULL DEFAULT '0',
  `pd` int(10) unsigned NOT NULL DEFAULT '0',
  `hit` int(10) unsigned NOT NULL DEFAULT '0',
  `mana` int(10) unsigned NOT NULL DEFAULT '0',
  `move` int(10) unsigned NOT NULL DEFAULT '0',
  `dam_no_dice` int(10) unsigned NOT NULL DEFAULT '0',
  `dam_size_dice` int(10) unsigned NOT NULL DEFAULT '0',
  `gold` bigint(20) unsigned NOT NULL DEFAULT '0',
  `position` int(10) unsigned NOT NULL DEFAULT '0',
  `default_position` int(10) unsigned NOT NULL DEFAULT '0',
  `sex` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `attack_type` int(10) unsigned NOT NULL DEFAULT '0',
  `strength` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `unknown24` varchar(255) NOT NULL DEFAULT '0',
  `agility` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `precision` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `perception` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `health` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `willpower` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `intelligence` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `charisma` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `luck` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `essence` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `unknown34` varchar(255) NOT NULL DEFAULT '0',
  `race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `val0` int(10) unsigned NOT NULL DEFAULT '0',
  `val1` int(10) unsigned NOT NULL DEFAULT '0',
  `val2` int(10) unsigned NOT NULL DEFAULT '0',
  `val3` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `mob_tutor_data`
--

DROP TABLE IF EXISTS `mob_tutor_data`;
CREATE TABLE IF NOT EXISTS `mob_tutor_data` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `no_skill` text NOT NULL,
  `no_req` text NOT NULL,
  `skilled` text NOT NULL,
  `no_cash` text NOT NULL,
  `buy_success` text NOT NULL,
  `flags` varchar(255) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `obj_affects`
--

DROP TABLE IF EXISTS `obj_affects`;
CREATE TABLE IF NOT EXISTS `obj_affects` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `vnum` int(11) unsigned NOT NULL DEFAULT '0',
  `location` int(11) unsigned NOT NULL DEFAULT '0',
  `modifier` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `obj_extradescs`
--

DROP TABLE IF EXISTS `obj_extradescs`;
CREATE TABLE IF NOT EXISTS `obj_extradescs` (
  `id` int(10) unsigned NOT NULL,
  `znum` int(11) NOT NULL,
  `vnum` int(11) NOT NULL,
  `keyword` varchar(255) NOT NULL,
  `description` text NOT NULL,
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `obj_index`
--

DROP TABLE IF EXISTS `obj_index`;
CREATE TABLE IF NOT EXISTS `obj_index` (
  `znum` int(11) NOT NULL,
  `vnum` int(11) NOT NULL,
  `name` varchar(255) NOT NULL DEFAULT 'New Object',
  `short_description` text NOT NULL,
  `description` text NOT NULL,
  `action_description` text NOT NULL,
  `type` int(11) unsigned NOT NULL DEFAULT '0',
  `extra_flags` varchar(255) NOT NULL DEFAULT '0',
  `wear_flags` varchar(255) NOT NULL DEFAULT '0',
  `obj_flags` varchar(255) NOT NULL DEFAULT '0',
  `val0` int(11) NOT NULL DEFAULT '0',
  `val1` int(11) NOT NULL DEFAULT '0',
  `val2` int(11) NOT NULL DEFAULT '0',
  `val3` int(11) NOT NULL DEFAULT '0',
  `weight` int(11) unsigned NOT NULL DEFAULT '0',
  `cost` int(11) unsigned NOT NULL DEFAULT '0',
  `rent` int(11) unsigned NOT NULL DEFAULT '0',
  `unknown1` int(10) unsigned NOT NULL DEFAULT '0',
  `unknown2` int(10) unsigned NOT NULL DEFAULT '0',
  `size` int(11) unsigned NOT NULL DEFAULT '0',
  `color` int(11) unsigned NOT NULL DEFAULT '0',
  `resource` int(11) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`znum`,`vnum`),
  FULLTEXT KEY `action_description` (`action_description`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `pending_users`
--

DROP TABLE IF EXISTS `pending_users`;
CREATE TABLE IF NOT EXISTS `pending_users` (
  `PlayerID` bigint(20) unsigned NOT NULL,
  `Code` varchar(255) DEFAULT NULL,
  `Verified` tinyint(1) unsigned DEFAULT '0',
  PRIMARY KEY (`PlayerID`),
  UNIQUE KEY `Code` (`Code`),
  KEY `Code_2` (`Code`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `player_affects`
--

DROP TABLE IF EXISTS `player_affects`;
CREATE TABLE IF NOT EXISTS `player_affects` (
  `id` bigint(20) unsigned NOT NULL,
  `type` int(10) unsigned NOT NULL,
  `duration` int(10) unsigned NOT NULL,
  `modifier` int(10) unsigned NOT NULL,
  `location` int(10) unsigned NOT NULL,
  `bitvector` varchar(255) NOT NULL DEFAULT '0',
  KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `player_index`
--

DROP TABLE IF EXISTS `player_index`;
CREATE TABLE IF NOT EXISTS `player_index` (
  `Name` varchar(255) NOT NULL DEFAULT '',
  `Password` varchar(255) NOT NULL DEFAULT '',
  `Email` varchar(255) NOT NULL DEFAULT '',
  `Title` varchar(255) NOT NULL DEFAULT '',
  `Background` text NOT NULL,
  `Sex` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `Class` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Race` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Home` int(10) unsigned NOT NULL DEFAULT '0',
  `Birth` int(10) unsigned NOT NULL DEFAULT '0',
  `Played` bigint(20) unsigned NOT NULL DEFAULT '0',
  `Last` bigint(20) unsigned NOT NULL DEFAULT '0',
  `BirthYear` int(10) unsigned NOT NULL DEFAULT '0',
  `BirthMonth` int(10) unsigned NOT NULL DEFAULT '0',
  `BirthDay` int(10) unsigned NOT NULL DEFAULT '0',
  `BirthHours` int(10) unsigned NOT NULL DEFAULT '0',
  `Host` varchar(255) NOT NULL DEFAULT '',
  `Height` int(10) unsigned NOT NULL DEFAULT '0',
  `Weight` int(10) unsigned NOT NULL DEFAULT '0',
  `Align` int(10) unsigned NOT NULL DEFAULT '0',
  `ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Flags` bigint(20) unsigned NOT NULL DEFAULT '0',
  `Aff` varchar(255) NOT NULL DEFAULT '0',
  `SaveThrow1` int(10) unsigned NOT NULL DEFAULT '0',
  `SaveThrow2` int(10) unsigned NOT NULL DEFAULT '0',
  `SaveThrow3` int(10) unsigned NOT NULL DEFAULT '0',
  `SaveThrow4` int(10) unsigned NOT NULL DEFAULT '0',
  `SaveThrow5` int(10) unsigned NOT NULL DEFAULT '0',
  `Wimpy` int(10) unsigned NOT NULL DEFAULT '0',
  `Invis` varchar(255) NOT NULL DEFAULT '0',
  `Room` int(10) unsigned NOT NULL DEFAULT '0',
  `Preferences` varchar(255) NOT NULL DEFAULT '',
  `Badpws` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Hunger` int(10) unsigned NOT NULL DEFAULT '0',
  `Thirst` int(10) unsigned NOT NULL DEFAULT '0',
  `Drunk` int(10) unsigned NOT NULL DEFAULT '0',
  `Practices` int(10) unsigned NOT NULL DEFAULT '0',
  `Strength` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Agility` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Precision` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Perception` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Health` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Willpower` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Intelligence` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Charisma` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Luck` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Essence` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Hit` int(11) NOT NULL DEFAULT '0',
  `Mana` int(11) NOT NULL DEFAULT '0',
  `Move` int(11) NOT NULL DEFAULT '0',
  `MaxHit` int(10) unsigned NOT NULL DEFAULT '0',
  `MaxMana` int(10) unsigned NOT NULL DEFAULT '0',
  `MaxMove` int(10) unsigned NOT NULL DEFAULT '0',
  `AC` int(10) unsigned NOT NULL,
  `Gold` bigint(20) unsigned NOT NULL DEFAULT '0',
  `Bank` bigint(20) unsigned NOT NULL DEFAULT '0',
  `Exp` bigint(20) unsigned NOT NULL DEFAULT '0',
  `Hitroll` int(11) NOT NULL DEFAULT '0',
  `Damroll` int(11) NOT NULL DEFAULT '0',
  `CurrentQuest` int(11) NOT NULL DEFAULT '0',
  `NumAffects` int(11) NOT NULL DEFAULT '0',
  `NumQuests` int(11) NOT NULL DEFAULT '0',
  `Unknown63` int(11) NOT NULL,
  `NumSkills` int(11) NOT NULL DEFAULT '0',
  `Contact` varchar(255) NOT NULL DEFAULT '',
  `Active` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `Language` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `EyeColor` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `HairColor` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `HairStyle` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `SkinTone` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `OKtoMail` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `Perms` varchar(255) NOT NULL DEFAULT '0',
  `Syslog` varchar(255) NOT NULL DEFAULT '0',
  `TrueName` varchar(255) NOT NULL DEFAULT '',
  `Colorpref_echo` char(1) NOT NULL DEFAULT '',
  `Colorpref_emote` char(1) NOT NULL DEFAULT '',
  `Colorpref_pose` char(1) NOT NULL DEFAULT '',
  `Colorpref_say` char(1) NOT NULL DEFAULT '',
  `Colorpref_osay` char(1) NOT NULL DEFAULT '',
  `RPxp` int(10) unsigned NOT NULL DEFAULT '0',
  `PageLength` int(10) unsigned NOT NULL DEFAULT '0',
  `ActiveDesc` tinyint(1) unsigned NOT NULL DEFAULT '0',
  `OLC` int(11) NOT NULL DEFAULT '0',
  `AwayMsg` varchar(255) NOT NULL DEFAULT '',
  `Doing` varchar(255) NOT NULL DEFAULT '',
  `ApprovedBy` varchar(255) NOT NULL DEFAULT '',
  `QuestPoints` int(11) NOT NULL DEFAULT '0',
  `ContactInfo` text NOT NULL,
  `Fatigue` int(10) unsigned NOT NULL DEFAULT '0',
  `Piety` int(10) unsigned NOT NULL DEFAULT '0',
  `Reputation` int(10) unsigned NOT NULL DEFAULT '0',
  `SocialRank` int(10) unsigned NOT NULL DEFAULT '0',
  `MilitaryRank` int(10) unsigned NOT NULL DEFAULT '0',
  `Sanity` int(10) unsigned NOT NULL DEFAULT '0',
  `Flux` int(11) NOT NULL DEFAULT '0',
  `MaxFlux` int(10) unsigned NOT NULL DEFAULT '0',
  `Verified` tinyint(1) NOT NULL DEFAULT '0',
  `SDesc` text NOT NULL,
  `LDesc` text NOT NULL,
  `Keywords` varchar(255) NOT NULL DEFAULT '',
  `Culture` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `Skillcap` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`ID`),
  UNIQUE KEY `Name` (`Name`),
  UNIQUE KEY `Email` (`Email`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=2 ;

-- --------------------------------------------------------

--
-- Table structure for table `player_notifylist`
--

DROP TABLE IF EXISTS `player_notifylist`;
CREATE TABLE IF NOT EXISTS `player_notifylist` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `PlayerID` bigint(20) unsigned NOT NULL,
  `NotifyID` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `player_quests`
--

DROP TABLE IF EXISTS `player_quests`;
CREATE TABLE IF NOT EXISTS `player_quests` (
  `PlayerID` bigint(20) unsigned NOT NULL,
  `QuestNum` int(10) unsigned NOT NULL,
  PRIMARY KEY (`PlayerID`,`QuestNum`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `player_recognized`
--

DROP TABLE IF EXISTS `player_recognized`;
CREATE TABLE IF NOT EXISTS `player_recognized` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `PlayerID` bigint(20) unsigned NOT NULL,
  `MemID` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `PlayerID` (`PlayerID`,`MemID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `player_rpdescriptions`
--

DROP TABLE IF EXISTS `player_rpdescriptions`;
CREATE TABLE IF NOT EXISTS `player_rpdescriptions` (
  `PlayerID` bigint(20) unsigned NOT NULL,
  `Row` int(10) unsigned NOT NULL,
  `Description` text NOT NULL,
  PRIMARY KEY (`PlayerID`,`Row`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `player_rplogs`
--

DROP TABLE IF EXISTS `player_rplogs`;
CREATE TABLE IF NOT EXISTS `player_rplogs` (
  `PlayerID` bigint(20) unsigned NOT NULL,
  `Row` int(10) unsigned NOT NULL,
  `Log` text NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `player_skills`
--

DROP TABLE IF EXISTS `player_skills`;
CREATE TABLE IF NOT EXISTS `player_skills` (
  `id` int(10) unsigned NOT NULL,
  `skillnum` int(10) unsigned NOT NULL,
  `level` int(10) unsigned NOT NULL,
  `state` int(10) unsigned NOT NULL,
  KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `puff_messages`
--

DROP TABLE IF EXISTS `puff_messages`;
CREATE TABLE IF NOT EXISTS `puff_messages` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `Message` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `qst_index`
--

DROP TABLE IF EXISTS `qst_index`;
CREATE TABLE IF NOT EXISTS `qst_index` (
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `short_description` varchar(255) NOT NULL,
  `description` text NOT NULL,
  `info` text NOT NULL,
  `ending` text NOT NULL,
  `type` int(10) unsigned NOT NULL DEFAULT '0',
  `mob_vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `flags` varchar(255) NOT NULL DEFAULT '0',
  `target` int(10) unsigned NOT NULL DEFAULT '0',
  `exp` int(10) unsigned NOT NULL DEFAULT '0',
  `next_quest` int(10) unsigned NOT NULL DEFAULT '0',
  `value1` int(10) unsigned NOT NULL DEFAULT '0',
  `value2` int(10) unsigned NOT NULL DEFAULT '0',
  `value3` int(10) unsigned NOT NULL DEFAULT '0',
  `value4` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `races_list`
--

DROP TABLE IF EXISTS `races_list`;
CREATE TABLE IF NOT EXISTS `races_list` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `Abbreviation` varchar(255) DEFAULT NULL,
  `Name` varchar(255) DEFAULT NULL,
  `Magic` enum('No','Yes') DEFAULT NULL,
  `Selectable` enum('No','Yes') DEFAULT NULL,
  `Cost` int(11) DEFAULT NULL,
  `Plane` enum('Undefined','Mortal','Ethereal','Astral') DEFAULT NULL,
  `Realm` enum('Undefined','Mundane','Faerie','Heaven','Hell','Ancient') DEFAULT NULL,
  `Description` text,
  `Const` int(11) DEFAULT NULL,
  `Size` enum('None','Tiny','Small','Medium','Large','Huge') DEFAULT NULL,
  `MinAttrib` int(11) DEFAULT NULL,
  `MaxAttrib` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=33 ;

-- --------------------------------------------------------

--
-- Table structure for table `rent_objects`
--

DROP TABLE IF EXISTS `rent_objects`;
CREATE TABLE IF NOT EXISTS `rent_objects` (
  `obj_order` int(10) unsigned NOT NULL,
  `location` int(10) unsigned NOT NULL,
  `vnum` int(10) unsigned NOT NULL,
  `unique_id` int(10) unsigned NOT NULL,
  `name` varchar(255) NOT NULL,
  `short_description` varchar(255) NOT NULL,
  `description` text NOT NULL,
  `action_description` text NOT NULL,
  `type` int(10) unsigned NOT NULL,
  `extra_flags` varchar(255) NOT NULL,
  `wear_flags` varchar(255) NOT NULL,
  `flags` bigint(20) unsigned NOT NULL,
  `val0` int(10) unsigned NOT NULL,
  `val1` int(10) unsigned NOT NULL,
  `val2` int(10) unsigned NOT NULL,
  `val3` int(10) unsigned NOT NULL,
  `weight` int(10) unsigned NOT NULL,
  `cost` int(10) unsigned NOT NULL,
  `rent` int(10) unsigned NOT NULL,
  `size` int(10) unsigned NOT NULL,
  `color` int(10) unsigned NOT NULL,
  `proto_number` int(10) unsigned NOT NULL,
  `resource` int(10) unsigned NOT NULL,
  `player_id` bigint(20) unsigned NOT NULL,
  `room_id` bigint(20) unsigned NOT NULL,
  UNIQUE KEY `unique_id` (`unique_id`),
  KEY `player_id` (`obj_order`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `rent_players`
--

DROP TABLE IF EXISTS `rent_players`;
CREATE TABLE IF NOT EXISTS `rent_players` (
  `PlayerID` bigint(20) unsigned NOT NULL,
  `RentCode` bigint(20) unsigned NOT NULL,
  `Timed` bigint(20) unsigned NOT NULL,
  `NetCost` bigint(20) unsigned NOT NULL,
  `Gold` bigint(20) unsigned NOT NULL,
  `Account` bigint(20) unsigned NOT NULL,
  `NumItems` int(10) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `shp_index`
--

DROP TABLE IF EXISTS `shp_index`;
CREATE TABLE IF NOT EXISTS `shp_index` (
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `no_such_item1` varchar(255) NOT NULL,
  `no_such_item2` varchar(255) NOT NULL,
  `do_not_buy` varchar(255) NOT NULL,
  `missing_cash1` varchar(255) NOT NULL,
  `missing_cash2` varchar(255) NOT NULL,
  `message_buy` varchar(255) NOT NULL,
  `message_sell` varchar(255) NOT NULL,
  `broke_temper` int(10) unsigned NOT NULL DEFAULT '0',
  `flags` varchar(255) NOT NULL DEFAULT '0',
  `shop_keeper` int(10) unsigned NOT NULL DEFAULT '0',
  `trade_with` int(10) unsigned NOT NULL DEFAULT '0',
  `open1` int(11) NOT NULL DEFAULT '0',
  `close1` int(11) NOT NULL DEFAULT '0',
  `open2` int(11) NOT NULL DEFAULT '0',
  `close2` int(11) NOT NULL DEFAULT '0',
  `buy_profit` float NOT NULL DEFAULT '0',
  `sell_profit` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `shp_keywords`
--

DROP TABLE IF EXISTS `shp_keywords`;
CREATE TABLE IF NOT EXISTS `shp_keywords` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `buy_type` int(10) unsigned NOT NULL DEFAULT '0',
  `buy_word` varchar(255) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `shp_products`
--

DROP TABLE IF EXISTS `shp_products`;
CREATE TABLE IF NOT EXISTS `shp_products` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `item` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `shp_rooms`
--

DROP TABLE IF EXISTS `shp_rooms`;
CREATE TABLE IF NOT EXISTS `shp_rooms` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  `room` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `socials`
--

DROP TABLE IF EXISTS `socials`;
CREATE TABLE IF NOT EXISTS `socials` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `command` varchar(255) NOT NULL,
  `sort_as` varchar(255) NOT NULL,
  `hide` int(10) unsigned NOT NULL DEFAULT '0',
  `min_char_position` int(10) unsigned NOT NULL DEFAULT '0',
  `min_victim_position` int(10) unsigned NOT NULL DEFAULT '0',
  `char_no_arg` varchar(255) NOT NULL,
  `others_no_arg` varchar(255) NOT NULL,
  `char_found` varchar(255) NOT NULL,
  `others_found` varchar(255) NOT NULL,
  `vict_found` varchar(255) NOT NULL,
  `not_found` varchar(255) NOT NULL,
  `char_auto` varchar(255) NOT NULL,
  `others_auto` varchar(255) NOT NULL,
  `char_body_found` varchar(255) NOT NULL,
  `others_body_found` varchar(255) NOT NULL,
  `vict_body_found` varchar(255) NOT NULL,
  `char_obj_found` varchar(255) NOT NULL,
  `others_obj_found` varchar(255) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `command` (`command`),
  UNIQUE KEY `sort_as` (`sort_as`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 AUTO_INCREMENT=5 ;

-- --------------------------------------------------------

--
-- Table structure for table `tip_messages`
--

DROP TABLE IF EXISTS `tip_messages`;
CREATE TABLE IF NOT EXISTS `tip_messages` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `title` varchar(255) NOT NULL,
  `tip` text NOT NULL,
  `rights` varchar(255) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `trg_assigns`
--

DROP TABLE IF EXISTS `trg_assigns`;
CREATE TABLE IF NOT EXISTS `trg_assigns` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `type` int(10) unsigned NOT NULL DEFAULT '0',
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `vnum` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `znum` (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `trg_index`
--

DROP TABLE IF EXISTS `trg_index`;
CREATE TABLE IF NOT EXISTS `trg_index` (
  `znum` int(10) unsigned NOT NULL,
  `vnum` int(10) unsigned NOT NULL,
  `name` int(11) NOT NULL,
  `attach_type` int(11) unsigned NOT NULL,
  `trigger_type` int(11) unsigned NOT NULL,
  `narg` int(11) unsigned NOT NULL,
  `arglist` text NOT NULL,
  `cmds` text NOT NULL,
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `usage_history`
--

DROP TABLE IF EXISTS `usage_history`;
CREATE TABLE IF NOT EXISTS `usage_history` (
  `Sockets` int(10) unsigned NOT NULL,
  `Players` int(10) unsigned NOT NULL,
  `IC` int(10) unsigned NOT NULL,
  `UTCDate` bigint(20) unsigned NOT NULL,
  `UserTime` bigint(20) unsigned NOT NULL,
  `SysTime` bigint(20) unsigned NOT NULL,
  `MinFaults` int(10) unsigned NOT NULL,
  `MajFaults` int(10) unsigned NOT NULL,
  `LastReboot` bigint(20) unsigned NOT NULL,
  `LastCopyover` bigint(20) unsigned NOT NULL,
  `Copyovers` tinyint(3) unsigned NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `wld_exits`
--

DROP TABLE IF EXISTS `wld_exits`;
CREATE TABLE IF NOT EXISTS `wld_exits` (
  `id` int(10) unsigned NOT NULL,
  `znum` int(10) unsigned NOT NULL,
  `vnum` int(10) unsigned NOT NULL,
  `dir` int(11) NOT NULL,
  `general_description` text NOT NULL,
  `keyword` varchar(255) NOT NULL,
  `flag` int(10) unsigned NOT NULL,
  `key` int(10) unsigned NOT NULL,
  `to_room` int(10) unsigned NOT NULL,
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `wld_extradescs`
--

DROP TABLE IF EXISTS `wld_extradescs`;
CREATE TABLE IF NOT EXISTS `wld_extradescs` (
  `id` int(10) unsigned NOT NULL,
  `znum` int(11) NOT NULL,
  `vnum` int(11) NOT NULL,
  `keyword` varchar(255) NOT NULL,
  `description` text NOT NULL,
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `wld_index`
--

DROP TABLE IF EXISTS `wld_index`;
CREATE TABLE IF NOT EXISTS `wld_index` (
  `znum` bigint(20) unsigned NOT NULL DEFAULT '0',
  `vnum` bigint(20) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `description` text NOT NULL,
  `flags` varchar(255) NOT NULL DEFAULT '0',
  `sectortype` int(10) unsigned NOT NULL DEFAULT '0',
  `specproc` int(10) unsigned NOT NULL DEFAULT '0',
  `magic_flux` int(10) unsigned NOT NULL DEFAULT '0',
  `resource_flags` int(10) unsigned NOT NULL DEFAULT '0',
  `res_val0` int(10) unsigned NOT NULL DEFAULT '0',
  `res_val1` int(10) unsigned NOT NULL DEFAULT '0',
  `res_val2` int(10) unsigned NOT NULL DEFAULT '0',
  `res_val3` int(10) unsigned NOT NULL DEFAULT '0',
  `res_val4` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`znum`,`vnum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `zon_commands`
--

DROP TABLE IF EXISTS `zon_commands`;
CREATE TABLE IF NOT EXISTS `zon_commands` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `znum` bigint(20) unsigned DEFAULT NULL,
  `command` varchar(255) DEFAULT NULL,
  `if_flag` int(11) DEFAULT NULL,
  `arg1` int(11) DEFAULT NULL,
  `arg2` int(11) DEFAULT NULL,
  `arg3` int(11) DEFAULT NULL,
  `sarg1` varchar(255) DEFAULT NULL,
  `sarg2` varchar(255) DEFAULT NULL,
  `cmdnum` tinyint(3) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `zon_index`
--

DROP TABLE IF EXISTS `zon_index`;
CREATE TABLE IF NOT EXISTS `zon_index` (
  `znum` int(10) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  `lifespan` int(10) unsigned NOT NULL DEFAULT '0',
  `bot` int(10) unsigned NOT NULL DEFAULT '0',
  `top` int(10) unsigned NOT NULL DEFAULT '0',
  `reset_mode` int(10) unsigned NOT NULL DEFAULT '2',
  `zone_flags` bigint(20) unsigned NOT NULL DEFAULT '0',
  `unknown` varchar(255) NOT NULL DEFAULT '0',
  `builders` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`znum`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;

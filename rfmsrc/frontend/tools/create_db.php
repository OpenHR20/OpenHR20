<?php

unlink ("/usb/home/db.sqlite");

$db = new SQLiteDatabase("/usb/home/db.sqlite");

// ************************************************************

$db->query("CREATE TABLE debug_log (
    id INTEGER PRIMARY KEY, 
    time INTEGER, 
    addr INTEGER,
    data CHAR(80))");

$db->query("CREATE INDEX debug_addr on debug_log (addr)");


// ************************************************************

$db->query("CREATE TABLE log (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    mode CHAR(10),
    valve INTEGER,
    real INTEGER,
    wanted INTEGER,
    battery INTEGER,
    error INTEGER DEFAULT 0,
    window INTEGER DEFAULT 0,
    force INTEGER DEFAULT 0)");

$db->query("CREATE INDEX log_addr on log (addr)");

// ************************************************************

$db->query("CREATE TABLE timers (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX timers_addr on timers (addr)");

// ************************************************************

$db->query("CREATE TABLE eeprom (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX eeprom_addr on eeprom (addr)");

// ************************************************************

$db->query("CREATE TABLE trace (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX _trace_addr on trace (addr)");

// ************************************************************

$db->query("CREATE TABLE command_queue (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    send INTEGER DEFAULT 0,
    data char(20) )");

$db->query("CREATE INDEX command_addr on command_queue (addr)");

// ************************************************************


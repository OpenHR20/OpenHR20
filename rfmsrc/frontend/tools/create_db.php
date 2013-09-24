<?php

//unlink ("/usb/home/db.sqlite");

$db = new SQLite3("/tmp/openhr20.sqlite");
$db->query("PRAGMA synchronous=OFF");

// ************************************************************

$db->query("CREATE TABLE debug_log (
    id INTEGER PRIMARY KEY, 
    time INTEGER, 
    addr INTEGER,
    data CHAR(80))");

//$db->query("CREATE INDEX debug_time on debug_log (time)");
$db->query("CREATE INDEX debug_time_addr on debug_log (time,addr)");


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

//$db->query("CREATE INDEX log_addr on log (addr)");
$db->query("CREATE INDEX log_time_addr on log (time,addr)");
//$db->query("CREATE INDEX log_time on log (time)");

// ************************************************************

$db->query("CREATE TABLE timers (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX timers_idx_addr on timers (idx,addr)");

// ************************************************************

$db->query("CREATE TABLE eeprom (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX eeprom_idx_addr_idx on eeprom (idx,addr)");

// ************************************************************

$db->query("CREATE TABLE trace (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    idx INTEGER,
    value INTEGER )");

$db->query("CREATE INDEX _trace_time_addr on trace (time,addr)");

// ************************************************************

$db->query("CREATE TABLE command_queue (
    id INTEGER PRIMARY KEY, 
    addr INTEGER,
    time INTEGER, 
    send INTEGER DEFAULT 0,
    data char(20) )");

$db->query("CREATE INDEX command_time_addr on command_queue (time,addr)");

// ************************************************************

$db->query("CREATE TABLE versions (
    addr INTEGER PRIMARY KEY, 
    time INTEGER,
    data char(80))");

<?php

$db = new SQLiteDatabase("/usb/home/db.sqlite");


$db->query("CREATE TABLE log (
    id INTEGER PRIMARY KEY, 
    time INTEGER, 
    data CHAR(80))");

$db->query("CREATE TABLE timmers (
    id INTEGER PRIMARY KEY, 
    address INTEGER,
    time INTEGER, 
    value INTEGER,
    timestamp INTEGER )");

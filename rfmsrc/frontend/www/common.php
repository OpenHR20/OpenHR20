<?php


function format_time($timestamp) {
    return date("Y-m-d H:i:s",$timestamp);
}

$db = new SQLiteDatabase("/usb/home/db.sqlite");

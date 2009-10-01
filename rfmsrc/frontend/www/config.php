<?php

$db = new SQLiteDatabase("/usb/home/db.sqlite");


  // translation table for valve names
  // example:
  /* $room_name = array (
    0x04 => 'test', // default setting in valves
    0x11 => 'room 1',
    0x12 => 'room 2',
    0x13 => 'room 3',
    0x14 => 'room 4',
    0x15 => 'room 5'
  ); */
  $room_name = array (
    0x11 => 'decak',
    0x12 => 'obyvak',
    0x13 => 'loznice',
    0x14 => 'kuchyne',
    0x15 => 'koupelna'
  );

  // translation table for timers name (weekdays)
  /*$timer_names =  array (
    'Week',
    'Monday'
    'Tuesday',
    'Wednesday',
    'Thursday'.
    'Friday',
    'Saturday',
    'Sunday'
  );*/
  $timer_names =  array (
    'tyden',
    'pondeli',
    'utery',
    'streda',
    'ctvrtek',
    'patek',
    'sobota',
    'nedele'
  );

  // symbols for 4 temperature mode
  // unicode version with nice moon/sun symbols, have problem on mobile Opera browser
  
  /* $symbols = array (
      'x',		//off
      '&#x263e;',	//Night
      '&#x2600;',	//Day
      '&#x263c;		//Comfort
    '); */

  // universal symbols, but not nice // english
  /* $symbols = array (
      'off',
      'Night',
      'Day',
      '+++' 		//Comfort
  ); */

  $symbols = array (
      'off',
      'Noc',
      'Den',
      '+++'
  );

  $refresh_value=15; // refresh time for command queue pending wait 

  $chart_hours = 12; // chart contain values from last 12 hours

  $warning_age = 8*60; // maximum data age for warning

  $error_age = 20*60; // maximum data age for error
  
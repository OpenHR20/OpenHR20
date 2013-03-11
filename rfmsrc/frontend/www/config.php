<?php

$db = new SQLite3("/tmp/openhr20.sqlite");
$TIMEZONE="Europe/Warsaw";
$RRD_ENABLE=true;
$PLOTS_DIR = "plots";
$RRD_DAYS = array (3, 7, 30, 90);



  // translation table for valve names
  // example:
   $room_name = array (
    0x0a => 'sypialnia', // default setting in valves
    0x0b => 'salon', // default setting in valves
    0x0c => 'dzieciecy', // default setting in valves
  ); 
/*  $room_name = array (
    0x11 => 'decak',
    0x12 => 'obyvak',
    0x13 => 'loznice',
    0x14 => 'kuchyne',
    0x15 => 'koupelna'
  );*/

  // translation table for timers name (weekdays)
  $timer_names =  array (
    'Week',
    'Monday',
    'Tuesday',
    'Wednesday',
    'Thursday',
    'Friday',
    'Saturday',
    'Sunday'
  );
  /*$timer_names =  array (
    'tyden',
    'pondeli',
    'utery',
    'streda',
    'ctvrtek',
    'patek',
    'sobota',
    'nedele'
  );*/

  // symbols for 4 temperature mode
  // unicode version with nice moon/sun symbols, have problem on mobile Opera browser
  
   $symbols = array (
      'x',		//off
      '&#x263e;',	//Night
      '&#x2600;',	//Day
      '&#x263c;		//Comfort
    '); 

  // universal symbols, but not nice // english
  /* $symbols = array (
      'off',
      'Night',
      'Day',
      '+++' 		//Comfort
  ); */

/*  $symbols = array (
      'off',
      'Noc',
      'Den',
      '+++'
  );*/

  $refresh_value=15; // refresh time for command queue pending wait 

  $chart_hours = 48; // chart contain values from last 12 hours

  $warning_age = 8*60; // maximum data age for warning

  $error_age = 20*60; // maximum data age for error
  

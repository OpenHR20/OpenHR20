<?php

$db = new SQLiteDatabase("/usb/home/db.sqlite");
$db->query("PRAGMA synchronous=OFF");
//$db->query("BEGIN TRANSACTION");
//	$db->query("COMMIT");
//	$db->query("BEGIN TRANSACTION");

$fp=fsockopen("192.168.62.230",3531);
//$fp=fopen("php://stdin","r"); 

while($line=trim(fgets($fp,65535))) { 
    
//while(($line=stream_get_line($fp,256,"\n"))!=FALSE) {

    // echo "time ".microtime()."\n";
    
    if ($line=="RTC?") {
	list($usec, $sec) = explode(" ", microtime());
	$items = getdate($sec);
	$time = sprintf("H%02x%02x%02x%02x\n",
	    $items['hours'],
	    $items['minutes'],
	    $items['seconds'],
	    round($usec*100));
	$date = sprintf("Y%02x%02x%02x\n",
	    $items['year']-2000,
	    $items['mon'],
	    $items['mday']);
	echo $time;
	fwrite($fp,$time);
	echo $date;
	fwrite($fp,$date);
    }
    else {
    	echo $line."\n"; 
	$db->unbufferedQuery("INSERT INTO log (time,data) VALUES (".time().",\"$line\")");
	//echo "time ".microtime()."\n";
    }

} 



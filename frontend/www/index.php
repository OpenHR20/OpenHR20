<?php
$ts=microtime(true);
include "config.php";
include "common.php";

$refresh=false;

// controller part
  { // common
    $addr = (int) $_GET['addr'];
    if (isset($room_name[$addr]))
      $name = $room_name[$addr];
    else 
      $name = $addr;
    if (isset($_GET['page'])) {
      $page=cleanString($_GET['page']);
    } else {
      $page = "status";
    }
    $contend_class = "contend_$page";
    if (isset($_GET['css'])) {
      $css=cleanString($_GET['css']).'.css';
    } else {
      $css = "browser_1.css";
    }
    include "contend/$page.php";
    $contend = new $contend_class($addr);
    $cmd1 = $contend->controller();
    $redirect2queue = false; 
    if ($cmd1) {
      if ($page!='queue') {
	include "contend/queue.php";
	$contend2 = new contend_queue($addr);
	$contend2->add_to_queue($cmd1);
      } else {
	$contend->add_to_queue($cmd1);
      }
      unset($cmd1);
    }
    if ($contend->protect) {
      if ($addr>0)
	$result = $db->query("SELECT count(*) AS cnt FROM command_queue WHERE addr=$addr");
      else
	$result = $db->query("SELECT count(*) AS cnt FROM command_queue");
      $row = $result->fetchArray();
      if ($row['cnt']>0) {
	// header('Location: http://'.$_SERVER['HTTP_HOST']."/?page=queue&addr=$addr");
	// It need absolute address (RFC) but it can't work with tunnels
	//header("Location: /?page=queue&addr=$addr");
	//exit;
	if (isset($contend2)) $contend=$contend2;
	else {
	  include "contend/queue.php";
	  $contend = new contend_queue($addr);
	}
	$contend->warning=true;
	$refresh=true;
      }
    }
  }
  
    
// view part
echo '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Heating</title>  <meta http-equiv="content-language" content="en" />
';
if ($refresh) {
  if ($page=='queue') echo '<meta http-equiv="Refresh" content="'.$refresh_value.'; URL='."/?page=queue&addr=$addr\" />\n";
  else echo '<meta http-equiv="Refresh" content="'.$refresh_value.'; URL='.$_SERVER["REQUEST_URI"]."\" />\n";
}
echo '<meta http-equiv="content-type" content="text/html; charset=utf-8" />
  <style media="screen" type="text/css"> @import url(';

echo "'css/$css'";
echo '); </style></head><body>';

echo '<div id="content">';
echo '<div id="sidebar"><div id="sidebar-content">';
  include "menu.php";
echo '</div></div>';

// just test
echo '<div id="main"><div id="main-content">';
    $contend->view();
echo '</div></div>';
echo '<hr class="cleaner" />';

echo '<div class="debug">';
  echo '<div>';
    $contend->debug();
  echo '</div>';
  echo '<div>script duration '.sprintf("%.2f",microtime(true)-$ts)."sec</div>\n";
echo '</div>';
echo "</body></html>";

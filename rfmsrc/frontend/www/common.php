<?php

include "config.php";
date_default_timezone_set($TIMEZONE);

function format_time($timestamp) {
    return date("Y-m-d H:i:s",$timestamp);
}

function cleanString($wild) {
    return preg_replace("/[^[_:alnum:]+]/","",$wild);
}

function get_config ($layout_names,$addr, $id) {
  global $db;
  if (isset($layout_names[$id])) {
    $idx = $layout_names[$id];
    $result = $db->query("SELECT * FROM eeprom WHERE addr=$addr AND idx=$idx");
    $row = $result->fetchArray();
    if ($row)
      return $row['value'];
  }
  return "NA";
}


class contend {
  protected $addr;
  public $protect = false;
  function __construct($addr) {
    $this->addr=$addr;
  }
  public function view() {;}
  public function controller() {
    return null;
  }
  public function debug() {;}
}
